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
#ifndef JAMMER_H
#define JAMMER_H

void SPI_init();
void bluetooth_jam();
void drone_jam();
void ble_jam();
void wifi_jam();
void wifi_channel(int channel);
void zigbee_jam();
void misc_jam(int channel1, int channel2);
void stop_jammer();

enum class ChannelType {
    BLUETOOTH,
    BLE, NONE
};

#endif // JAMMER_H
