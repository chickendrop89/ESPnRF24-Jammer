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
#include <RF24.h>

#include "src/jammer.h"
#include "src/server.h"
#include "src/main.h"
#include "src/display.h"

SPIClass *hspi_ptr;
SPIClass *fspi_ptr;

RF24 radio(RADIO_CE_PIN, RADIO_CSN_PIN, RADIO_CLOCK_SPEED);
RF24 radio1(RADIO1_CE_PIN, RADIO1_CSN_PIN, RADIO_CLOCK_SPEED);

rf24_pa_dbm_e getPowerLevel() {
  switch (radio_txpower_method) {
    case 0:
      return RF24_PA_MIN;
    case 1:
      return RF24_PA_LOW;
    case 2:
      return RF24_PA_HIGH;
    case 3:
      return RF24_PA_MAX;
    default: 
      return RF24_PA_MAX;
  }
}

#if defined(ALLOW_SLEEP)
void toggleRadioSleep(RF24 &radio, bool sleep, int num) {
  if (sleep){
      Serial.printf("Powering down a radio %d...\n", num);
      radio.powerDown();
  } else {
      Serial.printf("Waking up a radio %d...\n", num);
      radio.powerUp();
      delay(10); // 2x the recommended delay
  }
}
#endif // ALLOW_SLEEP

void configureRadio(RF24 &radio) {
  radio.setAutoAck(false);
  radio.stopListening();
  radio.setRetries(0, 0);
  radio.setRadiation(getPowerLevel(), RF24_2MBPS, false);
  radio.setCRCLength(RF24_CRC_DISABLED);
}

bool checkRadioStatus() {
  if (!radio.isChipConnected()) {
    Serial.println("ERROR: Radio 0 has disconnected. Stopping jammer.");
      
    #if defined(HAS_OLED_SCREEN)
    printOnDisplay("Radio 0 disconnected", "Jammer stopped");
    delay(3000);
    #endif

    jammerStopFlag = true;
    return false;
  }
  if (!radio1.isChipConnected()) {
    Serial.println("ERROR: Radio 1 has disconnected. Stopping jammer.");

    #if defined(HAS_OLED_SCREEN)
    printOnDisplay("Radio 1 disconnected", "Jammer stopped");
    delay(3000);
    #endif
      
    jammerStopFlag = true;
    return false;
  }
  return true;
}

void SPI_init() {
  Serial.printf("Setting up FSPI/VSPI and HSPI buses...");
  isSleeping = false;

  #if defined(HAS_OLED_SCREEN)
  printOnDisplay("Initializing radios");
  #endif

  #if defined(ALLOW_SLEEP)
  toggleRadioSleep(radio, false, 0);
  #endif
  hspi_ptr = new SPIClass(RADIO_SPI_BUS);
  hspi_ptr->begin(RADIO_SCK_PIN,
    RADIO_MISO_PIN, 
    RADIO_MOSI_PIN);
  hspi_ptr->setFrequency(RADIO_CLOCK_SPEED);
  hspi_ptr->setBitOrder(MSBFIRST);
  hspi_ptr->setDataMode(SPI_MODE0);

  #if defined(ALLOW_SLEEP)
  toggleRadioSleep(radio1, false, 1);
  #endif
  fspi_ptr = new SPIClass(RADIO1_SPI_BUS);
  fspi_ptr->begin(RADIO1_SCK_PIN,
    RADIO1_MISO_PIN,
    RADIO1_MOSI_PIN);
  fspi_ptr->setFrequency(RADIO_CLOCK_SPEED);
  fspi_ptr->setBitOrder(MSBFIRST);
  fspi_ptr->setDataMode(SPI_MODE0);

  auto [radio0_ok, radio1_ok] = std::make_pair(false, false);

  while (!radio0_ok || !radio1_ok) {
    Serial.println("Attempting to initialize radios...");

    if (!radio0_ok && radio.begin(fspi_ptr)) {
      Serial.println("Radio 0 successfully initialized!");
      radio0_ok = true;
    } else if (!radio0_ok) {
      Serial.printf("Radio 0 failed to initialize. Retrying in %d milliseconds...\n",
        RADIO_POLLING_MS
      );
    }
    if (!radio1_ok && radio1.begin(hspi_ptr)) {
      Serial.println("Radio 1 successfully initialized!");
      radio1_ok = true;
    } else if (!radio1_ok) {
      Serial.printf("Radio 1 failed to initialize. Retrying in %d milliseconds...\n",
        RADIO_POLLING_MS
      );
    }

    if (!radio0_ok || !radio1_ok) {
      delay(RADIO_POLLING_MS);
    }
  }

  configureRadio(radio);
  configureRadio(radio1);

  Serial.println("Radio 0 Details:");
  radio.printDetails();
  Serial.println("Radio 1 Details:");
  radio1.printDetails();
}
