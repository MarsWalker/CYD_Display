/*
 * logutils.h
 * -------------------------------------------------------
 * Funções de log (Serial + TFT) e um helper para fazer
 * pedidos HTTP/HTTPS conforme o URL fornecido.
 *
 * Incluir logo a seguir ao secrets.h:
 *
 *   #include "secrets.h"
 *   #include "logutils.h"
 *
 * IMPORTANTE:
 * - Assume que já existe um objecto global "tft" do tipo
 *   TFT_eSPI (o normal nos projectos CYD com TFT_eSPI).
 *   Se o teu sketch usar outro nome, muda a linha abaixo.
 * - Assume que WiFi já está ligado antes de chamar httpRequest().
 * -------------------------------------------------------
 */

#ifndef LOGUTILS_H
#define LOGUTILS_H

#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <HTTPClient.h>

// Se o objecto do TFT no teu .ino tiver outro nome, ajusta aqui:
extern TFT_eSPI tft;

// ---------------------------------------------------------------
// Coordenadas / configuração do log no ecrã
// ---------------------------------------------------------------
#define LOG_MARGIN_X      4      // margem esquerda (posição de reset do X)
#define LOG_START_Y       4      // posição Y inicial
#define LOG_LINE_HEIGHT   16     // altura de cada linha (depende do tamanho de letra)
#define LOG_TEXT_SIZE     1      // tamanho de letra usado no log
#define LOG_TEXT_COLOR    TFT_WHITE
#define LOG_BG_COLOR      TFT_BLACK

// Cursor actual do log no ecrã (vai avançando)
static int16_t logCursorX = LOG_MARGIN_X;
static int16_t logCursorY = LOG_START_Y;

// ---------------------------------------------------------------
// Reinicia o cursor do log (útil para limpar o ecrã e recomeçar)
// ---------------------------------------------------------------
inline void resetLog() {
    tft.fillScreen(LOG_BG_COLOR);
    logCursorX = LOG_MARGIN_X;
    logCursorY = LOG_START_Y;
}

// ---------------------------------------------------------------
// Se o cursor Y ultrapassar o ecrã, volta ao topo (simples "wrap")
// ---------------------------------------------------------------
inline void logCheckWrap() {
    if (logCursorY > tft.height() - LOG_LINE_HEIGHT) {
        logCursorX = LOG_MARGIN_X;
        logCursorY = LOG_START_Y;
        tft.fillScreen(LOG_BG_COLOR);
    }
}

// ---------------------------------------------------------------
// writeLog: escreve a mensagem no Serial e no TFT,
// e avança apenas o X (fica na mesma linha)
// ---------------------------------------------------------------
inline void writeLog(const String &msg) {
    Serial.print(msg);

    logCheckWrap();
    tft.setTextSize(LOG_TEXT_SIZE);
    tft.setTextColor(LOG_TEXT_COLOR, LOG_BG_COLOR);
    tft.setCursor(logCursorX, logCursorY);
    tft.print(msg);

    // avança X pelo comprimento do texto escrito
    logCursorX += tft.textWidth(msg);
}

// ---------------------------------------------------------------
// writeLogln: escreve a mensagem no Serial (com newline) e no TFT,
// e avança o Y para a linha seguinte, repondo o X na margem
// ---------------------------------------------------------------
inline void writeLogln(const String &msg) {
    Serial.println(msg);

    logCheckWrap();
    tft.setTextSize(LOG_TEXT_SIZE);
    tft.setTextColor(LOG_TEXT_COLOR, LOG_BG_COLOR);
    tft.setCursor(logCursorX, logCursorY);
    tft.print(msg);

    // nova linha: repõe X, avança Y
    logCursorX = LOG_MARGIN_X;
    logCursorY += LOG_LINE_HEIGHT;
}

// ---------------------------------------------------------------
// httpRequest: recebe um URL e escolhe automaticamente
// WiFiClientSecure (https) ou WiFiClient (http).
// Devolve o corpo da resposta como String (ou "" em caso de erro).
// httpCode (opcional) devolve o código HTTP de resposta.
// ---------------------------------------------------------------
inline String httpRequest(const String &url, int *httpCode = nullptr) {
    HTTPClient http;
    String payload = "";
    int code = -1;

    if (WiFi.status() != WL_CONNECTED) {
        writeLogln("WiFi nao ligado!");
        if (httpCode) *httpCode = code;
        return payload;
    }

    bool isHttps = url.startsWith("https://");

    if (isHttps) {
        WiFiClientSecure client;
        client.setInsecure();   // não valida certificado (simples). Trocar por setCACert() se precisares de validação.
        if (http.begin(client, url)) {
            code = http.GET();
            if (code > 0) {
                payload = http.getString();
            } else {
                writeLogln("Erro HTTPS: " + http.errorToString(code));
            }
            http.end();
        } else {
            writeLogln("Falha a iniciar ligacao HTTPS");
        }
    } else {
        WiFiClient client;
        if (http.begin(client, url)) {
            code = http.GET();
            if (code > 0) {
                payload = http.getString();
            } else {
                writeLogln("Erro HTTP: " + http.errorToString(code));
            }
            http.end();
        } else {
            writeLogln("Falha a iniciar ligacao HTTP");
        }
    }

    if (httpCode) *httpCode = code;
    return payload;
}

#endif // LOGUTILS_H