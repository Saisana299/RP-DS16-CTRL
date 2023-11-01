#include <Arduino.h>
#include <debug.h>

#define BAUD_RATE 115200

#define DEBUG_MODE 0 //0 or 1
Debug DEBUG(DEBUG_MODE, Serial2, 8, 9, BAUD_RATE);

#define S1_TX_PIN 16
#define S1_RX_PIN 17
#define S2_TX_PIN 4
#define S2_RX_PIN 5

/******************DEBUG*******************/
#define TX_INTERVAL 1000  // 送信間隔 (ミリ秒)
/******************************************/

void setup() {
    Serial.begin(BAUD_RATE);

    Serial1.setRX(S1_RX_PIN);
    Serial1.setTX(S1_TX_PIN);
    Serial1.begin(BAUD_RATE);

    Serial2.setRX(S2_RX_PIN);
    Serial2.setTX(S2_TX_PIN);
    Serial2.begin(BAUD_RATE);

    DEBUG.init();

    pinMode(LED_BUILTIN, OUTPUT);
}

void loop() {
    digitalWrite(LED_BUILTIN, LOW);
    delay(TX_INTERVAL / 2);
    digitalWrite(LED_BUILTIN, HIGH);
    Serial.println("Serial");
    Serial1.println("UART0");
    delay(100);
    Serial2.println("UART1");
    delay(TX_INTERVAL / 2);
}