#include <Arduino.h>

#ifndef NOTEMANAGER_H
#define NOTEMANAGER_H

class NoteManager {
private:
    static const int MAX_NOTES = 8;

    // notes から指定したノートのインデックスを返す
    int8_t getNotesIndex(uint8_t note) {
        for(int8_t i = 0; i < MAX_NOTES; i++) {
            if(notes[i].id == note) return i;
        }
        return -1;
    }

public:
    // モードチェンジ時にリセットする
    struct Note {
        uint8_t num;
        uint8_t id;
        uint8_t synth; //0x01-0x02
    };
    Note notes[MAX_NOTES];

    NoteManager() {
        for(Note note: notes) {
            note.num = 0;
            note.id = 0xff;
            note.synth = 0;
        }
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
                notes[i].num--;
            }
        }

        notes[index].num = 0;
        notes[index].id = 0xff;
        notes[index].synth = 0;
        return synth;
    }
};

#endif // NOTEMANAGER_H