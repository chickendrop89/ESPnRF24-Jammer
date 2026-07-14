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
#ifndef SERVER_H
#define SERVER_H

#include <WebServer.h>

void setupWebServer();

extern WebServer server;
extern int bluetooth_jam_method;
extern int drone_jam_method;
extern int ble_jam_method;
extern int wifi_jam_method;
extern int zigbee_jam_method;
extern int misc_jam_method;
extern int nrf24_module_config_method;
extern int radio_txpower_method;
extern int channel_method;

struct FileServerOptions {
    const char* filePath;
    const char* placeholder = nullptr;
    const String* value = nullptr;
    const String* replacements = nullptr;
    std::function<void()> jamFunction = nullptr;
    int replacementCount = 0;
};

#endif // SERVER_H
