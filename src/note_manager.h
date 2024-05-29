#include <synth_control.h>

#ifndef NOTEMANAGER_H
#define NOTEMANAGER_H

class NoteManager {
private:
    TwoWire& synth1 = Wire1;
    TwoWire& synth2 = Wire;
public:
    NoteManager() {}

    uint8_t setNotesNote(uint8_t note) {
        uint8_t synth = 0x00;

        // ノートが既に存在するか確認
        uint8_t data2[] = {INS_BEGIN, SYNTH_IS_NOTE, note};

        synth1.beginTransmission(S1_I2C_ADDR);
        synth1.write(data2, sizeof(data2));
        synth1.endTransmission();
        synth1.requestFrom(S1_I2C_ADDR, 1);
        uint8_t result = synth1.read();
        if(result == 0x01) return 0x01;

        synth2.beginTransmission(S2_I2C_ADDR);
        synth2.write(data2, sizeof(data2));
        synth2.endTransmission();
        synth2.requestFrom(S2_I2C_ADDR, 1);
        result = synth2.read();
        if(result == 0x01) return 0x02;

        // それぞれのsynthの使用状況を取得
        uint8_t data[] = {INS_BEGIN, SYNTH_GET_USED};

        synth1.beginTransmission(S1_I2C_ADDR);
        synth1.write(data, sizeof(data));
        synth1.endTransmission();
        synth1.requestFrom(S1_I2C_ADDR, 1);
        uint8_t s1 = synth1.read();

        synth2.beginTransmission(S2_I2C_ADDR);
        synth2.write(data, sizeof(data));
        synth2.endTransmission();
        synth2.requestFrom(S2_I2C_ADDR, 1);
        uint8_t s2 = synth2.read();

        if(s1 >= 4 && s2 >= 4) synth = 0x01;
        else if(s1 == s2) synth = 0x01;
        else if(s1 < s2) synth = 0x01;
        else if(s1 > s2) synth = 0x02;
        else synth = 0x01;

        return synth;
    }

    uint8_t removeNotesNote(uint8_t note) {
        uint8_t synth = 0x00;

        // noteが存在するシンセを確認
        uint8_t data[] = {INS_BEGIN, SYNTH_IS_NOTE, note};

        synth1.beginTransmission(S1_I2C_ADDR);
        synth1.write(data, sizeof(data));
        synth1.endTransmission();
        synth1.requestFrom(S1_I2C_ADDR, 1);
        uint8_t s1 = synth1.read();

        synth2.beginTransmission(S2_I2C_ADDR);
        synth2.write(data, sizeof(data));
        synth2.endTransmission();
        synth2.requestFrom(S2_I2C_ADDR, 1);
        uint8_t s2 = synth2.read();

        if(s1 == 0x01) synth = 0x01;
        else if(s2 == 0x01) synth = 0x02;
        else synth = 0xff;

        return synth;
    }
};

#endif // NOTEMANAGER_H