/**
 * @file board_brightness.h
 *
 * Backlight control using the pin and electrical polarity of the selected board.
 * The Arduino ESP32 3.x LEDC API attaches PWM to a pin rather than a manually
 * assigned channel. Attachment is lazy, and a failed attachment may be retried
 * on the next call. Duty-cycle rounding preserves useful low-brightness values.
 */

#pragma once
#include <Arduino.h>

// Same pin, polarity, PWM frequency and resolution as the validated tests.
/**
 * Set logical brightness as a percentage, clamped to 0..100.
 * PWM duty is inverted for active-low hardware; rounding is performed before
 * that inversion so percentage semantics stay the same across boards.
 */
inline void BoardBrightness(unsigned percent) {
    static bool initialized = false;
    if (percent > 100) percent = 100;
    if (!initialized) initialized = ledcAttach(CYD_BL_PIN, CYD_BL_PWM_HZ, CYD_BL_PWM_BITS);
    if (!initialized) return;
    const unsigned maximum = (1U << CYD_BL_PWM_BITS) - 1;
    unsigned duty = (maximum * percent + 50) / 100;
    if (!CYD_BL_ACTIVE_HIGH) duty = maximum - duty;
    ledcWrite(CYD_BL_PIN, duty);
}
