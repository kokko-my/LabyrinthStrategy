/**
 * \file    server_main.c
 *
 * サーバー側メインループ
*/

#include "server.h"

/**
 * グローバル変数
*/
ServerInfo gServer;
sPlayer_info sPlayer_info_all[player_num_init];
sitem_info   sitem_info_all[player_num_init * item_num_init + 2];
SDL_Thread  *threads[player_num_init];

/**
 * 関数
*/
static int FuncForThread(void*);

/* メイン関数 */
int main(int argc, char **argv) {
    u_short port = DEFAULT_PORT;            //    ポート番号（デフォルト設定）

    /* 引数によるクライアント数・ポート番号設定 */
    switch ( argc ) {
    case 2:     port = atoi(argv[1]);       //      ポート番号（第2引数）
    case 1:     break;
    default:
        fprintf(stderr, "Usage: %s [port number]\n", argv[0]);
        return 1;
    }

    /* クライアント数とポート番号を表示 */
    fprintf(stderr, "Number of player : %d\n", player_num_init);
    fprintf(stderr, "     Port number : %d\n", port);

    /* サーバを立ち上げる */
    SetupServer(port);

    /* マップ関係の送信 */
    Init_func();
    BroadcastMapData();
    sStart_game();

    /*************************************************************************************/

    /* スレッドを立てる */
    int num[player_num_init];
    for ( int i = 0; i < player_num_init; i++ ) {
        num[i] = i;
        threads[i] = SDL_CreateThread(FuncForThread, "FuncForThread", num + i);
    }

    /* 全てのスレッドが終了するまで待機 */
    for ( int i = 0; i < player_num_init; i++ ) {
        SDL_WaitThread(threads[i], NULL);
    }

    /* サーバーを閉じる */
    TerminateServer();

    return 0;
}


/** 各プレイヤーの送受信管理スレッド */
static int FuncForThread(void *data) {
    int id = *(int*)data;
#ifdef DEBUG
    fprintf(stderr, "Created thread ( %d ).\n", id);
#endif

    /** メインループ **/
    while ( gServer.game_stts != GS_Clear ) {
        /* 送受信管理 */
        ControlRequests(id);
    }

    return 0;
}

/* end of server_main.c */


/*
    ライセンス情報

    このソフトウェアでは以下のソフトウェアを使用しています。

    --- Zlib License ---
    Simple DirectMedia Layer (SDL)

    Copyright (C) 1997-2023 Sam Lantinga <slouken@libsdl.org>

    This software is provided 'as-is', without any express or implied
    warranty.  In no event will the authors be held liable for any damages
    arising from the use of this software.

    Permission is granted to anyone to use this software for any purpose,
    including commercial applications, and to alter it and redistribute it
    freely, subject to the following restrictions:

    1. The origin of this software must not be misrepresented; you must not
       claim that you wrote the original software. If you use this software
       in a product, an acknowledgment in the product documentation would be
       appreciated but is not required. 
    2. Altered source versions must be plainly marked as such, and must not be
       misrepresented as being the original software.
    3. This notice may not be removed or altered from any source distribution.

    --- MIT License ---
    joyconlib

    MIT License

    Copyright (c) 2022 K. Morita

    Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation  files (the "Software"), to deal in the Software without restriction, including without limitation the rights to use, copy,   modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software    is furnished to do so, subject to the following conditions:

    The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.

    THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES     OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE     LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR  IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

    --- Special Thanks ---
    ChatGPT , https://chat.openai.com/
    Bard , https://bard.google.com/
    DALL-E3, https://openai.com/dall-e-3

*/