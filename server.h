/*
    server.h
    サーバのヘッダファイル
*/
#ifndef _SERVER_H_
#define _SERVER_H_






#include <stdio.h>


#include <SDL2/SDL.h>        // SDLの排他処理を用いるために必要なヘッダファイルをインクルード
#include <time.h>
#include <unistd.h>

#include"common.h"

/* デバッグの有無、不要時はコメントアウト */
// #define DEBUG //デバッグの有無

typedef struct{
    int player_num;

    GameStts game_stts;

    //スレッド関連
    SDL_mutex *Mutex;

    SDL_Thread *player_thread[player_num_init];

    int key_1_ID;
    int key_2_ID;

    int finish_ID;
    int finish_team_ID;


    //マップ関係
    int map[MAP_HEIGHT][MAP_WIDTH];

} ServerInfo;   //サーバの情報
// ServerInfo gServer;

typedef struct{
    char player_name[MAX_L_NAME]; //プレイヤーの名前
    char command;     //コマンド
    int player_ID;
    xyz point;        //機体座標
    xyz vector;       //機体の方向
    xyz flont_vec;    //機体の前方方向の角度
    int HP;
    SDL_bool see;
    int init;         //初期値の更新状況
} sPlayer_info;   //プレイヤーの情報
// sPlayer_info sPlayer_info_all[player_num_init];

typedef struct{
    char command; //コマンド
    int kind;     //種類
    /*
    0 : 無視
    1 : 弾丸
    2 : ミサイル
    */
    xyz point;    //座標
    xyz start_point; //発射座標
    xyz vec;      //角度
    double speed; //速さ
    SDL_bool see; //描写するかどうか
    int target;

} sitem_info;
// sitem_info sitem_info_all[player_num_init * item_num_init];




/*
    関数一覧
*/
extern ServerInfo gServer;
extern sPlayer_info sPlayer_info_all[player_num_init];
extern sitem_info sitem_info_all[player_num_init * item_num_init + 2];


/*
    関数一覧
*/
//server_func.c
extern int Init_func(void);
extern int sStart_game(void);
extern void Manege_Client(void);
extern int End_server(void);


//MAP関係
extern void printMap(int, int);
extern void MakeMap(int, int);
extern void dig(int, int);


/* server_client.c */
//NetWork関係
//client -> server
extern int Server_network_c_s(void* data);
//server -> client
extern int Server_network_s_c(void* data);


extern void SetupServer(u_short);                           //          サーバーを立ち上げる
extern int  ControlRequests(int);                           //          送受信管理
extern void SendPlayerData();                            //          プレイヤーのデータを送信する
extern void ReceivePlayerData(int, CONTAINER);              //          アイテムのデータを送信する
extern void SendItemData();                                 //          プレイヤーのデータを受信する
extern void ReceiveItemData(int, CONTAINER);                //          アイテムのデータを受信する
extern void ReceiveKeyData(CONTAINER);                      //          キーのデータの受信
extern void BroadcastGameStts(void);                        //          ゲームステータスを全員に送信
extern void ReceiveGameStts(CONTAINER);                     //          ゲームステータスを受信
extern int  BroadcastMapData(void);                         //          迷路情報を送信
extern void TerminateServer(void);                          //          サーバーを終了させる


#endif