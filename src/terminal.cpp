// terminal.cpp
#include "terminal.h"
#include <RF24.h>

// Reference to the existing radio object from your main/radio files
extern RF24 radio;   // defined elsewhere (e.g., in radio.cpp or main.cpp)

// Terminal state variables (static so they don’t collide)
static uint8_t rxAddress[5] = {0xAA, 0x01, 0x02, 0x03, 0x04};
static uint8_t currentChannel = 75;
static uint8_t powerLevel = RF24_PA_MIN;  // store as enum value
static bool isJamming = false;
static bool isListening = false;
static uint8_t jamMode = 2;  // 1=carrier, 2=packets
static String inputBuffer = "";

// Forward declarations (internal to this file)
static void processCommand(String cmd);
static void printHelp();
static void scanChannels();
static void startListening(int channel);
static void listenAndPrint();
static void parseJamCommand(String cmd);
static void startJam(int channel, uint8_t mode);
static void carrierJam();
static void packetJam();
static void stopAllModes();
static void setAddress(String cmd);
static void setChannel(int ch);
static void setPower(int level);
static void printStatus();
static int extractChannel(String cmd);

// ------------------------------------------------------------------
void terminalInit() {
    Serial.println("\n[Terminal] Ready. Type 'help' for commands.");
    printStatus();
}

// ------------------------------------------------------------------
void terminalUpdate() {
    // Read serial input
    while (Serial.available()) {
        char c = Serial.read();
        if (c == '\n' || c == '\r') {
            if (inputBuffer.length() > 0) {
                processCommand(inputBuffer);
                inputBuffer = "";
            }
        } else {
            inputBuffer += c;
        }
    }

    // Handle active modes (jamming / listening)
    if (isJamming) {
        if (jamMode == 1) carrierJam();
        else if (jamMode == 2) packetJam();
    }

    if (isListening) {
        listenAndPrint();
    }
}

// ------------------------------------------------------------------
static void processCommand(String cmd) {
    cmd.trim();
    if (cmd.length() == 0) return;

    String lowerCmd = cmd;
    lowerCmd.toLowerCase();

    if (lowerCmd == "help") {
        printHelp();
    }
    else if (lowerCmd.startsWith("scan")) {
        scanChannels();
    }
    else if (lowerCmd.startsWith("listen")) {
        int ch = extractChannel(cmd);
        if (ch >= 0) startListening(ch);
        else startListening(currentChannel);
    }
    else if (lowerCmd.startsWith("jam")) {
        parseJamCommand(cmd);
    }
    else if (lowerCmd == "stop") {
        stopAllModes();
    }
    else if (lowerCmd.startsWith("setaddr")) {
        setAddress(cmd);
    }
    else if (lowerCmd.startsWith("setchannel")) {
        int ch = extractChannel(cmd);
        if (ch >= 0) setChannel(ch);
    }
    else if (lowerCmd.startsWith("setpower")) {
        int level = extractChannel(cmd); // reuses same extractor
        if (level >= 0 && level <= 3) setPower(level);
        else Serial.println("Power level must be 0..3");
    }
    else if (lowerCmd == "status") {
        printStatus();
    }
    else {
        Serial.println("Unknown command. Type 'help'");
    }
}

// ------------------------------------------------------------------
static void printHelp() {
    Serial.println("\n--- Terminal Commands ---");
    Serial.println("help                           show this menu");
    Serial.println("scan                           scan channels 0-125 for carrier");
    Serial.println("listen [channel]               promiscuous RX mode");
    Serial.println("jam <channel> [1|2]            jam: 1=carrier, 2=random packets");
    Serial.println("stop                           stop jamming/listening");
    Serial.println("setaddr <10 hex digits>        set RX address (e.g. AA01020304)");
    Serial.println("setchannel <0..125>            change channel");
    Serial.println("setpower <0..3>                set power (0=min,3=max)");
    Serial.println("status                         show current settings");
    Serial.println("----------------------------------\n");
}

// ------------------------------------------------------------------
static void scanChannels() {
    Serial.println("Scanning channels 0..125 for activity...");
    stopAllModes();

    uint8_t prevPower = radio.getPALevel();
    radio.setPALevel(RF24_PA_MAX);
    radio.startListening();

    int active = 0;
    for (int ch = 0; ch <= 125; ch++) {
        radio.setChannel(ch);
        delay(50);
        if (radio.testCarrier()) {
            Serial.print("CH ");
            Serial.print(ch);
            Serial.println(": ACTIVE");
            active++;
        } else if (ch % 20 == 0) {
            Serial.print(".");
        }
    }

    radio.stopListening();
    radio.setPALevel(prevPower);
    radio.setChannel(currentChannel);
    Serial.println("\nScan complete. Active channels: " + String(active));
}

// ------------------------------------------------------------------
static void startListening(int channel) {
    stopAllModes();
    setChannel(channel);

    radio.setAutoAck(false);
    radio.setPayloadSize(32);
    uint8_t broadcast[5] = {0xFF,0xFF,0xFF,0xFF,0xFF};
    radio.openReadingPipe(0, broadcast);
    radio.startListening();
    isListening = true;

    Serial.print("Listening on channel ");
    Serial.print(channel);
    Serial.println(" (broadcast address)");
}

// ------------------------------------------------------------------
static void listenAndPrint() {
    if (radio.available()) {
        uint8_t payload[32];
        uint8_t len = radio.getPayloadSize();
        if (len == 0 || len > 32) len = 32;
        radio.read(payload, len);

        Serial.print("[");
        Serial.print(millis());
        Serial.print("ms] ");
        for (uint8_t i = 0; i < len; i++) {
            if (payload[i] < 0x10) Serial.print("0");
            Serial.print(payload[i], HEX);
            Serial.print(" ");
        }
        Serial.print(" | ");
        for (uint8_t i = 0; i < len; i++) {
            char c = payload[i];
            Serial.print((c >= 32 && c <= 126) ? c : '.');
        }
        Serial.println();
    }
}

// ------------------------------------------------------------------
static void parseJamCommand(String cmd) {
    int firstSpace = cmd.indexOf(' ');
    if (firstSpace == -1) {
        Serial.println("Usage: jam <channel> [1|2]");
        return;
    }
    String rest = cmd.substring(firstSpace + 1);
    rest.trim();
    int secondSpace = rest.indexOf(' ');
    int channel;
    if (secondSpace == -1) {
        channel = rest.toInt();
        jamMode = 2;
    } else {
        channel = rest.substring(0, secondSpace).toInt();
        jamMode = rest.substring(secondSpace + 1).toInt();
        if (jamMode != 1 && jamMode != 2) jamMode = 2;
    }
    if (channel < 0 || channel > 125) {
        Serial.println("Channel 0..125");
        return;
    }
    startJam(channel, jamMode);
}

// ------------------------------------------------------------------
static void startJam(int channel, uint8_t mode) {
    stopAllModes();
    setChannel(channel);
    radio.setPALevel(RF24_PA_MAX);
    radio.setDataRate(RF24_2MBPS);
    radio.setAutoAck(false);
    radio.setPayloadSize(32);
    radio.disableCRC();
    radio.stopListening();
    radio.flush_tx();
    isJamming = true;
    jamMode = mode;
    Serial.print("Jamming channel ");
    Serial.print(channel);
    Serial.println(mode == 1 ? " (carrier)" : " (random packets)");
}

// ------------------------------------------------------------------
static void carrierJam() {
    uint8_t dummy[32] = {0};
    radio.writeFast(&dummy, 32);
}

// ------------------------------------------------------------------
static void packetJam() {
    uint8_t randomPayload[32];
    for (int i = 0; i < 32; i++) randomPayload[i] = random(256);
    radio.writeFast(&randomPayload, 32);
    delayMicroseconds(100);
}

// ------------------------------------------------------------------
static void stopAllModes() {
    if (isJamming || isListening) {
        isJamming = false;
        isListening = false;
        radio.stopListening();
        radio.flush_tx();
        radio.flush_rx();
        Serial.println("Stopped all radio activity.");
    }
}

// ------------------------------------------------------------------
static void setAddress(String cmd) {
    int space = cmd.indexOf(' ');
    if (space == -1) {
        Serial.println("Usage: setaddr <10 hex digits>");
        return;
    }
    String hexStr = cmd.substring(space + 1);
    hexStr.trim();
    hexStr.replace(" ", "");
    if (hexStr.length() != 10) {
        Serial.println("Need 10 hex chars (5 bytes)");
        return;
    }
    for (int i = 0; i < 5; i++) {
        String byteStr = hexStr.substring(i*2, i*2+2);
        rxAddress[i] = (uint8_t)strtol(byteStr.c_str(), NULL, 16);
    }
    Serial.print("Address set to: ");
    for (int i=0;i<5;i++) {
        if (rxAddress[i] < 0x10) Serial.print("0");
        Serial.print(rxAddress[i], HEX);
    }
    Serial.println();
}

// ------------------------------------------------------------------
static void setChannel(int ch) {
    if (ch < 0 || ch > 125) return;
    currentChannel = ch;
    radio.setChannel(currentChannel);
    Serial.print("Channel set to ");
    Serial.println(currentChannel);
}

// ------------------------------------------------------------------
static void setPower(int level) {
    if (level < 0 || level > 3) return;
    powerLevel = level;
    rf24_pa_dbm_e pa;
    switch(level) {
        case 0: pa = RF24_PA_MIN; break;
        case 1: pa = RF24_PA_LOW; break;
        case 2: pa = RF24_PA_HIGH; break;
        case 3: pa = RF24_PA_MAX; break;
    }
    radio.setPALevel(pa);
    Serial.print("Power set to ");
    Serial.print(level);
    Serial.print(" (");
    Serial.print(radio.getPALevel());
    Serial.println(" dBm)");
}

// ------------------------------------------------------------------
static void printStatus() {
    Serial.println("\n--- Terminal Status ---");
    Serial.print("Channel: "); Serial.println(currentChannel);
    Serial.print("Power: "); Serial.println(powerLevel);
    Serial.print("RX addr: ");
    for (int i=0;i<5;i++) {
        if (rxAddress[i] < 0x10) Serial.print("0");
        Serial.print(rxAddress[i], HEX);
    }
    Serial.println();
    Serial.print("Listening: "); Serial.println(isListening ? "YES" : "NO");
    Serial.print("Jamming: "); Serial.println(isJamming ? "YES" : "NO");
    Serial.println("------------------------\n");
}

// ------------------------------------------------------------------
static int extractChannel(String cmd) {
    int space = cmd.indexOf(' ');
    if (space == -1) return -1;
    String chStr = cmd.substring(space + 1);
    chStr.trim();
    return chStr.toInt();
}