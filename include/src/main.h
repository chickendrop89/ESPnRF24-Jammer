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
#ifndef MAIN_H
#define MAIN_H

#include <Preferences.h>

void jammerTask(void* pvParameters); 
void stopJamHandler();

extern volatile bool jammerStopFlag;
extern volatile bool isSleeping;

extern String ssid_method;
extern String password_method;

extern TaskHandle_t jammerTaskHandle;
extern SemaphoreHandle_t jammerSemaphore;
extern Preferences preferences;

struct JammerTaskData {
    std::function<void()> action;
};

#endif // MAIN_H
