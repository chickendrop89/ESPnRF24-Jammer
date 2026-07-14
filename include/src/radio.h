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
#ifndef RADIO_H
#define RADIO_H

#include <RF24.h>

rf24_pa_dbm_e getPowerLevel();
void toggleRadioSleep(RF24 &radio, bool sleep, int num);
bool checkRadioStatus();

extern RF24 radio;
extern RF24 radio1;

#endif // RADIO_H
