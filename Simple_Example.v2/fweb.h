#pragma once
// ===========================================================
// fweb.h
// Recebe pedidos HTTP GET do tipo:
//   http://<ip>/?screen=2
// e atualiza a variável global "RequestedScreen" (declarada em
// screen_global_vars.h) para o número pedido.
//
// Requer que "RequestedScreen" exista como variável externa, e que
// "Config" (struct Configuration) já tenha sido carregada via
// LoadConfigurationFromJson antes de InitWebServer() aceitar pedidos
// (Config.screenCount é usado para validar o range de "screen").
// ===========================================================

#include <FS.h>          // Necessario incluir antes de WebServer.h (core ESP32 3.x)
using fs::FS;            // TFT_eSPI define FS_NO_GLOBALS, que esconde este alias -> repor aqui
#include <WebServer.h>

extern int RequestedScreen; // Pointer state tracking active runtime display page
extern Configuration Config; // Config.screenCount = numero de ecras carregados

WebServer httpServer(80);   // Servidor HTTP a correr na porta 80

// -----------------------------------------------------------
// Handler chamado quando chega um pedido a "/"
// -----------------------------------------------------------
void HandleRootRequest()
{
    if (httpServer.hasArg("screen"))
    {
        String screenParam = httpServer.arg("screen");
        int requested = screenParam.toInt();

        // toInt() devolve 0 se a string não for numérica -> valida também isso
        bool isNumeric = screenParam.length() > 0;
        for (unsigned int i = 0; i < screenParam.length() && isNumeric; i++)
        {
            if (!isDigit(screenParam[i]))
            {
                isNumeric = false;
            }
        }

        if (isNumeric && requested >= 1 && requested <= Config.screenCount)
        {
            RequestedScreen = requested;
            writeLogln("HTTP: RequestedScreen alterado para " + String(RequestedScreen));

            httpServer.send(200, "text/plain",
                             "OK - RequestedScreen = " + String(RequestedScreen));
        }
        else
        {
            writeLogln("HTTP: pedido de screen invalido: '" + screenParam + "'");
            httpServer.send(400, "text/plain",
                             "Erro - 'screen' deve ser um numero entre 1 e " + String(Config.screenCount));
        }
    }
    else
    {
        // Sem parametro "screen": devolve o estado atual
        httpServer.send(200, "text/plain",
                         "RequestedScreen atual = " + String(RequestedScreen) +
                             " (usa ?screen=N para mudar, N entre 1 e " + String(Config.screenCount) + ")");
    }
}

// -----------------------------------------------------------
// Handler para qualquer rota nao definida
// -----------------------------------------------------------
void HandleNotFoundRequest()
{
    httpServer.send(404, "text/plain", "Not found");
}

// -----------------------------------------------------------
// Inicializa o servidor HTTP. Chamar em setup(), depois do WiFi ligado.
// -----------------------------------------------------------
void InitWebServer()
{
    httpServer.on("/", HTTP_GET, HandleRootRequest);
    httpServer.onNotFound(HandleNotFoundRequest);
    httpServer.begin();
    writeLogln("Web server iniciado na porta 80");
}

// -----------------------------------------------------------
// Tem de ser chamado frequentemente (loop() ou DataTask()) para
// processar pedidos recebidos.
// -----------------------------------------------------------
void HandleWebServer()
{
    httpServer.handleClient();
}