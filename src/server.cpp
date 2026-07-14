/*
 * A low-power 2,4GHz wireless jammer based on `ESP32` and `nRF24LO1+PA+LNA`
 * Copyright (C) 2025 chkndrp
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
#include <WebServer.h>
#include <SPIFFS.h>
#include <Preferences.h>
#include <mbedtls/sha256.h>

#include "src/jammer.h"
#include "src/server.h"
#include "src/main.h"

#include "options.h"

// Server methods
int bluetooth_jam_method, 
    drone_jam_method,
    ble_jam_method,
    wifi_jam_method,
    zigbee_jam_method,
    misc_jam_method,
    nrf24_module_config_method,
    radio_txpower_method;

WebServer server(WEBSERVER_PORT);

/*
 * RAII guard to close the SPIFFS file 
 * handle on exit to prevent mem leaks
*/
class SPIFFSFileGuard {
private:
    File& file;
    bool owned;
public:
    explicit SPIFFSFileGuard(File& f) : file(f), owned(true) {}
    SPIFFSFileGuard(SPIFFSFileGuard&& other) : file(other.file), owned(other.owned) {
        other.owned = false;
    }
    ~SPIFFSFileGuard() {
        if (owned && file) {
            file.close();
        }
    }
    SPIFFSFileGuard(const SPIFFSFileGuard&) = delete;
    SPIFFSFileGuard& operator=(const SPIFFSFileGuard&) = delete;
};

template<typename T>
void storePreferencesAndSet(const char* key, T value, T& targetVar) {
    preferences.begin(NVS_NAME, false);
    
    if constexpr (std::is_same_v<T, int>) {
        Serial.printf("Storing NVS value: key=%s, value=%d\n", key, value);
        preferences.putInt(key, value);
    } else if constexpr (std::is_same_v<T, String>) {
        Serial.printf("Storing NVS value: key=%s, value=%s\n", key, value.c_str());
        preferences.putString(key, value);
    }

    preferences.end();
    targetVar = value;
}

String getContentType(String filename) {
  if (filename.endsWith(".html")) 
    return "text/html";
  if (filename.endsWith(".css")) 
    return "text/css";
  if (filename.endsWith(".ttf")) 
    return "font/ttf";
  if (filename.endsWith(".js")) 
    return "text/javascript";
  return "text/plain";
}

/*
 * Generates SHA-256 hash of a SPIFFS file
 * so that it can be later used for an eTag
*/
String generateETag(File &file) {
  file.seek(0); 

  mbedtls_sha256_context sha_context;
  mbedtls_sha256_init(&sha_context);
  mbedtls_sha256_starts_ret(&sha_context, 0);

  uint8_t buffer[1024];
  int bytesRead;

  while ((bytesRead = file.read(buffer, sizeof(buffer))) > 0) {
    mbedtls_sha256_update_ret(&sha_context, buffer, bytesRead);
  }

  uint8_t hash[32];
  mbedtls_sha256_finish_ret(&sha_context, hash);
  mbedtls_sha256_free(&sha_context);

  file.seek(0); 

  String etag = "\"";
  int i;

  for (i = 0; i < sizeof(hash); i++) {
    char hex[3];
    sprintf(hex, "%02x", hash[i]);
    etag += hex;
  }
  etag += "\"";

  return etag;
}

void handleFileReqOrRedirect() {
    String path = server.uri();

    if (path.endsWith("/")) {
        path += ENTRY_PAGE;
    }
    if (path.endsWith("/favicon.ico")) {
      server.send(200, "image/x-icon", "");
    };

    if (SPIFFS.exists(path)) {
        File file = SPIFFS.open(path, "r");
        if (file) {
            String etagFromClient = "";
            if (server.hasHeader("If-None-Match")) {
                etagFromClient = server.header("If-None-Match");
            }
            String currentETag = generateETag(file);

            if (etagFromClient == currentETag) {
                server.send(304, "text/plain", "");
            } else {
                server.sendHeader("Cache-Control", "public, no-cache");
                server.sendHeader("ETag", currentETag);

                String contentType = getContentType(path);
                server.streamFile(file, contentType);
                file.close();
            }
            return;
        }
    }
    server.sendHeader("Location", "/" + String(ENTRY_PAGE));
    server.send(302);
    return;
}

void sendHtmlAndExecuteAsync(const char* htmlResponse, std::function<void()> jamFunction) {
    server.send(200, "text/html", htmlResponse);

    /*
     * Handle moment where user requests to run jammerTask, without 
     * the previous one being properly cancelled via StopJamHandler()
    */
    if (xSemaphoreTake(jammerSemaphore, 0) == pdTRUE) {
        if (jammerTaskHandle != NULL) {
            Serial.println("Stopping previous jammer task gracefully.");
            jammerStopFlag = true;
        }
        xSemaphoreGive(jammerSemaphore);
        vTaskDelay(pdMS_TO_TICKS(100));
    }
    
    Serial.println("Creating new jammer task on Core 1.");
    jammerStopFlag = false;

    auto dataPtr = std::make_unique<JammerTaskData>();
    dataPtr->action = jamFunction;

    // Run JammerTask on the second core with max priority
    xTaskCreatePinnedToCore(
        jammerTask,
        "JammerTask",
        4096,
        dataPtr.release(),
        1,
        &jammerTaskHandle,
        1);
}

bool handleFileOperation(const FileServerOptions& options) {
    if (!SPIFFS.exists(options.filePath)) {
        server.send(404);
        return false;
    }

    File file = SPIFFS.open(options.filePath, "r");
    if (!file) {
        server.send(500);
        return false;
    }

    SPIFFSFileGuard guard(file);

    String currentETag = generateETag(file);
    String etagFromClient = server.hasHeader("If-None-Match") ? 
                            server.header("If-None-Match") : "";

    if (etagFromClient == currentETag) {
        server.send(304, "text/plain", "");
        return true;
    }

    server.sendHeader("Cache-Control", "public, no-cache");
    server.sendHeader("ETag", currentETag);

    String htmlContent = file.readString();

    if (options.placeholder && options.value) {
        htmlContent.replace(options.placeholder, *options.value);
    }
    if (options.replacements && options.replacementCount > 0) {
        int i;
        for (i = 0; i < options.replacementCount * 2; i += 2) {
            htmlContent.replace(options.replacements[i], options.replacements[i + 1]);
        }
    }

    if (options.jamFunction) {
        sendHtmlAndExecuteAsync(htmlContent.c_str(), options.jamFunction);
    } else {
        server.send(200, getContentType(options.filePath), htmlContent);
    }
    return true;
}

void serveFile(const char* filePath, std::function<void()> jamFunction = nullptr) {
    FileServerOptions options;
    options.filePath = filePath;
    options.jamFunction = jamFunction;

    handleFileOperation(options);
}

void serveDynamicFile(const char* filePath, std::initializer_list<std::pair<const char*, String>> replacements) {
    auto replacementsArray = std::unique_ptr<String[]>(new String[replacements.size() * 2]);
    
    FileServerOptions options;
    options.filePath = filePath;

    int i = 0;
    for (const auto& pair : replacements) {
        replacementsArray[i * 2] = pair.first;
        replacementsArray[i * 2 + 1] = pair.second;
        i++;
    }
    
    options.replacements = replacementsArray.get();
    options.replacementCount = replacements.size();
    
    handleFileOperation(options);
}

void serveSettingsPage(const char* filePath, int currentMethod, int numMethods) {
    auto replacements = std::make_unique<String[]>(numMethods * 2);
    
    int i;
    for (i = 0; i < numMethods; ++i) {
        replacements[i * 2] = "[ACTIVE_CLASS_" + String(i) + "]";
        replacements[i * 2 + 1] = (currentMethod == i) ? "active-button" : "";
    }

    FileServerOptions options;
    options.filePath = filePath;
    options.replacements = replacements.get();
    options.replacementCount = numMethods;
    
    handleFileOperation(options);
}

void registerMethodHandlers(const char* urlPrefix, const char* key, int& targetVar, int numMethods) {
    int i;
    for (i = 0; i < numMethods; ++i) {
        String url = String(urlPrefix) + "_" + String(i);
        server.on(url.c_str(), [key, i, &targetVar, urlPrefix]() {
            /*
             * Validation for drone_method_ as it should not 
             * accept 0 as value because it does not use channels
            */
            if (strcmp(urlPrefix, "/drone_method") == 0 && i == 0) {
                server.send(400);
                return;
            }
            storePreferencesAndSet(key, i, targetVar);
            handleFileReqOrRedirect();
        });
    }
}

void setupWebServer() {
    /*
     * Main pages
    */
    server.on("/", []() {
        server.sendHeader("Location", "/" + String(ENTRY_PAGE));
        server.send(302);
    });
    server.on("/wifi_select", []() {
        serveFile("/wifi_select.html");
    });
    server.on("/bluetooth_jam", []() {
        serveFile("/bluetooth_jam.html", bluetooth_jam);
    });
    server.on("/drone_jam", []() {
        serveFile("/drone_jam.html", drone_jam);
    });
    server.on("/wifi_jam", []() {
        serveFile("/wifi_jam.html", wifi_jam);
    });
    server.on("/ble_jam", []() {
        serveFile("/ble_jam.html", ble_jam);
    });
    server.on("/zigbee_jam", []() {
        serveFile("/zigbee_jam.html", zigbee_jam);
    });
    server.on("/misc_select", []() {
        serveFile("/misc_select.html");
    });
    server.on("/misc_jam", []() {
        int channel1 = server.arg("start").toInt();
        int channel2 = server.arg("stop").toInt();
        serveFile("/misc_jam.html", [channel1, channel2]() { 
            misc_jam(channel1, channel2); 
        });
    });
    server.on("/wifi_selected_jam", []() {
        int channel = server.arg("channel").toInt();
        serveFile("/wifi_jam.html", [channel]() { 
            wifi_channel(channel); 
        });
    });
    server.on("/stop_jam", []() {
        handleFileReqOrRedirect();
        stopJamHandler();
    });

    /*
     * Settings pages
    */
    server.on("/settings_bluetooth_jam", []() {
        serveSettingsPage("/settings/bluetooth_jam.html", bluetooth_jam_method, 3);
    });
    server.on("/settings_drone_jam", []() {
        serveSettingsPage("/settings/drone_jam.html", drone_jam_method, 3);
    });
    server.on("/settings_nrf24_config", []() {
        serveSettingsPage("/settings/nrf24_config.html", nrf24_module_config_method, 2);
    });
    server.on("/settings_misc_jam", []() {
        serveSettingsPage("/settings/misc_jam.html", misc_jam_method, 2);
    });
    server.on("/wifi_channel", []() {
        serveFile("/wifi_channel.html");
    });
    server.on("/wifi_settings", []() {
        serveFile("/wifi_settings.html");
    });
    server.on("/settings_txpower_config", []() {
        serveDynamicFile("/settings/txpower_config.html", {
            {"[TXPOWER_VALUE]", String(radio_txpower_method)}
        });
    });
    server.on("/settings_access_point", []() {
        serveDynamicFile("/settings/access_point.html", {
            {"[SSID_VALUE]", String(ssid_method)},
            {"[PASSWORD_VALUE]", String(password_method)},
            {"[CHANNEL_VALUE]", String(channel_method)}
        });
    });

    /*
     * Settings methods
    */
    server.on("/radio_txpower_method", []() {
        int slider_val = server.arg("current_val").toInt();
        storePreferencesAndSet("txpower_method", slider_val, radio_txpower_method);
        handleFileReqOrRedirect();
    });
    server.on("/access_point_method", [](){
        int channel = server.arg("channel").toInt();
        String ssid = server.arg("ssid");
        String password = server.arg("password");

        /*
         * Backend validation so the board doesn't 
         * fail to setup if someone bypasses the HTML
         * input field validation somewhy
        */
        if (channel < 1 || channel > 13)
            channel = channel_method;
        else if (ssid.length() < 1 || ssid.length() > 32)
            ssid = ssid_method;
        else if (password.length() < 8 || password.length() > 63)
            password = password_method;

        storePreferencesAndSet(
            "wifi_channel", channel, channel_method
        );
        storePreferencesAndSet(
            "wifi_ssid", ssid, ssid_method
        );
        storePreferencesAndSet(
            "wifi_pass", password, password_method
        );
        handleFileReqOrRedirect();
        
        /* 
         * Client may be requesting resources. Delay for 2s
         * Reboot to apply SoftAP changes
        */
        delay(2000);
        ESP.restart();
    });

    registerMethodHandlers(
        "/bluetooth_method", 
        "nvs_bluetooth_jam", 
        bluetooth_jam_method, 3
    );
    registerMethodHandlers(
        "/drone_method",    
        "nvs_drone_jam", 
        drone_jam_method, 3
    );
    registerMethodHandlers(
        "/nrf24_config_method",
        "nvs_nrf24_cfg",
        nrf24_module_config_method, 2
    );
    registerMethodHandlers(
        "/misc_method", 
        "nvs_misc_jam", 
        misc_jam_method, 2
    );

    server.onNotFound(handleFileReqOrRedirect);
    server.begin();
}
