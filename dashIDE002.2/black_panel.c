/**
 * @file black_panel.c
 *
 * JC3248W535 transport bridge between BoardDisplay and the manufacturer driver.
 * A DMA-completion semaphore protects the reusable RGB565 strip buffer; a
 * second semaphore observes the panel's tearing-effect/frame-start signal.
 * Only the black profile compiles this implementation. Touch is polled through
 * the vendor I2C interface and returned in native portrait coordinates.
 */

// JC3248W535 panel transport, using the manufacturer's AXS15231B driver.
#include "board_config.h"
#if DASH_BLACK
#include "tft_setup.h"
#include "black_panel.h"
#include "esp_lcd_axs15231b.h"
#include "esp_lcd_panel_ops.h"
#include "driver/i2c.h"
#include "driver/spi_master.h"
#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"
#include "esp_check.h"
#include "black_init_sequence.h"

static esp_lcd_panel_handle_t panel;
static esp_lcd_touch_handle_t touch;
static SemaphoreHandle_t transfer_done;
static SemaphoreHandle_t frame_start;

/**
 * Panel-I/O completion callback running in ISR context.
 * Use only the ISR-safe semaphore API; return whether a waiting task should
 * be woken. The callback does not draw, allocate memory or log over Serial.
 */
static bool color_done(esp_lcd_panel_io_handle_t io, esp_lcd_panel_io_event_data_t *event, void *arg) {
    BaseType_t wake = pdFALSE;
    xSemaphoreGiveFromISR(transfer_done, &wake);
    return wake == pdTRUE;
}

/**
 * Tearing-effect GPIO interrupt: publish a frame-start notification.
 * A binary semaphore deliberately coalesces multiple edges between frames;
 * there is no requirement to queue every TE pulse.
 */
static void frame_interrupt(void *arg) {
    BaseType_t wake = pdFALSE;
    xSemaphoreGiveFromISR(frame_start, &wake);
    if (wake) portYIELD_FROM_ISR();
}

/**
 * Initialize QSPI, vendor register state, TE input and backlight pin.
 * Hard failures abort through ESP_ERROR_CHECK/assert rather than continue with
 * an unusable display. Per-frame RGB565 storage is owned by BoardDisplay.
 */
void BlackPanelInit(void) {
    transfer_done = xSemaphoreCreateBinary();
    frame_start = xSemaphoreCreateBinary();
    assert(transfer_done && frame_start);
    const spi_bus_config_t bus = AXS15231B_PANEL_BUS_QSPI_CONFIG(JC3248W535_LCD_CLK, JC3248W535_LCD_DATA0, JC3248W535_LCD_DATA1, JC3248W535_LCD_DATA2, JC3248W535_LCD_DATA3, CYD_NATIVE_WIDTH * 16 * 2);
    ESP_ERROR_CHECK(spi_bus_initialize(SPI2_HOST, &bus, SPI_DMA_CH_AUTO));
    esp_lcd_panel_io_handle_t io;
    esp_lcd_panel_io_spi_config_t io_cfg = AXS15231B_PANEL_IO_QSPI_CONFIG(JC3248W535_LCD_CS, color_done, NULL);
    io_cfg.trans_queue_depth = 1;
    ESP_ERROR_CHECK(esp_lcd_new_panel_io_spi((esp_lcd_spi_bus_handle_t)SPI2_HOST, &io_cfg, &io));
    const axs15231b_vendor_config_t vendor = {
        .init_cmds = lcd_init_cmds,
        .init_cmds_size = sizeof(lcd_init_cmds) / sizeof(lcd_init_cmds[0]),
        .flags.use_qspi_interface = 1,
    };
    const esp_lcd_panel_dev_config_t cfg = {
        .reset_gpio_num = JC3248W535_LCD_RST,
        .rgb_ele_order = LCD_RGB_ELEMENT_ORDER_RGB,
        .bits_per_pixel = 16,
        .vendor_config = (void*)&vendor,
    };
    ESP_ERROR_CHECK(esp_lcd_new_panel_axs15231b(io, &cfg, &panel));
    ESP_ERROR_CHECK(esp_lcd_panel_reset(panel));
    ESP_ERROR_CHECK(esp_lcd_panel_init(panel));
    // Manufacturer driver uses the legacy 'off' boolean in this callback.
    ESP_ERROR_CHECK(esp_lcd_panel_disp_on_off(panel, false));
    const gpio_config_t te = {
        .pin_bit_mask = 1ULL << JC3248W535_LCD_TE,
        .mode = GPIO_MODE_INPUT,
        .pull_up_en = GPIO_PULLUP_ENABLE,
        .intr_type = GPIO_INTR_NEGEDGE,
    };
    ESP_ERROR_CHECK(gpio_config(&te));
    esp_err_t result = gpio_install_isr_service(0);
    assert(result == ESP_OK || result == ESP_ERR_INVALID_STATE);
    ESP_ERROR_CHECK(gpio_isr_handler_add(JC3248W535_LCD_TE, frame_interrupt, NULL));
    ESP_ERROR_CHECK(gpio_set_direction(CYD_BL_PIN, GPIO_MODE_OUTPUT));
    ESP_ERROR_CHECK(gpio_set_level(CYD_BL_PIN, CYD_BL_ACTIVE_HIGH));
}

/**
 * Send consecutive native-width rows and retain the caller's buffer until DMA ends.
 * For the first strip, discard an old TE signal and wait at most 30 ms for a
 * fresh one. Missing TE does not block forever. DMA timeout is fatal because
 * reusing a still-active buffer could corrupt memory/display transfers.
 */
void BlackPanelWriteRows(int y, int rows, const uint16_t *pixels) {
    if (y == 0) {
        xSemaphoreTake(frame_start, 0);
        xSemaphoreTake(frame_start, pdMS_TO_TICKS(30));
    }
    ESP_ERROR_CHECK(esp_lcd_panel_draw_bitmap(panel, 0, y, 320, y + rows, pixels));
    // Buffer must not be reused until DMA finishes, including the last strip.
    if (xSemaphoreTake(transfer_done, pdMS_TO_TICKS(1000)) != pdTRUE) abort();
}

void BlackPanelInvert(bool invert) {
    ESP_ERROR_CHECK(esp_lcd_panel_invert_color(panel, invert));
}

bool BlackTouchInit(void) {
    const i2c_config_t cfg = {
        .mode = I2C_MODE_MASTER,
        .sda_io_num = CYD_TOUCH_SDA, .sda_pullup_en = GPIO_PULLUP_DISABLE,
        .scl_io_num = CYD_TOUCH_SCL, .scl_pullup_en = GPIO_PULLUP_DISABLE,
        .master.clk_speed = CYD_TOUCH_I2C_HZ,
    };
    if (i2c_param_config(I2C_NUM_0, &cfg) != ESP_OK ||
        i2c_driver_install(I2C_NUM_0, cfg.mode, 0, 0, 0) != ESP_OK) return false;
    esp_lcd_panel_io_handle_t io;
    esp_lcd_panel_io_i2c_config_t io_cfg = ESP_LCD_TOUCH_IO_I2C_AXS15231B_CONFIG();
    io_cfg.dev_addr = CYD_TOUCH_ADDRESS;
    if (esp_lcd_new_panel_io_i2c((esp_lcd_i2c_bus_handle_t)I2C_NUM_0, &io_cfg, &io) != ESP_OK) return false;
    const esp_lcd_touch_config_t tp_cfg = {
        .x_max = CYD_NATIVE_WIDTH, .y_max = CYD_NATIVE_HEIGHT,
        .rst_gpio_num = JC3248W535_TOUCH_RST, .int_gpio_num = JC3248W535_TOUCH_INT,
    };
    return esp_lcd_touch_new_i2c_axs15231b(io, &tp_cfg, &touch) == ESP_OK;
}

/**
 * Fetch one contact in native portrait coordinates.
 * False covers absent/uninitialized hardware, read errors and no contact;
 * rotation and debouncing are intentionally left to the higher touch adapter.
 */
static bool touchReleased = false;
bool BlackTouchReleased(void) { return touchReleased; }
bool BlackTouchRead(uint16_t *x, uint16_t *y) {
    touchReleased = false;
    if (!touch || esp_lcd_touch_read_data(touch) != ESP_OK) return false;
    uint8_t count = 0;
    bool pressed = esp_lcd_touch_get_coordinates(touch, x, y, NULL, &count, 1) && count > 0;
    touchReleased = !pressed;
    return pressed;
}
#endif
