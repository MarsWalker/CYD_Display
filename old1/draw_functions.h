//UPDATED
#ifndef DRAW_H
#define DRAW_H

#include <TFT_eSPI.h>
#include <HTTPClient.h>   // For network JPG streaming/downloading
#include <WiFiClient.h>
#include <WiFiClientSecure.h>
#include <SD.h>           // For fetching JPG assets from microSD storage
#include <JPEGDEC.h> // For high-performance JPG decoding/rendering
//#include <TJpg_Decoder.h> // For high-performance JPG decoding/rendering

// External reference to the TFT display instance instantiated in the main application
extern TFT_eSPI tft;

// ============================================================================
// FORWARD DECLARATIONS
// Necessary because high-level composite functions like DrawSmartImage() call 
// underlying rendering pipelines defined further down in this header file.
// ============================================================================
void DrawImage(int x, int y, const unsigned char* bitmap, int w, int h, uint16_t color, uint8_t zoom);
void DrawJPG(int x, int y, const String& url);
void DrawJPGFromSD(int x, int y, String path);
void DrawSmartImage(int x, int y, String source, uint16_t color, uint8_t zoom, bool isColorDefined = false);

JPEGDEC jpeg;

// ============================================================================
// CORE GRAPHICS PRIMITIVES
// ============================================================================

int JPEGDraw(JPEGDRAW *pDraw)
{
    tft.pushImage(pDraw->x, pDraw->y, pDraw->iWidth, pDraw->iHeight, pDraw->pPixels);
    return 1; // continuar a descodificar
}

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
// ===========================================================
// Funções de acesso a ficheiro para a JPEGDEC ler diretamente do SD
// ===========================================================
File jpegSdFile;

void *jpegOpenSD(const char *filename, int32_t *size)
{
    jpegSdFile = SD.open(filename);
    if (!jpegSdFile)
        return nullptr;
    *size = jpegSdFile.size();
    return &jpegSdFile;
}

void jpegCloseSD(void *handle)
{
    if (jpegSdFile)
        jpegSdFile.close();
}

int32_t jpegReadSD(JPEGFILE *pFile, uint8_t *buffer, int32_t length)
{
    if (!jpegSdFile)
        return 0;
    return jpegSdFile.read(buffer, length);
}

int32_t jpegSeekSD(JPEGFILE *pFile, int32_t position)
{
    if (!jpegSdFile)
        return 0;
    return jpegSdFile.seek(position) ? position : -1;
}

void DrawJPGFromSD(int x, int y, String path)
{
    // Hardware asset presence verification sanity check
    if (!SD.exists(path))
    {
        Serial.printf("DrawJPGFromSD Error: File resource target not found at context path '%s'\n", path.c_str());
        return;
    }

    if (jpeg.open(path.c_str(), jpegOpenSD, jpegCloseSD, jpegReadSD, jpegSeekSD, JPEGDraw))
    {
        jpeg.setPixelType(RGB565_BIG_ENDIAN);
        if (!jpeg.decode(x, y, 0))
        {
            Serial.printf("Erro na descodificação JPEG (SD): %d\n", jpeg.getLastError());
        }
        jpeg.close();
    }
    else
    {
        Serial.printf("DrawJPGFromSD Error: falha ao abrir '%s'\n", path.c_str());
    }
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
// CLOUD NETWORK/HTTP STREAMING IMAGE ENGINE
// ============================================================================

/**
 * @brief Streams an online remote JPG asset over HTTP, stores temporary payloads into internal heap
 * RAM segments, and calls the rendering pipeline to decompress live network arrays onto screens.
 * @warning Extreme caution is advised on devices with limited SRAM allocations (e.g., standard ESP32). 
 * Large images may trigger heap fragmentations or Fatal Out-Of-Memory Panic Crashes.
 */

void DrawJPG(int x, int y, const String &url)
{
    if (url.isEmpty())
        return;

    String targetUrl = url;
    HTTPClient http;
    WiFiClientSecure clientSecure;
    WiFiClient clientPlain;

    Serial.printf("DrawJPG: %s | heap livre: %u\n", targetUrl.c_str(), ESP.getFreeHeap());

    // --- PASSO 1: Detetar e seguir redirecionamentos manualmente ---
    bool redirected = true;
    int redirectCount = 0;
    
    // Desativamos o follow automático para controlar a memória
    http.setFollowRedirects(HTTPC_DISABLE_FOLLOW_REDIRECTS);
    http.setTimeout(5000);

    while (redirected && redirectCount < 3) // Limite de 3 redirecionamentos
    {
        bool https = targetUrl.startsWith("https://");
        if (https) {
            clientSecure.setInsecure();
            http.begin(clientSecure, targetUrl);
        } else {
            http.begin(clientPlain, targetUrl);
        }

        int httpCode = http.GET();
        
        // Se for redirecionamento (301, 302, 303, 307, 308)
        if (httpCode >= 300 && httpCode >= 308) {
            String newUrl = http.header("Location");
            if (newUrl.isEmpty()) {
                // Algumas APIs devolvem no formato normal de cabeçalhos, temos de pedir explicitamente
                const char* headerKeys[] = {"Location"};
                http.collectHeaders(headerKeys, 1);
                // Refaz o GET para apanhar o header
                httpCode = http.GET();
                newUrl = http.header("Location");
            }
            
            if (!newUrl.isEmpty()) {
                Serial.printf("A redirecionar para: %s\n", newUrl.c_str());
                targetUrl = newUrl;
                redirectCount++;
                http.end(); // Fecha a ligação anterior para libertar RAM antes da próxima
            } else {
                redirected = false;
            }
        } else {
            redirected = false; // Não é redirecionamento, prossegue com o download
        }
    }

    // --- PASSO 2: Fazer o download do destino final ---
    // Reinicia a ligação se fechada no ciclo acima
    if (!http.connected()) {
        bool https = targetUrl.startsWith("https://");
        if (https) {
            clientSecure.setInsecure();
            http.begin(clientSecure, targetUrl);
        } else {
            http.begin(clientPlain, targetUrl);
        }
        int httpCode = http.GET();
        if (httpCode != HTTP_CODE_OK) {
            Serial.printf("HTTP Error no destino final: %d (%s)\n", httpCode, http.errorToString(httpCode).c_str());
            http.end();
            return;
        }
    }

    int totalLen = http.getSize();
    if (totalLen <= 0)
    {
        Serial.println("Unknown content length.");
        http.end();
        return;
    }

    uint8_t *jpgBuffer = (uint8_t *)heap_caps_malloc(totalLen, MALLOC_CAP_SPIRAM);
    if (!jpgBuffer)
        jpgBuffer = (uint8_t *)malloc(totalLen);

    if (!jpgBuffer)
    {
        Serial.printf("Out of memory (precisava de %d bytes, heap livre: %u)\n",
                      totalLen, ESP.getFreeHeap());
        http.end();
        return;
    }

    WiFiClient *stream = http.getStreamPtr();
    int bytesRead = 0;
    while (http.connected() && bytesRead < totalLen)
    {
        int available = stream->available();
        if (available > 0)
        {
            bytesRead += stream->readBytes(
                jpgBuffer + bytesRead,
                min(available, totalLen - bytesRead));
        }
        delay(1);
    }

    if (bytesRead == totalLen)
    {
        if (jpeg.openRAM(jpgBuffer, totalLen, JPEGDraw))
        {
            jpeg.setPixelType(RGB565_BIG_ENDIAN);
            jpeg.setUserPointer(nullptr);
            if (!jpeg.decode(x, y, 0))
            {
                Serial.printf("Erro na descodificação JPEG: %d\n", jpeg.getLastError());
            }
            jpeg.close();
        }
        else
        {
            Serial.println("openRAM falhou.");
        }
    }
    else
    {
        Serial.printf("Incomplete download (%d/%d)\n", bytesRead, totalLen);
    }

    free(jpgBuffer);
    http.end();
}
#endif // DRAW_H