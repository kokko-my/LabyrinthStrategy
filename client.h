/*
    client.h
    クライアントのヘッダファイル
*/

#ifndef _CLIENT_H_
#define _CLIENT_H_

#include <stdio.h>
#include <SDL2/SDL.h>        // SDLを用いるために必要なヘッダファイルをインクルード
#include <SDL2/SDL_opengl.h> // SDLでOpenGLを扱うために必要なヘッダファイルをインクルード
// #include <SDL2/SDL2_gfxPrimitives.h>
#include <SDL2/SDL_image.h>
#include <SDL2/SDL_ttf.h>
#include <SDL2/SDL_mixer.h>
#include <time.h>

#include <math.h>
#include <GL/gl.h>
#include <GL/glu.h>
#include <GL/glut.h>

// #include <joyconlib.h>

#include"common.h"



/* デバッグの有無、不要時はコメントアウト */
// #define DEBUG //デバッグの有無
#define NON_NETWORK //ネットワークの干渉なく動作
#define NON_JOYCON //hoy-conの有無



/* 入力の状態 */
typedef struct {
    SDL_bool up;
    SDL_bool down;
    SDL_bool right;
    SDL_bool left;
    SDL_bool space;
    SDL_bool item1;
    SDL_bool item2;
    SDL_bool item3;
} InputStts;

typedef struct {
    SDL_Window* window; // ウィンドウ（サーフェイス）データへのポインタ
    SDL_GLContext context; //OpenGLコンテキスト

    SDL_Renderer* render;
    TTF_Font* font;   //フォント

    SDL_Surface* surface;
    GLenum texture_format;
    GLuint textTexture;

    GameStts game_stts; //ゲームの状態

    int window_reflesh; //ウィンドウ更新の必要の有無
    int point_reflesh;  //ポジションの更新の必要の有無

    int window_wide;  //ウィンドウサイズ
    int window_high;  //ウィンドウサイズ
    GLdouble window_camera_angle; //画角
    GLdouble max_far; //最大描画距離

    InputStts input_move; //キーボード入力

    //スレッド関連
    SDL_mutex *Mutex;  //CONTAINER*

    SDL_Thread *render_thread;  //画面描写のスレッド
    SDL_Thread *func_thread;    //キーボード入力のスレッド
    SDL_Thread *joycon_thread; //ジョイコンのスレッド
    SDL_Thread *move_thread;  //演算処理のスレッド
    SDL_Thread *network_thread; //ネットワークのスレッド

    int network_player;
    int network_item;
    char usrName[MAX_L_NAME];

    int key_switch;


    //音楽関係
    Mix_Music *shot;
    Mix_Music *shot_hit;
    Mix_Music *missoil;
    Mix_Music *missoil_hit;
    Mix_Music *explosion;

    int player_num;  //プレイヤーの総数
    int player_ID;   //自分のID
    double distance[player_num_init]; //各機体との距離
    double ship2vec[player_num_init];  //プレイヤーから見てどの方向にいるのかのステータス用。


    //ゲームの規定値関連
    double speed_normal;  //速度の通常時
    double speed_scale;   //加速時の最大倍率
    double ship_ship_distance; //機体同士の当たり判定(体当たり)

    //Joy-con関連
    // joyconlib_t joy_jc;
    // int joycon_num;

    //マップデータ
    int map[MAP_HEIGHT][MAP_WIDTH];
    double map_tile_size;

    //アイテムに関する情報
    double item1_dmax; //銃弾の最大飛距離
    double ship_item_distance1; //銃弾と機体の当たり判定

    double item2_dmax; //ミサイルの最大飛距離
    double ship_item_distance2; //ミサイルと機体の当たり判定
    int item2_nmax;    //ミサイルの弾数
    int item2_status; //Joycon用のステータス

    int item3_nmax;     //地雷の弾数

    double ship_item_distance3; //地雷と機体の当たり判定


    int finish_ID;          // 勝者
    int finish_team_ID;     // 勝者チーム



} GameInfo;

// GameInfo gGame;

typedef struct {
    char player_name[MAX_L_NAME]; //プレイヤーの名前
    char command;      //コマンド
    int player_ID;
    xyz point;         //機体座標
    double speed;      //機体の速さ
    xyz vector;        //機体の方向
    xyz vector_def;    //機体の方向の変位
    xyz flont_vec;     //機体の前方方向の方向
    xyz flont_vec_def; //機体の前方方向の方向の変位

    double ship_size;    //当たり判定の距離

    int HP; //HP
    SDL_bool see; //表示非表示


    xyz camera_point; //カメラの座標
    xyz camera_center;   //カメラの注視点
    xyz camera_top;  //カメラがどこを上とするか。

    

} player_info;  //プレイヤーの情報

// player_info player_info_all[player_num_init];

typedef struct{
    char command;  //コマンド
    int kind;     //種類
    /*
    0 : 無視
    1 : 弾丸
    2 : ミサイル
    3 : 地雷
    */
    xyz point;    //座標
    xyz start_point; //発射座標
    xyz vec;      //角度
    double speed; //速さ
    SDL_bool see; //描写するかどうか

    //ミサイル用の変数
    int target;
} item_info; //アイテムの情報



/*
    変数一覧
*/

extern GameInfo gGame;
extern player_info player_info_all[player_num_init];
extern item_info item_info_all[player_num_init * item_num_init + 2];

//Network向け

/*
    関数一覧
*/
/* client_func.c */
extern int Init_window(void);                               // SDLの初期化
extern void printMap(int, int);                             // 迷路の作成
extern void dig(int, int);                                  // 迷路の穴掘り
extern void MakeMap(int, int);                              // 迷路の作製
extern int Key_func(void* );                                // キーボード処理
extern int Joy_func(void* );                                // Joy-Con処理
extern int Move(void* );                                    // 移動            
extern double distance (xyz, xyz);                          // 距離計算
extern int Wall_clash(xyz);                                 // 機体の当たり判定
extern int Wall_clash_item(xyz);                            // アイテムの当たり判定
extern void Manege_HP(void);                                // HPの管理
extern void End_system(void);                               // システムの終了

/* client_win.c  */
extern int Init_func(void);                                 // 初期化
extern int Start_game(void);                                // ゲームの開始処理
extern int Render_window(void);                             // 画面描写
extern void DrawInfo(void);                                 // 情報の描写

extern xyz rotate_point(xyz, xyz);                          // 回転座標計算
extern xyz point_func(xyz, float, float, float, xyz);       // 機体のパーツの座標計算
extern void GameEnding(void);                               // ゲームの終了処理

/* client_net.c */
extern int Network(void* data);
extern void SetupClient(char*, u_short);                    // クライアントを立ち上げる
extern void ReceiveGameStts(CONTAINER);                     // ゲームステータスを受信
extern void SendGameStts(void);                             // ゲームステータスを送信
extern int  ReceiveMapData(void);                           // 迷路情報を受信
extern void SendPlayerData(void);                           // プレイヤーのデータを送信
extern void ReceivePlayerData(CONTAINER);                   // プレイヤーのデータを受信
extern void SendItemData(void);                             // アイテムのデータを送信
extern void ReceiveItemData(CONTAINER);                     // アイテムのデータを受信
extern void SendHitData(int, int);                          // 被弾したことを受信
extern void SendKeyData(int);                               // キーのデータを送信
extern void SendGoalData(void);                             // 結果のデータを送信
extern void ReceiveGoalData(CONTAINER);                     // 結果のデータを受信
extern void TerminateClient(void);                          // クライアントを終了させる
extern int Welcome(void);                                   // 最初の画面表示
extern void Result(void);                                   // 結果の画面表示
#endif