/*
    client_func.c
    クライアントの関数ファイル
*/

#include <stdio.h>
#include <SDL2/SDL.h>        // SDLを用いるために必要なヘッダファイルをインクルード
#include <SDL2/SDL_opengl.h> // SDLでOpenGLを扱うために必要なヘッダファイルをインクルード

#include <unistd.h>
#include <time.h>


#include "client.h"

#define PATH 0
#define WALL 1

static void Move_ship(void);
static void Move_items(void);

    SDL_Window* w_window;
    SDL_Renderer* w_renderer;
    SDL_Texture* w_texture;
    SDL_Surface* w_strings;
    SDL_Surface* w_cat_pic;
    SDL_Texture* w_pngtex;

    SDL_Event w_event;
    SDL_Color w_green = {0x00, 0xff, 0x00}; // フォントの色,緑
    SDL_Color w_red = {0xff, 0x00, 0x00};   //フォントの色,赤
    TTF_Font* w_font;             //通常フォント
    TTF_Font* w_option_w_font;      //オプションボタン用フォント


/*************************************
Init_func : いろんなシステムの初期化関数
SDL周りはInit_winodw関数で行う。
返り値は、エラーで-1.正常終了で0
**************************************/
int Init_func(void){
    #ifdef DEBUG
    fprintf(stderr, "Init_func start!\n");
    #endif

    //乱数準備
    time_t now;
    /* 現在の時刻を取得 */
    now = time(NULL);
    /* 現在の時刻を乱数の種として乱数の生成系列を変更 */
    srand((unsigned int)now);

    //準備開始
    gGame.game_stts = GS_Ready;

    //ウィンドウの基本的な設定
    gGame.window_wide = 1440;
    gGame.window_high = 960;
    gGame.window_camera_angle = 100.0;            //カメラの画角
    gGame.max_far = 50.0;                         //最大描画距離

    //ゲームの規定値
    gGame.speed_normal = 0.2;
    gGame.speed_scale  = 1.5;



    //プレイヤーの人数
    gGame.player_num = player_num_init;

    //マップ関係
    gGame.map_tile_size = 6.0;
    //マップ作製
    #ifdef NON_NETWORK
        MakeMap(MAP_WIDTH, MAP_HEIGHT);
    #endif



    //初期座標を設定(仮設)
    for (int i = 0; i < gGame.player_num; i++){
        if (i < gGame.player_num / 2) {  //チーム1
            player_info_all[i].point.x = 2.0 + gGame.map_tile_size + 1.0*i;
            player_info_all[i].point.y = 0.3;
            player_info_all[i].point.z = -2.0 - gGame.map_tile_size;
            player_info_all[i].vector.x = 0.0;
            player_info_all[i].vector.y = 0.0;
            player_info_all[i].vector.z = 0.0;
            player_info_all[i].flont_vec.x = 0.0;
            player_info_all[i].flont_vec.y = 0.0;
            player_info_all[i].flont_vec.z = 0.0;
        } else {  //チーム2
            player_info_all[i].point.x = -1.0*(i-gGame.player_num/2) + gGame.map_tile_size * (MAP_WIDTH-1) - 1.0;
            player_info_all[i].point.y = 0.3;
            player_info_all[i].point.z = -gGame.map_tile_size * (MAP_HEIGHT-1) + 1.0;
            player_info_all[i].vector.x = 0.0;
            player_info_all[i].vector.y = 180.0;
            player_info_all[i].vector.z = 0.0;
            player_info_all[i].flont_vec.x = 0.0;
            player_info_all[i].flont_vec.y = 180.0;
            player_info_all[i].flont_vec.z = 0.0;
        }


        player_info_all[i].vector_def.x = 0.0;
        player_info_all[i].vector_def.y = 0.0;
        player_info_all[i].vector_def.z = 0.0;
        player_info_all[i].flont_vec_def.x = 0.0;
        player_info_all[i].flont_vec_def.y = 0.0;
        player_info_all[i].flont_vec_def.z = 0.0;

        //HP
        player_info_all[i].HP = 100;
        player_info_all[i].see = SDL_TRUE;

        //速度
        player_info_all[i].speed = gGame.speed_normal;

        //距離系
        player_info_all[i].ship_size = 0.25;  //機体の壁との当たり半径
    }

    //アイテムの初期化
    for (int i = 0; i < gGame.player_num * item_num_init; i++){
        item_info_all[i].kind = 0;
        item_info_all[i].see = SDL_FALSE;
    }
    item_info_all[player_num_init * item_num_init + 0].see = SDL_FALSE;
    item_info_all[player_num_init * item_num_init + 0].vec = (xyz){0.0, 0.0, 0.0};
    item_info_all[player_num_init * item_num_init + 1].see = SDL_FALSE;
    item_info_all[player_num_init * item_num_init + 1].vec = (xyz){0.0, 0.0, 0.0};

    //アイテムの諸数値設定

    //Move_itemでの機体とアイテムの当たり判定の距離
    gGame.item1_dmax = 15.0; //銃弾の最高飛行距離
    gGame.ship_item_distance1 = 0.4; //弾丸

    gGame.item2_dmax = 40.0; //ミサイルの最高飛行距離
    gGame.ship_item_distance2 = 0.4; //ミサイル
    gGame.item2_nmax = 10;  //ミサイルの弾数

    gGame.ship_item_distance3 = 0.5; //地雷

    //Move_shipでの機体同士の当たり判定の距離
    gGame.ship_ship_distance  = 0.4;
    gGame.item3_nmax = 5;      //地雷の弾数

    

    //SDL周りの初期化
    Init_window();


    #ifdef DEBUG
    fprintf(stderr, "Init_func success!\n");
    #endif

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

    for (j = 0; j < h; j++) {
        for (i = 0; i < w; i++) {
            /* 配列の値に応じて記号/文字を表示 */
            if (gGame.map[j][i] == PATH) {
               fprintf(stderr, " ");
            }
            if (gGame.map[j][i] == WALL) {
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
                    if (gGame.map[j - 2][i] == WALL) {
                        gGame.map[j - 2][i] = PATH;
                        gGame.map[j - 1][i] = PATH;
                        dig(i, j - 2);
                    }
                }
                up++;
                break;
            case 1: //DOWN
                /* 下方向が掘れるなら掘る */
                if (j + 2 >= 0 && j + 2 <  MAP_HEIGHT) {
                    if (gGame.map[j + 2][i] == WALL) {
                        gGame.map[j + 2][i] = PATH;
                        gGame.map[j + 1][i] = PATH;
                        dig(i, j + 2);
                    }
                }
                down++;
                break;
            case 2: //LEFT
                /* 左方向が掘れるなら掘る */
                if (i - 2 >= 0 && i - 2 < MAP_WIDTH) {
                    if (gGame.map[j][i - 2] == WALL) {
                        gGame.map[j][i - 2] = PATH;
                        gGame.map[j][i - 1] = PATH;
                        dig(i - 2, j);
                    }
                }
                left++;
                break;
            case 3: //RIGHT
                /* 右方向が掘れるなら掘る */
                if (i + 2 >= 0 && i + 2 < MAP_WIDTH) {
                    if (gGame.map[j][i + 2] == WALL) {
                        gGame.map[j][i + 2] = PATH;
                        gGame.map[j][i + 1] = PATH;
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
            gGame.map[j][i] = WALL;
        }
    }

    /* 開始点をランダム（奇数座標）に決定する */
    i = 2 * (rand() % (w / 2)) + 1;
    j = 2 * (rand() % (h / 2)) + 1;

    /* i, j を通路に設定 */
    gGame.map[j][i] = PATH;

    /* マス(i,j)を起点に穴を掘る */
    dig(i, j);

    /* スタート地点とゴール地点をスペース開ける */
    gGame.map[1][1] = PATH;
    gGame.map[1][2] = PATH;
    gGame.map[2][1] = PATH;
    gGame.map[2][2] = PATH;

    gGame.map[MAP_HEIGHT-2][MAP_WIDTH-2] = PATH;
    gGame.map[MAP_HEIGHT-2][MAP_WIDTH-3] = PATH;
    gGame.map[MAP_HEIGHT-3][MAP_WIDTH-2] = PATH;
    gGame.map[MAP_HEIGHT-3][MAP_WIDTH-3] = PATH;


}

/*************************************
Key_func : キーボードの操作を取得
**************************************/
int Key_func(void* data){
    #ifdef DEBUG
        fprintf(stderr, "key_func start\n");
    #endif


    SDL_Event event;

    // ゲーム開始待機
    while (gGame.game_stts == GS_Ready){
        if (gGame.game_stts == GS_Playing){
            break;
        }
    }

    while (gGame.game_stts == GS_Playing){
        if (SDL_PollEvent(&event)) {
            SDL_LockMutex(gGame.Mutex);

            switch (event.type) {
            case SDL_QUIT:
                /** 終了指示 **/
                gGame.game_stts = GS_Clear;
                //loop = 0;
                break;
            case SDL_KEYDOWN:
                if (event.key.repeat)
                    break;

                /** キーが押された方向を設定 **/
                switch (event.key.keysym.sym) {
                case SDLK_ESCAPE:
                    /** 終了指示 **/
                    gGame.game_stts = GS_Clear;
                    // SendGameStts();
                    //loop  = 0;
                    break;
                case SDLK_UP:
                    if (gGame.key_switch){
                        gGame.input_move.down = SDL_TRUE;
                    } else {
                        gGame.input_move.up = SDL_TRUE;
                    }
                    gGame.window_reflesh = 1;  //描写指示
                    gGame.point_reflesh = 1;   //演算指示
                    break;
                case SDLK_DOWN:
                    if (gGame.key_switch){
                        gGame.input_move.up = SDL_TRUE;
                    } else {
                        gGame.input_move.down = SDL_TRUE;
                    }
                    gGame.window_reflesh = 1;  //描写指示
                    gGame.point_reflesh = 1;   //演算指示
                    break;
                case SDLK_RIGHT:
                    gGame.input_move.right = SDL_TRUE;
                    gGame.window_reflesh = 1;  //描写指示
                    gGame.point_reflesh = 1;   //演算指示
                    break;
                case SDLK_LEFT:
                    gGame.input_move.left = SDL_TRUE;
                    gGame.window_reflesh = 1;  //描写指示
                    gGame.point_reflesh = 1;   //演算指示
                    break;
                case SDLK_SPACE:
                    gGame.input_move.space =SDL_TRUE;
                    gGame.window_reflesh = 1;  //描写指示
                    gGame.point_reflesh = 1;   //演算指示
                    break;
                case SDLK_1:
                    gGame.input_move.item1 = SDL_TRUE;
                    gGame.window_reflesh = 1;  //描写指示
                    gGame.point_reflesh = 1;   //演算指示
                    break;
                case SDLK_2:
                    gGame.input_move.item2 = SDL_TRUE;
                    gGame.window_reflesh = 1;  //描写指示
                    gGame.point_reflesh = 1;   //演算指示
                    break;
                case SDLK_3:
                    gGame.input_move.item3 = SDL_TRUE;
                    gGame.window_reflesh = 1;  //描写指示
                    gGame.point_reflesh = 1;   //演算指示
                    break;
                }
                break;
            case SDL_KEYUP:
                if (event.key.repeat)
                    break;
                /* 離されたときは解除 */
                switch (event.key.keysym.sym) {
                case SDLK_UP:
                    if (gGame.key_switch){
                        gGame.input_move.down = SDL_FALSE;
                    } else {
                        gGame.input_move.up = SDL_FALSE;
                    }
                    gGame.window_reflesh = 1;  //描写指示
                    gGame.point_reflesh = 1;   //演算指示
                    break;
                case SDLK_RIGHT:
                    gGame.input_move.right = SDL_FALSE;
                    gGame.window_reflesh = 1;  //描写指示
                    gGame.point_reflesh = 1;   //演算指示
                    break;
                case SDLK_LEFT:
                    gGame.input_move.left = SDL_FALSE;
                    gGame.window_reflesh = 1;  //描写指示
                    gGame.point_reflesh = 1;   //演算指示
                    break;
                case SDLK_DOWN:
                    if (gGame.key_switch){
                        gGame.input_move.up = SDL_FALSE;
                    } else {
                        gGame.input_move.down = SDL_FALSE;
                    }
                    gGame.window_reflesh = 1;  //描写指示
                    gGame.point_reflesh = 1;   //演算指示
                    break;
                case SDLK_SPACE:
                    gGame.input_move.space = SDL_FALSE;
                    gGame.window_reflesh = 1;  //描写指示
                    gGame.point_reflesh = 1;   //演算指示
                    break;
                case SDLK_1:
                    gGame.input_move.item1 = SDL_FALSE;
                    gGame.window_reflesh = 1;  //描写指示
                    gGame.point_reflesh = 1;   //演算指示
                    break;
                case SDLK_2:
                    gGame.input_move.item2 = SDL_FALSE;
                    gGame.window_reflesh = 1;  //描写指示
                    gGame.point_reflesh = 1;   //演算指示
                    break;
                case SDLK_3:
                    gGame.input_move.item3 = SDL_FALSE;
                    gGame.window_reflesh = 1;  //描写指示
                    gGame.point_reflesh = 1;   //演算指示
                    break;
                }
                break;
            }
            SDL_UnlockMutex(gGame.Mutex);
        }
    }
    #ifdef DEBUG
        fprintf(stderr, "KeyFunc_thread return 0\n");
    #endif

    return 0;
}

/*
Joy_func : Joy-Conの操作を取得
*/
int Joy_func(void* data){
    #ifdef DEBUG
        fprintf(stderr, "joy_func start\n");
    #endif


    // SDL_Event event;

    // ゲーム開始待機
    while (gGame.game_stts == GS_Ready){
        if (gGame.game_stts == GS_Playing){
            break;
        }
    }
    // if (gGame.joycon_num){
    //     while (gGame.game_stts == GS_Playing){
    //         joycon_get_state(&gGame.joy_jc);

    //         if (gGame.joy_jc.button.btn.Home){
    //             joycon_rumble(&gGame.joy_jc, 50);
    //             gGame.game_stts = GS_Clear;
    //         }

    //         /*
            
    //         各種アイテムなどのボタン操作
    //         */

    //         if (gGame.joy_jc.button.btn.X){
    //             gGame.input_move.item1 = SDL_TRUE;
    //             gGame.window_reflesh = 1;  //描写指示
    //             gGame.point_reflesh = 1;   //演算指示
    //         }
    //         else{
    //             gGame.input_move.item1 = SDL_FALSE;
    //             gGame.window_reflesh = 1;  //描写指示
    //             gGame.point_reflesh = 1;   //演算指示
    //         }

    //         // ミサイル
    //         if (gGame.joy_jc.button.btn.B){
    //             if (gGame.item2_status != 1){
    //                 gGame.input_move.item2 = SDL_TRUE;
    //             }
    //             gGame.window_reflesh = 1;  //描写指示
    //             gGame.point_reflesh = 1;   //演算指示
    //         }
    //         else{
    //             gGame.input_move.item2 = SDL_FALSE;
    //             gGame.item2_status = 0;
    //             gGame.window_reflesh = 1;  //描写指示
    //             gGame.point_reflesh = 1;   //演算指示
    //         }

    //         if (gGame.joy_jc.button.btn.A){
    //             gGame.input_move.item3 = SDL_TRUE;
    //             gGame.window_reflesh = 1;  //描写指示
    //             gGame.point_reflesh = 1;   //演算指示
    //         }
    //         else{
    //             gGame.input_move.item3 = SDL_FALSE;
    //             gGame.window_reflesh = 1;  //描写指示
    //             gGame.point_reflesh = 1;   //演算指示
    //         }

    //         if (gGame.joy_jc.button.btn.Y){
    //             gGame.input_move.space = SDL_TRUE;
    //             gGame.window_reflesh = 1;  //描写指示
    //             gGame.point_reflesh = 1;   //演算指示
    //         }
    //         else{
    //             gGame.input_move.space = SDL_FALSE;
    //             gGame.window_reflesh = 1;  //描写指示
    //             gGame.point_reflesh = 1;   //演算指示
    //         }





    //         // 機体操作
    //         if (gGame.joy_jc.stick.x < -0.3){
    //             if (gGame.key_switch){
    //                 gGame.input_move.up = SDL_FALSE;
    //                 gGame.input_move.down = SDL_TRUE;
    //             } else {
    //                 gGame.input_move.up = SDL_TRUE;
    //                 gGame.input_move.down = SDL_FALSE;
    //             }
                
    //             gGame.window_reflesh = 1;  //描写指示
    //             gGame.point_reflesh = 1;   //演算指示
    //         } else if (gGame.joy_jc.stick.x > 0.3){
    //             if (gGame.key_switch){
    //                 gGame.input_move.down = SDL_FALSE;
    //                 gGame.input_move.up = SDL_TRUE;
    //             } else {
    //                 gGame.input_move.down = SDL_TRUE;
    //                 gGame.input_move.up = SDL_FALSE;
    //             }
                
    //             gGame.window_reflesh = 1;  //描写指示
    //             gGame.point_reflesh = 1;   //演算指示
    //         } else {
    //             gGame.input_move.up = SDL_FALSE;
    //             gGame.input_move.down = SDL_FALSE;
    //         }

    //         if (gGame.joy_jc.stick.y < -0.3){
    //             gGame.input_move.right = SDL_TRUE;
    //             gGame.input_move.left = SDL_FALSE;
    //             gGame.window_reflesh = 1;  //描写指示
    //             gGame.point_reflesh = 1;   //演算指示
    //         } else if (gGame.joy_jc.stick.y > 0.3){
    //             gGame.input_move.left = SDL_TRUE;
    //             gGame.input_move.right = SDL_FALSE;
    //             gGame.window_reflesh = 1;  //描写指示
    //             gGame.point_reflesh = 1;   //演算指示
    //         } else {
    //             gGame.input_move.left = SDL_FALSE;
    //             gGame.input_move.right = SDL_FALSE;
    //         }



    //     }
    // }
    // else{
    //     fprintf(stderr, "Joy-Con_thread has no Joy-Con\n");
    // }


    #ifdef DEBUG
        fprintf(stderr, "JoyFunc_thread return 0\n");
    #endif

    return 0;
}


/*************************************
Move : キーボードの操作を取得
**************************************/
int Move(void* data){

    // ゲーム開始待機
    while (gGame.game_stts == GS_Ready)
    {
        if (gGame.game_stts == GS_Playing){
            break;
        }
    }

    while (gGame.game_stts == GS_Playing)
    {
        if (gGame.point_reflesh == 1){
            SDL_LockMutex(gGame.Mutex);
            if (gGame.input_move.up == SDL_TRUE){
                player_info_all[gGame.player_ID].vector_def.x -= 1.0;
                player_info_all[gGame.player_ID].flont_vec_def.x -= 1.0;
            }
            if (gGame.input_move.down == SDL_TRUE){
                player_info_all[gGame.player_ID].vector_def.x += 1.0;
                player_info_all[gGame.player_ID].flont_vec_def.x += 1.0;
            }
            if (gGame.input_move.right == SDL_TRUE){
                player_info_all[gGame.player_ID].vector_def.y -= 1.0;
                player_info_all[gGame.player_ID].vector_def.z -= 0.60;
                player_info_all[gGame.player_ID].flont_vec_def.y -= 1.0;
            }
            if (gGame.input_move.left == SDL_TRUE){
                player_info_all[gGame.player_ID].vector_def.y += 1.0;
                player_info_all[gGame.player_ID].vector_def.z += 0.60;
                player_info_all[gGame.player_ID].flont_vec_def.y += 1.0;
            }

            if (gGame.input_move.up == SDL_FALSE && gGame.input_move.down == SDL_FALSE){
                player_info_all[gGame.player_ID].vector_def.x = 0.0;
                player_info_all[gGame.player_ID].flont_vec_def.x = 0.0;
            }
            if (gGame.input_move.right == SDL_FALSE && gGame.input_move.left == SDL_FALSE){
                player_info_all[gGame.player_ID].vector_def.z = 0.0;
                player_info_all[gGame.player_ID].flont_vec_def.z = 0.0;
            }



            //速度関係
            if (gGame.input_move.space == SDL_TRUE){  //スピードアップ
                player_info_all[gGame.player_ID].speed = gGame.speed_normal * gGame.speed_scale;
            } else if (gGame.input_move.space == SDL_FALSE){  //スピードを戻す
                player_info_all[gGame.player_ID].speed = gGame.speed_normal;
            }






            gGame.window_reflesh = 1;  //描写指示
            SDL_UnlockMutex(gGame.Mutex);
        }

        if (gGame.game_stts != GS_Clear && gGame.game_stts != GS_Ready){
            SDL_LockMutex(gGame.Mutex);
            if( player_info_all[gGame.player_ID].see == SDL_TRUE) {
                Move_ship();
            }
            Move_items();
            SDL_UnlockMutex(gGame.Mutex);
            usleep(5* 10000);   //ループの待機時間
        }
    }

    #ifdef DEBUG
        fprintf(stderr, "Move_thread return 0\n");
    #endif

    return 0;
}

/*************************************
    move_ship : 機体の移動
**************************************/
void Move_ship(void){
    //機体角度の最大値設定
    xyz vector_MAX;
    vector_MAX.x = 5.0;
    vector_MAX.y = 3.0;
    vector_MAX.z = 3.0;

    if (player_info_all[gGame.player_ID].vector_def.x > vector_MAX.x){
        player_info_all[gGame.player_ID].vector_def.x = vector_MAX.x;
    }
    else if (player_info_all[gGame.player_ID].vector_def.x < -vector_MAX.x){
        player_info_all[gGame.player_ID].vector_def.x = -vector_MAX.x;
    }
    if (player_info_all[gGame.player_ID].vector_def.y > vector_MAX.y){
        player_info_all[gGame.player_ID].vector_def.y = vector_MAX.y;
    }
    else if (player_info_all[gGame.player_ID].vector_def.y < -vector_MAX.y){
        player_info_all[gGame.player_ID].vector_def.y = -vector_MAX.y;
    }
    if (player_info_all[gGame.player_ID].vector_def.z > vector_MAX.z){
        player_info_all[gGame.player_ID].vector_def.z = vector_MAX.z;
    }
    else if (player_info_all[gGame.player_ID].vector_def.z < -vector_MAX.z){
        player_info_all[gGame.player_ID].vector_def.z = -vector_MAX.z;
    }

    //機体角度の最大値設定
    xyz flont_vec_MAX;
    flont_vec_MAX.x = 5.0;
    flont_vec_MAX.y = 3.0;
    flont_vec_MAX.z = 3.0;

    if (player_info_all[gGame.player_ID].flont_vec_def.x > flont_vec_MAX.x){
        player_info_all[gGame.player_ID].flont_vec_def.x = flont_vec_MAX.x;
    }
    else if (player_info_all[gGame.player_ID].flont_vec_def.x < -flont_vec_MAX.x){
        player_info_all[gGame.player_ID].flont_vec_def.x = -flont_vec_MAX.x;
    }
    if (player_info_all[gGame.player_ID].flont_vec_def.y > flont_vec_MAX.y){
        player_info_all[gGame.player_ID].flont_vec_def.y = flont_vec_MAX.y;
    }
    else if (player_info_all[gGame.player_ID].flont_vec_def.y < -flont_vec_MAX.y){
        player_info_all[gGame.player_ID].flont_vec_def.y = -flont_vec_MAX.y;
    }
    if (player_info_all[gGame.player_ID].flont_vec_def.z > flont_vec_MAX.z){
        player_info_all[gGame.player_ID].flont_vec_def.z = flont_vec_MAX.z;
    }
    else if (player_info_all[gGame.player_ID].flont_vec_def.z < -flont_vec_MAX.z){
        player_info_all[gGame.player_ID].flont_vec_def.z = -flont_vec_MAX.z;
    }


    //機体の回転角度を設定
    player_info_all[gGame.player_ID].vector.x += player_info_all[gGame.player_ID].vector_def.x;
    player_info_all[gGame.player_ID].vector.y += player_info_all[gGame.player_ID].vector_def.y;
    player_info_all[gGame.player_ID].vector.z += player_info_all[gGame.player_ID].vector_def.z;

    //機体の前方角度を設定
    player_info_all[gGame.player_ID].flont_vec.x += player_info_all[gGame.player_ID].flont_vec_def.x;
    player_info_all[gGame.player_ID].flont_vec.y += player_info_all[gGame.player_ID].flont_vec_def.y;
    player_info_all[gGame.player_ID].flont_vec.z += player_info_all[gGame.player_ID].flont_vec_def.z;

    //360°以上を補正
    if (player_info_all[gGame.player_ID].vector.x > 360.0){
        player_info_all[gGame.player_ID].vector.x -= 360.0;
    }
    if (player_info_all[gGame.player_ID].vector.x < -360.0){
        player_info_all[gGame.player_ID].vector.x += 360.0;
    }
    if (player_info_all[gGame.player_ID].vector.y > 360.0){
        player_info_all[gGame.player_ID].vector.y -= 360.0;
    }
    if (player_info_all[gGame.player_ID].vector.y < -360.0){
        player_info_all[gGame.player_ID].vector.y += 360.0;
    }
    if (player_info_all[gGame.player_ID].vector.z > 360.0){
        player_info_all[gGame.player_ID].vector.z -= 360.0;
    }
    if (player_info_all[gGame.player_ID].vector.z < -360.0){
        player_info_all[gGame.player_ID].vector.z += 360.0;
    }

    if (player_info_all[gGame.player_ID].flont_vec.x > 360.0){
        player_info_all[gGame.player_ID].flont_vec.x -= 360.0;
    }
    if (player_info_all[gGame.player_ID].flont_vec.x < -360.0){
        player_info_all[gGame.player_ID].flont_vec.x += 360.0;
    }
    if (player_info_all[gGame.player_ID].flont_vec.y > 360.0){
        player_info_all[gGame.player_ID].flont_vec.y -= 360.0;
    }
    if (player_info_all[gGame.player_ID].flont_vec.y < -360.0){
        player_info_all[gGame.player_ID].flont_vec.y += 360.0;
    }
    if (player_info_all[gGame.player_ID].flont_vec.z > 360.0){
        player_info_all[gGame.player_ID].flont_vec.z -= 360.0;
    }
    if (player_info_all[gGame.player_ID].flont_vec.z < -360.0){
        player_info_all[gGame.player_ID].flont_vec.z += 360.0;
    }

    //最大かカーブのときの角度
    double vec_MAX_y = 45.0;
    if(player_info_all[gGame.player_ID].vector.z > vec_MAX_y){
        player_info_all[gGame.player_ID].vector.z = vec_MAX_y;
    } else if(player_info_all[gGame.player_ID].vector.z < -vec_MAX_y){
        player_info_all[gGame.player_ID].vector.z = -vec_MAX_y;
    }
    
    if(player_info_all[gGame.player_ID].flont_vec.z > vec_MAX_y){
        player_info_all[gGame.player_ID].flont_vec.z = vec_MAX_y;
    } else if(player_info_all[gGame.player_ID].flont_vec.z < -vec_MAX_y){
        player_info_all[gGame.player_ID].flont_vec.z = -vec_MAX_y;
    }

    //機体を元に戻す
    double minus_v = 4.0;
    
    if (player_info_all[gGame.player_ID].vector_def.z == 0){
        if (player_info_all[gGame.player_ID].vector.z > 0){
            if (player_info_all[gGame.player_ID].vector.z < minus_v){
            } else {
                player_info_all[gGame.player_ID].vector.z -= minus_v;
            }
            player_info_all[gGame.player_ID].vector_def.y *= 0.6;
            player_info_all[gGame.player_ID].flont_vec_def.y  *= 0.6;
        }
        else if (player_info_all[gGame.player_ID].vector.z < 0.0){
            if (player_info_all[gGame.player_ID].vector.z > -minus_v){
            } else {
                player_info_all[gGame.player_ID].vector.z += minus_v;

            }
            player_info_all[gGame.player_ID].vector_def.y *= 0.6;
            player_info_all[gGame.player_ID].flont_vec_def.y  *= 0.6;
        }
    }

    if (fabs(player_info_all[gGame.player_ID].vector_def.y) < 0.1){
        player_info_all[gGame.player_ID].vector_def.y = 0.0;
    }
    if (fabs(player_info_all[gGame.player_ID].flont_vec_def.y) < 0.1){
        player_info_all[gGame.player_ID].flont_vec_def.y = 0.0;
    }
    
    



    //衝突時のための直前の座標を保存
    xyz point_buffa = (xyz){player_info_all[gGame.player_ID].point.x, player_info_all[gGame.player_ID].point.y, player_info_all[gGame.player_ID].point.z};
    //機体を前進(座標更新)

    player_info_all[gGame.player_ID].point
        = point_func(player_info_all[gGame.player_ID].point,
                        0.0, 0.0, -player_info_all[gGame.player_ID].speed,
                        player_info_all[gGame.player_ID].flont_vec);

    //壁とぶつかっていたら直前の座標に
    if(Wall_clash(player_info_all[gGame.player_ID].point) != 0){
        player_info_all[gGame.player_ID].point = (xyz){point_buffa.x, point_buffa.y, point_buffa.z};
    }

    //各機体との距離
    //各機体との角度(画面描写用)
    for (int i = 0; i < gGame.player_num; i++){
        if (i != gGame.player_ID){
            gGame.distance[i] = distance(player_info_all[i].point, player_info_all[gGame.player_ID].point);
            gGame.ship2vec[i] = acos((-player_info_all[i].point.z + player_info_all[gGame.player_ID].point.z)
                                 / sqrt((player_info_all[gGame.player_ID].point.x - player_info_all[i].point.x)*(player_info_all[gGame.player_ID].point.x - player_info_all[i].point.x) 
                                 + (player_info_all[gGame.player_ID].point.z - player_info_all[i].point.z)*(player_info_all[gGame.player_ID].point.z - player_info_all[i].point.z)))
                                 * 180.0 / M_PI;
            // fprintf(stderr, "vec = %f\n", gGame.ship2vec[i] );
            if ((player_info_all[i].point.x - player_info_all[gGame.player_ID].point.x) >= 0){
                gGame.ship2vec[i] = 180.0 - gGame.ship2vec[i];
            } else {
                gGame.ship2vec[i] = gGame.ship2vec[i] - 180.0;
            }   
        }
    }

    
    gGame.window_reflesh = 1;  //描写指示
    gGame.network_player = 1;
}


/*
    二点間の距離を計算
*/
double distance (xyz p1, xyz p2){
    return sqrt((p1.x - p2.x)*(p1.x - p2.x) + (p1.y - p2.y)*(p1.y - p2.y) + (p1.z - p2.z)*(p1.z - p2.z));
}

/*
    機体と壁がぶつかっているかを判定
    ぶつかっていたら当たりどころを1~6、ぶつかっていなかったら0
*/
int Wall_clash(xyz ship_point){
    int w, h; //現在地

    w = ship_point.x / gGame.map_tile_size;
    h = -ship_point.z / gGame.map_tile_size;

    double x_under, x_top, y_under, y_top, z_under, z_top;

    y_under = player_info_all[gGame.player_ID].ship_size;
    y_top   = gGame.map_tile_size - player_info_all[gGame.player_ID].ship_size;



    if (gGame.map[h-1][w] == WALL){
        z_top = -gGame.map_tile_size * h - player_info_all[gGame.player_ID].ship_size;
    } else {
        z_top = -gGame.map_tile_size * h;
    }
    if (gGame.map[h+1][w] == WALL){
        z_under = -gGame.map_tile_size * (h+1) + player_info_all[gGame.player_ID].ship_size ;
    } else {
        z_under = -gGame.map_tile_size * (h+1);
    }
    if (gGame.map[h][w+1] == WALL){
        x_top = gGame.map_tile_size * (w+1) - player_info_all[gGame.player_ID].ship_size;
    } else {
        x_top = gGame.map_tile_size * (w+1);
    }
    if (gGame.map[h][w-1] == WALL){
        x_under = gGame.map_tile_size * w + player_info_all[gGame.player_ID].ship_size;
    } else {
        x_under = gGame.map_tile_size * w;
    }

    if (ship_point.x >= x_top){
        return 1;
    }
    if (ship_point.x <= x_under){
        return 2;
    }
    if (ship_point.y >= y_top){
        return 3;
    }
    if (ship_point.y <= y_under){
        return 4;
    }
    if (ship_point.z >= z_top){
        return 5;
    }
    if (ship_point.z <= z_under){
        return 6;
    }

    return 0;
}



/*アイテムの当たり判定*/
int Wall_clash_item(xyz item_point){
    int w, h; //現在地

    w = item_point.x / gGame.map_tile_size;
    h = -item_point.z / gGame.map_tile_size;

    //x,z軸の当たり判定
    if (gGame.map[h][w] == WALL){
        return 1;
    }
    if (item_point.y > gGame.map_tile_size){
        return 2;
    }
    if (item_point.y < 0){
        return 3;
    }

    return 0;
}


/*
    アイテムの移動
*/
void Move_items(void){
    //アイテムの追加
    if (gGame.input_move.item1 == SDL_TRUE){ //銃弾の時
        //item_IDを設定
        int item_ID;
        item_ID = gGame.player_ID * item_num_init;
        while (item_ID < (gGame.player_ID+1)*item_num_init){
            if (item_info_all[item_ID].kind == 0){
                Mix_PlayMusic(gGame.shot, 0);
                break;
            }
            item_ID++;
        }

        //詳細を設定
        item_info_all[item_ID].kind = 1;
        item_info_all[item_ID].point = player_info_all[gGame.player_ID].point;
        item_info_all[item_ID].start_point = player_info_all[gGame.player_ID].point;
        item_info_all[item_ID].vec = player_info_all[gGame.player_ID].flont_vec;
        item_info_all[item_ID].speed = 0.4;
        item_info_all[item_ID].see = SDL_TRUE;

        Mix_PlayMusic(gGame.shot, 0);
        gGame.network_item = 1;

        #ifdef DEBUG
            fprintf(stderr, "Ammo is lanched : %d\n", item_ID);
        #endif
    }

    if (gGame.input_move.item2 == SDL_TRUE){ //ミサイルの時
        if (gGame.item2_nmax >= 0){
            //item_IDを設定
            int item_ID;
            item_ID = gGame.player_ID * item_num_init;
            while (item_ID < (gGame.player_ID+1)*item_num_init){
                if (item_info_all[item_ID].kind == 0){
                    break;
                }
                item_ID++;
            }

            //詳細を設定
            item_info_all[item_ID].kind = 2;
            item_info_all[item_ID].point = player_info_all[gGame.player_ID].point;
            item_info_all[item_ID].start_point = player_info_all[gGame.player_ID].point;
            item_info_all[item_ID].speed = 0.35;
            item_info_all[item_ID].see = SDL_TRUE;

            //一番近い敵の機体を追尾
            double temp = 10000000000.0;
            int target;
            if (gGame.player_ID < gGame.player_num/2){ //自分がチーム1なら
                target = gGame.player_num/2;
                for (int i = gGame.player_num/2; i < gGame.player_num; i++){
                    if (player_info_all[i].see == SDL_TRUE){
                        if (gGame.distance[i] < temp){
                            temp = gGame.distance[i];
                            target = i;
                        }
                    }
                }
            } else if (gGame.player_ID >= gGame.player_num/2){ //自分がチーム2なら
                target = 0;
                for (int i = 0; i < gGame.player_num/2; i++){
                    if (player_info_all[i].see == SDL_TRUE){
                        if (gGame.distance[i] < temp){
                            temp = gGame.distance[i];
                            target = i;
                        }
                    }
                }
            }
            item_info_all[item_ID].target = target;
            item_info_all[item_ID].vec = player_info_all[gGame.player_ID].flont_vec; //前に飛ばす

            gGame.item2_nmax--;
            gGame.item2_status = 1;

            Mix_PlayMusic(gGame.missoil, 1);
            gGame.network_item = 1;
            #ifdef DEBUG
            fprintf(stderr, "missoil is lanched Target is %d vec = %f,%f,%f\n",target, item_info_all[item_ID].vec.x,item_info_all[item_ID].vec.y, item_info_all[item_ID].vec.z);
            #endif
        }
        else { //ミサイルの残弾なし
            #ifdef DEBUG
            fprintf(stderr, "missoil is already lanched!!!\n");
            #endif
        }
        gGame.input_move.item2 = SDL_FALSE;
    }

    if (gGame.input_move.item3 == SDL_TRUE){ //地雷の時
        if (gGame.item3_nmax > 0){
            //item_IDを設定
            int item_ID;
            item_ID = gGame.player_ID * item_num_init;
            while (item_ID < (gGame.player_ID+1)*item_num_init){
                if (item_info_all[item_ID].kind == 0){
                    break;
                }
                item_ID++;
            }

            //詳細を設定
            item_info_all[item_ID].kind = 3;
            item_info_all[item_ID].point = player_info_all[gGame.player_ID].point;
            item_info_all[item_ID].start_point = player_info_all[gGame.player_ID].point;
            item_info_all[item_ID].speed = 0.0;
            item_info_all[item_ID].vec = player_info_all[gGame.player_ID].vector;
            item_info_all[item_ID].see = SDL_TRUE;

            gGame.item3_nmax--;
            gGame.network_item = 1;

            #ifdef DEBUG
                fprintf(stderr, "LandMine is lanched\n");
            #endif
        } else { //地雷の残弾なし
            #ifdef DEBUG
            fprintf(stderr, "Landmine is already lanched!!!\n");
            #endif
        }
        gGame.input_move.item3 = SDL_FALSE;
    }


    //アイテムの移動など

    for (int i = 0; i < gGame.player_num * item_num_init + 2; i++){
        switch (item_info_all[i].kind){
        case 0:
            break;
        case 1: //弾丸
            if (item_info_all[i].see == SDL_TRUE){
                //移動
                item_info_all[i].point = point_func(item_info_all[i].point, 0.0, 0.0, -item_info_all[i].speed, item_info_all[i].vec);
                //当たり判定
                if (Wall_clash_item(item_info_all[i].point) != 0){
                    item_info_all[i].kind = 0;
                    item_info_all[i].see = SDL_FALSE;
                }
                //発射地点からの飛行距離判定
                if (distance(item_info_all[i].point, item_info_all[i].start_point) > gGame.item1_dmax){
                    item_info_all[i].kind = 0;
                    item_info_all[i].see = SDL_FALSE;
                }
            }
            gGame.network_item = 1;
            break;

        case 2: //ミサイル
            if (item_info_all[i].see == SDL_TRUE){
                //角度調整のアルゴリズム
                xyz max_vec = (xyz){7.0, 5.0, 3.0};
                xyz buffer_vec;
                if (player_info_all[item_info_all[i].target].see == SDL_TRUE)
                {
                    //左右
                    double y_lengh = distance((xyz){item_info_all[i].point.x, 0.0, item_info_all[i].point.z}, (xyz){player_info_all[item_info_all[i].target].point.x, 0.0, player_info_all[item_info_all[i].target].point.z});
                    if(item_info_all[i].point.z > player_info_all[item_info_all[i].target].point.z){
                        buffer_vec.y = -(90.0 - acos(((-item_info_all[i].point.x) - (-player_info_all[item_info_all[i].target].point.x)) / y_lengh) * 180.0 / M_PI);
                    }
                    if(item_info_all[i].point.z <= player_info_all[item_info_all[i].target].point.z){
                        // buffer_vec.y = -(acos(((-item_info_all[i].point.x) - (-player_info_all[item_info_all[i].target].point.x)) / y_lengh) * 180.0 / M_PI);
                        buffer_vec.y = -90.0 - acos(((-item_info_all[i].point.x) - (-player_info_all[item_info_all[i].target].point.x)) / y_lengh) * 180.0 / M_PI;
                    }


                    //上下
                    double x_lengh = distance((xyz){0.0, item_info_all[i].point.y, item_info_all[i].point.z},(xyz){0.0, player_info_all[item_info_all[i].target].point.y, player_info_all[item_info_all[i].target].point.z});
                    buffer_vec.x = (acos(((-item_info_all[i].point.z) - (-player_info_all[item_info_all[i].target].point.z)) / x_lengh) * 180.0 / M_PI) - 180.0;
                   
                    //回転
                    buffer_vec.z = 0.0;



                    if (buffer_vec.x - item_info_all[i].vec.x > max_vec.x){
                        item_info_all[i].vec.x += max_vec.x;
                    } else if (buffer_vec.x - item_info_all[i].vec.x < -max_vec.x){
                        item_info_all[i].vec.x -= max_vec.x;
                    } else {
                        item_info_all[i].vec.x = buffer_vec.x;
                    }
                    if (buffer_vec.y - item_info_all[i].vec.y > max_vec.y){
                        item_info_all[i].vec.y += max_vec.y;
                    } else if (buffer_vec.y - item_info_all[i].vec.y < -max_vec.y){
                        item_info_all[i].vec.y -= max_vec.y;
                    } else {
                        item_info_all[i].vec.y = buffer_vec.y;
                    }
                    if (buffer_vec.z - item_info_all[i].vec.z > max_vec.z){
                        item_info_all[i].vec.z += max_vec.z;
                    } else if (buffer_vec.z - item_info_all[i].vec.z < -max_vec.z){
                        item_info_all[i].vec.z -= max_vec.z;
                    } else {
                        item_info_all[i].vec.z = buffer_vec.z;
                    }
                }
                //移動
                item_info_all[i].point = point_func(item_info_all[i].point, 0.0, 0.0, -item_info_all[i].speed, item_info_all[i].vec);




                //当たり判定
                if (Wall_clash_item(item_info_all[i].point) != 0){
                    item_info_all[i].kind = 0;
                    item_info_all[i].see = SDL_FALSE;
                    #ifdef DEBUG
                    fprintf(stderr, "missoil is clashed into wall!!\n");
                    #endif
                }
                //発射地点からの飛行距離判定
                if (distance(item_info_all[i].point, item_info_all[i].start_point) > gGame.item2_dmax){
                    item_info_all[i].kind = 0;
                    item_info_all[i].see = SDL_FALSE;
                    #ifdef DEBUG
                    fprintf(stderr, "missoil fly too long!!\n");
                    #endif
                }
            }
            gGame.network_item = 1;
            break;


        case 3: //地雷
            //特にする操作なし。
            break;
        
        case 4: //鍵
            //落ちてる鍵を取得
            if (item_info_all[i].see == SDL_TRUE && item_info_all[i].target == 100 && player_info_all[gGame.player_ID].see == SDL_TRUE){
                if (distance(item_info_all[i].point, player_info_all[gGame.player_ID].point) < gGame.ship_item_distance3){
                    SendKeyData(i- player_num_init*item_num_init);
                    // printf("GET KEY!!!! %d\n",i- player_num_init*item_num_init);

                }
            }

            //鍵を持ってゴール
            if (item_info_all[i].see == SDL_TRUE && item_info_all[i].target == gGame.player_ID && player_info_all[gGame.player_ID].see == SDL_TRUE){
                if (gGame.player_ID < player_num_init/2) { //自分がチーム1の時
                    if(distance(player_info_all[gGame.player_ID].point, (xyz){gGame.map_tile_size * (MAP_WIDTH-1), gGame.map_tile_size/2, -gGame.map_tile_size * (MAP_HEIGHT-1)}) < gGame.map_tile_size){
                        SendGoalData();
                    }                    
                } else {
                    if(distance(player_info_all[gGame.player_ID].point, (xyz){gGame.map_tile_size, gGame.map_tile_size/2, -gGame.map_tile_size}) < gGame.map_tile_size){
                        SendGoalData();
                    }
                }
            }
            
            
            break;
            

        default:
            #ifdef DEBUG
            fprintf(stderr, "ERROR! unknow item kind : %d\n", item_info_all[i].kind);
            break;
            #endif
            break;
        }


        //機体との当たり判定
        for (int j = 0; j < gGame.player_num; j++){
            //当たっていた時の処理
            switch (item_info_all[i].kind){
            case 0:
                break;
            case 1: //弾丸
                if (i/item_num_init != j) {  //自爆防止
                    if (player_info_all[j].see == SDL_TRUE) {
                        if (distance(item_info_all[i].point, player_info_all[j].point) < gGame.ship_item_distance1){
                            player_info_all[j].HP -= 20;
                            item_info_all[i].kind = 0; //リストから消去
                            item_info_all[i].see = SDL_FALSE; //当たったら消える
                            Mix_PlayMusic(gGame.shot_hit, 1);
                            SendHitData(j, 20);
                            #ifdef DEBUG
                            fprintf(stderr, "Ammo is hit to player_%d!!!\n", j);
                            #endif
                        }
                    }
                }
                gGame.network_player = 1;
                gGame.network_item = 1;
                break;
            case 2: //ミサイル
                if (i/item_num_init != j) {  //自爆防止
                    if (player_info_all[j].see == SDL_TRUE) {
                        if (distance(item_info_all[i].point, player_info_all[j].point) < gGame.ship_item_distance2){
                            player_info_all[j].HP -= 75;
                            item_info_all[i].kind = 0; //リストから消去
                            item_info_all[i].see = SDL_FALSE; //当たったら消える
                            Mix_PlayMusic(gGame.missoil_hit, 1);

                            SendHitData(j, 75);
                            #ifdef DEBUG
                            fprintf(stderr, "Missoil is hit to player_%d!!!\n", j);
                            #endif
                        }
                    }
                }
                gGame.network_player = 1;
                gGame.network_item = 1;
                break;
            case 3: //地雷
                if (i/item_num_init != j) {  //自爆防止
                    if (player_info_all[j].see == SDL_TRUE) {
                        if (distance(item_info_all[i].point, player_info_all[j].point) < gGame.ship_item_distance3){
                            player_info_all[j].HP -= 100;
                            item_info_all[i].kind = 0; //リストから消去
                            item_info_all[i].see = SDL_FALSE; //当たったら消える
                            SendHitData(j, 100);
                            Mix_PlayMusic(gGame.explosion, 1);
                            #ifdef DEBUG
                            fprintf(stderr, "landmine is hit to player_%d!!!\n", j);
                            #endif
                        }
                    }
                }
                gGame.network_player = 1;
                gGame.network_item = 1;
                break;
            case 4: //鍵
                break;

            default:
                #ifdef DEBUG
                fprintf(stderr, "ERROR! unknow item kind : %d\n", item_info_all[i].kind);
                #endif
                break;
            }
        }
    }
}


/*
    各機体のHP管理
*/
void Manege_HP(void){
    if (player_info_all[gGame.player_ID].HP <= 0){
        player_info_all[gGame.player_ID].see = SDL_FALSE;
    }
    
    
    int count1 = 0; //チーム1のHP0の人のカウント
    int count2 = 0; //チーム2のHP0の人のカウント
    for (int i = 0; i < gGame.player_num; i++){
        //ゲームオーバーの機体のカウント
        if (player_info_all[i].see == SDL_FALSE){
            if (i < gGame.player_num/2){  //チーム1のカウント
                count1++;
            }
            else { //チーム2のカウント
                count2++;
            }
        }
    }

    //ゲームの終了条件
    if (count1 == gGame.player_num/2){
        //チーム1の敗北
        #ifndef DEBUG
        fprintf(stderr, "Team 1 is losed\n");
        #endif
        gGame.finish_team_ID = 2;

    } else if (count2 == gGame.player_num/2) {
        //チーム2の敗北
        #ifndef DEBUG
        fprintf(stderr, "Team 2 is losed\n");
        #endif
        gGame.finish_team_ID = 1;
    }
}



/*************************************
End_system : システムの終了
**************************************/
void End_system(void){
    //サーバに終了を通知
    #ifndef NON_NETWORK
    SendGameStts();
    #endif

    //スレッドの終了
    //SDL_WaitThread(gGame.render_thread, NULL);
    SDL_WaitThread(gGame.func_thread, NULL);
    SDL_WaitThread(gGame.move_thread, NULL);

    #ifndef NON_JOYCON
    SDL_WaitThread(gGame.joycon_thread, NULL);
    #endif
    
    #ifndef NON_NETWORK
    SDL_WaitThread(gGame.network_thread, NULL);
    #endif

    #ifdef DEBUG
        fprintf(stderr, "ALL threads is finished\n");
    #endif
    SDL_DestroyMutex(gGame.Mutex); //排他処理の破棄

    //音楽関係
    Mix_HaltChannel(-1);
    Mix_FreeMusic(gGame.shot);
    Mix_FreeMusic(gGame.shot_hit);
    Mix_FreeMusic(gGame.missoil);
    Mix_FreeMusic(gGame.missoil_hit);
    Mix_FreeMusic(gGame.explosion);

    Mix_CloseAudio();


    SDL_GL_DeleteContext(gGame.context);
    SDL_DestroyWindow(gGame.window);
    SDL_Quit();


    #ifndef NON_NETWORK
    /* クライアントを閉じる */
    TerminateClient();
    #endif

    #ifdef DEBUG
    fprintf(stderr, "system finish!\n");
    #endif
}


/* 内部的な関数 */
/*
    座標と角度から回転行列を計算
    返り値はxyzの構造体
*/
xyz rotate_point(const xyz point, const xyz vec){
    double pi = 3.14159265358979323846264338327950288;
    double rad_x = vec.x * pi / 180.0;
    double rad_y = vec.y * pi / 180.0;
    double rad_z = vec.z * pi / 180.0;
    double matP[3][1] = {{point.x}, {point.y}, {point.z}}; //元の座標

    //回転行列
    double matX[3][3] = {
        {1, 0, 0},
        {0, cos(rad_x), -sin(rad_x)},
        {0, sin(rad_x), cos(rad_x)},
    };



    double matY[3][3] = {
        {cos(rad_y), 0, sin(rad_y)},
        {0, 1, 0},
        {-sin(rad_y), 0, cos(rad_y)},
    };


    double matZ[3][3] = {
        {cos(rad_z), -sin(rad_z), 0},
        {sin(rad_z), cos(rad_z), 0},
        {0, 0, 1},
    };


    //x軸周りに回転
    double tempX[3][1] = {};
    for (int i = 0; i < 3; i++){
        for (int j = 0; j < 1; j++){
            for (int k = 0; k < 3; k++){
                tempX[i][j] += matX[i][k] * matP[k][j];
            }
        }
    }


    //z軸周りに回転
    double tempZ[3][1] = {};
    for (int i = 0; i < 3; i++){
        for (int j = 0; j < 1; j++){
            for (int k = 0; k < 3; k++){
                tempZ[i][j] += matZ[i][k] * tempX[k][j];
            }
        }
    }


    //y軸周りに回転
    double tempY[3][1] = {};
    for (int i = 0; i < 3; i++){
        for (int j = 0; j < 1; j++){
            for (int k = 0; k < 3; k++){
                tempY[i][j] += matY[i][k] * tempZ[k][j];
            }
        }
    }


    xyz result;
    result.x = tempY[0][0];
    result.y = tempY[1][0];
    result.z = tempY[2][0];

    return result;
}

/*
    描写用の回転行列の計算結果を使用して絶対座標を計算する関数。
    基準となる座標、差をxyz分3個、角度
*/
xyz point_func(xyz base, float x, float y, float z, xyz vec){
    xyz temp_point;  //機体からの相対座標
    temp_point = rotate_point((xyz){x, y, z}, vec);  //差

    //新しい座標を計算
    xyz return_base;
    return_base.x = base.x + temp_point.x;
    return_base.y = base.y + temp_point.y;
    return_base.z = base.z + temp_point.z;

    return return_base;
}


//引数のy座標を元にボタンを表示する関数
void TYPE(int y1,int y2, char button[],int mode){
    // // boxcolor(w_renderer,200,y1,600,y2,0xff000000);
    w_font = TTF_OpenFont("/usr/share/fonts/truetype/fonts-japanese-gothic.ttf", 30);
    switch (mode){
    case 0:
        w_strings = TTF_RenderUTF8_Blended(w_font, button, w_red);
        break;
    default:
        w_strings = TTF_RenderUTF8_Blended(w_font, button, w_green);
        break;
    }    
    w_texture = SDL_CreateTextureFromSurface(w_renderer, w_strings); 
    SDL_Rect src_rect = {0, 0, w_strings->w, w_strings->h}; // 転送元
    SDL_Rect dst_rect = {360,(y1 + y2) / 2.1, w_strings->w, w_strings->h};
    SDL_RenderCopy(w_renderer, w_texture, &src_rect, &dst_rect);
}

    //オプション
void option(int mode){
    // // boxcolor(w_renderer,725,580,775,620,0xff000000);
    w_option_w_font = TTF_OpenFont("/usr/share/fonts/truetype/fonts-japanese-gothic.ttf", 16);
    switch (mode){
    case 0:
        w_strings = TTF_RenderUTF8_Blended(w_option_w_font, "設定", w_red);
        break;
    default:
        w_strings = TTF_RenderUTF8_Blended(w_option_w_font, "設定", w_green);
        break;
    }    
    w_texture = SDL_CreateTextureFromSurface(w_renderer, w_strings); 
    SDL_Rect src_rect = {0, 0, w_strings->w, w_strings->h}; // 転送元
    SDL_Rect dst_rect = {740,1250 / 2.1, w_strings->w, w_strings->h};
    SDL_RenderCopy(w_renderer, w_texture, &src_rect, &dst_rect);
}

//ボタンの選択を表示する関数
void choice(int button_num){
        switch (button_num){
            case 0:    
                //第4引数が0が選択中
                TYPE(100,175,"START",0);
                TYPE(250,325,"Manual",1);
                TYPE(400,475,"END",1);
                option(1);
                break;
            
            case 1:
                //第4引数が0が選択中
                TYPE(100,175,"START",1);
                TYPE(250,325,"Manual",0);
                TYPE(400,475,"END",1);
                option(1);
                break;

            case 2:
                //第4引数が0が選択中
                TYPE(100,175,"START",1);
                TYPE(250,325,"Manual",1);
                TYPE(400,475,"END",0);
                option(1);
                break;

            // 設定ボタン
            case 3:
                TYPE(100,175,"START",1);
                TYPE(250,325,"Manual",1);
                TYPE(400,475,"END",1);
                option(0);
                break;
        }

}

int Welcome(void)
{   
    w_cat_pic = IMG_Load("./img/mycat.png");
    // Mix_Music *music; // BGMデータ格納用構造体
    int button_num = 0;
    TTF_Init();
    // SDL_Texture* 
    int step = 0;
    


    // SDL初期化（初期化失敗の場合は終了）
    if(SDL_Init(SDL_INIT_EVERYTHING) < 0) {
        printf("failed to initialize SDL.\n");
        exit(-1);
    }

    // SDL_mixerの初期化（MP3ファイルを使用）
    Mix_Init(MIX_INIT_MP3);



    w_window = SDL_CreateWindow("welcome",SDL_WINDOWPOS_CENTERED,SDL_WINDOWPOS_CENTERED,800,640,0);
    w_renderer = SDL_CreateRenderer(w_window,-1,0);

    // 背景用意
    SDL_Surface* s = IMG_Load("./img/image_pic.png"); // 最初だけ必要なサーフェイスの作成
    if (NULL == s) {          
        // 画像の取得に失敗した場合
        printf("failed to load island image file ");
        return 0;
    }
    SDL_Texture* image_pic = SDL_CreateTextureFromSurface(w_renderer, s);

    if (image_pic == NULL) {
        printf("%s\n",SDL_GetError());
        return 0;
    }
    SDL_Rect src = { 0, 0, s->w, s->h };             // 転送元
    SDL_Rect dst = { 0, 0, s->w, s->h };             // 転送先
    SDL_RenderCopy(w_renderer, image_pic, &src, &dst); // レンダラーにコピー
    SDL_FreeSurface(s); //サーフェイスの解放

        // 背景用意
    SDL_Surface* howto = IMG_Load("./img/howto.png"); // 最初だけ必要なサーフェイスの作成
    if (NULL == howto) {               
        // 画像の取得に失敗した場合
        printf("failed to load island image file ");
        return 0;//
    }
    SDL_Texture* howto_pic = SDL_CreateTextureFromSurface(w_renderer, howto);

    if (howto_pic == NULL) {
        printf("%s\n",SDL_GetError());
        return 0;
    }

    SDL_Rect howto_src = { 0, 0, howto->w, howto->h }; // 転送元
    SDL_Rect howto_dst = { 50, 150, 560, 315 };             // 転送先            
    SDL_FreeSurface(howto); //サーフェイスの解放

    SDL_Color name_white = { 255, 0, 0, 255 };

    w_pngtex = SDL_CreateTextureFromSurface(w_renderer,w_cat_pic);
    SDL_RenderCopy(w_renderer, w_pngtex, NULL, NULL);

    //第４引数は選択中か判別
    TYPE(100,175,"START",0);
    TYPE(250,325,"Manual",1);
    TYPE(400,475,"END",1);
    option(1);

//最初の描画
    SDL_RenderPresent(w_renderer);

    while (1){
        if (SDL_PollEvent(&w_event)){
            switch(w_event.type){
            case SDL_MOUSEBUTTONDOWN:
                    if (w_event.button.x >= 200 && w_event.button.x <= 600){
                        if (w_event.button.y >= 100 && w_event.button.y <= 175){
                            SDL_RenderClear(w_renderer);
                            SDL_RenderCopy(w_renderer, w_pngtex, NULL, NULL);
                            choice(0);                            
                            SDL_RenderPresent(w_renderer);  
                            if (button_num == 0){    
                                while (step == 0)
                                {   
                                    // int verline;
                                    char player_name[100]  = "";
                                    char input_buffer[100] = ""; // 一時的なテキストデータを格納するバッファ
                                    // 入力受付
                                    SDL_Rect rect = { 50, 120, 230, 40 }; // 入力を受け付ける場所
                                    SDL_SetTextInputRect(&rect);
                                    SDL_StartTextInput();                 // テキスト入力の開始

                                    SDL_bool quit = SDL_FALSE;
                                    int backspace = 0; //削除文字数
                                    while (!quit) {
                                        SDL_Event w_event;
                                        if (SDL_PollEvent(&w_event)) {
                                            switch (w_event.type) {
                                            case SDL_QUIT:
                                                quit = SDL_TRUE;
                                                step = 100;
                                                // gGame.end_stts = 0;
                                                // return SDL_FALSE;
                                                break;
                                            case SDL_KEYDOWN:
                                                /* キーリピートは無視 */
                                                if (w_event.key.repeat)
                                                    break;
                                                switch (w_event.key.keysym.sym) {
                                                case SDLK_ESCAPE:
                                                    /** 終了指示 **/
                                                    quit = SDL_TRUE;
                                                    step = 100;
                                                    // gGame.end_stts = 0;
                                                    break;
                                                case SDLK_RETURN:
                                                    /* 決定 */
                                                    quit = SDL_TRUE;
                                                    step = 1;
                                                    break;
                                                case SDLK_BACKSPACE:
                                                    backspace = 1;
                                                    break;
                                                }
                                            case SDL_TEXTINPUT:
                                                strcat(input_buffer, w_event.text.text);
                                                break;
                                            default:
                                                break;
                                            }
                                        }
                                        if (iscntrl(input_buffer[0]) == 0)
                                        {
                                            strcat(player_name, input_buffer); // 文字化け対策
                                            input_buffer[0] = '\0';
                                        }
                                        input_buffer[0] = '\0';

                                        if (backspace != 0){
                                            int len = strlen(player_name);
                                            if (len > 0) {
                                                player_name[len - 1] = '\0';
                                            }
                                            backspace = 0;
                                        }
                                        SDL_RenderCopy(w_renderer, image_pic, &src, &dst); // レンダラーにコピー

                                        //名前の縦線
                                    // verline = strlen(player_name);


                                        // 名前のための四角
                                        // // boxcolor(w_renderer, 30, 80, 300, 300, 0x69696969);
                                        // // boxcolor(w_renderer, 50, 120, 280, 160, 0xC0C0C0bb);

                                        // 名前のやつ
                                        SDL_Surface* m_s  = TTF_RenderUTF8_Blended(w_font, "プレイヤー名", name_white);
                                        SDL_Texture* m_t  = SDL_CreateTextureFromSurface(w_renderer, m_s);
                                        SDL_Rect src_rect = { 0, 0, m_s->w, m_s->h };        // コピー元文字列の領域（x, y, w, h）
                                        SDL_Rect dst_rect = { 50, 100, m_s->w, m_s->h };     // 文字列のコピー先の座標と領域（x, y, w, h）
                                        SDL_RenderCopy(w_renderer, m_t, &src_rect, &dst_rect); // レンダラーにコピー
                                        SDL_FreeSurface(m_s); //サーフェイスの解放
                                        SDL_DestroyTexture(m_t);


                                        if (strlen(player_name) > 0) {
                                            SDL_Surface* player_name_s = TTF_RenderUTF8_Blended(w_font, player_name, name_white);
                                            SDL_Texture* player_name_t = SDL_CreateTextureFromSurface(w_renderer, player_name_s);
                                            SDL_Rect input_src_rect    = { 0, 0, player_name_s->w, player_name_s->h };    // コピー元文字列の領域（x, y, w, h）
                                            SDL_Rect input_dst_rect    = { 50, 120, player_name_s->w, player_name_s->h }; // 文字列のコピー先の座標と領域（x, y, w, h）
                                            SDL_RenderCopy(w_renderer, player_name_t, &input_src_rect, &input_dst_rect);    // レンダラーにコピー
                                            SDL_FreeSurface(player_name_s); //サーフェイスの解放
                                            SDL_DestroyTexture(player_name_t);
                                        }

                                        SDL_RenderPresent(w_renderer);
                                    }

                                    SDL_StopTextInput(); // テキスト入力の終了
                                    for (int i = 0; i < strlen(player_name); i++)
                                    {
                                        // gGame.player_name[i] = player_name[i];
                                        gGame.usrName[i] = player_name[i];
                                    }
                                    TTF_CloseFont(w_font);
                                    TTF_Quit();
                                    SDL_FreeSurface(w_strings);
                                    SDL_DestroyTexture(w_texture);
                                    SDL_DestroyRenderer(w_renderer);
                                    SDL_DestroyWindow(w_window);
                                    SDL_Quit();
                                    return 0;

                                }



                                while (step == 1)
                                {
                                    SDL_bool quit = SDL_FALSE;
                                    // int select_level = 0;
                                    while (!quit) {
                                        SDL_Event w_event;
                                        if (SDL_PollEvent(&w_event)) {
                                            switch (w_event.type) {
                                            case SDL_QUIT:
                                                quit = SDL_TRUE;
                                                step = 100;
                                                // gGame.end_stts = 0;
                                                break;
                                            case SDL_KEYDOWN:
                                                /* キーリピートは無視 */
                                                if (w_event.key.repeat)
                                                    break;
                                                switch (w_event.key.keysym.sym) {
                                                case SDLK_ESCAPE:
                                                    /** 終了指示 **/
                                                    quit = SDL_TRUE;
                                                    step = 100;
                                                    // gGame.end_stts = 0;
                                                    break;
                                                case SDLK_RETURN:
                                                    /* 決定 */
                                                    quit = SDL_TRUE;
                                                    // gGame.level = select_level;
                                                    step = 3;
                                                    break;
                                                    break;
                                                }

                                            case SDL_TEXTINPUT:
                                                break;
                                            default:
                                                break;
                                            }
                                        }

                                        //背景
                                        SDL_RenderCopy(w_renderer, image_pic, &src, &dst); // レンダラーにコピー
                                        // 名前のための四角
                                        // boxcolor(w_renderer, 30, 80, 450, 300, 0x69696969);
                                        // boxcolor(w_renderer, 50, 120, 280, 160, 0xC0C0C0bb);
                                        
                                        //説明
                                        SDL_Surface* m_s  = TTF_RenderUTF8_Blended(w_font, "ゲーム説明", name_white);
                                        SDL_Texture* m_t  = SDL_CreateTextureFromSurface(w_renderer, m_s);
                                        SDL_Rect src_rect0 = { 0, 0, m_s->w, m_s->h };        // コピー元文字列の領域（x, y, w, h）
                                        SDL_Rect dst_rect0 = { 50, 100, m_s->w, m_s->h };     // 文字列のコピー先の座標と領域（x, y, w, h）
                                        SDL_RenderCopy(w_renderer, m_t, &src_rect0, &dst_rect0); // レンダラーにコピー
                                        SDL_FreeSurface(m_s); //サーフェイスの解放
                                        SDL_DestroyTexture(m_t);

                                        SDL_Surface* m_s_1  = TTF_RenderUTF8_Blended(w_font, "Joy-Conを操作しながら", name_white);
                                        SDL_Texture* m_t_1  = SDL_CreateTextureFromSurface(w_renderer, m_s_1);
                                        SDL_Rect src_rect_1 = { 0, 0, m_s_1->w, m_s_1->h };        // コピー元文字列の領域（x, y, w, h）
                                        SDL_Rect dst_rect_1 = { 50, 150, m_s_1->w, m_s_1->h };     // 文字列のコピー先の座標と領域（x, y, w, h）
                                        SDL_RenderCopy(w_renderer, m_t_1, &src_rect_1, &dst_rect_1); // レンダラーにコピー
                                        SDL_FreeSurface(m_s_1); //サーフェイスの解放
                                        SDL_DestroyTexture(m_t_1);

                                        SDL_Surface* m_s_2  = TTF_RenderUTF8_Blended(w_font, "迷路の中を飛び回り", name_white);
                                        SDL_Texture* m_t_2  = SDL_CreateTextureFromSurface(w_renderer, m_s_2);
                                        SDL_Rect src_rect_2 = { 0, 0, m_s_2->w, m_s_2->h };        // コピー元文字列の領域（x, y, w, h）
                                        SDL_Rect dst_rect_2 = { 50, 200, m_s_2->w, m_s_2->h };     // 文字列のコピー先の座標と領域（x, y, w, h）
                                        SDL_RenderCopy(w_renderer, m_t_2, &src_rect_2, &dst_rect_2); // レンダラーにコピー
                                        SDL_FreeSurface(m_s_2); //サーフェイスの解放
                                        SDL_DestroyTexture(m_t_2);

                                        SDL_Surface* m_s_3  = TTF_RenderUTF8_Blended(w_font, "敵を倒しながら進んでください！", name_white);
                                        SDL_Texture* m_t_3  = SDL_CreateTextureFromSurface(w_renderer, m_s_3);
                                        SDL_Rect src_rect_3 = { 0, 0, m_s_3->w, m_s_3->h };        // コピー元文字列の領域（x, y, w, h）
                                        SDL_Rect dst_rect_3 = { 50, 250, m_s_3->w, m_s_3->h };     // 文字列のコピー先の座標と領域（x, y, w, h）
                                        SDL_RenderCopy(w_renderer, m_t_3, &src_rect_3, &dst_rect_3); // レンダラーにコピー
                                        SDL_FreeSurface(m_s_3); //サーフェイスの解放
                                        SDL_DestroyTexture(m_t_3);

                                        SDL_Surface* m_s_4  = TTF_RenderUTF8_Blended(w_font, "Enterキーを押して次へ", name_white);
                                        SDL_Texture* m_t_4  = SDL_CreateTextureFromSurface(w_renderer, m_s_4);
                                        SDL_Rect src_rect_4 = { 0, 0, m_s_4->w, m_s_4->h };        // コピー元文字列の領域（x, y, w, h）
                                        SDL_Rect dst_rect_4 = { 500, 450, m_s_4->w, m_s_4->h };     // 文字列のコピー先の座標と領域（x, y, w, h）
                                        SDL_RenderCopy(w_renderer, m_t_4, &src_rect_4, &dst_rect_4); // レンダラーにコピー
                                        SDL_FreeSurface(m_s_4); //サーフェイスの解放
                                        SDL_DestroyTexture(m_t_4);

                                        


                                        SDL_RenderPresent(w_renderer);

                                    }
                                }


                            }
                            button_num = 0;
                        }
                    }
                    if (w_event.button.x >= 200 && w_event.button.x <= 600){
                        if (w_event.button.y >= 250 && w_event.button.y <= 325 ){
                            SDL_RenderClear(w_renderer);
                            SDL_RenderCopy(w_renderer, w_pngtex, NULL, NULL);
                            choice(1);
                            SDL_RenderPresent(w_renderer);  
                            if (button_num == 1){
                                //背景
                                        SDL_RenderCopy(w_renderer, image_pic, &src, &dst); // レンダラーにコピー
                                        // 名前のための四角
                                        // boxcolor(w_renderer, 30, 80, 650, 450, 0x69696969);
                                        // boxcolor(w_renderer, 50, 120, 280, 160, 0xC0C0C0bb);
                                        
                                        //説明
                                        SDL_Surface* m_s  = TTF_RenderUTF8_Blended(w_font, "操作方法", name_white);
                                        SDL_Texture* m_t  = SDL_CreateTextureFromSurface(w_renderer, m_s);
                                        SDL_Rect src_rect0 = { 0, 0, m_s->w, m_s->h };        // コピー元文字列の領域（x, y, w, h）
                                        SDL_Rect dst_rect0 = { 50, 100, m_s->w, m_s->h };     // 文字列のコピー先の座標と領域（x, y, w, h）
                                        SDL_RenderCopy(w_renderer, m_t, &src_rect0, &dst_rect0); // レンダラーにコピー
                                        SDL_FreeSurface(m_s); //サーフェイスの解放
                                        SDL_DestroyTexture(m_t);

                                        SDL_RenderCopy(w_renderer, howto_pic, &howto_src, &howto_dst); // レンダラーにコピー




                                        SDL_RenderPresent(w_renderer);
                            }
                            
                            button_num = 1;
                        }
                    }
                    if (w_event.button.x >= 200 && w_event.button.x <= 600){
                        if (w_event.button.y >= 400 && w_event.button.y <= 475){
                            SDL_RenderClear(w_renderer);
                            SDL_RenderCopy(w_renderer, w_pngtex, NULL, NULL);
                            choice(2);
                            // SDL_RenderCopy(w_renderer, image_pic, &src, &dst); // レンダラーにコピー
                            SDL_RenderPresent(w_renderer);  
                            if (button_num == 2){
                                TTF_CloseFont(w_font);
                                TTF_Quit();
                                SDL_FreeSurface(w_strings);
                                SDL_DestroyTexture(w_texture);
                                SDL_DestroyRenderer(w_renderer);
                                SDL_DestroyWindow(w_window);
                                SDL_Quit();
                                return 0;
                            }
                            button_num = 2;
                            
                        }
                    }

                    if (w_event.button.x >= 725 && w_event.button.x <= 775){
                        if (w_event.button.y >= 580 && w_event.button.y <= 620){
                            SDL_RenderClear(w_renderer);
                            SDL_RenderCopy(w_renderer, w_pngtex, NULL, NULL);
                            choice(3);
                            if (button_num == 3){
                                //背景
                                SDL_RenderCopy(w_renderer, image_pic, &src, &dst); // レンダラーにコピー
                                // 名前のための四角
                                // boxcolor(w_renderer, 30, 80, 650, 450, 0x69696969);
                                
                                //説明
                                SDL_Surface* m_s  = TTF_RenderUTF8_Blended(w_font, "設定", name_white);
                                SDL_Texture* m_t  = SDL_CreateTextureFromSurface(w_renderer, m_s);
                                SDL_Rect src_rect0 = { 0, 0, m_s->w, m_s->h };        // コピー元文字列の領域（x, y, w, h）
                                SDL_Rect dst_rect0 = { 50, 100, m_s->w, m_s->h };     // 文字列のコピー先の座標と領域（x, y, w, h）
                                SDL_RenderCopy(w_renderer, m_t, &src_rect0, &dst_rect0); // レンダラーにコピー
                                SDL_FreeSurface(m_s); //サーフェイスの解放
                                SDL_DestroyTexture(m_t);

                            }
                                SDL_RenderPresent(w_renderer);
                            button_num = 3;
                        }
                    }
                    
            break;
                     

            case SDL_KEYDOWN:
                switch (w_event.key.keysym.sym){
                case SDLK_RETURN:
                    switch (button_num){
                        case 0:
                            while (step == 0)
                                {   
                                    // int verline;
                                    char player_name[100]  = "";
                                    char input_buffer[100] = ""; // 一時的なテキストデータを格納するバッファ
                                    // 入力受付
                                    SDL_Rect rect = { 50, 120, 230, 40 }; // 入力を受け付ける場所
                                    SDL_SetTextInputRect(&rect);
                                    SDL_StartTextInput();                 // テキスト入力の開始

                                    SDL_bool quit = SDL_FALSE;
                                    int backspace = 0; //削除文字数
                                    while (!quit) {
                                        SDL_Event w_event;
                                        if (SDL_PollEvent(&w_event)) {
                                            switch (w_event.type) {
                                            case SDL_QUIT:
                                                quit = SDL_TRUE;
                                                step = 100;
                                                break;
                                            case SDL_KEYDOWN:
                                                /* キーリピートは無視 */
                                                if (w_event.key.repeat)
                                                    break;
                                                switch (w_event.key.keysym.sym) {
                                                case SDLK_ESCAPE:
                                                    /** 終了指示 **/
                                                    quit = SDL_TRUE;
                                                    step = 100;
                                                    // gGame.end_stts = 0;
                                                    break;
                                                case SDLK_RETURN:
                                                    /* 決定 */
                                                    quit = SDL_TRUE;
                                                    step = 1;
                                                    break;
                                                case SDLK_BACKSPACE:
                                                    backspace = 1;
                                                    break;
                                                }
                                            case SDL_TEXTINPUT:
                                                strcat(input_buffer, w_event.text.text);
                                                break;
                                            default:
                                                break;
                                            }
                                        }
                                        if (iscntrl(input_buffer[0]) == 0)
                                        {
                                            strcat(player_name, input_buffer); // 文字化け対策
                                            input_buffer[0] = '\0';
                                        }
                                        input_buffer[0] = '\0';

                                        if (backspace != 0){
                                            int len = strlen(player_name);
                                            if (len > 0) {
                                                player_name[len - 1] = '\0';
                                            }
                                            backspace = 0;
                                        }
                                        
                                        SDL_RenderCopy(w_renderer, image_pic, &src, &dst); // レンダラーにコピー

                                        //名前の縦線の位置用
                                    // verline = strlen(player_name);


                                        // 名前のための四角
                                        // boxcolor(w_renderer, 30, 80, 300, 300, 0x69696969);
                                        // boxcolor(w_renderer, 50, 120, 280, 160, 0xC0C0C0bb);

                                        // 名前のやつ
                                        SDL_Surface* m_s  = TTF_RenderUTF8_Blended(w_font, "プレイヤー名", name_white);
                                        SDL_Texture* m_t  = SDL_CreateTextureFromSurface(w_renderer, m_s);
                                        SDL_Rect src_rect = { 0, 0, m_s->w, m_s->h };        // コピー元文字列の領域（x, y, w, h）
                                        SDL_Rect dst_rect = { 50, 90, m_s->w, m_s->h };     // 文字列のコピー先の座標と領域（x, y, w, h）
                                        SDL_RenderCopy(w_renderer, m_t, &src_rect, &dst_rect); // レンダラーにコピー
                                        SDL_FreeSurface(m_s); //サーフェイスの解放
                                        SDL_DestroyTexture(m_t);


                                        if (strlen(player_name) > 0) {
                                            SDL_Surface* player_name_s = TTF_RenderUTF8_Blended(w_font, player_name, name_white);
                                            SDL_Texture* player_name_t = SDL_CreateTextureFromSurface(w_renderer, player_name_s);
                                            SDL_Rect input_src_rect    = { 0, 0, player_name_s->w, player_name_s->h };    // コピー元文字列の領域（x, y, w, h）
                                            SDL_Rect input_dst_rect    = { 50, 120, player_name_s->w, player_name_s->h }; // 文字列のコピー先の座標と領域（x, y, w, h）
                                            SDL_RenderCopy(w_renderer, player_name_t, &input_src_rect, &input_dst_rect);    // レンダラーにコピー
                                            SDL_FreeSurface(player_name_s); //サーフェイスの解放
                                            SDL_DestroyTexture(player_name_t);
                                        }

                                        SDL_RenderPresent(w_renderer);
                                    }

                                    SDL_StopTextInput(); // テキスト入力の終了
                                    for (int i = 0; i < strlen(player_name); i++)
                                    {
                                        // gGame.player_name[i] = player_name[i];
                                        gGame.usrName[i] = player_name[i];
                                    }
                                    TTF_CloseFont(w_font);
                                    TTF_Quit();
                                    SDL_FreeSurface(w_strings);
                                    SDL_DestroyTexture(w_texture);
                                    SDL_DestroyRenderer(w_renderer);
                                    SDL_DestroyWindow(w_window);
                                    SDL_Quit();
                                    return 0;

                                }

                                

                                while (step == 1)
                                {
                                    SDL_bool quit = SDL_FALSE;
                                    // int select_level = 0;
                                    while (!quit) {
                                        SDL_Event w_event;
                                        if (SDL_PollEvent(&w_event)) {
                                            switch (w_event.type) {
                                            case SDL_QUIT:
                                                quit = SDL_TRUE;
                                                step = 100;
                                                // gGame.end_stts = 0;
                                                break;
                                            case SDL_KEYDOWN:
                                                /* キーリピートは無視 */
                                                if (w_event.key.repeat)
                                                    break;
                                                switch (w_event.key.keysym.sym) {
                                                case SDLK_ESCAPE:
                                                    /** 終了指示 **/
                                                    quit = SDL_TRUE;
                                                    step = 100;
                                                    // gGame.end_stts = 0;
                                                    break;
                                                case SDLK_RETURN:
                                                    /* 決定 */
                                                    quit = SDL_TRUE;
                                                    // gGame.level = select_level;
                                                    step = 3;
                                                    break;
                                                    break;
                                                }

                                            case SDL_TEXTINPUT:
                                                break;
                                            default:
                                                break;
                                            }
                                        }

                                        //背景
                                        SDL_RenderCopy(w_renderer, image_pic, &src, &dst); // レンダラーにコピー
                                        // 名前のための四角
                                        // boxcolor(w_renderer, 30, 80, 550, 350, 0x69696969);
                                        // boxcolor(w_renderer, 50, 120, 280, 160, 0xC0C0C0bb);
                                        
                                        //説明
                                        SDL_Surface* m_s  = TTF_RenderUTF8_Blended(w_font, "ゲーム説明", name_white);
                                        SDL_Texture* m_t  = SDL_CreateTextureFromSurface(w_renderer, m_s);
                                        SDL_Rect src_rect0 = { 0, 0, m_s->w, m_s->h };        // コピー元文字列の領域（x, y, w, h）
                                        SDL_Rect dst_rect0 = { 50, 100, m_s->w, m_s->h };     // 文字列のコピー先の座標と領域（x, y, w, h）
                                        SDL_RenderCopy(w_renderer, m_t, &src_rect0, &dst_rect0); // レンダラーにコピー
                                        SDL_FreeSurface(m_s); //サーフェイスの解放
                                        SDL_DestroyTexture(m_t);

                                        SDL_Surface* m_s_1  = TTF_RenderUTF8_Blended(w_font, "Joy-Conを操作しながら、", name_white);
                                        SDL_Texture* m_t_1  = SDL_CreateTextureFromSurface(w_renderer, m_s_1);
                                        SDL_Rect src_rect_1 = { 0, 0, m_s_1->w, m_s_1->h };        // コピー元文字列の領域（x, y, w, h）
                                        SDL_Rect dst_rect_1 = { 50, 150, m_s_1->w, m_s_1->h };     // 文字列のコピー先の座標と領域（x, y, w, h）
                                        SDL_RenderCopy(w_renderer, m_t_1, &src_rect_1, &dst_rect_1); // レンダラーにコピー
                                        SDL_FreeSurface(m_s_1); //サーフェイスの解放
                                        SDL_DestroyTexture(m_t_1);

                                        SDL_Surface* m_s_2  = TTF_RenderUTF8_Blended(w_font, "迷路の中を飛び回り", name_white);
                                        SDL_Texture* m_t_2  = SDL_CreateTextureFromSurface(w_renderer, m_s_2);
                                        SDL_Rect src_rect_2 = { 0, 0, m_s_2->w, m_s_2->h };        // コピー元文字列の領域（x, y, w, h）
                                        SDL_Rect dst_rect_2 = { 50, 200, m_s_2->w, m_s_2->h };     // 文字列のコピー先の座標と領域（x, y, w, h）
                                        SDL_RenderCopy(w_renderer, m_t_2, &src_rect_2, &dst_rect_2); // レンダラーにコピー
                                        SDL_FreeSurface(m_s_2); //サーフェイスの解放
                                        SDL_DestroyTexture(m_t_2);

                                        SDL_Surface* m_s_3  = TTF_RenderUTF8_Blended(w_font, "敵を倒しながら進んでください！", name_white);
                                        SDL_Texture* m_t_3  = SDL_CreateTextureFromSurface(w_renderer, m_s_3);
                                        SDL_Rect src_rect_3 = { 0, 0, m_s_3->w, m_s_3->h };        // コピー元文字列の領域（x, y, w, h）
                                        SDL_Rect dst_rect_3 = { 50, 250, m_s_3->w, m_s_3->h };     // 文字列のコピー先の座標と領域（x, y, w, h）
                                        SDL_RenderCopy(w_renderer, m_t_3, &src_rect_3, &dst_rect_3); // レンダラーにコピー
                                        SDL_FreeSurface(m_s_3); //サーフェイスの解放
                                        SDL_DestroyTexture(m_t_3);

                                        SDL_Surface* m_s_4  = TTF_RenderUTF8_Blended(w_font, "Enterキーを押して次へ", name_white);
                                        SDL_Texture* m_t_4  = SDL_CreateTextureFromSurface(w_renderer, m_s_4);
                                        SDL_Rect src_rect_4 = { 0, 0, m_s_4->w, m_s_4->h };        // コピー元文字列の領域（x, y, w, h）
                                        SDL_Rect dst_rect_4 = { 500, 450, m_s_4->w, m_s_4->h };     // 文字列のコピー先の座標と領域（x, y, w, h）
                                        SDL_RenderCopy(w_renderer, m_t_4, &src_rect_4, &dst_rect_4); // レンダラーにコピー
                                        SDL_FreeSurface(m_s_4); //サーフェイスの解放
                                        SDL_DestroyTexture(m_t_4);

                                        


                                        SDL_RenderPresent(w_renderer);

                                    }
                                }


                            break;
                        case 1:
                            // Mix_PlayMusic(music,1);

                            SDL_RenderClear(w_renderer);
                            SDL_RenderCopy(w_renderer, w_pngtex, NULL, NULL);
                                SDL_RenderCopy(w_renderer, image_pic, &src, &dst); // レンダラーにコピー
                                
                            //背景
                                // 名前のための四角
                                // boxcolor(w_renderer, 30, 80, 650, 450, 0x69696969);
                                // boxcolor(w_renderer, 50, 120, 280, 160, 0xC0C0C0bb);
                                
                                //説明
                                SDL_Surface* m_s  = TTF_RenderUTF8_Blended(w_font, "操作方法", name_white);
                                SDL_Texture* m_t  = SDL_CreateTextureFromSurface(w_renderer, m_s);
                                SDL_Rect src_rect0 = { 0, 0, m_s->w, m_s->h };        // コピー元文字列の領域（x, y, w, h）
                                SDL_Rect dst_rect0 = { 50, 100, m_s->w, m_s->h };     // 文字列のコピー先の座標と領域（x, y, w, h）
                                SDL_RenderCopy(w_renderer, m_t, &src_rect0, &dst_rect0); // レンダラーにコピー
                                SDL_FreeSurface(m_s); //サーフェイスの解放
                                SDL_DestroyTexture(m_t);

                                SDL_RenderCopy(w_renderer, howto_pic, &howto_src, &howto_dst); // レンダラーにコピー



                                SDL_RenderPresent(w_renderer);
                            break;
                        case 2:
                            TTF_CloseFont(w_font);
                            TTF_Quit();
                            SDL_FreeSurface(w_strings);
                            SDL_DestroyTexture(w_texture);
                            SDL_DestroyRenderer(w_renderer);
                            SDL_DestroyWindow(w_window);
                            SDL_Quit();
                            return 0;
                            break;

                        case 3:
                            SDL_RenderClear(w_renderer);
                            SDL_RenderCopy(w_renderer, w_pngtex, NULL, NULL);
                            choice(3);
                            if (button_num == 3){
                                //背景
                                SDL_RenderCopy(w_renderer, image_pic, &src, &dst); // レンダラーにコピー
                                // 名前のための四角
                                // boxcolor(w_renderer, 30, 80, 650, 450, 0x69696969);
                                
                                //説明
                                SDL_Surface* m_s  = TTF_RenderUTF8_Blended(w_font, "設定", name_white);
                                SDL_Texture* m_t  = SDL_CreateTextureFromSurface(w_renderer, m_s);
                                SDL_Rect src_rect0 = { 0, 0, m_s->w, m_s->h };        // コピー元文字列の領域（x, y, w, h）
                                SDL_Rect dst_rect0 = { 50, 100, m_s->w, m_s->h };     // 文字列のコピー先の座標と領域（x, y, w, h）
                                SDL_RenderCopy(w_renderer, m_t, &src_rect0, &dst_rect0); // レンダラーにコピー
                                SDL_FreeSurface(m_s); //サーフェイスの解放
                                SDL_DestroyTexture(m_t);

                                SDL_RenderPresent(w_renderer);
                            }
                                button_num = 3;
                            break;
                    }
                    break;
                case SDLK_DOWN:
                    button_num += 1;
                    if (button_num >= 4)    button_num = 0;
                    SDL_RenderClear(w_renderer);
                    SDL_RenderCopy(w_renderer, w_pngtex, NULL, NULL);
                    choice(button_num);
                    SDL_RenderPresent(w_renderer);  
                    break;
                case SDLK_UP:
                    button_num -= 1;
                    if (button_num < 0)    button_num = 3;
                    SDL_RenderClear(w_renderer);
                    SDL_RenderCopy(w_renderer, w_pngtex, NULL, NULL);
                    choice(button_num);
                    SDL_RenderPresent(w_renderer);  
                    break;
            
                case SDLK_ESCAPE:                
                    TTF_CloseFont(w_font);
                    TTF_Quit();
                    SDL_FreeSurface(w_strings);
                    SDL_DestroyTexture(w_texture);
                    SDL_DestroyRenderer(w_renderer);
                    SDL_DestroyWindow(w_window);
                    SDL_Quit();
                    return 0;
                }
            }
        }
    }
}

