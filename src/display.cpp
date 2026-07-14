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
#include "options.h"

#if defined(HAS_OLED_SCREEN)
#include <Wire.h>
#include <Adafruit_SSD1306.h>

#include "src/jammer.h"
#include "src/main.h"
#include "src/display.h"
#include "src/radio.h"

// https://github.com/ropg/truetype2gfx
#include "fonts/PixeloidSans4pt.h"
#include "fonts/PixeloidSans6pt.h"

volatile uint64_t lastDisplayUpdate = 0;
Adafruit_SSD1306 display(DISPLAY_WIDTH, DISPLAY_HEIGHT, &Wire, -1);

auto getPowerLevelString() -> const char* {
  switch (getPowerLevel()) {
    case RF24_PA_MIN:
      return "Low TxP";
    case RF24_PA_LOW:
      return "Medium TxP";
    case RF24_PA_HIGH:
      return "High TxP";
    case RF24_PA_MAX:
      return "Maximum TxP";
  }
  return "Unknown";
}

void printCenteredText(const char* text, int height) {
  int16_t x1, y1;
  uint16_t w, h;

  display.getTextBounds(text, 0, 0, &x1, &y1, &w, &h);
  display.setCursor((DISPLAY_WIDTH - w) / 2, height);
  display.println(text);
}

void printOnDisplay(const char* header, const char* footer) {
  lastDisplayUpdate = millis();
  display.clearDisplay();

  /*
   * Center the text to the middle of the-
   * display if footer doesn't exist
  */
  if (!footer){
	  display.setFont(&PixeloidSans6pt7b);
    printCenteredText(header, 35);
  } else {
	  display.setFont(&PixeloidSans6pt7b);
    printCenteredText(header, 25);

    display.setFont(&PixeloidSans4pt7b);
    printCenteredText(footer, 55);
  }

  display.display();
}

void returnDisplayToIdle(bool IsTaskCanceled, bool isBooting) {
  display.clearDisplay();

  if(isBooting){
    printOnDisplay("Powered on!");
    delay(1000);
  }
  if(IsTaskCanceled){
    printOnDisplay("Task cancelled", CANCELED_JAMMER_FOOTER_MSG);
    delay(2000);
	}
	display.clearDisplay();

  display.drawBitmap(
    0, 0, interference_bitmap, 
    DISPLAY_WIDTH, DISPLAY_HEIGHT, 1
  );

  display.display();
}

#if defined(SLEEP_DISPLAY)
/*
 * Disply a short SLEEP_DISPLAY_MSG instead of the drawn 
 * bitmap to save some energy as we are working with an
 * OLED display.
*/
void showSleepIcon(){
  display.clearDisplay();

	display.setFont(&PixeloidSans6pt7b);
  display.setTextSize(1.5);
  printCenteredText(SLEEP_DISPLAY_MSG, 35);

  display.display();
}
#endif // SLEEP_DISPLAY

void displayInit() {
  Wire.begin(DISPLAY_SDA_PIN, DISPLAY_SCL_PIN);
  display.begin(SSD1306_SWITCHCAPVCC, DISPLAY_ADDRESS);

	/*
   * Set white text color even if 
   * display supports more colors
  */
  display.setTextColor(SSD1306_WHITE);

  // Custom brightness
  display.ssd1306_command(0x81);
  display.ssd1306_command(DISPLAY_BRIGHTNESS);
	
  // Display bitmap on startup
  returnDisplayToIdle(false, true);
}
#endif // HAS_OLED_SCREEN
