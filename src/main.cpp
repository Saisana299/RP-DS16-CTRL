#include <Arduino.h>
#include <Wire.h>
#include <debug.h>

#define DEBUG_MODE 1 //0 or 1
Debug DEBUG(DEBUG_MODE, Serial2, 8, 9, 115200);

#define S1_SDA_PIN 26
#define S1_SCL_PIN 27
#define S1_I2C_ADDR 0x08
#define S2_SDA_PIN 20
#define S2_SCL_PIN 21
#define S2_I2C_ADDR 0x09

TwoWire& SYNTH1 = Wire1;
TwoWire& SYNTH2 = Wire;

#define MIDI_RX_PIN 1

SerialUART& MIDI = Serial1;

void midiLoop();

char* intToString(int value, char* output) {
    sprintf(output, "%d", value);
    return output;
}

void setup() {
    SYNTH1.setSDA(S1_SDA_PIN);
    SYNTH1.setSCL(S1_SCL_PIN);
    SYNTH2.setSDA(S2_SDA_PIN);
    SYNTH2.setSCL(S2_SCL_PIN);

    SYNTH1.begin();
    SYNTH2.begin();

    MIDI.setRX(MIDI_RX_PIN);
    MIDI.begin(31250);
    
    DEBUG.init();

    pinMode(LED_BUILTIN, OUTPUT);

    multicore_launch_core1(midiLoop);
}

void loop() {
    delay(10);
}

void midiLoop() {
    char buffer[32];
    while(1){
        if(MIDI.available()) {
            digitalWrite(LED_BUILTIN, HIGH);
            delay(10);
            digitalWrite(LED_BUILTIN, LOW);
            
            uint8_t statusByte = MIDI.read();
            // // ノートオンイベントの場合
            if ((statusByte & 0xF0) == 0x90) {
                if (MIDI.available()) {
                    uint8_t note = MIDI.read();
                    if (MIDI.available()) {
                        uint8_t velocity = MIDI.read();
                        if (velocity != 0) {
                            DEBUG.getSerial().print("Note On: ");
                            DEBUG.getSerial().print(note);
                            DEBUG.getSerial().print(" Velocity: ");
                            DEBUG.getSerial().println(velocity);

                            SYNTH1.beginTransmission(S1_I2C_ADDR);
                            char* stringValue = intToString(note, buffer);
                            SYNTH1.write(stringValue);
                            SYNTH1.endTransmission();

                            SYNTH2.beginTransmission(S2_I2C_ADDR);
                            stringValue = intToString(note+12, buffer);
                            SYNTH2.write(stringValue);
                            SYNTH2.endTransmission();
                        } else {
                            DEBUG.getSerial().print("Note Off: ");
                            DEBUG.getSerial().println(note);

                            SYNTH1.beginTransmission(S1_I2C_ADDR);
                            char* stringValue = intToString(note+10000, buffer);
                            SYNTH1.write(stringValue);
                            SYNTH1.endTransmission();

                            SYNTH2.beginTransmission(S2_I2C_ADDR);
                            stringValue = intToString(note+12+10000, buffer);
                            SYNTH2.write(stringValue);
                            SYNTH2.endTransmission();
                        }
                    }
                }
            }
            // ノートオフイベントの場合
            else if ((statusByte & 0xF0) == 0x80) {
                if (MIDI.available()) {
                    uint8_t note = MIDI.read();
                    MIDI.read(); // velocityも読み取るが、ここでは無視
                    DEBUG.getSerial().print("Note Off: ");
                    DEBUG.getSerial().println(note);

                    SYNTH1.beginTransmission(S1_I2C_ADDR);
                    char* stringValue = intToString(note+10000, buffer);
                    SYNTH1.write(stringValue);
                    SYNTH1.endTransmission();

                    SYNTH2.beginTransmission(S2_I2C_ADDR);
                    stringValue = intToString(note+12+10000, buffer);
                    SYNTH2.write(stringValue);
                    SYNTH2.endTransmission();
                }
            }
        }
    }
}