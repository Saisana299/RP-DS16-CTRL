#ifndef DISPLAYCONTROL_H
#define DISPLAYCONTROL_H

#define DISP_SW_PIN 11
#define DISP_SDA_PIN 12
#define DISP_SCL_PIN 13
#define DISP_I2C_ADDR 0x0A

class DisplayControl {
private:
    TwoWire& disp = Wire;
    TwoWire& synth1 = Wire1;
    TwoWire& synth2 = Wire;
    bool* pI2c_is_synth;
    bool* pI2c_is_debug;
    uint8_t* pSynthMode;
    String* pSynthCacheData;
    uint8_t* pSynthCacheId;
    uint8_t* pResponse;
    bool* pIsPause;
    bool* pIsDispMidi;

    static DisplayControl* instance;

    static void requestWrapper() {
        instance->onRequestEvent();
    }

    static void receiveWrapper(int bytes) {
        instance->onReceiveEvent(bytes);
    }
    
public:
    DisplayControl(
        bool* addr1, bool* addr2, uint8_t* addr3,
        String* addr4, uint8_t* addr5, uint8_t* addr6, bool* addr7, bool* addr8
    ){
        pI2c_is_synth = addr1;
        pI2c_is_debug = addr2;
        pSynthMode = addr3;
        pSynthCacheData = addr4;
        pSynthCacheId = addr5;
        pResponse = addr6;
        pIsPause = addr7;
        pIsDispMidi = addr8;
        instance = this;
    }

    void beginDebug() {
        synth1.end();
        synth2.end();

        // Wire = disp
        Wire.setSDA(DISP_SDA_PIN);
        Wire.setSCL(DISP_SCL_PIN);
        Wire.begin();
        Wire.setClock(1000000);

        *pI2c_is_debug = true;
    }

    void beginDisp() {
        synth1.end();
        synth2.end();

        disp.setSDA(DISP_SDA_PIN);
        disp.setSCL(DISP_SCL_PIN);
        disp.begin(DISP_I2C_ADDR);
        disp.setClock(1000000);
        disp.onReceive(receiveWrapper);
        disp.onRequest(requestWrapper);

        *pI2c_is_synth = false;
    }

    void onRequestEvent() {
        disp.write(*pResponse);
        *pResponse = 0x00;
    }

    void onReceiveEvent(int bytes) {
        // 1バイト以上のみ受け付ける
        if(bytes < 1) return;

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

        uint8_t instruction = receivedData[0];

        switch (instruction)
        {
            case CTRL_CONNECT:
                *pResponse = RES_OK;
                break;

            // 例: {SYNTH_SET_SHAPE, 0x01, 0x01, 0x01}
            case SYNTH_SET_SHAPE:
            {
                if(bytes < 4) {
                    *pResponse = RES_ERROR;
                    return;
                }
                uint8_t data[] = {
                    SYNTH_SET_SHAPE, receivedData[2], receivedData[3]
                };
                *pSynthCacheId = receivedData[1];
                for (uint8_t byte: data) {
                    *pSynthCacheData += static_cast<char>(byte);
                }
                *pResponse = RES_OK;
            }
                break;

            // 例: {CTRL_SET_SYNTH, 0x02}
            case CTRL_SET_SYNTH:
            {
                if(bytes < 2) {
                    *pResponse = RES_ERROR;
                    return;
                }
                *pSynthMode = receivedData[1];
                uint8_t data[] = {SYNTH_SOUND_STOP};
                *pSynthCacheId = 0xff;
                for (uint8_t byte: data) {
                    *pSynthCacheData += static_cast<char>(byte);
                }
                *pResponse = RES_OK;
            }
                break;

            // 例: {SYNTH_SET_PAN, 0x02, 0x01}
            case SYNTH_SET_PAN:
            {
                if(bytes < 3) {
                    *pResponse = RES_ERROR;
                    return;
                }
                uint8_t data[] = {
                    SYNTH_SET_PAN, receivedData[2]
                };
                *pSynthCacheId = receivedData[1];
                for (uint8_t byte: data) {
                    *pSynthCacheData += static_cast<char>(byte);
                }
                *pResponse = RES_OK;
            }
                break;

            // 例: {CTRL_RESET_SYNTH, 0xff}
            case CTRL_RESET_SYNTH:
            {
                if(bytes < 2) {
                    *pResponse = RES_ERROR;
                    return;
                }
                uint8_t data[] = {SYNTH_SOUND_STOP};
                *pSynthCacheId = receivedData[1];
                for (uint8_t byte: data) {
                    *pSynthCacheData += static_cast<char>(byte);
                }
                *pResponse = RES_OK;
            }
                break;

            // 例: {SYNTH_SET_ATTACK, 0xff, 0x00, 0x60, 0x00, 0x00, 0x00}
            case SYNTH_SET_ATTACK:
            case SYNTH_SET_DECAY:
            case SYNTH_SET_RELEASE:
            {
                if(bytes < 7) {
                    *pResponse = RES_ERROR;
                    return;
                }

                uint8_t message = receivedData[1];

                uint8_t data[] = {
                    message, receivedData[2],
                    receivedData[3], receivedData[4], receivedData[5], receivedData[6]
                };
                *pSynthCacheId = receivedData[1];
                for (uint8_t byte: data) {
                    *pSynthCacheData += static_cast<char>(byte);
                }
                *pResponse = RES_OK;
            }
                break;

            // 例: {SYNTH_SET_SUSTAIN, 0xff, 0x60, 0x00, 0x00, 0x00}
            case SYNTH_SET_SUSTAIN:
            {
                if(bytes < 6) {
                    *pResponse = RES_ERROR;
                    return;
                }
                uint8_t data[] = {
                    SYNTH_SET_SUSTAIN, receivedData[2], receivedData[3], receivedData[4], receivedData[5]
                };
                *pSynthCacheId = receivedData[1];
                for (uint8_t byte: data) {
                    *pSynthCacheData += static_cast<char>(byte);
                }
                *pResponse = RES_OK;
            }
                break;

            // 例: {CTRL_DEBUG_ON}
            case CTRL_DEBUG_ON:
            {
                if(bytes < 1) {
                    *pResponse = RES_ERROR;
                    return;
                }
                // 通信が終わった後にDEBUGモードを有効化
                *pSynthCacheData = "debug";
                *pResponse = RES_OK;
            }
                break;

            // 例: {SYNTH_SET_CSHAPE, 0x01, 0x02, WAVE_DATA...}
            case SYNTH_SET_CSHAPE:
            {
                if(bytes < 27) {
                    *pResponse = RES_ERROR;
                    return;
                }
                receivedData[0] = SYNTH_SET_CSHAPE;
                *pSynthCacheId = receivedData[1];
                for (uint8_t byte: receivedData) {
                    *pSynthCacheData += static_cast<char>(byte);
                }
                *pResponse = RES_OK;
            }
                break;

            // 例: {CTRL_STOP_SYNTH}
            case CTRL_STOP_SYNTH:
            {
                if(bytes < 1) {
                    *pResponse = RES_ERROR;
                    return;
                }
                *pIsPause = true;
                *pResponse = RES_OK;
            }
                break;

            // 例: {CTRL_START_SYNTH}
            case CTRL_START_SYNTH:
            {
                if(bytes < 1) {
                    *pResponse = RES_ERROR;
                    return;
                }
                *pIsPause = false;
                *pResponse = RES_OK;
            }
                break;

            // 例: {SYNTH_SET_VOICE, 0xff, 0x01, 0x01}
            case SYNTH_SET_VOICE:
            {
                if(bytes < 4) {
                    *pResponse = RES_ERROR;
                    return;
                }
                uint8_t data[] = {
                    SYNTH_SET_VOICE, receivedData[2], receivedData[3]
                };
                *pSynthCacheId = receivedData[1];
                for (uint8_t byte: data) {
                    *pSynthCacheData += static_cast<char>(byte);
                }
                *pResponse = RES_OK;
            }
                break;

            // 例: {SYNTH_SET_DETUNE, 0xff, 0xA0, 0x01}
            case SYNTH_SET_DETUNE:
            {
                if(bytes < 4) {
                    *pResponse = RES_ERROR;
                    return;
                }
                uint8_t data[] = {
                    SYNTH_SET_DETUNE, receivedData[2], receivedData[3]
                };
                *pSynthCacheId = receivedData[1];
                for (uint8_t byte: data) {
                    *pSynthCacheData += static_cast<char>(byte);
                }
                *pResponse = RES_OK;
            }
                break;

            // 例: {CTRL_MIDI_ON}
            case CTRL_MIDI_ON:
            {
                if(bytes < 1) {
                    *pResponse = RES_ERROR;
                    return;
                }
                *pIsDispMidi = true;
                *pResponse = RES_OK;
            }
                break;

            // 例: {CTRL_MIDI_OFF}
            case CTRL_MIDI_OFF:
            {
                if(bytes < 1) {
                    *pResponse = RES_ERROR;
                    return;
                }
                *pIsDispMidi = false;
                *pResponse = RES_OK;
            }
                break;

            // 例: {SYNTH_SET_SPREAD, 0xff, 0xA0, 0x01}
            case SYNTH_SET_SPREAD:
            {
                if(bytes < 4) {
                    *pResponse = RES_ERROR;
                    return;
                }
                uint8_t data[] = {
                    SYNTH_SET_SPREAD, receivedData[2], receivedData[3]
                };
                *pSynthCacheId = receivedData[1];
                for (uint8_t byte: data) {
                    *pSynthCacheData += static_cast<char>(byte);
                }
                *pResponse = RES_OK;
            }
                break;

            // 例: {SYNTH_SET_LPF, 0xff, 0x01, 0x05, 0x05, 0x05, 0x05, 0x05, 0x05, 0x05, 0x05}
            // 例: {SYNTH_SET_LPF, 0xff, 0x00}
            case SYNTH_SET_LPF:
            {
                if(bytes < 3) {
                    *pResponse = RES_ERROR;
                    return;
                }
                *pSynthCacheId = receivedData[1];

                if(receivedData[2] == 0x01) {
                    uint8_t data[] = {
                        SYNTH_SET_LPF, receivedData[2],
                        receivedData[3], receivedData[4], receivedData[5], receivedData[6],
                        receivedData[7], receivedData[8], receivedData[9], receivedData[10]
                    };
                    for (uint8_t byte: data) {
                        *pSynthCacheData += static_cast<char>(byte);
                    }
                } else {
                    uint8_t data[] = {
                        SYNTH_SET_LPF, receivedData[2]
                    };
                    for (uint8_t byte: data) {
                        *pSynthCacheData += static_cast<char>(byte);
                    }
                }
                
                *pResponse = RES_OK;
            }
                break;

            // 例: {SYNTH_SET_HPF, 0xff, 0x01, 0x05, 0x05, 0x05, 0x05, 0x05, 0x05, 0x05, 0x05}
            // 例: {SYNTH_SET_HPF, 0xff, 0x00}
            case SYNTH_SET_HPF:
            {
                if(bytes < 3) {
                    *pResponse = RES_ERROR;
                    return;
                }
                *pSynthCacheId = receivedData[1];

                if(receivedData[2] == 0x0A) {
                    uint8_t data[] = {
                        SYNTH_SET_HPF, receivedData[2],
                        receivedData[3], receivedData[4], receivedData[5], receivedData[6],
                        receivedData[7], receivedData[8], receivedData[9], receivedData[10]
                    };
                    for (uint8_t byte: data) {
                        *pSynthCacheData += static_cast<char>(byte);
                    }
                } else {
                    uint8_t data[] = {
                        SYNTH_SET_HPF, receivedData[5]
                    };
                    for (uint8_t byte: data) {
                        *pSynthCacheData += static_cast<char>(byte);
                    }
                }
                
                *pResponse = RES_OK;
            }
                break;
        }
    }
};

#endif // DISPLAYCONTROL_H