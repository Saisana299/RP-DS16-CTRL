#include <Arduino.h>
#include <Wire.h>
#include <SPISlave.h>
#include <debug.h>

#define DEBUG_MODE 1 //0 or 1
Debug debug(DEBUG_MODE, Serial2, 8, 9, 115200);

#define S1_SDA_PIN 26
#define S1_SCL_PIN 27
#define S1_I2C_ADDR 0x08
#define S2_SDA_PIN 20
#define S2_SCL_PIN 21
#define S2_I2C_ADDR 0x09

TwoWire& synth1 = Wire1;
TwoWire& synth2 = Wire;

#define MIDI_RX_PIN 1

SerialUART& midi = Serial1;

#define DISP_SCK 2
#define DISP_TX 3
#define DISP_RX 4
#define DISP_CS 5

SPISlaveClass& disp = SPISlave;
SPISettings spisettings(1000000, MSBFIRST, SPI_MODE0);

int noteid = -1;

void midiLoop();

char* intToString(int value, char* output) {
    sprintf(output, "%d", value);
    return output;
}

void synthWrite(TwoWire synth, uint8_t addr, char* value) {
    synth.beginTransmission(addr);
    synth.write(value);
    synth.endTransmission();
}

volatile bool recvBuffReady = false;
char recvBuff[1024] = "";
int recvIdx = 0;
void recvCallback(uint8_t *data, size_t len) {
    memcpy(recvBuff + recvIdx, data, len);
    recvIdx += len;
    if (recvIdx == sizeof(recvBuff)) {
        recvBuffReady = true;
        recvIdx = 0;
    }
}

char sendBuff[1024];
void sentCallback() {
    memset(sendBuff, 0, sizeof(sendBuff));
    if(recvBuffReady) {
        if(String(recvBuff) == "note"){
            if(noteid == -1) sprintf(sendBuff, "none");
            else sprintf(sendBuff, "%d", noteid);
        } else {  
            sprintf(sendBuff, "ok");
        }
        recvBuffReady = false;
    } else {  
        sprintf(sendBuff, "ok");
    }
    disp.setData((uint8_t*)sendBuff, sizeof(sendBuff));
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

    disp.setRX(DISP_RX);
    disp.setCS(DISP_CS);
    disp.setSCK(DISP_SCK);
    disp.setTX(DISP_TX);
    disp.onDataRecv(recvCallback);
    disp.onDataSent(sentCallback);
    disp.begin(spisettings);
    
    debug.init();

    pinMode(LED_BUILTIN, OUTPUT);

    multicore_launch_core1(midiLoop);
}

void loop() {
    //
}

void midiLoop() {
    char buffer[32];
    while(1){
        if(midi.available()) {
            digitalWrite(LED_BUILTIN, HIGH);
            delay(10);
            digitalWrite(LED_BUILTIN, LOW);
            
            uint8_t statusByte = midi.read();
            // // ノートオンイベントの場合
            if ((statusByte & 0xF0) == 0x90) {
                if (midi.available()) {
                    uint8_t note = midi.read();
                    if (midi.available()) {
                        uint8_t velocity = midi.read();
                        if (velocity != 0) {
                            debug.getSerial().print("Note On: ");
                            debug.getSerial().print(note);
                            debug.getSerial().print(" Velocity: ");
                            debug.getSerial().println(velocity);
                            noteid = note;

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
        }
    }
}