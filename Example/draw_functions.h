//UPDATED
#ifndef DRAW_H
#define DRAW_H

#include <TFT_eSPI.h>
#include <HTTPClient.h>   // For network JPG streaming/downloading
#include <TJpg_Decoder.h> // For high-performance JPG decoding/rendering
#include <SD.h>           // For fetching JPG assets from microSD storage

// External reference to the TFT display instance instantiated in the main application
extern TFT_eSPI tft;

// ============================================================================
// FORWARD DECLARATIONS
// Necessary because high-level composite functions like DrawSmartImage() call 
// underlying rendering pipelines defined further down in this header file.
// ============================================================================
void DrawImage(int x, int y, const unsigned char* bitmap, int w, int h, uint16_t color, uint8_t zoom);
void DrawJPG(int x, int y, String url);
void DrawJPGFromSD(int x, int y, String path);
void DrawSmartImage(int x, int y, String source, uint16_t color, uint8_t zoom, bool isColorDefined = false);

// ============================================================================
// CORE GRAPHICS PRIMITIVES
// ============================================================================

/**
 * @brief Draws a single pixel at specified coordinates.
 * @param x Horizontal pixel position.
 * @param y Vertical pixel position.
 * @param color 16-bit RGB565 color value.
 */
void DrawPixel(int x, int y, uint16_t color)
{
    tft.drawPixel(x, y, color);
}

/**
 * @brief Renders a solid geometric rectangle complete with an independent outer border.
 * @param x1 Top-left horizontal coordinate.
 * @param y1 Top-left vertical coordinate.
 * @param x2 Bottom-right horizontal coordinate.
 * @param y2 Bottom-right vertical coordinate.
 * @param borderColor 16-bit RGB565 color for the perimeter outline.
 * @param backgroundColor 16-bit RGB565 color for the internal filled area.
 */
void DrawRectangle(
    int x1, int y1,
    int x2, int y2,
    uint16_t borderColor,
    uint16_t backgroundColor)
{
    // Derive width and height from absolute coordinate differences
    int width  = x2 - x1;
    int height = y2 - y1;

    // Fill interior first to prevent layout overlapping issues
    tft.fillRect(x1, y1, width, height, backgroundColor);
    // Draw the boundary framework on top
    tft.drawRect(x1, y1, width, height, borderColor);
}

/**
 * @brief Renders a basic functional UI button element containing a single line of text.
 * @param x Top-left horizontal coordinate.
 * @param y Top-left vertical coordinate.
 * @param w Fixed width of the button bounds.
 * @param h Fixed height of the button bounds.
 * @param text Inner label string to display.
 * @param backgroundColor UI button internal surface color.
 * @param textColor Text font character color.
 */
void DrawButton(
    int x, int y,
    int w, int h,
    String text,
    uint16_t backgroundColor,
    uint16_t textColor)
{
    // Draw button foundation geometry
    tft.fillRect(x, y, w, h, backgroundColor);
    tft.drawRect(x, y, w, h, TFT_WHITE); // Default clear white border highlighting

    // Text asset configurations
    tft.setTextColor(textColor, backgroundColor);
    tft.setTextFont(2); // Set to standard legible Medium Font

    // Dynamic inner layout calculations for padding/centering properties
    int tx = x + 5;
    int ty = y + (h / 2) - 8; // Approximates vertical text centering alignments

    tft.setCursor(tx, ty);
    tft.print(text);
}

/**
 * @brief Draws a filled circle bordered by a distinctive parameter ring.
 * @param x Center point horizontal coordinate.
 * @param y Center point vertical coordinate.
 * @param radius Distance from center to edge.
 * @param borderColor Color of the thin outer boundary path.
 * @param backgroundColor Color of the filled interior space.
 */
void DrawCircle(
    int x, int y,
    int radius,
    uint16_t borderColor,
    uint16_t backgroundColor)
{
    tft.fillCircle(x, y, radius, backgroundColor);
    tft.drawCircle(x, y, radius, borderColor);
}

/**
 * @brief Standardized text printing mechanism supporting built-in Adafruit/TFT_eSPI fonts.
 * @param x Horizontal positioning anchor.
 * @param y Vertical positioning anchor.
 * @param text String characters to display on the display panel.
 * @param color Text foreground paint mask color.
 * @param font Font Index Selection Matrix: 1=Small, 2=Medium, 4=Large, 6=Very Large
 */
void DrawText(
    int x, int y,
    String text,
    uint16_t color,
    int font)
{
    tft.setTextColor(color, TFT_BLACK); // Draws with a rigid solid black background block to prevent trailing artifacts
    tft.setTextFont(font);
    tft.setCursor(x, y);
    tft.print(text);
}

/**
 * @brief Renders a vector line between two separate coordinate points.
 */
void DrawLine(
    int x1, int y1,
    int x2, int y2,
    uint16_t color)
{
    tft.drawLine(x1, y1, x2, y2, color);
}

// ============================================================================
// COMPOSITE AND IMAGE RENDERING ENGINE PIPELINES
// ============================================================================

/**
 * @brief Polimorphic Smart Router interface. Automatically detects structural image type,
 * handles local file tracking, initiates downloads, or falls back to PROGMEM tables.
 * * Evaluation Routing Constraints:
 * - Begins with "http://" or "https://" -> Handles as network asset stream (DrawJPG)
 * - Begins with "/" or contains ".jpg"/".jpeg" -> Handles as localized microSD file asset (DrawJPGFromSD)
 * - Catch-all fallback -> Searches local flash tables for compile-time monochome structures (IMAGE_TABLE)
 */
void DrawSmartImage(int x, int y, String source, uint16_t color, uint8_t zoom, bool isColorDefined)
{
    // Sanity boundary guard check against empty references
    if (source.length() == 0) return;

    // Normalizing string comparisons to lower-case state structures
    String lower = source;
    lower.toLowerCase();

    // ---------- Routing Option 1: Remote Internet Web Asset URL ----------
    if (lower.startsWith("http://") || lower.startsWith("https://"))
    {
        DrawJPG(x, y, source);
        return;
    }

    // ---------- Routing Option 2: Local MicroSD File Storage Path ----------
    if (lower.startsWith("/") || lower.endsWith(".jpg") || lower.endsWith(".jpeg"))
    {
        DrawJPGFromSD(x, y, source);
        return;
    }

    // ---------- Routing Option 3: Internal Embedded Monochome Bitmap Array ----------
    int imgIndex = FindImageIndex(source);
    if (imgIndex != -1)
    {
        // Resolve structural canvas color layer formatting requirements
        uint16_t finalColor = isColorDefined ? color : IMAGE_TABLE[imgIndex].defaultColor;

        DrawImage(
            x, y,
            IMAGE_TABLE[imgIndex].data,
            IMAGE_TABLE[imgIndex].width,
            IMAGE_TABLE[imgIndex].height,
            finalColor,
            zoom
        );
    }
    else
    {
        // Throw localized asset matching parsing exception warnings out across Serial systems
        Serial.printf("DrawSmartImage Error: Unrecognized asset source identifier '%s'\n", source.c_str());
    }
}

/**
 * @brief Streams and decodes JPEG raw binary image data directly out of local MicroSD cards.
 * @note Requires the hardware SD peripheral bus to be initialized prior to execution.
 */
void DrawJPGFromSD(int x, int y, String path)
{
    // Hardware asset presence verification sanity check
    if (!SD.exists(path)) {
        Serial.printf("DrawJPGFromSD Error: File resource target not found at context path '%s'\n", path.c_str());
        return;
    }

    // Prepare driver state structures for 16-bit Big-Endian/Little-Endian structural pixel byte swaps
    tft.setSwapBytes(true);
    
    // Scale option targets: 1 = Normal size, 2 = 1/2 size, 4 = 1/4 size down-scaling passes
    TJpgDec.setJpgScale(1);
    
    // Direct hardware streaming decompression directly out of file system pointers
    TJpgDec.drawSdJpg(x, y, path.c_str());
    
    // Reset driver register flags to maintain drawing standard normalities across standard rendering pipelines
    tft.setSwapBytes(false);
}

/**
 * @brief Renders raw 1-bit monochome byte-packed bitmaps with integer scaling/zoom calculations.
 * @param bitmap Pointer to compile-time array assets residing natively inside PROGMEM Flash storage structures.
 */
void DrawImage(
    int x,
    int y,
    const unsigned char* bitmap,
    int w,
    int h,
    uint16_t color,
    uint8_t zoom)
{
    // Performance optimization path bypass: If no scaling factor is applied, utilize low-level block drawing methods
    if (zoom <= 1)
    {
        tft.drawBitmap(x, y, bitmap, w, h, color);
        return;
    }

    // Manual pixel matrix coordinate generation scaling pipeline loop loops
    for (int py = 0; py < h; py++)
    {
        for (int px = 0; px < w; px++)
        {
            // Calculate specific 1-bit packed byte offsets within standard horizontal scanning maps
            int byteIndex = py * ((w + 7) / 8) + (px / 8);

            // Bitwise checking calculation tracking to determine active pixel configurations
            if (bitmap[byteIndex] & (0x80 >> (px & 7)))
            {
                // Scale individual bitmap pixels up into larger filled square geometry areas
                tft.fillRect(
                    x + px * zoom,
                    y + py * zoom,
                    zoom,
                    zoom,
                    color
                );
            }
        }
    }
}

// ============================================================================
// TJPG DECODER DRIVER CALLBACK INTERFACE
// ============================================================================

/**
 * @brief Mandatory driver level callback mechanism requested by the TJpg_Decoder rendering engines.
 * Pushes parsed block-matrices (Minimum Coding Units - MCUs) directly into display registers.
 */
static bool tft_output(int16_t x, int16_t y, uint16_t w, uint16_t h, uint16_t* bitmap)
{
    // Terminate parser evaluations safely if block operations slip past the physical layout boundaries
    if (y >= tft.height()) return false;
    
    // Direct hardware DMA or block transfer write operations execution pass
    tft.pushImage(x, y, w, h, bitmap);
    return true;
}

// ============================================================================
// CLOUD NETWORK/HTTP STREAMING IMAGE ENGINE
// ============================================================================

/**
 * @brief Streams an online remote JPG asset over HTTP, stores temporary payloads into internal heap
 * RAM segments, and calls the rendering pipeline to decompress live network arrays onto screens.
 * @warning Extreme caution is advised on devices with limited SRAM allocations (e.g., standard ESP32). 
 * Large images may trigger heap fragmentations or Fatal Out-Of-Memory Panic Crashes.
 */
void DrawJPG(int x, int y, String url)
{
    if (url == "") return;

    HTTPClient http;
    http.begin(url);
    http.setTimeout(3000); // Fail gracefully if network handshakes hang past 3 seconds
    
    int httpCode = http.GET();
    if (httpCode == HTTP_CODE_OK) {
        int totalLen = http.getSize();
        if (totalLen > 0) {
            
            // Dynamic heap allocation pass for complete payload binary data buffers
            uint8_t* jpgBuffer = (uint8_t*)malloc(totalLen);
            
            if (jpgBuffer != NULL) {
                WiFiClient* stream = http.getStreamPtr();
                int bytesRead = 0;
                
                uint32_t startTime = millis();
                const uint32_t DOWNLOAD_TIMEOUT = 5000; // Hard streaming safety threshold: 5 seconds

                // Active network read extraction data processing sequence loops
                while (http.connected() && bytesRead < totalLen) {
                    // Watchdog safety checks against connection drop freezes
                    if (millis() - startTime > DOWNLOAD_TIMEOUT) {
                        Serial.println("DrawJPG Error: Download streaming session timeout exceeded.");
                        break;
                    }
                    
                    size_t size = stream->available();
                    if (size) {
                        // Securely read incoming chunks into sequential positions inside the heap buffer array
                        int c = stream->readBytes(&jpgBuffer[bytesRead], size);
                        bytesRead += c;
                        startTime = millis(); // Reset structural timeout tracking while data transactions remain ongoing
                    }
                    delay(1); // Small yields to allow system context operations to execute smoothly
                }

                // Integrity verification validation passes on downloaded network packets
                if (bytesRead < totalLen) {
                    Serial.printf("DrawJPG Error: Incomplete packet download stream (%d/%d bytes resolved)\n", bytesRead, totalLen);
                    free(jpgBuffer); // Release heap allocations immediately to prevent memory leak states
                    http.end();
                    return;
                }
                
                // Pipeline Execution: Execute image conversions from memory array block configurations
                tft.setSwapBytes(true); 
                TJpgDec.setJpgScale(1); 
                TJpgDec.drawJpg(x, y, jpgBuffer, totalLen); // Decompress and dump directly onto the display via the callback function
                tft.setSwapBytes(false);

                // CRITICAL: Always release allocated memory buffers back into standard memory block pools!
                free(jpgBuffer); 
            } else {
                Serial.println("DrawJPG Panic: Insufficient RAM available for dynamic JPG processing structures.");
            }
        }
    } else {
        Serial.printf("DrawJPG Error: HTTP Request execution failure occurred (Response Status Code: %d)\n", httpCode);
    }
    http.end(); // Safely terminate and close standard underlying sockets
}

#endif // DRAW_H