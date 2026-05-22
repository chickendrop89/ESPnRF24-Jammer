#ifndef OPTIONS_H
#define OPTIONS_H

/*
 * nRF24 radio pins.
 * 
 * RADIO_POLLING_MS: defines how many milliseconds
 * to wait before retrying to initialize radio again
 * (if the first attempt failed)
 * 
 * RADIO_CLOCK_SPEED: values above 10MHz can increase
 * performance, but should be used only with short wires.
 * Otherwise will cause problems
 * 
 * DEFAULT_TXPOWER_METHOD: 0 = MIN, 3 = MAX
 * https://nrf24.github.io/RF24/group__PALevel.html
 * 
 * (!) If using FSPI Bus, look up your board 
 * pinout for their respective GPIO pins as they can
 * not be mapped just anywhere unlike HSPI.
 */
#define RADIO_POLLING_MS 1000 // 1s
#define RADIO_CLOCK_SPEED 12000000 // 12MHz
#define DEFAULT_TXPOWER_METHOD 3 // RF24_PA_MAX

#define RADIO_CE_PIN 1
#define RADIO_CSN_PIN 2
#define RADIO_SCK_PIN 12
#define RADIO_MISO_PIN 13
#define RADIO_MOSI_PIN 11
#define RADIO_SPI_BUS FSPI

#define RADIO1_CE_PIN 10
#define RADIO1_CSN_PIN 9
#define RADIO1_SCK_PIN 5
#define RADIO1_MISO_PIN 6
#define RADIO1_MOSI_PIN 4
#define RADIO1_SPI_BUS HSPI

/*
 * Access point options.
 *
 * (x) DEFAULT_SSID, PASSWORD: Basic AP credentials
 * (x) DEFAULT_CHANNEL: Default Wi-Fi channel to use for web 
 * interface, and avoid jamming on.
 *
 * The value of 0 is tolerated, 
 * but will cause the project to default to channel 1,
 * and jam it's own access point.
 * 
 * (x) HIDDEN_SSID: whenever to not broadcast the SSID
 * (x) BOARD_ADDRESS: IP Address of the board for 
 * hosting the webserver
 *
 * (x) CAPTIVE_PORTAL: whenever to automatically redirect/notify 
 * the user to the web interface in browser
 *
 * (x) BAD_BOARD_REVISION: Set this to 1 if your board has
 * problems with higher WLAN TxPower (e.g. high unreliability).
 * 
 * This will set the board to only 8,5 dBm.
*/
#define DEFAULT_SSID ""
#define DEFAULT_PASSWORD ""
#define DEFAULT_CHANNEL 13
#define HIDDEN_SSID false

#define CAPTIVE_PORTAL true
#define BOARD_ADDRESS IPAddress(192, 168, 0, 1)
#define BAD_BOARD_REVISION 1

/*
 * OLED screen options
 *
 * (x) HAS_OLED_SCREEN: whenever to compile in display code
 * (x) DISPLAY_SDA_PIN: board's SDA pin you connected your display to
 * (x) DISPLAY_SCL_PIN: board's SCL pin you connected your display to
 * (x) DISPLAY_ADDRESS: should be on the back of the display module
 * (x) DISPLAY_WIDTH:   should be in display specs
 * (X) DISPLAY_HEIGHT:  should be in display specs
 * (x) DISPLAY_BRIGHTNESS: 225 is max, 127 is recommended
 * 
 * (x) JAMMER_FOOTER_MSG: footer message while jamming
 * (x) CANCELED_JAMMER_FOOTER_MSG: footer message if the jammer task
 * is cancelled via the web interface
 */
#define HAS_OLED_SCREEN true
#define DISPLAY_SDA_PIN 18
#define DISPLAY_SCL_PIN 8

#define DISPLAY_ADDRESS 0x3C
#define DISPLAY_WIDTH 128
#define DISPLAY_HEIGHT 64
#define DISPLAY_BRIGHTNESS 127

#define JAMMER_FOOTER_MSG getPowerLevelString()
#define CANCELED_JAMMER_FOOTER_MSG "o_o"

/*
 * Sleeping
 *
 * (x) ALLOW_SLEEP: whenever to power down radios and
 * set display to lower power consumption on idle 
 * (x) SLEEP_TIMEOUT: idle timeout before trying to save power in ms
 * 
 * (x) SLEEP_DISPLAY: whenever to switch to displaying a small
 * message instead of the bitmap, as less pixels = lower amperage
 * (x) SLEEP_DISPLAY_MSG: a short message to print on the display
*/
#define ALLOW_SLEEP true
#define SLEEP_TIMEOUT 600000
#define SLEEP_DISPLAY true
#define SLEEP_DISPLAY_MSG "zzz..."

/* 
 * Advanced preferences
*/
#define WEBSERVER_PORT 80 // HTTP only
#define DNS_PORT 53 // Unencrypted
#define JAM_TEXT "xxxxxxxxxxxxxxxx"
#define ENTRY_PAGE "index.html"
#define NVS_NAME "jammer_config"

#endif // PREFERENCES_H
