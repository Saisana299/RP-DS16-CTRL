#include <Arduino.h>
#include <Wire.h>
#include <debug.h>
#include <midi1.h>
#include <instructionSet.h>

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

void synthWrite(TwoWire synth, uint8_t addr, const uint8_t * data, size_t size) {
    synth.beginTransmission(addr);
    synth.write(data, size);
    synth.endTransmission();
}

uint8_t response = 0x00; // レスポンス用
void receiveEvent(int bytes) {
    // 2バイト以上のみ受け付ける
    if(bytes < 2) return;

    int i = 0;
    uint8_t receivedData[bytes];
    while (disp.available()) {
        uint8_t received = disp.read();
        receivedData[i] = received;
        i++;
        if (i >= bytes) {
            break;
        }
    }

    uint8_t instruction = 0x00; // コード種別
    if(receivedData[0] == INS_BEGIN) {
        instruction = receivedData[1];
    }

    switch (instruction)
    {
        case DISP_CONNECT:
            response = RES_OK;
            break;

        case DISP_SET_PRESET:
            response = RES_ERROR;
            break;
    }
}

void requestEvent() {
    disp.write(response);
    response = 0x00;
}

void beginSynth() {
    disp.end();

    synth1.setSDA(S1_SDA_PIN);
    synth1.setSCL(S1_SCL_PIN);
    synth2.setSDA(S2_SDA_PIN);
    synth2.setSCL(S2_SCL_PIN);

    synth1.begin();
    synth2.begin();

    synthMode = true;
}

void beginDisp() {
    synth1.end();
    synth2.end();

    synthMode = false;

    disp.setSDA(DISP_SDA_PIN);
    disp.setSCL(DISP_SCL_PIN);
    disp.begin(DISP_I2C_ADDR);
    disp.onReceive(receiveEvent);
    disp.onRequest(requestEvent);
}

void dispISR() {
    if(synthMode) {
        digitalWrite(LED_BUILTIN, HIGH);
        beginDisp();
    }else{
        digitalWrite(LED_BUILTIN, LOW);
        beginSynth();
    }
}

void setup() {
    beginSynth();

    midi.setRX(MIDI_RX_PIN);
    midi.begin(31250);
    
    debug.init();

    pinMode(LED_BUILTIN, OUTPUT);

    // １音目にフェードアウトが適用されないため１音鳴らしておく
    uint8_t firstin[] = {INS_BEGIN, SYNTH_NOTE_ON, DATA_BEGIN, 0x01, 0x00};
    uint8_t firstout[] = {INS_BEGIN, SYNTH_NOTE_OFF, DATA_BEGIN, 0x01, 0x00};
    synthWrite(synth1, S1_I2C_ADDR, firstin, sizeof(firstin));
    synthWrite(synth2, S2_I2C_ADDR, firstin, sizeof(firstin));
    delay(10);
    synthWrite(synth1, S1_I2C_ADDR, firstout, sizeof(firstout));
    synthWrite(synth2, S2_I2C_ADDR, firstout, sizeof(firstout));

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
        if(!synthMode) {
            continue;
        }

        digitalWrite(LED_BUILTIN, HIGH);
        delay(1);
        
        uint8_t statusByte = midi.read();
        // // ノートオンイベントの場合
        if ((statusByte & MIDI_EXCL) == MIDI_CH1_NOTE_ON) {
            if(!midi.available()) {
                continue;
            }

            uint8_t note = midi.read();
            if(!midi.available()) {
                continue;
            }

            uint8_t velocity = midi.read();
            if (velocity != 0) {

                uint8_t data[] = {INS_BEGIN, SYNTH_NOTE_ON, DATA_BEGIN, 0x01, note};
                synthWrite(synth1, S1_I2C_ADDR, data, sizeof(data));

                uint8_t data1[] = {
                    INS_BEGIN,
                    SYNTH_NOTE_ON,
                    DATA_BEGIN,
                    0x01,
                    static_cast<uint8_t>(note+0x0C)
                };
                synthWrite(synth2, S2_I2C_ADDR, data1, sizeof(data1));
            } else {

                uint8_t data[] = {INS_BEGIN, SYNTH_NOTE_OFF, DATA_BEGIN, 0x01, note};
                synthWrite(synth1, S1_I2C_ADDR, data, sizeof(data));

                uint8_t data1[] = {
                    INS_BEGIN,
                    SYNTH_NOTE_OFF,
                    DATA_BEGIN,
                    0x01,
                    static_cast<uint8_t>(note+0x0C)
                };
                synthWrite(synth2, S2_I2C_ADDR, data1, sizeof(data1));
            }
        }
        // ノートオフイベントの場合
        else if ((statusByte & MIDI_EXCL) == MIDI_CH1_NOTE_OFF) {
            if (midi.available()) {
                uint8_t note = midi.read();
                midi.read(); // velocityも読み取るが、ここでは無視

                uint8_t data[] = {INS_BEGIN, SYNTH_NOTE_OFF, DATA_BEGIN, 0x01, note};
                synthWrite(synth1, S1_I2C_ADDR, data, sizeof(data));

                uint8_t data1[] = {
                    INS_BEGIN,
                    SYNTH_NOTE_OFF,
                    DATA_BEGIN,
                    0x01,
                    static_cast<uint8_t>(note+0x0C)
                };
                synthWrite(synth2, S2_I2C_ADDR, data1, sizeof(data1));
            }
        }
        digitalWrite(LED_BUILTIN, LOW);
    }
}