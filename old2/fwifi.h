//UPDATED
#ifndef MYWIFI_H
#define MYWIFI_H

/**
 * @brief Establishes a connection to the specified Wi-Fi network.
 * Attempts to connect up to a maximum number of retries.
 */
void ConnectWifi()
{
    // Print the connection initialization message to the Serial Monitor
    writeLogln("Connecting to WiFi: " + String(WIFI_SSID));

    // Initialize the Wi-Fi connection using credentials defined globally
    WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

    // Counter to keep track of the current connection attempt
    int tentativas = 0;
    
    // Maximum number of attempts allowed before timing out (40 * 500ms = 20 seconds)
    int maxtentativas = 40;

    delay(500);

    // Loop runs while Wi-Fi is not connected AND the maximum number of attempts has not been reached
    while (WiFi.status() != WL_CONNECTED && tentativas < maxtentativas)
    {
        // Wait for 500 milliseconds between each connection check
        delay(500);
        
        // Character buffer to store the formatted status message
        char buffer[32];
        
        // Format the current attempt message (tentativas + 1 makes it 1-indexed for better readability)
        sprintf(buffer, "Attempt %d of %d", tentativas + 1, maxtentativas);
        
        // Print the current attempt progress to the Serial Monitor
        Serial.println(buffer);

        // Increment the attempt counter
        tentativas++;
    }

    // Check if the connection was successfully established after the loop
    if (WiFi.status() == WL_CONNECTED)
    {
        Serial.println();
        // Print success message and the assigned local IP address
        writeLogln("Connected! IP: " + WiFi.localIP().toString());
    }
    else
    {
        // Print an error message if the loop finished without connecting (Timeout)
        writeLogln("ERROR Connecting to WiFi!");
    }
}

#endif