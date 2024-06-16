/*
    server_finc.c
    サーバの関数ファイル
*/
#include <unistd.h>
#include <stdio.h>
#include <time.h>

#include "server.h"

#define PATH 0
#define WALL 1

int Init_func(void){
    #ifdef DEBUG
    fprintf(stderr, "Init_func start\n");
    #endif

    gServer.game_stts = GS_Ready;

    //乱数を用意
    time_t now;
    /* 現在の時刻を取得 */
    now = time(NULL);
    /* 現在の時刻を乱数の種として乱数の生成系列を変更 */
    srand((unsigned int)now);

    //マップ作製
    MakeMap(MAP_WIDTH, MAP_HEIGHT);

    #ifdef DEBUG
        printMap(MAP_WIDTH, MAP_HEIGHT);
    #endif

    //各種数値設定
    gServer.player_num = player_num_init; //プレイヤー数


    return 0;
}


/*
    ゲームのスタート
*/
int sStart_game(void){
   //乱数準備
    time_t now;
    /* 現在の時刻を取得 */
    now = time(NULL);
    /* 現在の時刻を乱数の種として乱数の生成系列を変更 */
    srand((unsigned int)now);

    //鍵の所有者を決定
    gServer.key_1_ID = rand() % (player_num_init/2);
    gServer.key_2_ID = (rand() % (player_num_init/2)) + (player_num_init/2);
    sitem_info_all[player_num_init * item_num_init + 0].kind = 4;
    sitem_info_all[player_num_init * item_num_init + 0].target = gServer.key_1_ID;
    sitem_info_all[player_num_init * item_num_init + 0].see = SDL_FALSE;
    sitem_info_all[player_num_init * item_num_init + 1].kind = 4;
    sitem_info_all[player_num_init * item_num_init + 1].target = gServer.key_2_ID;
    sitem_info_all[player_num_init * item_num_init + 1].see = SDL_FALSE;

    for (int i = 0; i < player_num_init; i++){
        sPlayer_info_all[i].see = SDL_TRUE;
        sPlayer_info_all[i].HP = 100;
        sPlayer_info_all[i].init = 0;
    }
    

    #ifdef DEBUG
    fprintf(stderr, "%d & %d have key!\n", gServer.key_1_ID, gServer.key_2_ID);
    #endif

    gServer.game_stts = GS_Playing;
    SendItemData();

    return 0;
}

void Manege_Client(void){
    // 死活監視
	int count1 = 0; //チーム1のHP0の人のカウント
    int count2 = 0; //チーム2のHP0の人のカウント
    for (int i = 0; i < player_num_init; i++){
        //ゲームオーバーの機体のカウント
        if (sPlayer_info_all[i].init == 1){
            if (sPlayer_info_all[i].see == SDL_FALSE){
                // printf("ID : %d see = %d\n",i,sPlayer_info_all[i].see);
                if (i < player_num_init/2){  //チーム1のカウント
                    count1++;
                }
                else { //チーム2のカウント
                    count2++;
                }
            }
        }
    }

    //ゲームの終了条件
    if (count1 == player_num_init/2){
        //チーム1の敗北
        #ifdef DEBUG
        fprintf(stderr, "Team 1 is losed\n");
        #endif
        gServer.game_stts = GS_Clear;
        SendPlayerData();

    } 
    if (count2 == player_num_init/2) {
        //チーム2の敗北
        #ifdef DEBUG
        fprintf(stderr, "Team 2 is losed\n");
        #endif
        gServer.game_stts = GS_Clear;
        SendPlayerData();
    }
	//else { //鍵の処理
    if (gServer.game_stts != GS_Clear) {
    
		for (int i = 0; i < player_num_init; i++){
            if (sPlayer_info_all[i].init == 1){
			    if (sPlayer_info_all[i].see == SDL_FALSE){ //死んでる
			    	if (i < player_num_init / 2) {  //チーム1
			    		if (i == gServer.key_1_ID){ //味方が鍵を持って死んだ場合
                            sitem_info_all[player_num_init * item_num_init + 0].kind = 4;
			    			sitem_info_all[player_num_init * item_num_init + 0].see = SDL_TRUE;
                            sitem_info_all[player_num_init * item_num_init + 0].target = 100;
			    			sitem_info_all[player_num_init * item_num_init + 0].point.x = sPlayer_info_all[gServer.key_1_ID].point.x;
			    			sitem_info_all[player_num_init * item_num_init + 0].point.y = sPlayer_info_all[gServer.key_1_ID].point.y;
			    			sitem_info_all[player_num_init * item_num_init + 0].point.z = sPlayer_info_all[gServer.key_1_ID].point.z;
                            sitem_info_all[player_num_init * item_num_init + 0].vec.y += 2.0;
			    			SendItemData();
			    		}
                        if (i == gServer.key_2_ID){ //敵のカギを持って死んだ場合
                            sitem_info_all[player_num_init * item_num_init + 1].kind = 4;
			    			sitem_info_all[player_num_init * item_num_init + 1].see = SDL_TRUE;
                            sitem_info_all[player_num_init * item_num_init + 1].target = 100;
			    			sitem_info_all[player_num_init * item_num_init + 1].point.x = sPlayer_info_all[gServer.key_2_ID].point.x;
			    			sitem_info_all[player_num_init * item_num_init + 1].point.y = sPlayer_info_all[gServer.key_2_ID].point.y;
			    			sitem_info_all[player_num_init * item_num_init + 1].point.z = sPlayer_info_all[gServer.key_2_ID].point.z;
                            sitem_info_all[player_num_init * item_num_init + 1].vec.y += 2.0;
			    			SendItemData();
                        }
                        
    		    	} 
                    if (i >= player_num_init / 2 ) {  //チーム2
			    		if (i == gServer.key_2_ID ){ //見方が鍵を持って死んだ場合
                            sitem_info_all[player_num_init * item_num_init + 1].kind = 4;
			    			sitem_info_all[player_num_init * item_num_init + 1].see = SDL_TRUE;
                            sitem_info_all[player_num_init * item_num_init + 1].target = 100;
			    			sitem_info_all[player_num_init * item_num_init + 1].point.x = sPlayer_info_all[gServer.key_2_ID].point.x;
			    			sitem_info_all[player_num_init * item_num_init + 1].point.y = sPlayer_info_all[gServer.key_2_ID].point.y;
			    			sitem_info_all[player_num_init * item_num_init + 1].point.z = sPlayer_info_all[gServer.key_2_ID].point.z;
                            sitem_info_all[player_num_init * item_num_init + 1].vec.y += 2.0;
			    			SendItemData();
			    		}
                        if (i == gServer.key_1_ID){ //敵の鍵を持って死んだ場合
                            sitem_info_all[player_num_init * item_num_init + 0].kind = 4;
			    			sitem_info_all[player_num_init * item_num_init + 0].see = SDL_TRUE;
                            sitem_info_all[player_num_init * item_num_init + 0].target = 100;
			    			sitem_info_all[player_num_init * item_num_init + 0].point.x = sPlayer_info_all[gServer.key_1_ID].point.x;
			    			sitem_info_all[player_num_init * item_num_init + 0].point.y = sPlayer_info_all[gServer.key_1_ID].point.y;
			    			sitem_info_all[player_num_init * item_num_init + 0].point.z = sPlayer_info_all[gServer.key_1_ID].point.z;
                            sitem_info_all[player_num_init * item_num_init + 0].vec.y += 2.0;
			    			SendItemData();
			    		}
    		    	}
			    }
            }
		}
	}
}


/*
    終了処理
*/
int End_server(void){
    for (int i = 0; i < gServer.player_num; i++){
        SDL_WaitThread(gServer.player_thread[i], NULL);
    }

    /* サーバーを閉じる */
    TerminateServer();

    return 0;
}





/**
 * 迷路を表示する
 *
 * 引数
 * w：迷路の横幅（マス数）
 * h：迷路の高さ（マス数）
 *
 * 返却値
 * なし
 */
void printMap(int w, int h) {
    int i, j;

    #define PATH 0
    #define WALL 1

    for (j = 0; j < h; j++) {
        for (i = 0; i < w; i++) {
            /* 配列の値に応じて記号/文字を表示 */
            if (gServer.map[j][i] == PATH) {
               fprintf(stderr, " ");
            }
            if (gServer.map[j][i] == WALL) {
               fprintf(stderr, "0");
            }
        }
        fprintf(stderr, "\n");
    }
    fprintf(stderr, "\n");
}

/**
 * 穴を掘る
 *
 * 引数
 * i：穴を掘る起点座標（横方向）
 * j：穴を掘る起点座標（縦方向）
 *
 * 返却値
 * なし
 */
void dig(int i, int j) {
    /* どの方向を掘ろうとしたかを覚えておく変数 */
    int up, down, left, right;

    up = 0;
    down = 0;
    left = 0;
    right = 0;


    /* 全方向試すまでループ */
    while (up == 0 || down == 0 || left == 0 || right == 0) {
        /* 0 - 3 の乱数を取得 */
        int d = rand() % 4;

        switch(d) {
            case 0: //UP
                /* 上方向が掘れるなら掘る */
                if (j - 2 >= 0 && j - 2 <  MAP_HEIGHT) {
                    if (gServer.map[j - 2][i] == WALL) {
                        gServer.map[j - 2][i] = PATH;
                        gServer.map[j - 1][i] = PATH;
                        dig(i, j - 2);
                    }
                }
                up++;
                break;
            case 1: //DOWN
                /* 下方向が掘れるなら掘る */
                if (j + 2 >= 0 && j + 2 <  MAP_HEIGHT) {
                    if (gServer.map[j + 2][i] == WALL) {
                        gServer.map[j + 2][i] = PATH;
                        gServer.map[j + 1][i] = PATH;
                        dig(i, j + 2);
                    }
                }
                down++;
                break;
            case 2: //LEFT
                /* 左方向が掘れるなら掘る */
                if (i - 2 >= 0 && i - 2 < MAP_WIDTH) {
                    if (gServer.map[j][i - 2] == WALL) {
                        gServer.map[j][i - 2] = PATH;
                        gServer.map[j][i - 1] = PATH;
                        dig(i - 2, j);
                    }
                }
                left++;
                break;
            case 3: //RIGHT
                /* 右方向が掘れるなら掘る */
                if (i + 2 >= 0 && i + 2 < MAP_WIDTH) {
                    if (gServer.map[j][i + 2] == WALL) {
                        gServer.map[j][i + 2] = PATH;
                        gServer.map[j][i + 1] = PATH;
                        dig(i + 2, j);
                    }
                }
                right++;
                break;
        }
    }
}

/**
 * 迷路を作成する
 *
 * 引数
 * w：迷路の横幅（マス数）
 * h：迷路の高さ（マス数）
 *
 * 返却値
 * なし
 */
void MakeMap(int w, int h) {
    int i, j;

    /* 全マス壁にする */
    for (j = 0; j < h; j++) {
        for (i = 0; i < w; i++) {
            gServer.map[j][i] = WALL;
        }
    }

    /* 開始点をランダム（奇数座標）に決定する */
    i = 2 * (rand() % (w / 2)) + 1;
    j = 2 * (rand() % (h / 2)) + 1;

    /* i, j を通路に設定 */
    gServer.map[j][i] = PATH;

    /* マス(i,j)を起点に穴を掘る */
    dig(i, j);

    /* スタート地点とゴール地点をスペース開ける */
    gServer.map[1][1] = PATH;
    gServer.map[1][2] = PATH;
    gServer.map[2][1] = PATH;
    gServer.map[2][2] = PATH;

    gServer.map[MAP_HEIGHT-2][MAP_WIDTH-2] = PATH;
    gServer.map[MAP_HEIGHT-2][MAP_WIDTH-3] = PATH;
    gServer.map[MAP_HEIGHT-3][MAP_WIDTH-2] = PATH;
    gServer.map[MAP_HEIGHT-3][MAP_WIDTH-3] = PATH;


}
