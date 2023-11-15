/*
 * led_matrix_32_32_invader
 *
*
/*******************************************/
/* LEDドットマトリクスの                    */
/* invaderゲーム
 */
/* LEDドットマトリクス 32x32  */
/* 使用マイコンボード Nucleo  */
/*　使用LEDドットマトリクス（32x32 color　）            */
/*******************************************/
/* A:PIN1(PA0:A0) DCBA=0000でLINE0,8          */
/* B:PIN1(PA1:A1) 0001でLINE1,9               */
/* C:PIN2(PA4:A2)　0010でLINE2,10              */
/* D:PIN3(PA5:D13) 1111でLINE15,31             */
/*　R0_SIN:PIN(PA6:D12)　赤シリアルデータROW1-16　　　   */
/*　R1_SIN:PIN(PA7:D11)　赤シリアルデータROW17-32　    */
/*　G0_SIN:PIN(PA8:D7)　緑シリアルデータROW1-16　　　　   */
/*　G1_SIN:PIN(PA9:D8)　緑シリアルデータROW17-32　　　   */
/*　B0_SIN:PIN(PA10:D2） 青シリアルデータROW1-16　　　　   */
/*　B1_SIN:PIN(PA11:CN10_EV_105)　青シリアルデータROW17-32　　　   */
/*　CLK:PIN(PB5:D4)　シフトクロック     　            */
/*　STB:PIN(PB6:CN10_17)　1でスルー、0で保持     　　 　    */
/*　OE:PIN(PB7:CN7_21)　　0で出力 　　　　　　　　　　　       */
/* MON(PB8:D15) */

// インクルードファイル
#include <stddef.h>
#include "stm32f10x.h"

#define waite_time 1

// スイッチの設定
// 0bFEDCBA9876543210
#define SW1 0b0000000000010000             // Sw1(PC4)用のマスク値
#define SW2 0b0000000000100000             // Sw2(PC5)用のマスク値
#define SW3 0b0000000001000000             // Sw3(PC6)用のマスク値
#define SW4 0b0000000010000000             // Sw4(PC7)用のマスク値
#define LP_Count_Enable 0b0000000000001000 // スイッチ長押し有効を示すフラグ
#define LP_Interval 500                    // スイッチ長押しの間隔を計測

// 圧電ブザーの音程設定
#define C4 523 		　  //ド
#define D4 587       // レ
#define E4 659       // ミ
#define F4 698 		　  //ファ
#define G4 783       // ファ
#define A4 880       // ラ
#define B4 987       // シ
#define C5 1046      // ド
#define D5 1174      // レ
#define bom_tune 523 // ボムの発射音
#define destory 50   // 障害物の破壊音
#define LEN1 50      // 音の長さ

// LEDマトリックスカラー設定
#define A_HI 0x1
#define B_HI 0x2
#define C_HI 0x10
#define D_HI 0x20
#define LINE_MASK 0x33
#define R0_HI 0x40
#define R1_HI 0x80
#define G0_HI 0x100
#define G1_HI 0x200
#define B0_HI 0x400
#define B1_HI 0x800
#define CLK_HI 0x20
#define STB_HI 0x40
#define OE_HI 0x80
#define MON_HI 0x100

// グローバル変数
int beam_flag = 0;      // 敵の攻撃の判定（beam_countで計測）
int beam_next_flag = 0; // LEDマトリックスの下画面への遷移を判定するフラグ
int beam_count = 0;     // 敵の攻撃の間隔
int beam_count_1 = 0;   // LEDマトリックスの下画面のカウント
int beam_count_0 = 0;   // LEDマトリックスの上画面のカウント

int Switch_ID = 0;         // 現在長押し中のスイッチ（0：なし、1：スイッチ0、2：スイッチ1)
int LP_Count_Flag = 0;     // （飛行機）スイッチ長押し時のカウントアップタイミングを示すフラグ変数
int LP_Bom_Count_Flag = 0; // （ボム）スイッチ長押し時のカウントアップタイミングを示すフラグ変数

int TimeInt = 0;     // delay処理（Waitする間隔を指定するため）のグローバル変数（SysTickで減算）
int pattern_num = 0; // 表示させたい8x8LEDマトリクスのフォントデータの番号

int set_time = 10; // ルーレットの回転スピード

int up = 0;            // スイッチの立ち上がり検出フラグ、0で初期化
int sw_now = 0;        // 現在のスイッチ押下状態、0で初期化
int Bom_Flag = 0;      // スイッチが押された場合に、弾を発射するフラグ変数
int Bom_Next_Flag = 1; // スイッチが押された場合に、弾を発射するフラグ変数(下の画面に遷移するようにBom_Flagを受け継ぐ)

int Bom_Count_0 = 0; // 弾が無限にシフトローテーションしないように弾の移動をカウントする変数(下画面)
int Bom_Count_1 = 0; // 弾が無限にシフトローテーションしないように弾の移動をカウントする変数(上画面)
int Bom_Loss = 0;    // 弾が発射済みのかどうかのフラグ変数
int Beam_Loss = 0;   // 弾が発射済みのかどうかのフラグ変数

int enemy_number = 0; // 壁、敵を倒したかどうか確認する変数
int mask = 0;         // 弾が対象に当たった時の判定用変数
int death_flag = 0;   // 敵の攻撃を受けた際のフラグ

int judge = 0;             // 弾が対象に当たった時の判定用変数
int damage = 0;            // 敵の攻撃を受けた際の飛行機の状態一時保管
int delay_time;            // 時間の計測
int stage_clear_music = 4; // melody配列のスタート位置

// ダイナミック点灯用変数
int clk_cnt = 0;   // ダイナミック点灯列数カウント
int line = 0;      // ダイナミック点灯行
int line_bits = 0; // ダイナミック点灯行の制御
int line_data_r0;  // 赤色上部
int line_data_r1;  // 赤色下部
int line_data_g0;  // 緑色上部
int line_data_g1;  // 緑色下部
int line_data_b0;  // 青色上部
int line_data_b1;  // 青色上部

int data_r0[16] = {0x0, 0x0, 0x0, 0x0,
                   0x0, 0x0, 0x0, 0x0,
                   0x0, 0x0, 0x0, 0x0,
                   0x0, 0x0, 0x0, 0x0};
int data_r1[16] = {0x0, 0x0, 0x0, 0x0,
                   0x0, 0x0, 0x0, 0x0,
                   0x0, 0x0, 0x0, 0x0,
                   0x0, 0x0, 0x0, 0x0};
int data_g0[16] = {0x0, 0x0, 0x0, 0x0,
                   0x0, 0x0, 0x0, 0x0,
                   0x0, 0x0, 0x0, 0x0,
                   0x0, 0x0, 0x0, 0x0};
int data_g1[16] = {0x0, 0x0, 0x0, 0x0,
                   0x0, 0x0, 0x0, 0x0,
                   0x0, 0x0, 0x0, 0x0,
                   0x0, 0x0, 0x0, 0x0};
int data_b0[16] = {0x0, 0x0, 0x0, 0x0,
                   0x0, 0x0, 0x0, 0x0,
                   0x0, 0x0, 0x0, 0x0,
                   0x0, 0x0, 0x0, 0x0};
int data_b1[16] = {0x0, 0x0, 0x0, 0x0,
                   0x0, 0x0, 0x0, 0x0,
                   0x0, 0x0, 0x0, 0x0,
                   0x0, 0x0, 0x0, 0x0};

// 音符データの構造体
typedef struct
{
    int pitch;
    int duration;
} Note;

Note *melody_address; // 演奏させる曲の楽譜データの先頭アドレス

// 楽曲の音符データ
Note melody[][20] = {
    {{bom_tune, LEN1}}, // ボムの発射
    {{destory, LEN1}},  // 敵の破壊
    {{D5, LEN1}},
    {{E4, LEN1}}, // オープニング
    {{D4, LEN1}},
    {{A4, LEN1}},
    {{B4, LEN1}},
    {{A4, LEN1}},
    {{D4, LEN1}},
    {{G4, LEN1 * 2}}, // クリア
    {{C5, LEN1}},
    {{D5, LEN1}},          // 敵の出現（セカンドステージ）
    {{destory, LEN1 / 2}}, // 敵の破壊2
};

// 飛行機の描画
int airplane_p0[16] =
    {0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0};
int airplane_p1[16] =
    {0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x8000, 0x8000, 0x1C000, 0x3E000, 0x3E000, 0x3E000, 0x3E000};
int airplane_stock_p1[16] =
    {0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x8000, 0x8000, 0x1C000, 0x3E000, 0x3E000, 0x3E000, 0x3E000};

// 壁の描画
int kabe_p0[16] =
    {0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x11224488, 0x1F3E7CF8};
int kabe_stock_p0[16] =
    {0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x11224488, 0x1F3E7CF8};

// 敵の攻撃の描画（上画面）
int beam_p0[16] =
    {0x0, 0x100800, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0};
int beam_stock_p0[16] =
    {0x0, 0x100800, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0};
int beam_set_p0[16] =
    {0x0, 0x100800, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0};

// 敵の攻撃の描画（下画面）
int beam_p1[16] =
    {0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0};
int beam_stock_p1[16] =
    {0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0};

// 動く敵キャラの描画（黄色）
int enemy_p0[16] =
    {0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0};
int enemy_stock_p0[16] =
    {0x0, 0x100800, 0x381C00, 0x542A00, 0x7C3E00, 0x281400, 0x542A00, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0};

int enemy_down_p0[16] =
    {0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0};
int enemy_down_stock_p0[16] =
    {0x0, 0x100800, 0x381C00, 0x542A00, 0x7C3E00, 0x281400, 0x542A00, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0};

int enemy_up_p0[16] =
    {0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0};
int enemy_up_stock_p0[16] =
    {0x0, 0x100800, 0x381C00, 0x542A00, 0x7C3E00, 0x281400, 0x542A00, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0};

// 動く敵キャラの描画（青色）
int enemy_p1[16] =
    {0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0};
int enemy_stock_p1[16] =
    {0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x50220A0, 0x1741C2E8, 0xA82A150, 0x703E0E0, 0x8814110, 0x0, 0x0, 0x0};

int enemy_down_p1[16] =
    {0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0};
int enemy_down_stock_p1[16] =
    {0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x50220A0, 0x1741C2E8, 0xA82A150, 0x703E0E0, 0x8814110, 0x0, 0x0, 0x0};

int enemy_up_p1[16] =
    {0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0};
int enemy_up_stock_p1[16] =
    {0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x8814110, 0x705D0E0, 0xA82A150, 0xF81C1F0, 0x50220A0, 0x0, 0x0, 0x0};

// 弾薬の描写
int bom_p0[16] =
    {0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0};
int bom_p1[16] =
    {0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x8000, 0x8000, 0x8000};
int bom_set_p0[16] =
    {0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0};
int bom_set_p1[16] =
    {0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x8000, 0x8000, 0x8000};
int bom_stock_p1[16] =
    {0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x8000, 0x8000, 0x8000};

// オープニング描画
int first_stage_p0[3][16] = {
    {0xF0000F0, 0xF0000F0, 0xF0000F0, 0xF0000F0, 0xF00F00, 0xF00F00, 0xF00F00, 0xF00F00, 0xFFFF00, 0xFFFF00, 0xFFFF00, 0xFFFF00, 0xF0FF0F0, 0xF0FF0F0, 0xF0FF0F0, 0xF0FF0F0},
    {0xF0000F0, 0xF0000F0, 0xF0000F0, 0xF0000F0, 0xF00F00, 0xF00F00, 0xF00F00, 0xF00F00, 0xF0FFFF0F, 0xF0FFFF0F, 0xF0FFFF0F, 0xF0FFFF0F, 0xFF0FF0FF, 0xFF0FF0FF, 0xFF0FF0FF, 0xFF0FF0FF},
    {0xF0000F0, 0xF0000F0, 0xF0000F0, 0xF0000F0, 0xF00F00, 0xF00F00, 0xF00F00, 0xF00F00, 0xFFFF00, 0xFFFF00, 0xFFFF00, 0xFFFF00, 0xF0FF0F0, 0xF0FF0F0, 0xF0FF0F0, 0xF0FF0F0}};
int first_stage_p1[3][16] = {
    {0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xF0F00F0F, 0xF0F00F0F, 0xF0F00F0F, 0xF0F00F0F, 0xFFFF00, 0xFFFF00, 0xFFFF00, 0xFFFF00},
    {0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFF0, 0xFFFFFF0, 0xFFFFFF0, 0xFFFFFF0, 0xF00F00, 0xF00F00, 0xF00F00, 0xF00F00, 0xF0000F0, 0xF0000F0, 0xF0000F0, 0xF0000F0},
    {0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xF0F00F0F, 0xF0F00F0F, 0xF0F00F0F, 0xF0F00F0F, 0xFFFF00, 0xFFFF00, 0xFFFF00, 0xFFFF00}};

// 敵キャラの出現の描画
int second_stage_p0[8][16] = {
    {0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0},
    {0x0, 0x0, 0x0, 0x4200420, 0x2400240, 0x3C00BD0, 0x5A00DB0, 0xFF00FF0, 0xFF007E0, 0xA500240, 0x3C00420, 0x0, 0x0, 0x0, 0x0, 0x0},
    {0x0, 0x0, 0x0, 0x4200420, 0x2400240, 0xBD003C0, 0xDB005A0, 0xFF00FF0, 0x7E00FF0, 0x2400A50, 0x42003C0, 0x0, 0x0, 0x0, 0x0, 0x0},
    {0x0, 0x0, 0x0, 0x4200420, 0x2400240, 0x3C00BD0, 0x5A00DB0, 0xFF00FF0, 0xFF007E0, 0xA500240, 0x3C00420, 0x0, 0x0, 0x0, 0x0, 0x0},
    {0x0, 0x0, 0x0, 0x4200420, 0x2400240, 0xBD003C0, 0xDB005A0, 0xFF00FF0, 0x7E00FF0, 0x2400A50, 0x42003C0, 0x0, 0x0, 0x0, 0x0, 0x0},
    {0x0, 0x0, 0x0, 0x4200420, 0x2400240, 0x3C00BD0, 0x5A00DB0, 0xFF00FF0, 0xFF007E0, 0xA500240, 0x3C00420, 0x0, 0x0, 0x0, 0x0, 0x0},
    {0x0, 0x0, 0x0, 0x4200420, 0x2400240, 0xBD003C0, 0xDB005A0, 0xFF00FF0, 0x7E00FF0, 0x2400A50, 0x42003C0, 0x0, 0x0, 0x0, 0x0, 0x0},
    {0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0},
};

// エンディングの描画
int End_p0[4][16] = {
    {0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0xFF000, 0xFF000, 0xFF000, 0xFF000},
    {0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0xFFFF00, 0xFFFF00, 0xFFFF00, 0xFFFF00, 0xF00F00, 0xF00F00, 0xF00F00, 0xF00F00},
    {0x0, 0x0, 0x0, 0x0, 0xFFFFFF0, 0xFFFFFF0, 0xFFFFFF0, 0xFFFFFF0, 0xF0000F0, 0xF0000F0, 0xF0000F0, 0xF0000F0, 0xF0000F0, 0xF0000F0, 0xF0000F0, 0xF0000F0},
    {0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xF000000F, 0xF000000F, 0xF000000F, 0xF000000F, 0xF000000F, 0xF000000F, 0xF000000F, 0xF000000F, 0xF000000F, 0xF000000F, 0xF000000F, 0xF000000F}};
int End_p1[4][16] = {
    {0xFF000, 0xFF000, 0xFF000, 0xFF000, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0},
    {0xF00F00, 0xF00F00, 0xF00F00, 0xF00F00, 0xFFFF00, 0xFFFF00, 0xFFFF00, 0xFFFF00, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0},
    {0xF0000F0, 0xF0000F0, 0xF0000F0, 0xF0000F0, 0xF0000F0, 0xF0000F0, 0xF0000F0, 0xF0000F0, 0xFFFFFF0, 0xFFFFFF0, 0xFFFFFF0, 0xFFFFFF0, 0x0, 0x0, 0x0, 0x0},
    {0xF000000F, 0xF000000F, 0xF000000F, 0xF000000F, 0xF000000F, 0xF000000F, 0xF000000F, 0xF000000F, 0xF000000F, 0xF000000F, 0xF000000F, 0xF000000F, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF}};

// 関数のプロトタイプ
void init_system(void); // ハードウェアの初期設定
void waite_func(int);   // 時間待機

void First_Stage();  // オープニングの描写
void Second_Stage(); // 敵キャラの出現の描写
void Ending();       // エンディングの描写

void EnemyRightShift(int *enemy, int *arr, int *arr_next, int size); // 敵キャラの右シフト
void EnemyLeftShift(int *enemy, int *arr, int *arr_next, int size);  // 敵キャラの左シフト
void rightShift(int *arr, int *bom, int *bom_set, int size);         // 飛行機の右シフト
void leftShift(int *arr, int *bom, int *bom_set, int size);          // 飛行機の左シフト

void bom_r1(int *bom_p1, int *bom_p0, int size); /// ボムの発射LEDマットリックスの画面半分
void bom_r0(int *arr, int size);                 /// ボムの発射LEDマットリックスの画面半分

void Judge(int location, int *enemy_p, int *enemy_down_p, int *enemy_up_p); // 動く敵の当たり判定(2ndステージ)
void kabe_Judge(int location, int *enemy_p);                                // 壁の当たり判定(1stステージ)

void waite_func(int time); // 時間待ち関数
unsigned int Get_SW(void); // スイッチの読み取り
void Wait_Time(int time);  // 時間待ち関数

// ブザー関連の関数
void BuzzerOn(int rate, int time);
void play_tone(unsigned int frequency);
void stop_tone();
void play_music(Note data[]);

void beam_r0(int *beam_p0, int *beam_p1, int size);
void beam_r1(int *beam_p1, int size);
void Damage(int location, int *airplane_p1, int *bom_p1, int *beam_p1);

int main(void)
{
    int color;

    RCC_ClocksTypeDef RCC_Clocks;
    init_system();
    RCC_GetClocksFreq(&RCC_Clocks);

    SysTick_Config(RCC_Clocks.HCLK_Frequency / 1000); // 1msでシスティックタイマー

    GPIOA->ODR = 0x0;
    GPIOB->ODR = 0x0;

    // データのコピー
    for (int i = 0; i < 16; i++)
    {
        data_r0[i] = 0x0;
        data_r1[i] = 0x0;
        data_g0[i] = 0x0;
        data_g1[i] = 0x0;
        data_b0[i] = 0x0;
        data_b1[i] = 0x0;
    }

    // 電源起動後に自動的に1stステージへ移行
    while (1)
    {

        First_Stage();
        if (up)
        {

            melody_address = melody[2];
            play_music(melody_address);
            waite_func(300);

            break;
        }
    }

    for (int i = 0; i < 16; i++)
    {
        enemy_p0[i] = enemy_stock_p0[i];
        enemy_p1[i] = enemy_stock_p1[i];
        enemy_up_p0[i] = enemy_up_stock_p0[i];
        enemy_up_p1[i] = enemy_up_stock_p1[i];
        enemy_down_p0[i] = enemy_down_stock_p0[i];
        enemy_down_p1[i] = enemy_down_stock_p1[i];
    }

    while (1)
    {

        // LEDマトリックスへの描写
        for (color = 0; color < 8; color++)
        {
            for (int i = 0; i < 16; i++)
            {
                data_r0[i] = airplane_p0[i] | bom_p0[i] | enemy_p0[i] | beam_p0[i]; // 上部部17~32
                data_r1[i] = airplane_p1[i] | bom_p1[i] | beam_p1[i];               // 下部1~16
                data_g0[i] = kabe_p0[i] | enemy_p0[i] | bom_p0[i];
                data_g1[i] = bom_p1[i];
                data_b0[i] = enemy_p1[i] | beam_p0[i];
                data_b1[i] = beam_p1[i];
            }
        }

        // ボムが発射された場合に、Bom_Flagが1になり、ボムがシフトする
        if (Bom_Flag == 1)
        {
            waite_func(5);
            if (Bom_Count_1 == 16)
            {
                Bom_Count_1 = 0;
                Bom_Next_Flag = 1;
                Bom_Flag = 0;
            }
            else
            {
                bom_r1(bom_p1, bom_p0, 16);
                Bom_Count_1++;
            }
        }

        if (Bom_Next_Flag == 1)
        { // LEDマトリックスの後半のビットを描写
            waite_func(5);
            if (Bom_Count_0 == 16)
            {
                Bom_Count_0 = 0;
                Bom_Next_Flag = 0;
                Bom_Loss = 1;
            }
            else
            {
                bom_r0(bom_p0, 16);
                Bom_Count_0++;
            }
        }

        if (Bom_Loss)
        { // ボム発射後にボムを充てんする。
            Bom_Loss = 0;
            for (int i = 0; i < 16; i++)
            {
                bom_p1[i] = bom_set_p1[i];
            }
        }

        for (int j = 0; j < 16; j++)
        {
            enemy_number |= enemy_p0[j] + enemy_up_p0[j] + enemy_down_p0[j];
        }

        // 動く敵がすべて倒された場合にはエンディングへ
        if (enemy_number == 0)
        {
            Ending();
            enemy_number = 1;

            while (1)
            {

                First_Stage();
                if (up)
                {
                    melody_address = melody[2];
                    play_music(melody_address);
                    waite_func(300);

                    break;
                }
            }
            for (int i = 0; i < 16; i++)
            {
                kabe_p0[i] = kabe_stock_p0[i];
                enemy_p0[i] = enemy_stock_p0[i];
                enemy_p1[i] = enemy_stock_p1[i];
                enemy_up_p0[i] = enemy_up_stock_p0[i];
                enemy_up_p1[i] = enemy_up_stock_p1[i];
                enemy_down_p0[i] = enemy_down_stock_p0[i];
                enemy_down_p1[i] = enemy_down_stock_p1[i];
                beam_p0[i] = beam_stock_p0[i];
                airplane_p1[i] = airplane_stock_p1[i];
                bom_p1[i] = bom_stock_p1[i];
                bom_set_p1[i] = bom_stock_p1[i];
            }
        }
        else
        {
            enemy_number = 0;
        }

        if (beam_flag != 1)
        {
            if (up & LP_Count_Enable)
            {
                if (LP_Count_Flag)
                {
                    LP_Count_Flag = 0;
                    switch (Switch_ID)
                    {
                    case 1:
                        leftShift(airplane_p1, bom_p1, bom_set_p1, 16);
                        break;
                    case 4:
                        rightShift(airplane_p1, bom_p1, bom_set_p1, 16);
                        break;
                    default:
                        break;
                    }
                }

                if (LP_Bom_Count_Flag)
                {
                    LP_Bom_Count_Flag = 0;
                    switch (Switch_ID)
                    {

                    case 2:
                        Bom_Flag = 1;
                        break;
                    case 3:
                        Bom_Flag = 1;
                        break;
                    default:
                        break;
                    }
                }
            }
            else
            {

                if (up & SW1)
                {
                    // SW1スイッチが押されたら左シフトする
                    up &= ~SW1;
                    leftShift(airplane_p1, bom_p1, bom_set_p1, 16);
                }
                if (up & SW2)
                {
                    // SW2スイッチが押されたら弾を発射する
                    up &= ~SW2;
                    Bom_Flag = 1;
                    melody_address = melody[5];
                    play_music(melody_address);
                }
                if (up & SW3)
                {
                    // SW3スイッチが押されたら弾を発射する
                    up &= ~SW3;
                    Bom_Flag = 1;
                    melody_address = melody[5];
                    play_music(melody_address);
                }
                if (up & SW4)
                {
                    // SW4スイッチが押されたら右シフトする
                    up &= ~SW4;
                    rightShift(airplane_p1, bom_p1, bom_set_p1, 16);
                }
            }
        }

        // 敵の攻撃。ビームが発射された場合に、beam_Flagが1になり、ビームがシフトする
        if (beam_flag == 1)
        {
            waite_func(20);
            if (beam_count_0 == 15)
            {
                beam_count_0 = 0;
                beam_next_flag = 1;
                beam_flag = 0;
            }
            else
            {
                beam_r0(beam_p0, beam_p1, 16);
                beam_count_0++;
            }
        }

        if (beam_next_flag == 1)
        { // LEDマトリックスの後半のビットを描写
            waite_func(20);
            if (beam_count_1 == 16)
            {
                beam_count_1 = 0;
                beam_next_flag = 0;
                Beam_Loss = 1;
            }
            else
            {
                beam_r1(beam_p1, 16);
                beam_count_1++;
            }
        }

        if (Beam_Loss)
        { // ボム発射後にボムを充てんする。
            Beam_Loss = 0;
            beam_p0[1] = beam_set_p0[1];
        }

        // 飛行機に弾が当たった場合の処理
        if (death_flag == 1)
        {

            death_flag = 0;
            Ending();

            while (1)
            {

                First_Stage();
                if (up)
                {
                    melody_address = melody[2];
                    play_music(melody_address);
                    waite_func(300);
                    break;
                }
            }
            for (int i = 0; i < 16; i++)
            {
                kabe_p0[i] = kabe_stock_p0[i];
                enemy_p0[i] = enemy_stock_p0[i];
                enemy_p1[i] = enemy_stock_p1[i];
                enemy_up_p0[i] = enemy_up_stock_p0[i];
                enemy_up_p1[i] = enemy_up_stock_p1[i];
                enemy_down_p0[i] = enemy_down_stock_p0[i];
                enemy_down_p1[i] = enemy_down_stock_p1[i];
                beam_p0[i] = beam_stock_p0[i];
                airplane_p1[i] = airplane_stock_p1[i];
                bom_p1[i] = bom_stock_p1[i];
                bom_set_p1[i] = bom_stock_p1[i];
            }
        }

    } // while文終端
} // main関数の終端

// 電源起動後にオープニング画面を描写する
void First_Stage()
{

    int number = 0;

    for (int j = 0; j < 3; j++)
    {
        for (int i = 0; i < 16; i++)
        {
            if (number == 3)
                number = 0;

            switch (number)
            {
            case 0:
                data_r0[i] = first_stage_p0[j][i];
                data_r1[i] = first_stage_p1[j][i];
                break;
            case 1:
                data_g0[i] = first_stage_p0[j][i];
                data_g1[i] = first_stage_p1[j][i];
                break;
            case 2:
                data_b0[i] = first_stage_p0[j][i];
                data_b1[i] = first_stage_p1[j][i];
                break;
            }
        }

        waite_func(1000);
        number++;
        for (int i = 0; i < 16; i++)
        {
            data_r0[i] = 0x0;
            data_r1[i] = 0x0;
            data_g0[i] = 0x0;
            data_g1[i] = 0x0;
            data_b0[i] = 0x0;
            data_b1[i] = 0x0;
        }
    }

    for (int i = 0; i < 16; i++)
    {
        data_r0[i] = 0x0;
        data_r1[i] = 0x0;
        data_g0[i] = 0x0;
        data_g1[i] = 0x0;
        data_b0[i] = 0x0;
        data_b1[i] = 0x0;
    }
}

// 動く敵がすべて倒された場合にエンディングを描写する
void Ending()
{

    int number = 0;
    for (int j = 0; j < 6; j++)
    {
        for (int i = 0; i < 16; i++)
        {

            switch (number)
            {
            case 0:
                data_r0[i] = End_p0[0][i]; // 上部17~32
                data_r1[i] = End_p1[0][i]; // 上部17~32
                break;
            case 1:
                data_r0[i] = End_p0[0][i]; // 上部17~32
                data_r1[i] = End_p1[0][i]; // 上部17~32
                data_g0[i] = End_p0[1][i]; // 上部17~32
                data_g1[i] = End_p1[1][i]; // 上部17~32
                break;
            case 2:
                data_r0[i] = End_p0[0][i]; // 上部17~32
                data_r1[i] = End_p1[0][i]; // 上部17~32
                data_g0[i] = End_p0[1][i]; // 上部17~32
                data_g1[i] = End_p1[1][i]; // 上部17~32
                data_b0[i] = End_p0[2][i]; // 上部17~32
                data_b1[i] = End_p1[2][i]; // 上部17~32
                break;
            case 3:
                data_r0[i] = End_p0[0][i] | End_p0[3][i];
                data_r1[i] = End_p1[0][i] | End_p1[3][i];
                data_g0[i] = End_p0[1][i] | End_p0[3][i];
                data_g1[i] = End_p1[1][i] | End_p1[3][i];
                data_b0[i] = End_p0[2][i] | End_p0[3][i];
                data_b1[i] = End_p1[2][i] | End_p1[3][i];
                break;
            case 4:
                data_r0[i] = End_p0[0][i] | End_p0[1][i];
                data_r1[i] = End_p1[0][i] | End_p1[1][i];
                data_g0[i] = End_p0[1][i] | End_p0[2][i];
                data_g1[i] = End_p1[1][i] | End_p1[2][i];
                data_b0[i] = End_p0[2][i] | End_p0[3][i];
                data_b1[i] = End_p1[2][i] | End_p1[3][i];
                break;

            case 5:
                data_r0[i] = End_p0[0][i] | End_p0[1][i] | End_p0[2][i] | End_p0[3][i];
                data_r1[i] = End_p1[0][i] | End_p1[1][i] | End_p1[2][i] | End_p1[3][i];
                data_g0[i] = End_p0[0][i] | End_p0[1][i] | End_p0[2][i] | End_p0[3][i];
                data_g1[i] = End_p1[0][i] | End_p1[1][i] | End_p1[2][i] | End_p1[3][i];
                data_b0[i] = End_p0[0][i] | End_p0[1][i] | End_p0[2][i] | End_p0[3][i];
                data_b1[i] = End_p1[0][i] | End_p1[1][i] | End_p1[2][i] | End_p1[3][i];
                break;
            }
        }
        number++;
        waite_func(500);
        melody_address = melody[stage_clear_music];
        play_music(melody_address);
        stage_clear_music++;
    }
    for (int i = 0; i < 16; i++)
    {
        data_r0[i] = 0x0;
        data_r1[i] = 0x0;
        data_g0[i] = 0x0;
        data_g1[i] = 0x0;
        data_b0[i] = 0x0;
        data_b1[i] = 0x0;
    }
    stage_clear_music = 4;
}

void EnemyRightShift(int *enemy, int *arr, int *arr_next, int size)
{

    if (arr[7] & 0x80000000)
    {
        return;
    }

    for (int i = 0; i < size; i++)
    {

        arr[i] = arr[i] << 1;
        arr_next[i] = arr_next[i] << 1;

        enemy[i] = arr[i];
    }
}

void EnemyLeftShift(int *enemy, int *arr, int *arr_next, int size)
{

    if (arr[7] & 0x1)
    {
        return;
    }

    for (int i = 0; i < size; i++)
    {

        arr[i] = (int)(((unsigned int)arr[i]) >> 1);
        arr_next[i] = (int)(((unsigned int)arr_next[i]) >> 1);
        enemy[i] = (int)((unsigned int)arr[i]);
    }
}

void rightShift(int *arr, int *bom, int *bom_set, int size)
{

    if (arr[size - 1] & 0x80000000)
    {
        return;
    }

    for (int i = 0; i < size; i++)
    {
        arr[i] = arr[i] << 1;
        bom[i] = bom[i] << 1;
        bom_set[i] = bom_set[i] << 1;
    }
}

void leftShift(int *arr, int *bom, int *bom_set, int size)
{

    if (arr[size - 1] & 0x1)
    {
        return;
    }

    for (int i = 0; i < size; i++)
    {

        // 右にシフトされる際、符号ビットが拡張される。0xF8000000は　マイナスとして解釈され、右シフトされる際、最上位ビット（符号ビット）が拡張
        // 操作を行う前に、整数を符号なし整数としてキャストする
        arr[i] = (int)(((unsigned int)arr[i]) >> 1);
        bom[i] = (int)(((unsigned int)bom[i]) >> 1);
        bom_set[i] = (int)(((unsigned int)bom_set[i]) >> 1);
    }
}

void bom_r1(int *bom_p1, int *bom_p0, int size)
{

    int firstElement = bom_p1[0];
    for (int i = 0; i < size - 1; i++)
    {
        if (Bom_Count_1 == 14)
        {
            bom_p0[15] = bom_p1[0];
        }
        bom_p1[i] = bom_p1[i + 1];
    }
    bom_p1[15] = firstElement; // 最初の要素を最後にセット
}

void bom_r0(int *bom_p0, int size)
{

    int firstElement = bom_p0[0];
    for (int i = 0; i < size - 1; i++)
    {

        kabe_Judge(size - 1 - i, kabe_p0);
        Judge(size - 1 - i, enemy_p0, enemy_down_p0, enemy_up_p0);
        Judge(size - 1 - i, enemy_p1, enemy_down_p1, enemy_up_p1);

        bom_p0[i] = bom_p0[i + 1];
    }

    bom_p0[15] = firstElement; // 最初の要素を最後にセット
    bom_p0[0] = 0x0;           // 画面からボムを消す
}

void beam_r0(int *beam_p0, int *beam_p1, int size)
{
    // 位置を探す
    int pos = -1;
    for (int i = 1; i < size - 1; i++)
    {
        if (beam_p0[i] != 0x0)
        {
            pos = i;
            break;
        }
        if (i == 14)
            beam_p0[15] = 0x0;
    }

    // 位置が見つからない、またはすでに最後の位置にある場合は何もしない
    if (pos == -1)
        return;
    if (pos == 14)
    {
        beam_p1[0] = beam_p0[pos];
    }

    // 指定された位置の要素を0x0に設定し、次の位置に0x100800を設定する

    beam_p0[pos + 1] = beam_p0[pos];
    beam_p0[pos] = 0x0;
}

void beam_r1(int *beam_p1, int size)
{
    // 位置を探す
    int pos = -1;
    for (int i = 0; i < size - 1; i++)
    {
        Damage(size - 1 - i, airplane_p1, bom_p1, beam_p1);

        if (beam_p1[i] != 0x0)
        {
            pos = i;
            break;
        }
        if (i == 14)
            beam_p1[15] = 0x0;
    }

    // 位置が見つからない、またはすでに最後の位置にある場合は何もしない
    if (pos == -1)
        return;
    if (pos == 15)
        beam_p1[pos] = 0x0;

    // 指定された位置の要素を0x0に設定し、次の位置に0x100800を設定する
    beam_p1[pos + 1] = beam_p1[pos];
    beam_p1[pos] = 0x0;
}

void Damage(int location, int *airplane_p1, int *bom_p1, int *beam_p1)
{

    int before = airplane_p1[location];
    damage = beam_p1[location] + airplane_p1[location];

    if (airplane_p1[location] != damage)
    {

        mask = damage ^ beam_p1[location];                    // 変化したビットを取得
        airplane_p1[location] &= ~(mask & beam_p1[location]); // ビットを0にする
        bom_p1[location] &= ~(mask & beam_p1[location]);      // ビットを0にする
        Beam_Loss = 1;

        if (before != airplane_p1[location])
        {
            death_flag = 1;
        }
    }
}

void Judge(int location, int *enemy_p, int *enemy_down_p, int *enemy_up_p)
{

    int before = enemy_p[location];
    judge = bom_p0[location] + enemy_p[location];

    if (enemy_p[location] != judge)
    {

        mask = judge ^ bom_p0[location];                      // 変化したビットを取得
        enemy_p[location] &= ~(mask & bom_p0[location]);      // ビットを0にする
        enemy_down_p[location] &= ~(mask & bom_p0[location]); // ビットを0にする
        enemy_up_p[location] &= ~(mask & bom_p0[location]);   // ビットを0にする
        beam_p0[location] &= ~(mask & bom_p0[location]);      // ビットを0にする
        beam_set_p0[location] &= ~(mask & bom_p0[location]);  // ビットを0にする

        if (before != enemy_p[location])
        {
            melody_address = melody[0];
            play_music(melody_address);
        }
    }
    judge = 0x0;
}

void kabe_Judge(int location, int *enemy_p)
{

    int before = enemy_p[location];
    judge = bom_p0[location] + enemy_p[location];

    if (enemy_p[location] != judge)
    {

        mask = judge ^ bom_p0[location];                 // 変化したビットを取得
        enemy_p[location] &= ~(mask & bom_p0[location]); // ビットを0にする （弾が敵に当たった場合には敵を消去する）
        bom_p0[location] &= ~(mask & bom_p0[location]);  // ビットを0にする （弾が敵に当たった場合には弾を消去する）
        if (before != enemy_p[location])
        {
            melody_address = melody[0];
            play_music(melody_address);
        }
    }
    judge = 0;
}

void init_system(void)
{
    RCC->APB2ENR = RCC_APB2ENR_IOPAEN | RCC_APB2ENR_IOPBEN | RCC_APB2ENR_IOPCEN;

    RCC->APB1ENR |= RCC_APB1ENR_TIM3EN;

    GPIOA->CRL = 0x11114411; // PA7-4,1,0 OUT
    GPIOA->CRH = 0x44441111; // PA1-8 OUT
    GPIOB->CRL = 0x1114444B; // PB7-5 OUT
    GPIOB->CRH = 0x44444441; // PB8 OUT

    GPIOC->CRL = 0x44444444; // PA7-4,1,0 OUT

    // PWMモードを選択し、PWM信号をピンに出力する
    TIM3->CCMR2 |= TIM_CCMR2_OC3M_1 | TIM_CCMR2_OC3M_2;
    TIM3->CCER |= TIM_CCER_CC3E;

    // プリスケーラを設定する
    TIM3->PSC = 720 - 1;

    // タイマを有効にするコントロールレジスタ。　タイマのON・OFFなど設定
    TIM3->CR1 |= TIM_CR1_CEN;
}

void waite_func(int time)
{
    delay_time = time;
    while (delay_time > 0)
        ;
}

unsigned int Get_SW(void)
{

    unsigned int sw;
    sw = GPIOC->IDR;
    sw = ~sw;
    sw = sw & (SW1 | SW2 | SW3 | SW4);

    return sw;
}

void Wait_Time(int time)
{
    TimeInt = time;
    while (TimeInt > 0)
        ; // カウンタの値が0になるまで待つ。カウンタはSysTick_Handlerで減算されている。
}

void BuzzerOn(int rate, int time)
{
    int i;
    for (i = 0; i < rate; i++)
    {
        GPIOB->ODR = 1;
        waite_func((time / rate) / 2);
        GPIOB->ODR = 0;
        waite_func((time / rate) / 2);
    }
}

void play_tone(unsigned int frequency)
{
    // 自動リロードレジスタと比較レジスタを周波数に応じて設定する
    TIM3->ARR = (72000000 / frequency / 720) - 1;
    TIM3->EGR = TIM_EGR_UG;           // 追加: レジスタの変更をすぐに反映する
    TIM3->CCR3 = (TIM3->ARR + 1) / 2; // 50% デューティサイクル
}

void stop_tone()
{
    // PWM信号を停止する
    TIM3->CCR3 = 0;
}

void play_music(Note data[])
{

    int i = 0;
    play_tone(data[i].pitch);
    waite_func(data[i].duration);
    stop_tone();
}

void SysTick_Handler(void)
{

    static int swc = 0;  // 今回読み込んだスイッチの値
    static int swp1 = 0; // 前回読み込んだスイッチの値
    static int swp2 = 0; // 前々回に読み込んだスイッチの値

    static int sw_last = 0; // 前回のスイッチの確定値
    static int chat_count = 0;

    static int lp_start = 0;
    static int lp_counter = 0;
    static int lp_int_cnt = 0;
    static int lp_bom_cnt = 0;

    if (chat_count == 9)
    { // 10msでチャタリング検知

        chat_count = 0;
        swp2 = swp1;    // 前々回の値を保存する
        swp1 = swc;     // 前回の値を保存する
        swc = Get_SW(); // 新たに今回のスイッチの値を読み込む

        if ((swp2 == swp1) && (swp1 == swc))
        {                 // 今回、前回、前々回の値が全て等しい
            sw_now = swc; // 場合、今回の値を現在の確定値とする
        }

        if (sw_now != sw_last)
        { // 前回のスイッチの確定値と異なる場合だけ以下の処理を行う
            if (sw_now & ~sw_last)
            { // 立上りを検出したら
                up |= sw_now & ~sw_last;

                if (up & SW1)
                {
                    if (!Switch_ID)
                    {
                        Switch_ID = 1;
                    }
                }
                if (up & SW2)
                {
                    if (!Switch_ID)
                    {
                        Switch_ID = 2;
                    }
                }
                if (up & SW3)
                {
                    if (!Switch_ID)
                    {
                        Switch_ID = 3;
                    }
                }
                if (up & SW4)
                {
                    if (!Switch_ID)
                    {
                        Switch_ID = 4;
                    }
                }

                lp_start = 1;
                lp_counter = 0;
                lp_int_cnt = 0;
                lp_bom_cnt = 0;
            }

            sw_last = sw_now; // 現在のスイッチの確定値を sw_last に保存する
        }

        // スイッチ長押しの終了処理
        if (Switch_ID)
        {
            switch (Switch_ID)
            {
            case 1:
                if (!(sw_now & SW1))
                {
                    lp_start = 0;
                    up &= ~LP_Count_Enable;
                    Switch_ID = 0;
                }
                break;

            case 2:
                if (!(sw_now & SW2))
                {
                    lp_start = 0;
                    up &= ~LP_Count_Enable;
                    Switch_ID = 0;
                }
                break;

            case 3:
                if (!(sw_now & SW3))
                {
                    lp_start = 0;
                    up &= ~LP_Count_Enable;
                    Switch_ID = 0;
                }
                break;

            case 4:
                if (!(sw_now & SW4))
                {
                    lp_start = 0;
                    up &= ~LP_Count_Enable;
                    Switch_ID = 0;
                }
                break;
            }
        }
    }
    else
    {

        chat_count++;
    }

    // 敵の攻撃シフト
    if (beam_count == 7000)
    { // 7秒ごとに敵の攻撃が行われる
        beam_flag = 1;
        beam_count = 0;
    }
    else
    {
        beam_count++;
    }

    // スイッチ長押しの有効化チェック
    if (lp_start)
    {
        if (lp_counter < LP_Interval)
        {
            lp_counter++;
        }
        else
        {
            up |= LP_Count_Enable;
        }
    }

    // 飛行機の移動のスピード
    if (up & LP_Count_Enable)
    {
        if (lp_int_cnt < 99)
        {
            lp_int_cnt++;
        }
        else
        {
            lp_int_cnt = 0;
            LP_Count_Flag = 1;
        }
    }

    // ボムの連射のスピード
    if (up & LP_Count_Enable)
    {
        if (lp_bom_cnt < 499)
        {
            lp_bom_cnt++;
        }
        else
        {
            lp_bom_cnt = 0;
            LP_Bom_Count_Flag = 1;
        }
    }

    // 待ち時間関数　Wait_Time()用の変数の処理
    if (TimeInt > 0)
        TimeInt--;

    static int enemy_move = 0;
    static int enemy_count = 0;

    enemy_count++;

    if (enemy_move <= 1)
    {

        if (enemy_count == 999)
        {
            EnemyLeftShift(enemy_p1, enemy_up_p1, enemy_down_p1, 16);

            enemy_count = 0;
            enemy_move++;
        }
        else if (enemy_count == 499)
        {
            EnemyLeftShift(enemy_p1, enemy_down_p1, enemy_up_p1, 16);

            enemy_move++;
        }
        else
        {
        }
    }
    else if (enemy_move <= 3)
    {

        if (enemy_count == 999)
        {
            EnemyRightShift(enemy_p1, enemy_up_p1, enemy_down_p1, 16);

            enemy_count = 0;
            enemy_move++;
        }
        else if (enemy_count == 499)
        {
            EnemyRightShift(enemy_p1, enemy_down_p1, enemy_up_p1, 16);

            enemy_move++;
        }
    }
    else
    {
        enemy_move = 0;
    }

    // delay count
    if (delay_time == 0)
        delay_time = 0;
    else
        delay_time--;
    //	//LED matrix
    if (line >= 15)
        line = 0;
    else
        line++;
    for (clk_cnt = 0; clk_cnt < 32; clk_cnt++)
    {
        if (clk_cnt == 0)
        {
            line_data_r0 = data_r0[line];
            line_data_r1 = data_r1[line];
            line_data_g0 = data_g0[line];
            line_data_g1 = data_g1[line];
            line_data_b0 = data_b0[line];
            line_data_b1 = data_b1[line];
        }
        else
        {
            line_data_r0 = line_data_r0 >> 1;
            line_data_r1 = line_data_r1 >> 1;
            line_data_g0 = line_data_g0 >> 1;
            line_data_g1 = line_data_g1 >> 1;
            line_data_b0 = line_data_b0 >> 1;
            line_data_b1 = line_data_b1 >> 1;
        }
        // データ出力
        if ((line_data_r0 & 0x1) == 0)
            GPIOA->ODR &= ~R0_HI; // R0-SIN=0
        else
            GPIOA->ODR |= R0_HI; // R0-SIN=1
        if ((line_data_r1 & 0x1) == 0)
            GPIOA->ODR &= ~R1_HI; // R1-SIN=0
        else
            GPIOA->ODR |= R1_HI; // R1-SIN=1
        if ((line_data_g0 & 0x1) == 0)
            GPIOA->ODR &= ~G0_HI; // G0-SIN=0
        else
            GPIOA->ODR |= G0_HI; // G0-SIN=1
        if ((line_data_g1 & 0x1) == 0)
            GPIOA->ODR &= ~G1_HI; // G1-SIN=0
        else
            GPIOA->ODR |= G1_HI; // G1-SIN=1
        if ((line_data_b0 & 0x1) == 0)
            GPIOA->ODR &= ~B0_HI; // B0-SIN=0
        else
            GPIOA->ODR |= B0_HI; // B0-SIN=1
        if ((line_data_b1 & 0x1) == 0)
            GPIOA->ODR &= ~B1_HI; // B1-SIN=0
        else
            GPIOA->ODR |= B1_HI; // B1-SIN=1
        GPIOB->ODR &= ~CLK_HI;   // CLK=0
        GPIOB->ODR |= CLK_HI;    // CLK=1
        if (clk_cnt == 31)
        {
            GPIOB->ODR &= ~CLK_HI; // CLK=0
            GPIOB->ODR |= STB_HI;  // STB=1
            GPIOB->ODR &= ~STB_HI; // STB=0
            // line
            line_bits = ((line & 0xc) << 2) | (line & 0x3);
            GPIOA->ODR &= ~LINE_MASK | line_bits;
            GPIOA->ODR |= LINE_MASK & line_bits;
        }
        // 出力イネーブル
        if (clk_cnt == 23)
        {
            GPIOB->ODR |= OE_HI; // OEB=1
        }
        if (clk_cnt == 0)
        {
            GPIOB->ODR &= ~OE_HI; // OEB=0
        }
    }
}