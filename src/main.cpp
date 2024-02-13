#include <Arduino.h>
#include <Wire.h>
#include <debug.h>
#include <midi1msg.h>
#include <instructionSet.h>

// debug 関連
#define DEBUG_MODE 1 //0 or 1
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
bool i2c_is_synth = true;
uint8_t synthMode = SYNTH_SINGLE;

// モードチェンジ時にリセットする
struct Note {
    uint8_t num;
    uint8_t id;
    uint8_t synth; //0x01-0x02
};
static const int MAX_NOTES = 8;
Note notes[MAX_NOTES];

void loop1();

void synthWrite(TwoWire synth, uint8_t addr, const uint8_t * data, size_t size) {
    synth.beginTransmission(addr);
    synth.write(data, size);
    synth.endTransmission();
}

uint8_t response = 0x00; // レスポンス用
String synthCacheData = ""; // 次のSYNTH通信開始時に命令を送信するためのキャッシュ
uint8_t synthCacheId = 0x00; // 〃送信対象(0xffはブロードキャスト)
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

        // 例: {INS_BEGIN, DISP_SET_PRESET, DATA_BEGIN, 0x02, 0x01, 0x01}
        case DISP_SET_PRESET:
        {
            if(bytes < 6) {
                response = RES_ERROR;
                return;
            }
            uint8_t data[] = {
                INS_BEGIN, SYNTH_SET_PRESET,
                DATA_BEGIN, 0x01, receivedData[5]
            };
            synthCacheId = receivedData[4];
            for (uint8_t byte: data) {
                synthCacheData += static_cast<char>(byte);
            }
            response = RES_OK;
        }
            break;

        // 例: {INS_BEGIN, DISP_SET_SYNTH, DATA_BEGIN, 0x01, 0x02}
        case DISP_SET_SYNTH:
        {
            if(bytes < 5) {
                response = RES_ERROR;
                return;
            }
            synthMode = receivedData[4];
            for(Note note: notes) {
                note.num = 0;
                note.id = 0xff;
                note.synth = 0;
            }
            uint8_t data[] = {INS_BEGIN, SYNTH_SOUND_STOP};
            synthCacheId = 0xff;
            for (uint8_t byte: data) {
                synthCacheData += static_cast<char>(byte);
            }
            response = RES_OK;
        }
            break;

        // 例: {INS_BEGIN, DISP_SET_PAN, DATA_BEGIN, 0x02, 0x02, 0x01}
        case DISP_SET_PAN:
        {
            if(bytes < 6) {
                response = RES_ERROR;
                return;
            }
            uint8_t data[] = {
                INS_BEGIN, SYNTH_SET_PAN,
                DATA_BEGIN, 0x01, receivedData[5]
            };
            synthCacheId = receivedData[4];
            for (uint8_t byte: data) {
                synthCacheData += static_cast<char>(byte);
            }
            response = RES_OK;
        }
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

    // cache処理
    if(!synthCacheData.equals("")) {
        size_t size = synthCacheData.length();
        uint8_t data[size];
        for (int i = 0; i < size; i++) {
            data[i] = static_cast<uint8_t>(synthCacheData[i]);
        }
        if (synthCacheId == 0x01) {
            synthWrite(synth1, S1_I2C_ADDR, data, size);
        } else if(synthCacheId == 0x02) {
            synthWrite(synth2, S2_I2C_ADDR, data, size);
        } else if(synthCacheId == 0xff) {
            synthWrite(synth1, S1_I2C_ADDR, data, size);
            synthWrite(synth2, S2_I2C_ADDR, data, size);
        }
        synthCacheData = "";
        synthCacheId = 0x00;
    }

    i2c_is_synth = true;
}

void beginDisp() {
    synth1.end();
    synth2.end();

    disp.setSDA(DISP_SDA_PIN);
    disp.setSCL(DISP_SCL_PIN);
    disp.begin(DISP_I2C_ADDR);
    disp.onReceive(receiveEvent);
    disp.onRequest(requestEvent);

    i2c_is_synth = false;
}

void dispISR() {
    if(i2c_is_synth) {
        digitalWrite(LED_BUILTIN, HIGH);
        beginDisp();
    }else{
        digitalWrite(LED_BUILTIN, LOW);
        beginSynth();
    }
}

bool availableMIDI(uint8_t timeout = 100) {
    unsigned long startTime = millis();
    while(!midi.available()) {
        if(millis() - startTime > timeout) {
            break;
        }
    }
    if(!midi.available()) return false;
    return true;
}

// notes から指定したノートのインデックスを返す
int8_t getNotesIndex(uint8_t note) {
    int8_t index = -1;
    for(int8_t i = 0; i < MAX_NOTES; i++) {
        if(notes[i].id == note) index = i;
    }
    return index;
}

// notesにノートを記録
int8_t setNotesNote(uint8_t note) {
    int8_t index = getNotesIndex(note);
    if(index != -1) return -1;
    int8_t synth = -1;
    
    // synthの割り当て確認
    uint8_t count1 = 0;
    uint8_t count2 = 0;
    for(uint8_t i = 0; i < MAX_NOTES; i++) {
        if(notes[i].synth == 0x01) count1++;
        else if(notes[i].synth == 0x02) count2++;
    }

    // 空き又は一番古いnoteを取得
    uint8_t empty = 0;
    for(uint8_t i = 0; i < MAX_NOTES; i++) {
        if(notes[i].num == 0) {
            empty = i;
            break;
        }
        if(notes[i].num == 1) {
            empty = i;
        }
    }

    // もし両方で4音使用されている場合
    if(count1 == 4 && count2 == 4) {
        // numの順番を整理する
        for(uint8_t i = 0; i < MAX_NOTES; i++) {
            if(notes[empty].num < notes[i].num) {
                notes[i].num = notes[i].num - 1;
            }
        }

        notes[empty].num = 8;
        notes[empty].id = note;
        synth = notes[empty].synth;
    }
    else {
        if(count1 <= count2){
            notes[empty].num = count1 + count2 + 1;
            notes[empty].id = note;
            notes[empty].synth = 0x01;
            synth = 0x01;
        }else{
            notes[empty].num = count1 + count2 + 1;
            notes[empty].id = note;
            notes[empty].synth = 0x02;
            synth = 0x02;
        }
    }

    return synth;
}

// notesからノートを削除
int8_t removeNotesNote(uint8_t note) {
    int8_t index = getNotesIndex(note);
    if(index == -1) return -1;
    int8_t synth = notes[index].synth;
    
    // numの順番を整理する
    for(uint8_t i = 0; i < MAX_NOTES; i++) {
        if(notes[index].num < notes[i].num) {
            notes[i].num = notes[i].num - 1;
        }
    }

    notes[index].num = 0;
    notes[index].id = 0xff;
    notes[index].synth = 0;
    return synth;
}

void setup() {
    // 初期化
    for(Note note: notes) {
        note.num = 0;
        note.id = 0xff;
        note.synth = 0;
    }

    beginSynth();

    midi.setRX(MIDI_RX_PIN);
    midi.begin(31250);
    
    debug.init();

    pinMode(LED_BUILTIN, OUTPUT);

    delay(10);

    // １音目にフェードアウトが適用されないため１音鳴らしておく
    uint8_t firstin[] = {INS_BEGIN, SYNTH_NOTE_ON, DATA_BEGIN, 0x02, 0x00, 0x00};
    uint8_t firstout[] = {INS_BEGIN, SYNTH_NOTE_OFF, DATA_BEGIN, 0x02, 0x00, 0x00};
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
    while(1){
        if(!midi.available()) {
            continue;
        }
        if(!i2c_is_synth) {
            continue;
        }

        digitalWrite(LED_BUILTIN, HIGH);

        uint8_t statusByte = midi.read();
        // ノートオン・オフイベントの場合
        if (statusByte == MIDI_CH1_NOTE_ON  || statusByte == MIDI_CH1_NOTE_OFF ||
            statusByte == MIDI_CH2_NOTE_ON  || statusByte == MIDI_CH2_NOTE_OFF ) {
                
            if(!availableMIDI()) continue;
            uint8_t note = midi.read();

            if(!availableMIDI()) continue;
            uint8_t velocity = midi.read();

            uint8_t midiChannel = 1;
            if( statusByte == MIDI_CH2_NOTE_ON  ||
                statusByte == MIDI_CH2_NOTE_OFF ) {
                midiChannel = 2;
            }
            bool noteOn = (
                statusByte == MIDI_CH1_NOTE_ON ||
                statusByte == MIDI_CH2_NOTE_ON ) && velocity != 0;
            uint8_t command = noteOn ? SYNTH_NOTE_ON : SYNTH_NOTE_OFF;

        // 鳴らすシンセID=1 //
            // シングルモード以外で ch=1 (鳴らすのはsynth1)
            if(midiChannel == 1 && synthMode != SYNTH_SINGLE) {
                uint8_t data1[] = {INS_BEGIN, command, DATA_BEGIN, 0x02, note, velocity};
                synthWrite(synth1, S1_I2C_ADDR, data1, sizeof(data1));
            }
            // シングルモードで ch=1
            else if(midiChannel == 1 && synthMode == SYNTH_SINGLE) {
                int8_t synth = -1;
                
                // noteOnの場合
                if(command == SYNTH_NOTE_ON) {
                    synth = setNotesNote(note);
                }

                // noteOffの場合
                else if(command == SYNTH_NOTE_OFF) {
                    synth = removeNotesNote(note);
                }

                if(synth == 1) {
                    uint8_t data1[] = {INS_BEGIN, command, DATA_BEGIN, 0x02, note, velocity};
                    synthWrite(synth1, S1_I2C_ADDR, data1, sizeof(data1));
                }
                else if(synth == 2) {
                    uint8_t data2[] = {INS_BEGIN, command, DATA_BEGIN, 0x02, note, velocity};
                    synthWrite(synth2, S2_I2C_ADDR, data2, sizeof(data2));
                }
            }
            // デュアルモードで ch=1
            else if(midiChannel == 1 && synthMode == SYNTH_DUAL) {
                uint8_t data1[] = {INS_BEGIN, command, DATA_BEGIN, 0x02, note, velocity};
                synthWrite(synth1, S1_I2C_ADDR, data1, sizeof(data1));
            }

        // 鳴らすシンセID=2 //
            // オクターブモードで ch=1
            if(midiChannel == 1 && synthMode == SYNTH_OCTAVE) {
                uint8_t data2[] = {
                    INS_BEGIN,
                    command,
                    DATA_BEGIN,
                    0x02,
                    static_cast<uint8_t>(note+0x0C),
                    velocity
                };
                synthWrite(synth2, S2_I2C_ADDR, data2, sizeof(data2));
            }
            // マルチモードで ch=2
            else if(midiChannel == 2 && synthMode == SYNTH_MULTI) {
                uint8_t data2[] = {INS_BEGIN, command, DATA_BEGIN, 0x02, note, velocity};
                synthWrite(synth2, S2_I2C_ADDR, data2, sizeof(data2));
            }
            // デュアルモードで ch=1
            else if(midiChannel == 1 && synthMode == SYNTH_DUAL) {
                uint8_t data2[] = {INS_BEGIN, command, DATA_BEGIN, 0x02, note, velocity};
                synthWrite(synth2, S2_I2C_ADDR, data2, sizeof(data2));
            }
        }
        delay(1);
        digitalWrite(LED_BUILTIN, LOW);
    }
}