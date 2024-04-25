//// 共通システムコード (予約済み) 127~
#define INS_BEGIN     0x7F // 命令送信ビット
#define DATA_BEGIN    0x80 // データ送信ビット
#define DATA_SEPARATE 0x81 // データ区切り
#define RES_OK        0x82 // 処理正常終了
#define RES_ERROR     0x83 // エラー発生時


//// DISP受信用の命令コード (0x00と0xffは除外) 63~
//// 例：{0x7F, 0x40, 0x80, 0x01(データサイズ 1~255), 0x04(データ)}
////
//// データサイズは255が最大だが区切りを利用するとさらに多くのデータを送れます。
//// 例：{..., 0x80, 0xff, ..., 0x84, 0xff, ...}
#define DISP_CONNECT     0x3F // 接続開始
#define DISP_SET_SHAPE   0x40 // 基本波形を設定
#define DISP_SET_SYNTH   0x41 // シンセモード設定
#define DISP_SET_PAN     0x42 // パンを設定
#define DISP_RESET_SYNTH 0x43 // シンセをリセット
#define DISP_SET_ATTACK  0x44 // アタックを設定
#define DISP_SET_RELEASE 0x45 // リリースを設定
#define DISP_SET_DECAY   0x46 // ディケイを設定
#define DISP_SET_SUSTAIN 0x47 // サステインを設定
#define DISP_DEBUG_ON    0x48 // デバッグモードを有効化


//// SYNTH送信用の命令コード (0x00と0xffは除外) 190~
#define SYNTH_NOTE_ON     0xBE // ノートオン
#define SYNTH_NOTE_OFF    0xBF // ノートオフ
#define SYNTH_SET_SHAPE   0xC0 // 基本波形設定
#define SYNTH_SOUND_STOP  0xC1 // 音の再生を停止する
#define SYNTH_SET_PAN     0xC2 // パン(C/L/R)を設定
#define SYNTH_SET_ATTACK  0xC3 // アタックを設定
#define SYNTH_SET_RELEASE 0xC4 // リリースを設定
#define SYNTH_SET_DECAY   0xC5 // ディケイを設定
#define SYNTH_SET_SUSTAIN 0xC6 // サステインを設定

//// 共通シンセ演奏状態コード
#define SYNTH_SINGLE 0x00
#define SYNTH_OCTAVE 0x01
#define SYNTH_DUAL 0x02
#define SYNTH_MULTI  0x03