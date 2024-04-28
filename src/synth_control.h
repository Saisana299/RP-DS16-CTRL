#include <Arduino.h>
#include <display_control.h>

#ifndef SYNTHCONTROL_H
#define SYNTHCONTROL_H

#define S1_SDA_PIN 26
#define S1_SCL_PIN 27
#define S1_I2C_ADDR 0x08
#define S2_SDA_PIN 20
#define S2_SCL_PIN 21
#define S2_I2C_ADDR 0x09

class SynthControl {
private:
    TwoWire& disp = Wire;
    TwoWire& synth1 = Wire1;
    TwoWire& synth2 = Wire;
    bool* pI2c_is_synth;
    String* pSynthCacheData;
    uint8_t* pSynthCacheId;
    DisplayControl* pDisplay;

public:
    SynthControl(bool* addr1, String* addr2, uint8_t* addr3, DisplayControl* addr4) {
        pI2c_is_synth = addr1;
        pSynthCacheData = addr2;
        pSynthCacheId = addr3;
        pDisplay = addr4;
    }

    void synth1Write(const uint8_t * data, size_t size) {
        synth1.beginTransmission(S1_I2C_ADDR);
        synth1.write(data, size);
        synth1.endTransmission();
    }

    void synth2Write(const uint8_t * data, size_t size) {
        synth2.beginTransmission(S2_I2C_ADDR);
        synth2.write(data, size);
        synth2.endTransmission();
    }

    void beginSynth() {//synthCacheData 
        disp.end();//todo

        synth1.setSDA(S1_SDA_PIN);
        synth1.setSCL(S1_SCL_PIN);
        synth2.setSDA(S2_SDA_PIN);
        synth2.setSCL(S2_SCL_PIN);

        synth1.begin();
        synth1.setClock(1000000);
        synth2.begin();
        synth2.setClock(1000000);

        // cache処理
        if(pSynthCacheData->equals("debug")) {
            pDisplay->beginDebug();
        }
        else if(!pSynthCacheData->equals("")) {
            size_t size = pSynthCacheData->length();
            uint8_t data[size];
            for (int i = 0; i < size; i++) {
                data[i] = static_cast<uint8_t>(pSynthCacheData->c_str()[i]);
            }
            if (*pSynthCacheId == 0x01) {
                synth1Write(data, size);
            } else if(*pSynthCacheId == 0x02) {
                synth2Write(data, size);
            } else if(*pSynthCacheId == 0xff) {
                synth1Write(data, size);
                synth2Write(data, size);
            }
            *pSynthCacheData = "";
            *pSynthCacheId = 0x00;
        }

        *pI2c_is_synth = true;
    }
};

#endif // SYNTHCONTROL_H