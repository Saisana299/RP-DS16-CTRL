# RP-DS16-CTRL
[![GitHub license](https://img.shields.io/badge/RP--DS16-Rev.3.0-seagreen)](https://github.com/Saisana299/RP-DS16)　

RP2040を利用したWavetableシンセ「RP-DS16」のシンセ制御部  
MIDI-IN処理、RP-DS16-SYNTH制御

<span style="font-size: 200%;">This project has been suspended.</span>  
<span style="font-size: 200%;">Successor project : [Cranberry-Synth](https://github.com/Saisana299/Cranberry-Synth)</span>


## 概要
- RP2040
    - オーバークロック - 200MHz
    - SYNTHとの通信に I2C0, I2C1 を使用
    - DISPとの通信に I2C0 を使用
- 機能
    - MIDI信号の処理
    - 2台のSYNTHを制御

## GPIO
| RP2040 | MIDI | Note |
|:---:|:---:|:---------:|
| GP1 | TX | MIDI IN |

| RP2040 | UART | Note |
|:---:|:---:|:---------:|
| GP8 | RX | - |
| GP9 | TX | - |

| RP2040 | DISP | Note |
|:---:|:---:|:---------:|
| GP11 | - | Switch Pin |
| GP12 | SDA | - |
| GP13 | SCL | - |

| RP2040 | SYNTH | Note |
|:---:|:---:|:---------:|
| GP20 | SDA | id=2 |
| GP21 | SCL | id=2 |
| GP26 | SDA | id=1 |
| GP27 | SCL | id=1 |