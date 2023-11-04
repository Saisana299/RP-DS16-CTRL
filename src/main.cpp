#include <Arduino.h>
#include <debug.h>

#define BAUD_RATE 115200

#define DEBUG_MODE 1 //0 or 1
Debug DEBUG(DEBUG_MODE, Serial2, 8, 9, BAUD_RATE);

#define S1_TX_PIN 16
#define S1_RX_PIN 17
#define S2_TX_PIN 4
#define S2_RX_PIN 5

void setup() {
    Serial1.setRX(S1_RX_PIN);
    Serial1.setTX(S1_TX_PIN);
    #if DEBUG_MODE == 1
        Serial1.begin(31250);
    #else
        Serial1.beginn(BAUD_RATE);
    #endif

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
            digitalWrite(LED_BUILTIN, HIGH);
            delay(10);
            digitalWrite(LED_BUILTIN, LOW);
            
            uint8_t statusByte = Serial1.read();
            // ノートオンイベントの場合
            if ((statusByte & 0xF0) == 0x90) {
                if (Serial1.available()) {
                    uint8_t note = Serial1.read();
                    if (Serial1.available()) {
                        uint8_t velocity = Serial1.read();
                        if (velocity != 0) {
                            DEBUG.getSerial().print("Note On: ");
                            DEBUG.getSerial().print(note);
                            DEBUG.getSerial().print(" Velocity: ");
                            DEBUG.getSerial().println(velocity);
                        } else {
                            DEBUG.getSerial().print("Note Off: ");
                            DEBUG.getSerial().println(note);
                        }
                    }
                }
            }
            // ノートオフイベントの場合
            else if ((statusByte & 0xF0) == 0x80) {
                if (Serial1.available()) {
                    uint8_t note = Serial1.read();
                    Serial1.read(); // velocityも読み取るが、ここでは無視
                    DEBUG.getSerial().print("Note Off: ");
                    DEBUG.getSerial().println(note);
                }
            }
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