#include <Arduino.h>
#include <Wire.h>
#include <debug.h>

#define DEBUG_MODE 1 //0 or 1
Debug DEBUG(DEBUG_MODE, Serial2, 8, 9, 115200);

#define S1_SDA_PIN 12
#define S1_SCL_PIN 13
#define S1_I2C_ADDR 0x08
#define S2_SDA_PIN 2
#define S2_SCL_PIN 3
#define S2_I2C_ADDR 0x09

#define MIDI_RX_PIN 17

void midiLoop();

float midiNoteToFrequency(int midiNote) {
    return 440.0 * pow(2.0, (midiNote - 69) / 12.0);
}

char* floatToString(float value, char* output) {
    int wholePart = (int)value; // 整数部分
    int decimalPart = (int)((value - wholePart) * 100); // 小数点以下2桁
    sprintf(output, "%d.%02d", wholePart, decimalPart);
    return output;
}

void setup() {
    Wire.setSDA(S1_SDA_PIN);
    Wire.setSCL(S1_SCL_PIN);
    Wire1.setSDA(S2_SDA_PIN);
    Wire1.setSCL(S2_SCL_PIN);

    Wire.begin();
    Wire1.begin();

    Serial1.setRX(MIDI_RX_PIN);
    Serial1.begin(31250);
    
    DEBUG.init();

    pinMode(LED_BUILTIN, OUTPUT);

    multicore_launch_core1(midiLoop);
}

void loop() {
    delay(10);
    /*
    delay(1000);
    Wire.beginTransmission(S1_I2C_ADDR);
    Wire.write("523.25");
    Wire.endTransmission();
    Wire1.beginTransmission(S2_I2C_ADDR);
    Wire1.write("659.255");
    Wire1.endTransmission();
    delay(1000);
    */
}

void midiLoop() {
    char buffer[32];//todo
    while(1){
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
                            Wire.beginTransmission(S1_I2C_ADDR);
                            float fq = midiNoteToFrequency(note);
                            floatToString(fq, buffer);
                            Wire.write(buffer);
                            Wire.endTransmission();
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
    }
}