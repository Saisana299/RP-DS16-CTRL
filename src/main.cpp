#include <Arduino.h>
#include <debug.h>

#define BAUD_RATE 115200

#define DEBUG_MODE 0 //0 or 1
Debug DEBUG(DEBUG_MODE, Serial2, 8, 9, BAUD_RATE);

#define S1_TX_PIN 16
#define S1_RX_PIN 17
#define S2_TX_PIN 4
#define S2_RX_PIN 5

void setup() {
    Serial1.setRX(S1_RX_PIN);
    Serial1.setTX(S1_TX_PIN);
    Serial1.begin(BAUD_RATE);

    #if DEBUG_MODE == 0
        Serial2.setTX(S2_TX_PIN);
        Serial2.begin(BAUD_RATE);
    #endif
    
    DEBUG.init();

    pinMode(LED_BUILTIN, OUTPUT);
}

void loop() {
    #if DEBUG_MODE == 1
        if(Serial1.available()) {
            uint8_t midiByte = Serial1.read();
            digitalWrite(LED_BUILTIN, HIGH);
            delay(100);
            digitalWrite(LED_BUILTIN, LOW);
            DEBUG.getSerial().println(midiByte, HEX);
        }
    #else
        digitalWrite(LED_BUILTIN, HIGH);
        Serial1.println("261.63");//C4
        delay(1000);
        Serial2.println("329.63");//E4
        digitalWrite(LED_BUILTIN, LOW);
        delay(1000);
    #endif
}