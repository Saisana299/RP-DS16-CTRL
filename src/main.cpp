#include <Arduino.h>
#include <Wire.h>
#include <debug.h>

// debug 関連
#define DEBUG_MODE 0 //0 or 1
Debug debug(DEBUG_MODE, Serial2, 8, 9, 115200);

// SYNTH 関連
#define S1_SDA_PIN 26
#define S1_SCL_PIN 27
#define S1_I2C_ADDR 0x08
#define S2_SDA_PIN 20
#define S2_SCL_PIN 21
#define S2_I2C_ADDR 0x09

TwoWire& synth1 = Wire1;
TwoWire& synth2 = Wire;

// MIDI 関連
#define MIDI_RX_PIN 1

SerialUART& midi = Serial1;

// DISP 関連
#define DISP_SW_PIN 11
#define DISP_SDA_PIN 12
#define DISP_SCL_PIN 13
#define DISP_I2C_ADDR 0x0A

TwoWire& disp = Wire;

// その他
bool synthMode = true;

void loop1();

char* intToString(int value, char* output) {
    sprintf(output, "%d", value);
    return output;
}

void synthWrite(TwoWire synth, uint8_t addr, char* value) {
    synth.beginTransmission(addr);
    synth.write(value);
    synth.endTransmission();
}

int a = 0;
void receiveEvent(int bytes) {
    while (disp.available()) {
        char requestType = disp.read();
        if (requestType == 'T') {
            a = 8;
        }
    }
}

void requestEvent() {
    disp.write(a);
    digitalWrite(LED_BUILTIN, HIGH);
    delay(1000);
    digitalWrite(LED_BUILTIN, LOW);
}

void dispISR() {
    Wire.end();
    disp.setSDA(DISP_SDA_PIN);
    disp.setSCL(DISP_SCL_PIN);
    disp.begin(DISP_I2C_ADDR);
    disp.onReceive(receiveEvent);
    disp.onRequest(requestEvent);
    digitalWrite(LED_BUILTIN, HIGH);
    delay(10);
    digitalWrite(LED_BUILTIN, LOW);
}

void setup() {
    synth1.setSDA(S1_SDA_PIN);
    synth1.setSCL(S1_SCL_PIN);
    synth2.setSDA(S2_SDA_PIN);
    synth2.setSCL(S2_SCL_PIN);

    synth1.begin();
    synth2.begin();

    midi.setRX(MIDI_RX_PIN);
    midi.begin(31250);
    
    debug.init();

    pinMode(LED_BUILTIN, OUTPUT);

    // １音目にフェードアウトが適用されないため１音鳴らしておく
    char firstin[] = "0";
    char firstout[] = "10000";
    synthWrite(synth1, S1_I2C_ADDR, firstin);
    synthWrite(synth2, S2_I2C_ADDR, firstin);
    delay(10);
    synthWrite(synth1, S1_I2C_ADDR, firstout);
    synthWrite(synth2, S2_I2C_ADDR, firstout);

    pinMode(DISP_SW_PIN, INPUT_PULLUP);
    attachInterrupt(digitalPinToInterrupt(DISP_SW_PIN), dispISR, FALLING);

    multicore_launch_core1(loop1);
}

void loop() {} // 使用しない

void loop1() {
    char buffer[32];
    while(1){
        if(!midi.available()) {
            continue;
        }

        digitalWrite(LED_BUILTIN, HIGH);
        delay(1);
        
        uint8_t statusByte = midi.read();
        // // ノートオンイベントの場合
        if ((statusByte & 0xF0) == 0x90) {
            if(!midi.available()) {
                continue;
            }
            uint8_t note = midi.read();
            if(!midi.available()) {
                continue;
            }
            uint8_t velocity = midi.read();
            if (velocity != 0) {
                debug.getSerial().print("Note On: ");
                debug.getSerial().print(note);
                debug.getSerial().print(" Velocity: ");
                debug.getSerial().println(velocity);

                char* stringValue = intToString(note, buffer);
                synthWrite(synth1, S1_I2C_ADDR, stringValue);

                stringValue = intToString(note+12, buffer);
                synthWrite(synth2, S2_I2C_ADDR, stringValue);
            } else {
                debug.getSerial().print("Note Off: ");
                debug.getSerial().println(note);

                char* stringValue = intToString(note+10000, buffer);
                synthWrite(synth1, S1_I2C_ADDR, stringValue);

                stringValue = intToString(note+12+10000, buffer);
                synthWrite(synth2, S2_I2C_ADDR, stringValue);
            }
        }
        // ノートオフイベントの場合
        else if ((statusByte & 0xF0) == 0x80) {
            if (midi.available()) {
                uint8_t note = midi.read();
                midi.read(); // velocityも読み取るが、ここでは無視
                debug.getSerial().print("Note Off: ");
                debug.getSerial().println(note);

                char* stringValue = intToString(note+10000, buffer);
                synthWrite(synth1, S1_I2C_ADDR, stringValue);

                stringValue = intToString(note+12+10000, buffer);
                synthWrite(synth2, S2_I2C_ADDR, stringValue);
            }
        }
        digitalWrite(LED_BUILTIN, LOW);
    }
}