#include <note_manager.h>
#include <synth_control.h>

#define MIDI_RX_PIN 1

class MIDIControl {
private:
    bool* pI2c_is_synth;
    bool* pI2c_is_debug;
    uint8_t* pSynthMode;
    bool* pIsLed;
    NoteManager* pNote;
    SynthControl* pSynth;
    bool* pIsPause;
    bool* pIsDispMidi;
    bool* pIsWaitingInit;

    bool availableMIDI(uint8_t timeout = 100) {
        SerialUART* midi = &Serial1;
        if(*pIsDispMidi) midi = &Serial2;

        unsigned long startTime = millis();
        while(!midi->available()) {
            if(millis() - startTime > timeout) {
                break;
            }
        }
        if(!midi->available()) return false;
        return true;
    }

public:
    MIDIControl(
        bool* addr1, bool* addr2, uint8_t* addr3, bool* addr4,
        NoteManager* addr5, SynthControl* addr6, bool* addr7, bool* addr8, bool* addr9)
    {
        pI2c_is_synth = addr1;
        pI2c_is_debug = addr2;
        pSynthMode = addr3;
        pIsLed = addr4;
        pNote = addr5;
        pSynth = addr6;
        pIsPause = addr7;
        pIsDispMidi = addr8;
        pIsWaitingInit = addr9;
    }

    void begin() {
        SerialUART* midi = &Serial1;
        if(*pIsDispMidi) midi = &Serial2;

        midi->setRX(MIDI_RX_PIN);
        midi->begin(31250);
    }

    /**
     * @brief MIDI信号を受け取る
     * 通常時は外部MIDI信号を受け取り処理します。
     * ディスプレイMIDIモード時はDISPからMIDI信号を受け取ります。
     */
    void read() {
        // dispmidiモードが解除されたときに実行する
        if(*pIsWaitingInit){
            Serial2.end();
            Serial2.setTX(8);
            Serial2.setRX(9);
            Serial2.begin(115200);
            *pIsWaitingInit = false;
        }

        SerialUART* midi = &Serial1;
        if(*pIsDispMidi) midi = &Serial2;

        if(*pIsPause) return;

        if(!midi->available()) return;

        if(!*pI2c_is_synth) return;

        *pIsLed = true;

        uint8_t statusByte = midi->read();

        if (*pI2c_is_debug) {
            uint8_t synth = 0x00;
            uint8_t byte2 = 0xff;
            uint8_t byte3 = 0xff;

            if(availableMIDI()) byte2 = midi->read();

            if(availableMIDI()){
                byte3 = midi->read();
                // ch1 noteOn/noteOff
                if(statusByte == MIDI_CH1_NOTE_ON) {
                    synth = pNote->setNotesNote(byte2);
                } else if (statusByte == MIDI_CH1_NOTE_OFF) {
                    synth = pNote->removeNotesNote(byte2);
                }
            }

            if(byte3 != 0xff) {
                // 3rd
                uint8_t data[] = {
                    CTRL_DEBUG_DATA, statusByte, byte2, byte3, synth
                };
                Wire.beginTransmission(DISP_I2C_ADDR);
                Wire.write(data, sizeof(data));
                Wire.endTransmission();
            }
            else if(byte2 != 0xff) {
                // 2nd
                uint8_t data[] = {
                    CTRL_DEBUG_DATA, statusByte, byte2, synth
                };
                Wire.beginTransmission(DISP_I2C_ADDR);
                Wire.write(data, sizeof(data));
                Wire.endTransmission();
            }
            else {
                // 1st
                uint8_t data[] = {
                    CTRL_DEBUG_DATA, statusByte, synth
                };
                Wire.beginTransmission(DISP_I2C_ADDR);
                Wire.write(data, sizeof(data));
                Wire.endTransmission();
            }

            *pIsLed = false;
            return;
        }

        // ノートオン・オフイベントの場合
        if (statusByte == MIDI_CH1_NOTE_ON  || statusByte == MIDI_CH1_NOTE_OFF ||
            statusByte == MIDI_CH2_NOTE_ON  || statusByte == MIDI_CH2_NOTE_OFF ) {

            if(!availableMIDI()) return;
            uint8_t note = midi->read();

            if(!availableMIDI()) return;
            uint8_t velocity = midi->read();

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
            // モノ・デュアル・マルチで ch=1
            if(midiChannel == 1 && *pSynthMode != SYNTH_POLY) {
                uint8_t data1[] = {command, note, velocity};
                pSynth->synth1Write(data1, sizeof(data1));
            }
            // ポリモードで ch=1
            else if(midiChannel == 1 && *pSynthMode == SYNTH_POLY) {
                int8_t synth = -1;

                // noteOnの場合
                if(command == SYNTH_NOTE_ON) {
                    synth = pNote->setNotesNote(note);
                }

                // noteOffの場合
                else if(command == SYNTH_NOTE_OFF) {
                    synth = pNote->removeNotesNote(note);
                }

                if(synth == 1) {
                    uint8_t data1[] = {command, note, velocity};
                    pSynth->synth1Write(data1, sizeof(data1));
                }
                else if(synth == 2) {
                    uint8_t data2[] = {command, note, velocity};
                    pSynth->synth2Write(data2, sizeof(data2));
                }
            }

        // 鳴らすシンセID=2 //
            // モノモードで ch=2
            if(midiChannel == 2 && *pSynthMode == SYNTH_MONO) {
                uint8_t data2[] = {command, note, velocity};
                pSynth->synth2Write(data2, sizeof(data2));
            }
            // マルチモードで ch=2
            else if(midiChannel == 2 && *pSynthMode == SYNTH_MULTI) {
                uint8_t data2[] = {command, note, velocity};
                pSynth->synth2Write(data2, sizeof(data2));
            }
            // デュアルモードで ch=1
            else if(midiChannel == 1 && *pSynthMode == SYNTH_DUAL) {
                uint8_t data2[] = {command, note, velocity};
                pSynth->synth2Write(data2, sizeof(data2));
            }
        }
        *pIsLed = false;
    }
};