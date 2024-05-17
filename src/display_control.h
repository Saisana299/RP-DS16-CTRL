#include <note_manager.h>

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
    NoteManager* pNote;

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
        String* addr4, uint8_t* addr5, uint8_t* addr6, NoteManager* addr7
    ){
        pI2c_is_synth = addr1;
        pI2c_is_debug = addr2;
        pSynthMode = addr3;
        pSynthCacheData = addr4;
        pSynthCacheId = addr5;
        pResponse = addr6;
        pNote = addr7;
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
                *pResponse = RES_OK;
                break;

            // 例: {INS_BEGIN, DISP_SET_SHAPE, DATA_BEGIN, 0x02, 0x01, 0x01}
            case DISP_SET_SHAPE:
            {
                if(bytes < 6) {
                    *pResponse = RES_ERROR;
                    return;
                }
                uint8_t data[] = {
                    INS_BEGIN, SYNTH_SET_SHAPE,
                    DATA_BEGIN, 0x01, receivedData[5]
                };
                *pSynthCacheId = receivedData[4];
                for (uint8_t byte: data) {
                    *pSynthCacheData += static_cast<char>(byte);
                }
                *pResponse = RES_OK;
            }
                break;

            // 例: {INS_BEGIN, DISP_SET_SYNTH, DATA_BEGIN, 0x01, 0x02}
            case DISP_SET_SYNTH:
            {
                if(bytes < 5) {
                    *pResponse = RES_ERROR;
                    return;
                }
                *pSynthMode = receivedData[4];
                for(NoteManager::Note note: pNote->notes) {
                    note.num = 0;
                    note.id = 0xff;
                    note.synth = 0;
                }
                uint8_t data[] = {INS_BEGIN, SYNTH_SOUND_STOP};
                *pSynthCacheId = 0xff;
                for (uint8_t byte: data) {
                    *pSynthCacheData += static_cast<char>(byte);
                }
                *pResponse = RES_OK;
            }
                break;

            // 例: {INS_BEGIN, DISP_SET_PAN, DATA_BEGIN, 0x02, 0x02, 0x01}
            case DISP_SET_PAN:
            {
                if(bytes < 6) {
                    *pResponse = RES_ERROR;
                    return;
                }
                uint8_t data[] = {
                    INS_BEGIN, SYNTH_SET_PAN,
                    DATA_BEGIN, 0x01, receivedData[5]
                };
                *pSynthCacheId = receivedData[4];
                for (uint8_t byte: data) {
                    *pSynthCacheData += static_cast<char>(byte);
                }
                *pResponse = RES_OK;
            }
                break;

            // 例: {INS_BEGIN, DISP_RESET_SYNTH, DATA_BEGIN, 0x01, 0xff}
            case DISP_RESET_SYNTH:
            {
                if(bytes < 5) {
                    *pResponse = RES_ERROR;
                    return;
                }
                uint8_t data[] = {INS_BEGIN, SYNTH_SOUND_STOP};
                *pSynthCacheId = receivedData[4];
                for (uint8_t byte: data) {
                    *pSynthCacheData += static_cast<char>(byte);
                }
                *pResponse = RES_OK;
            }
                break;

            // 例: {INS_BEGIN, DISP_SET_ATTACK, DATA_BEGIN, 0x06, 0xff, 0x00, 0x60, 0x00, 0x00, 0x00}
            case DISP_SET_ATTACK:
            case DISP_SET_DECAY:
            case DISP_SET_RELEASE:
            {
                if(bytes < 10) {
                    *pResponse = RES_ERROR;
                    return;
                }

                uint8_t message = SYNTH_SET_ATTACK;
                if(receivedData[1] == DISP_SET_DECAY) message = SYNTH_SET_DECAY;
                else if(receivedData[1] == DISP_SET_RELEASE) message = SYNTH_SET_RELEASE;

                uint8_t data[] = {
                    INS_BEGIN, message,
                    DATA_BEGIN, 0x05, receivedData[5],
                    receivedData[6], receivedData[7], receivedData[8], receivedData[9]
                };
                *pSynthCacheId = receivedData[4];
                for (uint8_t byte: data) {
                    *pSynthCacheData += static_cast<char>(byte);
                }
                *pResponse = RES_OK;
            }
                break;

            // 例: {INS_BEGIN, DISP_SET_SUSTAIN, DATA_BEGIN, 0x05, 0xff, 0x60, 0x00, 0x00, 0x00}
            case DISP_SET_SUSTAIN:
            {
                if(bytes < 9) {
                    *pResponse = RES_ERROR;
                    return;
                }
                uint8_t data[] = {
                    INS_BEGIN, SYNTH_SET_SUSTAIN,
                    DATA_BEGIN, 0x05, receivedData[5], receivedData[6], receivedData[7], receivedData[8]
                };
                *pSynthCacheId = receivedData[4];
                for (uint8_t byte: data) {
                    *pSynthCacheData += static_cast<char>(byte);
                }
                *pResponse = RES_OK;
            }
                break;

            // 例: {INS_BEGIN, DISP_DEBUG_ON}
            case DISP_DEBUG_ON:
            {
                if(bytes < 2) {
                    *pResponse = RES_ERROR;
                    return;
                }
                // 通信が終わった後にDEBUGモードを有効化
                *pSynthCacheData = "debug";
                *pResponse = RES_OK;
            }
                break;
        }
    }
};

#endif // DISPLAYCONTROL_H