// System Exclusive
#define MIDI_EXCL                 0xF0

// 2nd:note[0-127], 3rd:velocity[0-127]
#define MIDI_CH1_NOTE_ON          0x90
// 2nd:note[0-127], 3rd:velocity[0-127]
#define MIDI_CH1_NOTE_OFF         0x80
// 2nd:note[0-127], 3rd:pressure[0-127]
#define MIDI_CH1_POLY_AFTERTOUCH  0xA0
// 2nd:program no.[0-127]
#define MIDI_CH1_PROGRAM_CHANGE   0xC0
// 2nd:pressure[0-127]
#define MIDI_CH1_AFTERTOUCH       0xD0
// 2nd:LSB[7bit] 3rd:MSB[7bit]
#define MIDI_CH1_PITCH_BEND       0xE0

// 2nd:note[0-127], 3rd:velocity[0-127]
#define MIDI_CH2_NOTE_ON          0x91
// 2nd:note[0-127], 3rd:velocity[0-127]
#define MIDI_CH2_NOTE_OFF         0x81
// 2nd:note[0-127], 3rd:pressure[0-127]
#define MIDI_CH2_POLY_AFTERTOUCH  0xA1
// 2nd:program no.[0-127]
#define MIDI_CH2_PROGRAM_CHANGE   0xC1
// 2nd:pressure[0-127]
#define MIDI_CH2_AFTERTOUCH       0xD1
// 2nd:LSB[7bit] 3rd:MSB[7bit]
#define MIDI_CH2_PITCH_BEND       0xE1

// 2nd:Ctrl no.[0-119] or Mode no.[120-127] 3rd:data[0-127]
#define MIDI_CH1_CTRL_MODE_CHANGE 0xB0
    // CTRL CHANGE //

    #define CTRL_CHG_SUSTAIN 0x40 // OFF 

    // MODE CHANGE //

    #define MODE_CHG_SOUND_OFF  0x78 // 0
    #define MODE_CHG_RESET_CTRL 0x79 // 0
    #define MODE_CHG_LC_CTRL    0x7A // 0,127
    #define MODE_CHG_NOTE_OFF   0x7B // 0
    #define MODE_CHG_OMNI_OFF   0x7C // 0
    #define MODE_CHG_OMNI_ON    0x7D // 0
    #define MODE_CHG_MONO_MODE  0x7E // on:0,off:voice
    #define MODE_CHG_POLY_MODE  0x7F // 0