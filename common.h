/*
    commoon.h
    共通のヘッダファイル
*/
#ifndef _COMMON_H_
#define _COMMON_H_

#include <stdio.h>

//各種設定値
#define player_num_init 4 //プレイヤーの数
#define item_num_init   100 //アイテムの数

//マップ関係
/* 迷路のXサイズ(奇数) */
#define MAP_WIDTH 11
/* 迷路のZサイズ(奇数) */
#define MAP_HEIGHT 11


/* 送受信コマンド */
#define PLAYER_COMMAND      'P'
#define ITEM_COMMAND        'I'
#define KEY_COMMAND         'K'
#define HIT_COMMAND         'H'
#define GAME_COMMAND        'S'
#define GAME_FINISHED       'F'
#define DEFAULT_COMMAND     '\0'


/* double型の座標 */
typedef struct {
    double x;
    double y;
    double z;
} xyz; //座標

/* ゲームの状態 */
typedef enum {
    GS_End     = 0, /* 終了 */
    GS_Playing = 1, /* 通常 */
    GS_Ready   = 2,  /* 開始前 */
    GS_Clear   = 3   /*クリア*/
} GameStts;



/* Network.c向け */
#include <time.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <netinet/in.h>
#include <SDL2/SDL.h>


#define u_short     unsigned short

#define MAX_L_ADDR      32                  //      アドレスの最大桁
#define MAX_L_NAME	    10 				    //      名前の文字数上限
#define MAX_N_DATA		256 			    //      送受信するデータのサイズ上限
#define DEFAULT_PORT    (u_short)8888	    //      デフォルトポート番号

/* クライアント情報 */
typedef struct {
	int id;                           //    クライアントのID
	int sock;                         //    ソケット
	struct sockaddr_in addr;          //    ソケットのアドレス
	char name[MAX_L_NAME];            //    クライアント名
} ClientInfo;

typedef struct {
    xyz point;
    xyz vec;
    xyz flont_vec;

    int HP;
    //見えてるかどうかも？

    // int game_stts;
    SDL_bool see; //表示非表示
} container_pitem;

typedef struct {
    int kind;     //種類
    xyz point;
    xyz vec;      //角度
    double speed; //速さ
    int target;
    SDL_bool see; //描写するかどうか
} container_iitem;



/* ネットワークのデータ */
typedef struct {
    char command;   //      コマンド

    int  ID;        //      プレイヤーID（プレイヤー用）
    int  index;     //      添字（アイテム用）

    char string[MAX_N_DATA];

    GameStts game_stts;

    container_pitem pinfo[player_num_init];
    container_iitem iinfo[player_num_init * item_num_init + 2];

} CONTAINER;



#endif