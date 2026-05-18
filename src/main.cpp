/*
 * A low-power 2,4GHz wireless jammer based on `ESP32` and `nRF24LO1+PA+LNA`
 * Copyright (C) 2025 chickendrop89
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
*/
#include <FreeRTOS.h>
#include <esp_bt.h>
#include <SPIFFS.h>
#include <Preferences.h>
#include <DNSServer.h>
#include "terminal.h"
#include "src/main.h"
#include "src/jammer.h"
#include "src/server.h"
#include "src/display.h"
#include "src/radio.h"

volatile bool jammerStopFlag = false;
#if defined(ALLOW_SLEEP)
volatile bool isSleeping = false;
#endif

int channel_method;
String ssid_method, password_method;

TaskHandle_t jammerTaskHandle = NULL;
SemaphoreHandle_t jammerSemaphore;
Preferences preferences;
DNSServer dnsServer;

void halt_esp(){
    Serial.println("ESP halted. Check previous logs");
    esp_deep_sleep_start();
};

void loadPreferences() {
    preferences.begin(NVS_NAME, false);

    // Integers
    bluetooth_jam_method = preferences.getInt(
        "nvs_bluetooth_jam", 0 );
    drone_jam_method = preferences.getInt(
        "nvs_drone_jam", 0 );
    nrf24_module_config_method = preferences.getInt(
        "nvs_nrf24_cfg", 0 );
    misc_jam_method = preferences.getInt(
        "nvs_misc_jam", 0 );
    radio_txpower_method = preferences.getInt(
        "nvs_txpower", DEFAULT_TXPOWER_METHOD
    );

    // Strings
    ssid_method = preferences.getString(
        "wifi_ssid", DEFAULT_SSID );
    password_method = preferences.getString(
        "wifi_pass", DEFAULT_PASSWORD );
    channel_method = preferences.getInt(
        "wifi_channel", DEFAULT_CHANNEL 
    );
    preferences.end();
}

void jammerTask(void* pvParameters) {
    Serial.println("Jammer task started on Core 1.");
    
    // Smart pointer
    std::unique_ptr<JammerTaskData> dataPtr(
        static_cast<JammerTaskData*>(pvParameters)
    );
    
    SPI_init(); 
    jammerStopFlag = false;

    if (dataPtr->action) {
        dataPtr->action();
    }

    Serial.println("Jammer task finished.");
    stop_jammer();

    if (xSemaphoreTake(jammerSemaphore, portMAX_DELAY) == pdTRUE) {
        jammerTaskHandle = NULL;
        xSemaphoreGive(jammerSemaphore);
    }
    vTaskDelete(NULL);
}

void stopJamHandler() {
    Serial.println("Stop jam handler called.");
    
    if (xSemaphoreTake(jammerSemaphore, portMAX_DELAY) == pdTRUE) {
        if (jammerTaskHandle != NULL) {
            Serial.println("Signaling current jammer task to stop.");
            jammerStopFlag = true;
        } else {
            Serial.println("No jammer task is currently running.");
        }
        xSemaphoreGive(jammerSemaphore);
    }
}

#if defined(ALLOW_SLEEP)
void SleepTimeoutTask(void* _parameter) {
    while(true) {
        uint64_t currentTime = millis();
        static uint64_t lastActivityTime = currentTime;

        // Check if there's any jammer activity
        if (jammerTaskHandle != NULL) {
            lastActivityTime = currentTime;
        }

        if (!isSleeping && (currentTime - lastActivityTime) >= SLEEP_TIMEOUT) {
            Serial.printf("Sleeping after %dms of inactivity\n", SLEEP_TIMEOUT);
                
            // Put radios to sleep
            toggleRadioSleep(radio, true, 0);
            toggleRadioSleep(radio1, true, 1);
            
            #if defined(HAS_OLED_SCREEN) && defined(SLEEP_DISPLAY)
            showSleepIcon();
            #endif

            isSleeping = true;
        }
        vTaskDelay(pdMS_TO_TICKS(10000)); 
    }
}
#endif // ALLOW_SLEEP

void setup() {
    Serial.begin(SERIAL_BAUD_RATE);

    /*
     * Won't be using the bluetooth stack in this project.
     * Save power by disabling the controller.
    */
    esp_bt_controller_disable();

    if (!SPIFFS.begin(true)) {
        Serial.println("An error has occurred while mounting SPIFFS");
        halt_esp();
    }
    Serial.println("SPIFFS mounted successfully.");
    Serial.println("Trying to initialise terminal");
    terminalInit();
    loadPreferences();
    Serial.println("Non-volatile settings loaded.");

    WiFi.softAPConfig(
        BOARD_ADDRESS, BOARD_ADDRESS,
        IPAddress(255, 255, 255, 0)
    );
    
    if (!WiFi.softAP(
    ssid_method.c_str(), 
    password_method.c_str(),
    channel_method,
    HIDDEN_SSID)) {
        Serial.println("Failed to start Access Point!");
        halt_esp();
    }
    Serial.printf("Started the access point: %s\n", ssid_method.c_str());
    
    #if defined(CAPTIVE_PORTAL)
    dnsServer.start(DNS_PORT, "*", BOARD_ADDRESS);
    #endif

    #if defined(BAD_BOARD_REVISION)
    WiFi.setTxPower(WIFI_POWER_8_5dBm);
    #else
    WiFi.setTxPower(WIFI_POWER_19_5dBm);
    #endif

    setupWebServer();
    Serial.println("HTTP server started.");

    jammerSemaphore = xSemaphoreCreateBinary();
    xSemaphoreGive(jammerSemaphore);

    #if defined(HAS_OLED_SCREEN)
    displayInit();
    Serial.printf("%dx%d OLED initialized.\n", DISPLAY_WIDTH, DISPLAY_HEIGHT);
    #endif

    #if defined(ALLOW_SLEEP)
    xTaskCreatePinnedToCore(
        SleepTimeoutTask,
        "SleepTimeoutMonitor",
        2048,
        NULL,
        0, // Low priority
        NULL,
        0
    );
    #endif
}

void loop() {
    terminalUpdate();
    #if defined(CAPTIVE_PORTAL)
    dnsServer.processNextRequest();
    #endif
    server.handleClient();
}
