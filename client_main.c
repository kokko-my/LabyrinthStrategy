/*
    client_main.c
    クライアントのメインファイル
*/

//コンパイルオプション：-lSDL -lGL -L/usr/lib

#include <stdio.h>
#include <SDL2/SDL.h>        // SDLを用いるために必要なヘッダファイルをインクルード
#include <SDL2/SDL_opengl.h> // SDLでOpenGLを扱うために必要なヘッダファイルをインクルード
#include <GL/glu.h>
#include <time.h>
#include <unistd.h>

#include "common.h"
#include "client.h"

/**
 * グローバル変数
*/
GameInfo gGame;
player_info player_info_all[player_num_init];
item_info item_info_all[player_num_init * item_num_init + 2];

int main (int argc, char **argv){
    Welcome();

    #ifdef DEBUG
    fprintf(stderr, "client start!\n");
    #endif

    #ifndef NON_NETWORK
    u_short port = DEFAULT_PORT;                    //    ポート番号（デフォルト設定）
    char serverName[MAX_L_NAME] = "localhost";      //    サーバ名（デフォルト設定）

    /* 引数によるポート番号・サーバー名設定 */
    gGame.key_switch = 0;
    switch (argc) {
    case 4:     port = (u_short)atoi(argv[3]);          //      ポート番号（第3引数）
    case 3:     gGame.key_switch = 1;                   //　　　キーボード操作どうするの？
    case 2:     sprintf(serverName, "%s", argv[2]);     //      サーバー名（第2引数）
    case 1:     break;
    default:
        fprintf(stderr, "Usage: %s [server name] [port number]\n", argv[0]);
        return 1;
    }

    

    /* クライアントを立ち上げる */
    SetupClient(serverName, port);
    #endif

    //初期化
    Init_func();

    //ゲーム開始
    if (Start_game() != 0){
        fprintf(stderr, "failed to start game.\n");
        return 0;
    }

    //メインで画面描写を行う
    Render_window();

    //終了処理
    if (gGame.game_stts == GS_End){
        #ifdef DEBUG
        fprintf(stderr, "END_system\n");
        #endif

        End_system();
        return 0;
    }

    //  終了画面描画
    GameEnding();

    return 0;
}

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