/*
    client_win.c
    クライアントの描写処理のファイル
*/

#include <stdio.h>
#include <SDL2/SDL.h>        // SDLを用いるために必要なヘッダファイルをインクルード
#include <SDL2/SDL_opengl.h> // SDLでOpenGLを扱うために必要なヘッダファイルをインクルード
#include <SDL2/SDL_mixer.h>
#include <math.h>
#include <GL/gl.h>
#include <GL/glu.h>
#include <GL/glut.h>

#include "common.h"
#include "client.h"


#ifdef DEBUG
static void Drawgrand(void);
static void DrawTriangle(double x, double y, double z);
#endif

//機体
static void DrawShip(xyz base_point, xyz vec);

//アイテム
static void DrawItems(void);
static void DrawAmmo(xyz base_point, xyz vec);
static void DrawMissoil(xyz base_point, xyz vec);
static void DrawLandmine(xyz base_point, xyz vec);
static void DrawKey(xyz base_point, xyz vec, int type);

static void DrawWorld(void);
static int reflesh_window_judge(void);  //画面更新の必要性の判断
static void Camera_set(void);  //カメラの設定

GLuint texture = 0;




/*************************************
Init_window : ウィンドウ周りのの初期化関数
返り値は、エラーで-1.正常終了で0
**************************************/
int Init_window(void){

    // SDL初期化
	if(SDL_Init(SDL_INIT_EVERYTHING) < 0) {
		fprintf(stderr, "failed to initialize SDL.\n");
		return -1;
	}
    if (TTF_Init() < 0) {
        fprintf(stderr, "font ERROR!");
    }
    Mix_Init(MIX_INIT_MP3);

    if(Mix_OpenAudio(MIX_DEFAULT_FREQUENCY, MIX_DEFAULT_FORMAT, 2, 1024) < 0) {
		fprintf(stderr, "failed to initialize SDL_mixer. %s\n", Mix_GetError() );
    }



    // OpenGLの設定
	SDL_GL_SetAttribute(SDL_GL_RED_SIZE, 5);	// 赤色の確保するサイズ
	SDL_GL_SetAttribute(SDL_GL_GREEN_SIZE, 5);	// 緑色の確保するサイズ
	SDL_GL_SetAttribute(SDL_GL_BLUE_SIZE, 5);	// 青色の確保するサイズ
	SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 16);	// 深度の確保するサイズ
	SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);	// ダブルバッファを使用する


    //音楽関係
    if ((gGame.shot = Mix_LoadMUS("./media/shot.wav")) == NULL){
        fprintf(stderr, "failed to load music! %s \n", Mix_GetError());
    }
    if ((gGame.shot_hit = Mix_LoadMUS("./media/shot_hit.wav")) == NULL){
        fprintf(stderr, "failed to load music! %s \n", Mix_GetError());
    }
    if ((gGame.missoil = Mix_LoadMUS("./media/missoil.wav")) == NULL){
        fprintf(stderr, "failed to load music! %s \n", Mix_GetError());
    }
    if ((gGame.missoil_hit = Mix_LoadMUS("./media/missoil_hit.wav")) == NULL){
        fprintf(stderr, "failed to load music! %s \n", Mix_GetError());
    }
    if ((gGame.explosion = Mix_LoadMUS("./media/explosion.wav")) == NULL){
        fprintf(stderr, "failed to load music! %s \n", Mix_GetError());
    }



    #ifdef DEBUG
        fprintf(stderr, "Init_window success!\n");
    #endif
    return 0;

}

int Start_game(void){

    /* マップ関係 */
    #ifndef NON_NETWORK
        ReceiveMapData();
    #endif
    #ifdef DEBUG
        printMap(MAP_WIDTH, MAP_HEIGHT);
    #endif

    // ウィンドウ生成（OpenGLを用いるウィンドウを生成）
    if((gGame.window = SDL_CreateWindow("window", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, 0, 0, SDL_WINDOW_OPENGL|SDL_WINDOW_FULLSCREEN_DESKTOP)) == NULL) {
		fprintf(stderr, "failed to initialize window.\n");
		return -1;
	}

    SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 16);
    //コンテキストを作成
    gGame.context = SDL_GL_CreateContext(gGame.window);
    if (!gGame.context) {
        fprintf(stderr, "failed to create OpenGL context.\n");
        return -1;
    }

    glViewport(0, 0, gGame.window_wide, gGame.window_high);
    glEnable(GL_DEPTH_TEST); //深度テストを有効化
    
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    gluPerspective(gGame.window_camera_angle, (double)gGame.window_wide /  (double)gGame.window_high, 0.1, gGame.max_far);
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();





    //スレッド関連
    gGame.Mutex = SDL_CreateMutex();  //排他処理

    //スレッド作成
    gGame.func_thread   = SDL_CreateThread(Key_func, "key_thread", gGame.Mutex);       //キーボードスレッド
    gGame.move_thread   = SDL_CreateThread(Move, "move_thread", gGame.Mutex);
    
    #ifndef NON_JOYCON
    gGame.joycon_thread = SDL_CreateThread(Joy_func, "joycon_thread", gGame.Mutex);
    #endif

    #ifndef NON_NETWORK
    gGame.network_thread   = SDL_CreateThread(Network, "network_thread", gGame.Mutex);
    #endif

    #ifndef NON_JOYCON
    joycon_err err = joycon_open(&gGame.joy_jc, JOYCON_R);
    if (JOYCON_ERR_NONE == err){
        gGame.joycon_num = 1;
    }
    else{
        fprintf(stderr, "Joy-Con open failed : %d \n", err);
        gGame.joycon_num = 0;
    }
    #endif


    //ゲームスタート
    SDL_LockMutex(gGame.Mutex);
    gGame.game_stts = GS_Playing; //ステータス変更
    gGame.window_reflesh = 1;     //レンダラー開始
    SDL_UnlockMutex(gGame.Mutex);

    #ifdef DEBUG
        fprintf(stderr, "Game start!\n");
    #endif

    return 0;
}

/*************************************
Render_window : 画面描写
赤（Red）: glColor3d(1.0, 0.0, 0.0);
緑（Green）: glColor3d(0.0, 1.0, 0.0);
青（Blue）: glColor3d(0.0, 0.0, 1.0);
黒（Black）: glColor3d(0.0, 0.0, 0.0);
白（White）: glColor3d(1.0, 1.0, 1.0);
黄色（Yellow）: glColor3d(1.0, 1.0, 0.0);
オレンジ（Orange）: glColor3d(1.0, 0.5, 0.0);
ピンク（Pink）: glColor3d(1.0, 0.5, 0.5);
シアン（Cyan）: glColor3d(0.0, 1.0, 1.0);
紫（Purple）: glColor3d(0.5, 0.0, 0.5);　　　　
**************************************/
int Render_window(void){
    while (gGame.game_stts == GS_Playing)
    {

        if (reflesh_window_judge() == 1) //描写が必要なとき
        {
            SDL_LockMutex(gGame.Mutex);

            glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

            Camera_set();

            DrawWorld();

            Manege_HP();

            for (int i = 0; i < gGame.player_num; i++){
                if (player_info_all[i].see == SDL_TRUE){ //描写するかどうか
                    if ((i < gGame.player_num/2) ^ (gGame.player_ID < gGame.player_num/2)){  //敵
                        glColor3d(1.0, 0.33, 0.61); //四国めたんカラー
                    } else { //味方
                        glColor3d(0.62, 0.898, 0.20); //ずんだもんカラー
                    }
                    DrawShip(player_info_all[i].point, player_info_all[i].vector);
                }
            }


            DrawItems();
            DrawInfo(); //文字とかの描写

            SDL_GL_SwapWindow(gGame.window); // 描画を反映する

            SDL_UnlockMutex(gGame.Mutex);


        }
        SDL_Delay(5);
    }
    return 0;
}



/*
    各種関数達
*/
#ifdef DEBUG
//ワールドテスト用の格子
void Drawgrand(void){
    glBegin(GL_QUADS); // 多角形の開始
	    glColor3d(1.0, 1.0, 1.0); // 頂点を白色に設定
	    glVertex3d(99.9, 0.0, 99.9);     // 頂点を作成

	    glVertex3d(99.9, 0.0, -99.9);    // 頂点を作成

	    glVertex3d(-99.9, 0.0, -99.9);   // 頂点を作成

		glVertex3d(-99.9, 0.0, 99.9);   // 頂点を作成
	glEnd(); // 図形終了


    glBegin(GL_LINES);
        double temp = -100.0;
        for (int i = 0; i < 1000; i++){
            glColor3d(0.0, 0.0, 0.0);
            glVertex3d(temp + i * 0.1, 0.01, 100.0);
            glVertex3d(temp + i * 0.1, 0.01, -100.0);
            glVertex3d(100.0, 0.01, temp + i * 0.1);
            glVertex3d(-100.0, 0.01, temp + i * 0.1);
            glVertex3d(-(temp + i * 0.1), 0.01, 100.0);
            glVertex3d(-(temp + i * 0.1), 0.01, -100.0);
            glVertex3d(100.0, 0.01, -(temp + i * 0.1));
            glVertex3d(-100.0, 0.01, -(temp + i * 0.1));
        }
        glColor3d(1.0, 0.0, 0.0);
        glVertex3d(0.0, 0.01, 100.0);
        glVertex3d(0.0, 0.01, -100.0);
        glVertex3d(100.0, 0.01, 0.0);
        glVertex3d(-100.0, 0.01, 0.0);
    glEnd();
}
#endif

#ifdef DEBUG
//テスト用
void DrawTriangle(double x, double y, double z){
	glBegin(GL_TRIANGLES);	// 三角形の開始
		glVertex3d(x + 0.1 , y + 0.1, z + 0.2);
		glVertex3d(x -0.1 , y + 0.1, z + 0.2);
		glVertex3d(x , y + 0.1, z - 0.2);
	glEnd();
}
#endif

//壁の描写
void DrawQuad(xyz p1, xyz p2, xyz p3, xyz p4){
    glBegin(GL_QUADS);
        glVertex3d(p1.x, p1.y, p1.z);
        glVertex3d(p2.x, p2.y, p2.z);
        glVertex3d(p3.x, p3.y, p3.z);
        glVertex3d(p4.x, p4.y, p4.z);
    glEnd();

    //外周の線
    #ifdef DEBUG
    glColor3d(0.0, 0.0, 0.0); //黒
    glBegin(GL_LINE_LOOP);
        glVertex3d(p1.x, p1.y, p1.z);
        glVertex3d(p2.x, p2.y, p2.z);
        glVertex3d(p2.x, p2.y, p2.z);
        glVertex3d(p3.x, p3.y, p3.z);
        glVertex3d(p3.x, p3.y, p3.z);
        glVertex3d(p4.x, p4.y, p4.z);
        glVertex3d(p4.x, p4.y, p4.z);
        glVertex3d(p1.x, p1.y, p1.z);
    glEnd();
    #endif
}
/*
    ワールドを描写
*/
void DrawWorld(void){
    #define PATH 0
    #define WALL 1


    //床
    // glColor3d(0.99, 0.9, 0.7);
    // glColor3d(1.0, 1.0, 1.0);
    glColor3d(0.92, 0.97, 0.80);
    DrawQuad((xyz){0.0, 0.0, 0.0}, (xyz){gGame.map_tile_size * MAP_WIDTH, 0.0, 0.0},
        (xyz){gGame.map_tile_size * MAP_WIDTH, 0.0, -gGame.map_tile_size * MAP_HEIGHT}, (xyz){0.0, 0.0, -gGame.map_tile_size * MAP_HEIGHT});
    //天井
    glColor3d(1.0, 1.0, 1.0);
    // glColor3d(1.0, 1.0, 1.0);
    DrawQuad((xyz){0.0, gGame.map_tile_size, 0.0}, (xyz){gGame.map_tile_size * MAP_WIDTH, gGame.map_tile_size, 0.0},
        (xyz){gGame.map_tile_size * MAP_WIDTH, gGame.map_tile_size, -gGame.map_tile_size * MAP_HEIGHT}, (xyz){0.0, gGame.map_tile_size, -gGame.map_tile_size * MAP_HEIGHT});

    for (int i = 0; i < MAP_WIDTH; i++){
        for (int j = 0; j < MAP_HEIGHT; j++){
            if (gGame.map[j][i] == WALL){
                // glColor3d(0.70, 0.48, 0.37); //ベージュ
                // glColor3d(1.0, 1.0, 1.0);
                // glColor3d(0.7, 0.7, 0.7); //グレー
                glColor3d(0.39, 0.35, 0.35);
                DrawQuad((xyz){gGame.map_tile_size*i, 0.0, -gGame.map_tile_size*j}, (xyz){gGame.map_tile_size*(i+1), 0.0, -gGame.map_tile_size*j},
                    (xyz){gGame.map_tile_size*(i+1), gGame.map_tile_size, -gGame.map_tile_size*j}, (xyz){gGame.map_tile_size*i, gGame.map_tile_size, -gGame.map_tile_size*j});
                // glColor3d(0.7, 0.7, 0.7); //グレー
                // glColor3d(0.70, 0.48, 0.37); //ベージュ
                // glColor3d(1.0, 1.0, 1.0);
                glColor3d(0.29, 0.25, 0.25);
                DrawQuad((xyz){gGame.map_tile_size*(i+1), 0.0, -gGame.map_tile_size*j}, (xyz){gGame.map_tile_size*(i+1), 0.0, -gGame.map_tile_size*(j+1)},
                    (xyz){gGame.map_tile_size*(i+1), gGame.map_tile_size, -gGame.map_tile_size*(j+1)}, (xyz){gGame.map_tile_size*(i+1), gGame.map_tile_size, -gGame.map_tile_size*j});
                // glColor3d(0.7, 0.7, 0.7); //グレー
                // glColor3d(0.70, 0.48, 0.37); //ベージュ
                // glColor3d(1.0, 1.0, 1.0);
                glColor3d(0.19, 0.15, 0.15);
                DrawQuad((xyz){gGame.map_tile_size*(i+1), 0.0, -gGame.map_tile_size*(j+1)}, (xyz){gGame.map_tile_size*i, 0.0, -gGame.map_tile_size*(j+1)},
                    (xyz){gGame.map_tile_size*i, gGame.map_tile_size, -gGame.map_tile_size*(j+1)}, (xyz){gGame.map_tile_size*(i+1), gGame.map_tile_size, -gGame.map_tile_size*(j+1)});
                // glColor3d(0.7, 0.7, 0.7); //グレー
                // glColor3d(0.70, 0.48, 0.37); //ベージュ
                // glColor3d(1.0, 1.0, 1.0);
                glColor3d(0.29, 0.25, 0.25);
                DrawQuad((xyz){gGame.map_tile_size*i, 0.0, -gGame.map_tile_size*(j+1)}, (xyz){gGame.map_tile_size*i, 0.0, -gGame.map_tile_size*j},
                    (xyz){gGame.map_tile_size*i, gGame.map_tile_size, -gGame.map_tile_size*j}, (xyz){gGame.map_tile_size*i, gGame.map_tile_size, -gGame.map_tile_size*(j+1)});
            }
        }
    }
    glBegin(GL_TRIANGLES);
        if (gGame.player_ID < player_num_init/2){
            glColor3d(0.62, 0.898, 0.20); //ずんだもんカラー
        } else {
            glColor3d(1.0, 0.33, 0.61); //四国めたんカラー
        }
        
        glVertex3d(gGame.map_tile_size,   0.01, -gGame.map_tile_size);
        glVertex3d(gGame.map_tile_size,   0.01, -gGame.map_tile_size*2);
        glVertex3d(gGame.map_tile_size*2, 0.01, -gGame.map_tile_size);

        if (gGame.player_ID < player_num_init/2){
            glColor3d(1.0, 0.33, 0.61); //四国めたんカラー
        } else {
            glColor3d(0.62, 0.898, 0.20); //ずんだもんカラー
        }
        glVertex3d(gGame.map_tile_size * (MAP_WIDTH-1), 0.01, -gGame.map_tile_size * (MAP_HEIGHT-1));
        glVertex3d(gGame.map_tile_size * (MAP_WIDTH-2), 0.01, -gGame.map_tile_size * (MAP_HEIGHT-1));
        glVertex3d(gGame.map_tile_size * (MAP_WIDTH-1), 0.01, -gGame.map_tile_size * (MAP_HEIGHT-2));
    glEnd();
}

/*
    機体を描写
*/
void DrawShip(xyz base_point, xyz vec){
    #ifdef DEBUG
    // fprintf(stderr, "Ship_point = (%lf, %lf, %lf) vec = (%lf, %lf, %lf) vec_def = (%f,%f,%f) flont_vec = (%f, %f, %f) float_def = (%f, %f, %f) HP = %d\n",
    //     base_point.x,base_point.y,base_point.z, vec.x,vec.y,vec.z,
    //     player_info_all[gGame.player_ID].vector_def.x, player_info_all[gGame.player_ID].vector_def.y, player_info_all[gGame.player_ID].vector_def.z,
    //     player_info_all[gGame.player_ID].flont_vec.x,player_info_all[gGame.player_ID].flont_vec.y,player_info_all[gGame.player_ID].flont_vec.z,
    //     player_info_all[gGame.player_ID].flont_vec_def.x, player_info_all[gGame.player_ID].flont_vec_def.y, player_info_all[gGame.player_ID].flont_vec_def.z,
    //     player_info_all[gGame.player_ID].HP);
    fprintf(stderr, "Ship_point = (%lf, %lf, %lf)\n", base_point.x,base_point.y,base_point.z);
    #endif


   /* 本番の機体 */
   double scale = 0.03;
   glBegin(GL_QUADS);
        glVertex3d(point_func(base_point, -7.5*scale, 5.5*scale, -4.5*scale, vec).x, point_func(base_point, -7.5*scale, 5.5*scale, -4.5*scale, vec).y, point_func(base_point, -7.5*scale, 5.5*scale, -4.5*scale, vec).z);
        glVertex3d(point_func(base_point, -10.5*scale, 2.5*scale, -4.5*scale, vec).x, point_func(base_point, -10.5*scale, 2.5*scale, -4.5*scale, vec).y, point_func(base_point, -10.5*scale, 2.5*scale, -4.5*scale, vec).z);
        glVertex3d(point_func(base_point, -10.5*scale, 2.5*scale, 4.5*scale, vec).x, point_func(base_point, -10.5*scale, 2.5*scale, 4.5*scale, vec).y, point_func(base_point, -10.5*scale, 2.5*scale, 4.5*scale, vec).z);
        glVertex3d(point_func(base_point, -7.5*scale, 5.5*scale, 4.5*scale, vec).x, point_func(base_point, -7.5*scale, 5.5*scale, 4.5*scale, vec).y, point_func(base_point, -7.5*scale, 5.5*scale, 4.5*scale, vec).z);
        glVertex3d(point_func(base_point, -10.5*scale, 2.5*scale, -4.5*scale, vec).x, point_func(base_point, -10.5*scale, 2.5*scale, -4.5*scale, vec).y, point_func(base_point, -10.5*scale, 2.5*scale, -4.5*scale, vec).z);
        glVertex3d(point_func(base_point, -10.5*scale, -2.5*scale, -4.5*scale, vec).x, point_func(base_point, -10.5*scale, -2.5*scale, -4.5*scale, vec).y, point_func(base_point, -10.5*scale, -2.5*scale, -4.5*scale, vec).z);
        glVertex3d(point_func(base_point, -10.5*scale, -2.5*scale, 4.5*scale, vec).x, point_func(base_point, -10.5*scale, -2.5*scale, 4.5*scale, vec).y, point_func(base_point, -10.5*scale, -2.5*scale, 4.5*scale, vec).z);
        glVertex3d(point_func(base_point, -10.5*scale, 2.5*scale, 4.5*scale, vec).x, point_func(base_point, -10.5*scale, 2.5*scale, 4.5*scale, vec).y, point_func(base_point, -10.5*scale, 2.5*scale, 4.5*scale, vec).z);
        glVertex3d(point_func(base_point, -10.5*scale, -2.5*scale, -4.5*scale, vec).x, point_func(base_point, -10.5*scale, -2.5*scale, -4.5*scale, vec).y, point_func(base_point, -10.5*scale, -2.5*scale, -4.5*scale, vec).z);
        glVertex3d(point_func(base_point, -7.5*scale, -5.5*scale, -4.5*scale, vec).x, point_func(base_point, -7.5*scale, -5.5*scale, -4.5*scale, vec).y, point_func(base_point, -7.5*scale, -5.5*scale, -4.5*scale, vec).z);
        glVertex3d(point_func(base_point, -7.5*scale, -5.5*scale, 4.5*scale, vec).x, point_func(base_point, -7.5*scale, -5.5*scale, 4.5*scale, vec).y, point_func(base_point, -7.5*scale, -5.5*scale, 4.5*scale, vec).z);
        glVertex3d(point_func(base_point, -10.5*scale, -2.5*scale, 4.5*scale, vec).x, point_func(base_point, -10.5*scale, -2.5*scale, 4.5*scale, vec).y, point_func(base_point, -10.5*scale, -2.5*scale, 4.5*scale, vec).z);
        glVertex3d(point_func(base_point, 7.5*scale, 5.5*scale, -4.5*scale, vec).x, point_func(base_point, 7.5*scale, 5.5*scale, -4.5*scale, vec).y, point_func(base_point, 7.5*scale, 5.5*scale, -4.5*scale, vec).z);
        glVertex3d(point_func(base_point, 10.5*scale, 2.5*scale, -4.5*scale, vec).x, point_func(base_point, 10.5*scale, 2.5*scale, -4.5*scale, vec).y, point_func(base_point, 10.5*scale, 2.5*scale, -4.5*scale, vec).z);
        glVertex3d(point_func(base_point, 10.5*scale, 2.5*scale, 4.5*scale, vec).x, point_func(base_point, 10.5*scale, 2.5*scale, 4.5*scale, vec).y, point_func(base_point, 10.5*scale, 2.5*scale, 4.5*scale, vec).z);
        glVertex3d(point_func(base_point, 7.5*scale, 5.5*scale, 4.5*scale, vec).x, point_func(base_point, 7.5*scale, 5.5*scale, 4.5*scale, vec).y, point_func(base_point, 7.5*scale, 5.5*scale, 4.5*scale, vec).z);
        glVertex3d(point_func(base_point, 10.5*scale, 2.5*scale, -4.5*scale, vec).x, point_func(base_point, 10.5*scale, 2.5*scale, -4.5*scale, vec).y, point_func(base_point, 10.5*scale, 2.5*scale, -4.5*scale, vec).z);
        glVertex3d(point_func(base_point, 10.5*scale, -2.5*scale, -4.5*scale, vec).x, point_func(base_point, 10.5*scale, -2.5*scale, -4.5*scale, vec).y, point_func(base_point, 10.5*scale, -2.5*scale, -4.5*scale, vec).z);
        glVertex3d(point_func(base_point, 10.5*scale, -2.5*scale, 4.5*scale, vec).x, point_func(base_point, 10.5*scale, -2.5*scale, 4.5*scale, vec).y, point_func(base_point, 10.5*scale, -2.5*scale, 4.5*scale, vec).z);
        glVertex3d(point_func(base_point, 10.5*scale, 2.5*scale, 4.5*scale, vec).x, point_func(base_point, 10.5*scale, 2.5*scale, 4.5*scale, vec).y, point_func(base_point, 10.5*scale, 2.5*scale, 4.5*scale, vec).z);
        glVertex3d(point_func(base_point, 10.5*scale, -2.5*scale, -4.5*scale, vec).x, point_func(base_point, 10.5*scale, -2.5*scale, -4.5*scale, vec).y, point_func(base_point, 10.5*scale, -2.5*scale, -4.5*scale, vec).z);
        glVertex3d(point_func(base_point, 7.5*scale, -5.5*scale, -4.5*scale, vec).x, point_func(base_point, 7.5*scale, -5.5*scale, -4.5*scale, vec).y, point_func(base_point, 7.5*scale, -5.5*scale, -4.5*scale, vec).z);
        glVertex3d(point_func(base_point, 7.5*scale, -5.5*scale, 4.5*scale, vec).x, point_func(base_point, 7.5*scale, -5.5*scale, 4.5*scale, vec).y, point_func(base_point, 7.5*scale, -5.5*scale, 4.5*scale, vec).z);
        glVertex3d(point_func(base_point, 10.5*scale, -2.5*scale, 4.5*scale, vec).x, point_func(base_point, 10.5*scale, -2.5*scale, 4.5*scale, vec).y, point_func(base_point, 10.5*scale, -2.5*scale, 4.5*scale, vec).z);


        glVertex3d(point_func(base_point, -10.5*scale, 0.5*scale, 0.5*scale, vec).x, point_func(base_point, -10.5*scale, 0.5*scale, 0.5*scale, vec).y, point_func(base_point, -10.5*scale, 0.5*scale, 0.5*scale, vec).z);
        glVertex3d(point_func(base_point, -10.5*scale, -0.5*scale, 0.5*scale, vec).x, point_func(base_point, -10.5*scale, -0.5*scale, 0.5*scale, vec).y, point_func(base_point, -10.5*scale, -0.5*scale, 0.5*scale, vec).z);
        glVertex3d(point_func(base_point, -4.5*scale, -0.5*scale, 0.5*scale, vec).x, point_func(base_point, -4.5*scale, -0.5*scale, 0.5*scale, vec).y, point_func(base_point, -4.5*scale, -0.5*scale, 0.5*scale, vec).z);
        glVertex3d(point_func(base_point, -4.5*scale, 0.5*scale, 0.5*scale, vec).x, point_func(base_point, -4.5*scale, 0.5*scale, 0.5*scale, vec).y, point_func(base_point, -4.5*scale, 0.5*scale, 0.5*scale, vec).z);
        glVertex3d(point_func(base_point, -10.5*scale, 0.5*scale, -0.5*scale, vec).x, point_func(base_point, -10.5*scale, 0.5*scale, -0.5*scale, vec).y, point_func(base_point, -10.5*scale, 0.5*scale, -0.5*scale, vec).z);
        glVertex3d(point_func(base_point, -10.5*scale, -0.5*scale, -0.5*scale, vec).x, point_func(base_point, -10.5*scale, -0.5*scale, -0.5*scale, vec).y, point_func(base_point, -10.5*scale, -0.5*scale, -0.5*scale, vec).z);
        glVertex3d(point_func(base_point, -4.5*scale, -0.5*scale, -0.5*scale, vec).x, point_func(base_point, -4.5*scale, -0.5*scale, -0.5*scale, vec).y, point_func(base_point, -4.5*scale, -0.5*scale, -0.5*scale, vec).z);
        glVertex3d(point_func(base_point, -4.5*scale, 0.5*scale, -0.5*scale, vec).x, point_func(base_point, -4.5*scale, 0.5*scale, -0.5*scale, vec).y, point_func(base_point, -4.5*scale, 0.5*scale, -0.5*scale, vec).z);
        glVertex3d(point_func(base_point, -10.5*scale, 0.5*scale, 0.5*scale, vec).x, point_func(base_point, -10.5*scale, 0.5*scale, 0.5*scale, vec).y, point_func(base_point, -10.5*scale, 0.5*scale, 0.5*scale, vec).z);
        glVertex3d(point_func(base_point, -10.5*scale, 0.5*scale, -0.5*scale, vec).x, point_func(base_point, -10.5*scale, 0.5*scale, -0.5*scale, vec).y, point_func(base_point, -10.5*scale, 0.5*scale, -0.5*scale, vec).z);
        glVertex3d(point_func(base_point, -4.5*scale, 0.5*scale, -0.5*scale, vec).x, point_func(base_point, -4.5*scale, 0.5*scale, -0.5*scale, vec).y, point_func(base_point, -4.5*scale, 0.5*scale, -0.5*scale, vec).z);
        glVertex3d(point_func(base_point, -4.5*scale, 0.5*scale, 0.5*scale, vec).x, point_func(base_point, -4.5*scale, 0.5*scale, 0.5*scale, vec).y, point_func(base_point, -4.5*scale, 0.5*scale, 0.5*scale, vec).z);
        glVertex3d(point_func(base_point, -10.5*scale, -0.5*scale, 0.5*scale, vec).x, point_func(base_point, -10.5*scale, -0.5*scale, 0.5*scale, vec).y, point_func(base_point, -10.5*scale, -0.5*scale, 0.5*scale, vec).z);
        glVertex3d(point_func(base_point, -10.5*scale, -0.5*scale, -0.5*scale, vec).x, point_func(base_point, -10.5*scale, -0.5*scale, -0.5*scale, vec).y, point_func(base_point, -10.5*scale, -0.5*scale, -0.5*scale, vec).z);
        glVertex3d(point_func(base_point, -4.5*scale, -0.5*scale, -0.5*scale, vec).x, point_func(base_point, -4.5*scale, -0.5*scale, -0.5*scale, vec).y, point_func(base_point, -4.5*scale, -0.5*scale, -0.5*scale, vec).z);
        glVertex3d(point_func(base_point, -4.5*scale, -0.5*scale, 0.5*scale, vec).x, point_func(base_point, -4.5*scale, -0.5*scale, 0.5*scale, vec).y, point_func(base_point, -4.5*scale, -0.5*scale, 0.5*scale, vec).z);
        glVertex3d(point_func(base_point, 10.5*scale, 0.5*scale, 0.5*scale, vec).x, point_func(base_point, 10.5*scale, 0.5*scale, 0.5*scale, vec).y, point_func(base_point, 10.5*scale, 0.5*scale, 0.5*scale, vec).z);
        glVertex3d(point_func(base_point, 10.5*scale, -0.5*scale, 0.5*scale, vec).x, point_func(base_point, 10.5*scale, -0.5*scale, 0.5*scale, vec).y, point_func(base_point, 10.5*scale, -0.5*scale, 0.5*scale, vec).z);
        glVertex3d(point_func(base_point, 4.5*scale, -0.5*scale, 0.5*scale, vec).x, point_func(base_point, 4.5*scale, -0.5*scale, 0.5*scale, vec).y, point_func(base_point, 4.5*scale, -0.5*scale, 0.5*scale, vec).z);
        glVertex3d(point_func(base_point, 4.5*scale, 0.5*scale, 0.5*scale, vec).x, point_func(base_point, 4.5*scale, 0.5*scale, 0.5*scale, vec).y, point_func(base_point, 4.5*scale, 0.5*scale, 0.5*scale, vec).z);
        glVertex3d(point_func(base_point, 10.5*scale, 0.5*scale, -0.5*scale, vec).x, point_func(base_point, 10.5*scale, 0.5*scale, -0.5*scale, vec).y, point_func(base_point, 10.5*scale, 0.5*scale, -0.5*scale, vec).z);
        glVertex3d(point_func(base_point, 10.5*scale, -0.5*scale, -0.5*scale, vec).x, point_func(base_point, 10.5*scale, -0.5*scale, -0.5*scale, vec).y, point_func(base_point, 10.5*scale, -0.5*scale, -0.5*scale, vec).z);
        glVertex3d(point_func(base_point, 4.5*scale, -0.5*scale, -0.5*scale, vec).x, point_func(base_point, 4.5*scale, -0.5*scale, -0.5*scale, vec).y, point_func(base_point, 4.5*scale, -0.5*scale, -0.5*scale, vec).z);
        glVertex3d(point_func(base_point, 4.5*scale, 0.5*scale, -0.5*scale, vec).x, point_func(base_point, 4.5*scale, 0.5*scale, -0.5*scale, vec).y, point_func(base_point, 4.5*scale, 0.5*scale, -0.5*scale, vec).z);
        glVertex3d(point_func(base_point, 10.5*scale, 0.5*scale, 0.5*scale, vec).x, point_func(base_point, 10.5*scale, 0.5*scale, 0.5*scale, vec).y, point_func(base_point, 10.5*scale, 0.5*scale, 0.5*scale, vec).z);
        glVertex3d(point_func(base_point, 10.5*scale, 0.5*scale, -0.5*scale, vec).x, point_func(base_point, 10.5*scale, 0.5*scale, -0.5*scale, vec).y, point_func(base_point, 10.5*scale, 0.5*scale, -0.5*scale, vec).z);
        glVertex3d(point_func(base_point, 4.5*scale, 0.5*scale, -0.5*scale, vec).x, point_func(base_point, 4.5*scale, 0.5*scale, -0.5*scale, vec).y, point_func(base_point, 4.5*scale, 0.5*scale, -0.5*scale, vec).z);
        glVertex3d(point_func(base_point, 4.5*scale, 0.5*scale, 0.5*scale, vec).x, point_func(base_point, 4.5*scale, 0.5*scale, 0.5*scale, vec).y, point_func(base_point, 4.5*scale, 0.5*scale, 0.5*scale, vec).z);
        glVertex3d(point_func(base_point, 10.5*scale, -0.5*scale, 0.5*scale, vec).x, point_func(base_point, 10.5*scale, -0.5*scale, 0.5*scale, vec).y, point_func(base_point, 10.5*scale, -0.5*scale, 0.5*scale, vec).z);
        glVertex3d(point_func(base_point, 10.5*scale, -0.5*scale, -0.5*scale, vec).x, point_func(base_point, 10.5*scale, -0.5*scale, -0.5*scale, vec).y, point_func(base_point, 10.5*scale, -0.5*scale, -0.5*scale, vec).z);
        glVertex3d(point_func(base_point, 4.5*scale, -0.5*scale, -0.5*scale, vec).x, point_func(base_point, 4.5*scale, -0.5*scale, -0.5*scale, vec).y, point_func(base_point, 4.5*scale, -0.5*scale, -0.5*scale, vec).z);
        glVertex3d(point_func(base_point, 4.5*scale, -0.5*scale, 0.5*scale, vec).x, point_func(base_point, 4.5*scale, -0.5*scale, 0.5*scale, vec).y, point_func(base_point, 4.5*scale, -0.5*scale, 0.5*scale, vec).z);

        glVertex3d(point_func(base_point, -1.5*scale, 4.5*scale, -1.5*scale, vec).x, point_func(base_point, -1.5*scale, 4.5*scale, -1.5*scale, vec).y, point_func(base_point, -1.5*scale, 4.5*scale, -1.5*scale, vec).z);
        glVertex3d(point_func(base_point, 1.5*scale, 4.5*scale, -1.5*scale, vec).x, point_func(base_point, 1.5*scale, 4.5*scale, -1.5*scale, vec).y, point_func(base_point, 1.5*scale, 4.5*scale, -1.5*scale, vec).z);
        glVertex3d(point_func(base_point, 1.5*scale, 4.5*scale, 1.5*scale, vec).x, point_func(base_point, 1.5*scale, 4.5*scale, 1.5*scale, vec).y, point_func(base_point, 1.5*scale, 4.5*scale, 1.5*scale, vec).z);
        glVertex3d(point_func(base_point, -1.5*scale, 4.5*scale, 1.5*scale, vec).x, point_func(base_point, -1.5*scale, 4.5*scale, 1.5*scale, vec).y, point_func(base_point, -1.5*scale, 4.5*scale, 1.5*scale, vec).z);
        glVertex3d(point_func(base_point, 1.5*scale, 4.5*scale, -1.5*scale, vec).x, point_func(base_point, 1.5*scale, 4.5*scale, -1.5*scale, vec).y, point_func(base_point, 1.5*scale, 4.5*scale, -1.5*scale, vec).z);
        glVertex3d(point_func(base_point, -1.5*scale, 4.5*scale, -1.5*scale, vec).x, point_func(base_point, -1.5*scale, 4.5*scale, -1.5*scale, vec).y, point_func(base_point, -1.5*scale, 4.5*scale, -1.5*scale, vec).z);
        glVertex3d(point_func(base_point, -2.5*scale, 1.5*scale, -4.5*scale, vec).x, point_func(base_point, -2.5*scale, 1.5*scale, -4.5*scale, vec).y, point_func(base_point, -2.5*scale, 1.5*scale, -4.5*scale, vec).z);
        glVertex3d(point_func(base_point, 2.5*scale, 1.5*scale, -4.5*scale, vec).x, point_func(base_point, 2.5*scale, 1.5*scale, -4.5*scale, vec).y, point_func(base_point, 2.5*scale, 1.5*scale, -4.5*scale, vec).z);
        glVertex3d(point_func(base_point, 1.5*scale, 4.5*scale, 1.5*scale, vec).x, point_func(base_point, 1.5*scale, 4.5*scale, 1.5*scale, vec).y, point_func(base_point, 1.5*scale, 4.5*scale, 1.5*scale, vec).z);
        glVertex3d(point_func(base_point, 1.5*scale, 4.5*scale, -1.5*scale, vec).x, point_func(base_point, 1.5*scale, 4.5*scale, -1.5*scale, vec).y, point_func(base_point, 1.5*scale, 4.5*scale, -1.5*scale, vec).z);
        glVertex3d(point_func(base_point, 4.5*scale, 1.5*scale, -2.5*scale, vec).x, point_func(base_point, 4.5*scale, 1.5*scale, -2.5*scale, vec).y, point_func(base_point, 4.5*scale, 1.5*scale, -2.5*scale, vec).z);
        glVertex3d(point_func(base_point, 4.5*scale, 1.5*scale, 2.5*scale, vec).x, point_func(base_point, 4.5*scale, 1.5*scale, 2.5*scale, vec).y, point_func(base_point, 4.5*scale, 1.5*scale, 2.5*scale, vec).z);
        glVertex3d(point_func(base_point, -1.5*scale, 4.5*scale, 1.5*scale, vec).x, point_func(base_point, -1.5*scale, 4.5*scale, 1.5*scale, vec).y, point_func(base_point, -1.5*scale, 4.5*scale, 1.5*scale, vec).z);
        glVertex3d(point_func(base_point, 1.5*scale, 4.5*scale, 1.5*scale, vec).x, point_func(base_point, 1.5*scale, 4.5*scale, 1.5*scale, vec).y, point_func(base_point, 1.5*scale, 4.5*scale, 1.5*scale, vec).z);
        glVertex3d(point_func(base_point, 2.5*scale, 1.5*scale, 4.5*scale, vec).x, point_func(base_point, 2.5*scale, 1.5*scale, 4.5*scale, vec).y, point_func(base_point, 2.5*scale, 1.5*scale, 4.5*scale, vec).z);
        glVertex3d(point_func(base_point, -2.5*scale, 1.5*scale, 4.5*scale, vec).x, point_func(base_point, -2.5*scale, 1.5*scale, 4.5*scale, vec).y, point_func(base_point, -2.5*scale, 1.5*scale, 4.5*scale, vec).z);
        glVertex3d(point_func(base_point, -1.5*scale, 4.5*scale, -1.5*scale, vec).x, point_func(base_point, -1.5*scale, 4.5*scale, -1.5*scale, vec).y, point_func(base_point, -1.5*scale, 4.5*scale, -1.5*scale, vec).z);
        glVertex3d(point_func(base_point, -1.5*scale, 4.5*scale, 1.5*scale, vec).x, point_func(base_point, -1.5*scale, 4.5*scale, 1.5*scale, vec).y, point_func(base_point, -1.5*scale, 4.5*scale, 1.5*scale, vec).z);
        glVertex3d(point_func(base_point, -4.5*scale, 1.5*scale, 2.5*scale, vec).x, point_func(base_point, -4.5*scale, 1.5*scale, 2.5*scale, vec).y, point_func(base_point, -4.5*scale, 1.5*scale, 2.5*scale, vec).z);
        glVertex3d(point_func(base_point, -4.5*scale, 1.5*scale, -2.5*scale, vec).x, point_func(base_point, -4.5*scale, 1.5*scale, -2.5*scale, vec).y, point_func(base_point, -4.5*scale, 1.5*scale, -2.5*scale, vec).z);
        glVertex3d(point_func(base_point, -1.5*scale, 4.5*scale, -1.5*scale, vec).x, point_func(base_point, -1.5*scale, 4.5*scale, -1.5*scale, vec).y, point_func(base_point, -1.5*scale, 4.5*scale, -1.5*scale, vec).z);
        glVertex3d(point_func(base_point, -2.5*scale, 1.5*scale, -4.5*scale, vec).x, point_func(base_point, -2.5*scale, 1.5*scale, -4.5*scale, vec).y, point_func(base_point, -2.5*scale, 1.5*scale, -4.5*scale, vec).z);
        glVertex3d(point_func(base_point, -4.5*scale, 1.5*scale, -2.5*scale, vec).x, point_func(base_point, -4.5*scale, 1.5*scale, -2.5*scale, vec).y, point_func(base_point, -4.5*scale, 1.5*scale, -2.5*scale, vec).z);
        glVertex3d(point_func(base_point, 1.5*scale, 4.5*scale, -1.5*scale, vec).x, point_func(base_point, 1.5*scale, 4.5*scale, -1.5*scale, vec).y, point_func(base_point, 1.5*scale, 4.5*scale, -1.5*scale, vec).z);
        glVertex3d(point_func(base_point, 2.5*scale, 1.5*scale, -4.5*scale, vec).x, point_func(base_point, 2.5*scale, 1.5*scale, -4.5*scale, vec).y, point_func(base_point, 2.5*scale, 1.5*scale, -4.5*scale, vec).z);
        glVertex3d(point_func(base_point, 4.5*scale, 1.5*scale, -2.5*scale, vec).x, point_func(base_point, 4.5*scale, 1.5*scale, -2.5*scale, vec).y, point_func(base_point, 4.5*scale, 1.5*scale, -2.5*scale, vec).z);
        glVertex3d(point_func(base_point, 1.5*scale, 4.5*scale, 1.5*scale, vec).x, point_func(base_point, 1.5*scale, 4.5*scale, 1.5*scale, vec).y, point_func(base_point, 1.5*scale, 4.5*scale, 1.5*scale, vec).z);
        glVertex3d(point_func(base_point, 4.5*scale, 1.5*scale, 2.5*scale, vec).x, point_func(base_point, 4.5*scale, 1.5*scale, 2.5*scale, vec).y, point_func(base_point, 4.5*scale, 1.5*scale, 2.5*scale, vec).z);
        glVertex3d(point_func(base_point, 2.5*scale, 1.5*scale, 4.5*scale, vec).x, point_func(base_point, 2.5*scale, 1.5*scale, 4.5*scale, vec).y, point_func(base_point, 2.5*scale, 1.5*scale, 4.5*scale, vec).z);
        glVertex3d(point_func(base_point, -1.5*scale, 4.5*scale, 1.5*scale, vec).x, point_func(base_point, -1.5*scale, 4.5*scale, 1.5*scale, vec).y, point_func(base_point, -1.5*scale, 4.5*scale, 1.5*scale, vec).z);
        glVertex3d(point_func(base_point, -2.5*scale, 1.5*scale, 4.5*scale, vec).x, point_func(base_point, -2.5*scale, 1.5*scale, 4.5*scale, vec).y, point_func(base_point, -2.5*scale, 1.5*scale, 4.5*scale, vec).z);
        glVertex3d(point_func(base_point, -4.5*scale, 1.5*scale, 2.5*scale, vec).x, point_func(base_point, -4.5*scale, 1.5*scale, 2.5*scale, vec).y, point_func(base_point, -4.5*scale, 1.5*scale, 2.5*scale, vec).z);
        glVertex3d(point_func(base_point, -2.5*scale, 1.5*scale, -4.5*scale, vec).x, point_func(base_point, -2.5*scale, 1.5*scale, -4.5*scale, vec).y, point_func(base_point, -2.5*scale, 1.5*scale, -4.5*scale, vec).z);
        glVertex3d(point_func(base_point, 2.5*scale, 1.5*scale, -4.5*scale, vec).x, point_func(base_point, 2.5*scale, 1.5*scale, -4.5*scale, vec).y, point_func(base_point, 2.5*scale, 1.5*scale, -4.5*scale, vec).z);
        glVertex3d(point_func(base_point, 2.5*scale, -1.5*scale, -4.5*scale, vec).x, point_func(base_point, 2.5*scale, -1.5*scale, -4.5*scale, vec).y, point_func(base_point, 2.5*scale, -1.5*scale, -4.5*scale, vec).z);
        glVertex3d(point_func(base_point, -2.5*scale, -1.5*scale, -4.5*scale, vec).x, point_func(base_point, -2.5*scale, -1.5*scale, -4.5*scale, vec).y, point_func(base_point, -2.5*scale, -1.5*scale, -4.5*scale, vec).z);
        glVertex3d(point_func(base_point, 2.5*scale, 1.5*scale, -4.5*scale, vec).x, point_func(base_point, 2.5*scale, 1.5*scale, -4.5*scale, vec).y, point_func(base_point, 2.5*scale, 1.5*scale, -4.5*scale, vec).z);
        glVertex3d(point_func(base_point, 4.5*scale, 1.5*scale, -2.5*scale, vec).x, point_func(base_point, 4.5*scale, 1.5*scale, -2.5*scale, vec).y, point_func(base_point, 4.5*scale, 1.5*scale, -2.5*scale, vec).z);
        glVertex3d(point_func(base_point, 4.5*scale, -1.5*scale, -2.5*scale, vec).x, point_func(base_point, 4.5*scale, -1.5*scale, -2.5*scale, vec).y, point_func(base_point, 4.5*scale, -1.5*scale, -2.5*scale, vec).z);
        glVertex3d(point_func(base_point, 2.5*scale, -1.5*scale, -4.5*scale, vec).x, point_func(base_point, 2.5*scale, -1.5*scale, -4.5*scale, vec).y, point_func(base_point, 2.5*scale, -1.5*scale, -4.5*scale, vec).z);
        glVertex3d(point_func(base_point, 4.5*scale, 1.5*scale, -2.5*scale, vec).x, point_func(base_point, 4.5*scale, 1.5*scale, -2.5*scale, vec).y, point_func(base_point, 4.5*scale, 1.5*scale, -2.5*scale, vec).z);
        glVertex3d(point_func(base_point, 4.5*scale, 1.5*scale, 2.5*scale, vec).x, point_func(base_point, 4.5*scale, 1.5*scale, 2.5*scale, vec).y, point_func(base_point, 4.5*scale, 1.5*scale, 2.5*scale, vec).z);
        glVertex3d(point_func(base_point, 4.5*scale, -1.5*scale, 2.5*scale, vec).x, point_func(base_point, 4.5*scale, -1.5*scale, 2.5*scale, vec).y, point_func(base_point, 4.5*scale, -1.5*scale, 2.5*scale, vec).z);
        glVertex3d(point_func(base_point, 4.5*scale, -1.5*scale, -2.5*scale, vec).x, point_func(base_point, 4.5*scale, -1.5*scale, -2.5*scale, vec).y, point_func(base_point, 4.5*scale, -1.5*scale, -2.5*scale, vec).z);
        glVertex3d(point_func(base_point, 4.5*scale, 1.5*scale, 2.5*scale, vec).x, point_func(base_point, 4.5*scale, 1.5*scale, 2.5*scale, vec).y, point_func(base_point, 4.5*scale, 1.5*scale, 2.5*scale, vec).z);
        glVertex3d(point_func(base_point, 2.5*scale, 1.5*scale, 4.5*scale, vec).x, point_func(base_point, 2.5*scale, 1.5*scale, 4.5*scale, vec).y, point_func(base_point, 2.5*scale, 1.5*scale, 4.5*scale, vec).z);
        glVertex3d(point_func(base_point, 2.5*scale, -1.5*scale, 4.5*scale, vec).x, point_func(base_point, 2.5*scale, -1.5*scale, 4.5*scale, vec).y, point_func(base_point, 2.5*scale, -1.5*scale, 4.5*scale, vec).z);
        glVertex3d(point_func(base_point, 4.5*scale, -1.5*scale, 2.5*scale, vec).x, point_func(base_point, 4.5*scale, -1.5*scale, 2.5*scale, vec).y, point_func(base_point, 4.5*scale, -1.5*scale, 2.5*scale, vec).z);
        glVertex3d(point_func(base_point, 2.5*scale, 1.5*scale, 4.5*scale, vec).x, point_func(base_point, 2.5*scale, 1.5*scale, 4.5*scale, vec).y, point_func(base_point, 2.5*scale, 1.5*scale, 4.5*scale, vec).z);
        glVertex3d(point_func(base_point, -2.5*scale, 1.5*scale, 4.5*scale, vec).x, point_func(base_point, -2.5*scale, 1.5*scale, 4.5*scale, vec).y, point_func(base_point, -2.5*scale, 1.5*scale, 4.5*scale, vec).z);
        glVertex3d(point_func(base_point, -2.5*scale, -1.5*scale, 4.5*scale, vec).x, point_func(base_point, -2.5*scale, -1.5*scale, 4.5*scale, vec).y, point_func(base_point, -2.5*scale, -1.5*scale, 4.5*scale, vec).z);
        glVertex3d(point_func(base_point, 2.5*scale, -1.5*scale, 4.5*scale, vec).x, point_func(base_point, 2.5*scale, -1.5*scale, 4.5*scale, vec).y, point_func(base_point, 2.5*scale, -1.5*scale, 4.5*scale, vec).z);
        glVertex3d(point_func(base_point, -2.5*scale, 1.5*scale, 4.5*scale, vec).x, point_func(base_point, -2.5*scale, 1.5*scale, 4.5*scale, vec).y, point_func(base_point, -2.5*scale, 1.5*scale, 4.5*scale, vec).z);
        glVertex3d(point_func(base_point, -4.5*scale, 1.5*scale, 2.5*scale, vec).x, point_func(base_point, -4.5*scale, 1.5*scale, 2.5*scale, vec).y, point_func(base_point, -4.5*scale, 1.5*scale, 2.5*scale, vec).z);
        glVertex3d(point_func(base_point, -4.5*scale, -1.5*scale, 2.5*scale, vec).x, point_func(base_point, -4.5*scale, -1.5*scale, 2.5*scale, vec).y, point_func(base_point, -4.5*scale, -1.5*scale, 2.5*scale, vec).z);
        glVertex3d(point_func(base_point, -2.5*scale, -1.5*scale, 4.5*scale, vec).x, point_func(base_point, -2.5*scale, -1.5*scale, 4.5*scale, vec).y, point_func(base_point, -2.5*scale, -1.5*scale, 4.5*scale, vec).z);
        glVertex3d(point_func(base_point, -4.5*scale, 1.5*scale, 2.5*scale, vec).x, point_func(base_point, -4.5*scale, 1.5*scale, 2.5*scale, vec).y, point_func(base_point, -4.5*scale, 1.5*scale, 2.5*scale, vec).z);
        glVertex3d(point_func(base_point, -4.5*scale, 1.5*scale, -2.5*scale, vec).x, point_func(base_point, -4.5*scale, 1.5*scale, -2.5*scale, vec).y, point_func(base_point, -4.5*scale, 1.5*scale, -2.5*scale, vec).z);
        glVertex3d(point_func(base_point, -4.5*scale, -1.5*scale, -2.5*scale, vec).x, point_func(base_point, -4.5*scale, -1.5*scale, -2.5*scale, vec).y, point_func(base_point, -4.5*scale, -1.5*scale, -2.5*scale, vec).z);
        glVertex3d(point_func(base_point, -4.5*scale, -1.5*scale, 2.5*scale, vec).x, point_func(base_point, -4.5*scale, -1.5*scale, 2.5*scale, vec).y, point_func(base_point, -4.5*scale, -1.5*scale, 2.5*scale, vec).z);
        glVertex3d(point_func(base_point, -4.5*scale, 1.5*scale, -2.5*scale, vec).x, point_func(base_point, -4.5*scale, 1.5*scale, -2.5*scale, vec).y, point_func(base_point, -4.5*scale, 1.5*scale, -2.5*scale, vec).z);
        glVertex3d(point_func(base_point, -2.5*scale, 1.5*scale, -4.5*scale, vec).x, point_func(base_point, -2.5*scale, 1.5*scale, -4.5*scale, vec).y, point_func(base_point, -2.5*scale, 1.5*scale, -4.5*scale, vec).z);
        glVertex3d(point_func(base_point, -2.5*scale, -1.5*scale, -4.5*scale, vec).x, point_func(base_point, -2.5*scale, -1.5*scale, -4.5*scale, vec).y, point_func(base_point, -2.5*scale, -1.5*scale, -4.5*scale, vec).z);
        glVertex3d(point_func(base_point, -4.5*scale, -1.5*scale, -2.5*scale, vec).x, point_func(base_point, -4.5*scale, -1.5*scale, -2.5*scale, vec).y, point_func(base_point, -4.5*scale, -1.5*scale, -2.5*scale, vec).z);
        glVertex3d(point_func(base_point, -1.5*scale, -4.5*scale, -1.5*scale, vec).x, point_func(base_point, -1.5*scale, -4.5*scale, -1.5*scale, vec).y, point_func(base_point, -1.5*scale, -4.5*scale, -1.5*scale, vec).z);
        glVertex3d(point_func(base_point, 1.5*scale, -4.5*scale, -1.5*scale, vec).x, point_func(base_point, 1.5*scale, -4.5*scale, -1.5*scale, vec).y, point_func(base_point, 1.5*scale, -4.5*scale, -1.5*scale, vec).z);
        glVertex3d(point_func(base_point, 1.5*scale, -4.5*scale, 1.5*scale, vec).x, point_func(base_point, 1.5*scale, -4.5*scale, 1.5*scale, vec).y, point_func(base_point, 1.5*scale, -4.5*scale, 1.5*scale, vec).z);
        glVertex3d(point_func(base_point, -1.5*scale, -4.5*scale, 1.5*scale, vec).x, point_func(base_point, -1.5*scale, -4.5*scale, 1.5*scale, vec).y, point_func(base_point, -1.5*scale, -4.5*scale, 1.5*scale, vec).z);
        glVertex3d(point_func(base_point, 1.5*scale, -4.5*scale, -1.5*scale, vec).x, point_func(base_point, 1.5*scale, -4.5*scale, -1.5*scale, vec).y, point_func(base_point, 1.5*scale, -4.5*scale, -1.5*scale, vec).z);
        glVertex3d(point_func(base_point, -1.5*scale, -4.5*scale, -1.5*scale, vec).x, point_func(base_point, -1.5*scale, -4.5*scale, -1.5*scale, vec).y, point_func(base_point, -1.5*scale, -4.5*scale, -1.5*scale, vec).z);
        glVertex3d(point_func(base_point, -2.5*scale, -1.5*scale, -4.5*scale, vec).x, point_func(base_point, -2.5*scale, -1.5*scale, -4.5*scale, vec).y, point_func(base_point, -2.5*scale, -1.5*scale, -4.5*scale, vec).z);
        glVertex3d(point_func(base_point, 2.5*scale, -1.5*scale, -4.5*scale, vec).x, point_func(base_point, 2.5*scale, -1.5*scale, -4.5*scale, vec).y, point_func(base_point, 2.5*scale, -1.5*scale, -4.5*scale, vec).z);
        glVertex3d(point_func(base_point, 1.5*scale, -4.5*scale, 1.5*scale, vec).x, point_func(base_point, 1.5*scale, -4.5*scale, 1.5*scale, vec).y, point_func(base_point, 1.5*scale, -4.5*scale, 1.5*scale, vec).z);
        glVertex3d(point_func(base_point, 1.5*scale, -4.5*scale, -1.5*scale, vec).x, point_func(base_point, 1.5*scale, -4.5*scale, -1.5*scale, vec).y, point_func(base_point, 1.5*scale, -4.5*scale, -1.5*scale, vec).z);
        glVertex3d(point_func(base_point, 4.5*scale, -1.5*scale, -2.5*scale, vec).x, point_func(base_point, 4.5*scale, -1.5*scale, -2.5*scale, vec).y, point_func(base_point, 4.5*scale, -1.5*scale, -2.5*scale, vec).z);
        glVertex3d(point_func(base_point, 4.5*scale, -1.5*scale, 2.5*scale, vec).x, point_func(base_point, 4.5*scale, -1.5*scale, 2.5*scale, vec).y, point_func(base_point, 4.5*scale, -1.5*scale, 2.5*scale, vec).z);
        glVertex3d(point_func(base_point, -1.5*scale, -4.5*scale, 1.5*scale, vec).x, point_func(base_point, -1.5*scale, -4.5*scale, 1.5*scale, vec).y, point_func(base_point, -1.5*scale, -4.5*scale, 1.5*scale, vec).z);
        glVertex3d(point_func(base_point, 1.5*scale, -4.5*scale, 1.5*scale, vec).x, point_func(base_point, 1.5*scale, -4.5*scale, 1.5*scale, vec).y, point_func(base_point, 1.5*scale, -4.5*scale, 1.5*scale, vec).z);
        glVertex3d(point_func(base_point, 2.5*scale, -1.5*scale, 4.5*scale, vec).x, point_func(base_point, 2.5*scale, -1.5*scale, 4.5*scale, vec).y, point_func(base_point, 2.5*scale, -1.5*scale, 4.5*scale, vec).z);
        glVertex3d(point_func(base_point, -2.5*scale, -1.5*scale, 4.5*scale, vec).x, point_func(base_point, -2.5*scale, -1.5*scale, 4.5*scale, vec).y, point_func(base_point, -2.5*scale, -1.5*scale, 4.5*scale, vec).z);
        glVertex3d(point_func(base_point, -1.5*scale, -4.5*scale, -1.5*scale, vec).x, point_func(base_point, -1.5*scale, -4.5*scale, -1.5*scale, vec).y, point_func(base_point, -1.5*scale, -4.5*scale, -1.5*scale, vec).z);
        glVertex3d(point_func(base_point, -1.5*scale, -4.5*scale, 1.5*scale, vec).x, point_func(base_point, -1.5*scale, -4.5*scale, 1.5*scale, vec).y, point_func(base_point, -1.5*scale, -4.5*scale, 1.5*scale, vec).z);
        glVertex3d(point_func(base_point, -4.5*scale, -1.5*scale, 2.5*scale, vec).x, point_func(base_point, -4.5*scale, -1.5*scale, 2.5*scale, vec).y, point_func(base_point, -4.5*scale, -1.5*scale, 2.5*scale, vec).z);
        glVertex3d(point_func(base_point, -4.5*scale, -1.5*scale, -2.5*scale, vec).x, point_func(base_point, -4.5*scale, -1.5*scale, -2.5*scale, vec).y, point_func(base_point, -4.5*scale, -1.5*scale, -2.5*scale, vec).z);
        glVertex3d(point_func(base_point, -1.5*scale, -4.5*scale, -1.5*scale, vec).x, point_func(base_point, -1.5*scale, -4.5*scale, -1.5*scale, vec).y, point_func(base_point, -1.5*scale, -4.5*scale, -1.5*scale, vec).z);
        glVertex3d(point_func(base_point, -2.5*scale, -1.5*scale, -4.5*scale, vec).x, point_func(base_point, -2.5*scale, -1.5*scale, -4.5*scale, vec).y, point_func(base_point, -2.5*scale, -1.5*scale, -4.5*scale, vec).z);
        glVertex3d(point_func(base_point, -4.5*scale, -1.5*scale, -2.5*scale, vec).x, point_func(base_point, -4.5*scale, -1.5*scale, -2.5*scale, vec).y, point_func(base_point, -4.5*scale, -1.5*scale, -2.5*scale, vec).z);
        glVertex3d(point_func(base_point, 1.5*scale, -4.5*scale, -1.5*scale, vec).x, point_func(base_point, 1.5*scale, -4.5*scale, -1.5*scale, vec).y, point_func(base_point, 1.5*scale, -4.5*scale, -1.5*scale, vec).z);
        glVertex3d(point_func(base_point, 2.5*scale, -1.5*scale, -4.5*scale, vec).x, point_func(base_point, 2.5*scale, -1.5*scale, -4.5*scale, vec).y, point_func(base_point, 2.5*scale, -1.5*scale, -4.5*scale, vec).z);
        glVertex3d(point_func(base_point, 4.5*scale, -1.5*scale, -2.5*scale, vec).x, point_func(base_point, 4.5*scale, -1.5*scale, -2.5*scale, vec).y, point_func(base_point, 4.5*scale, -1.5*scale, -2.5*scale, vec).z);
        glVertex3d(point_func(base_point, 1.5*scale, -4.5*scale, 1.5*scale, vec).x, point_func(base_point, 1.5*scale, -4.5*scale, 1.5*scale, vec).y, point_func(base_point, 1.5*scale, -4.5*scale, 1.5*scale, vec).z);
        glVertex3d(point_func(base_point, 4.5*scale, -1.5*scale, 2.5*scale, vec).x, point_func(base_point, 4.5*scale, -1.5*scale, 2.5*scale, vec).y, point_func(base_point, 4.5*scale, -1.5*scale, 2.5*scale, vec).z);
        glVertex3d(point_func(base_point, 2.5*scale, -1.5*scale, 4.5*scale, vec).x, point_func(base_point, 2.5*scale, -1.5*scale, 4.5*scale, vec).y, point_func(base_point, 2.5*scale, -1.5*scale, 4.5*scale, vec).z);
        glVertex3d(point_func(base_point, -1.5*scale, -4.5*scale, 1.5*scale, vec).x, point_func(base_point, -1.5*scale, -4.5*scale, 1.5*scale, vec).y, point_func(base_point, -1.5*scale, -4.5*scale, 1.5*scale, vec).z);
        glVertex3d(point_func(base_point, -2.5*scale, -1.5*scale, 4.5*scale, vec).x, point_func(base_point, -2.5*scale, -1.5*scale, 4.5*scale, vec).y, point_func(base_point, -2.5*scale, -1.5*scale, 4.5*scale, vec).z);
        glVertex3d(point_func(base_point, -4.5*scale, -1.5*scale, 2.5*scale, vec).x, point_func(base_point, -4.5*scale, -1.5*scale, 2.5*scale, vec).y, point_func(base_point, -4.5*scale, -1.5*scale, 2.5*scale, vec).z);

glEnd();
}


/*
    弾丸を描写
*/
void DrawAmmo(xyz base_point, xyz vec){
    glBegin(GL_QUADS); //側面
        // glColor3d(1.0, 0.0, 0.0);
        glColor3d(0.93, 0.46, 0.84);
        //上
        glVertex3d(point_func(base_point, -0.01, 0.01, -0.01, vec).x, point_func(base_point, -0.01, 0.01, -0.01, vec).y, point_func(base_point, -0.01, 0.01, -0.01, vec).z);
        glVertex3d(point_func(base_point, -0.01, 0.01, 0.01, vec).x, point_func(base_point, -0.01, 0.01, 0.01, vec).y, point_func(base_point, -0.01, 0.01, 0.01, vec).z);
        glVertex3d(point_func(base_point, 0.01, 0.01, 0.01, vec).x, point_func(base_point, 0.01, 0.01, 0.01, vec).y, point_func(base_point, 0.01, 0.01, 0.01, vec).z);
        glVertex3d(point_func(base_point, 0.01, 0.01, -0.01, vec).x, point_func(base_point, 0.01, 0.01, -0.01, vec).y, point_func(base_point, 0.01, 0.01, -0.01, vec).z);
        //下
        glVertex3d(point_func(base_point, -0.01, -0.01, -0.01, vec).x, point_func(base_point, -0.01, -0.01, -0.01, vec).y, point_func(base_point, -0.01, -0.01, -0.01, vec).z);
        glVertex3d(point_func(base_point, -0.01, -0.01, 0.01, vec).x, point_func(base_point, -0.01, -0.01, 0.01, vec).y, point_func(base_point, -0.01, -0.01, 0.01, vec).z);
        glVertex3d(point_func(base_point, 0.01, -0.01, 0.01, vec).x, point_func(base_point, 0.01, -0.01, 0.01, vec).y, point_func(base_point, 0.01, -0.01, 0.01, vec).z);
        glVertex3d(point_func(base_point, 0.01, -0.01, -0.01, vec).x, point_func(base_point, 0.01, -0.01, -0.01, vec).y, point_func(base_point, 0.01, -0.01, -0.01, vec).z);
        //右
        glVertex3d(point_func(base_point, 0.01, -0.01, -0.01, vec).x, point_func(base_point, 0.01, -0.01, -0.01, vec).y, point_func(base_point, 0.01, -0.01, -0.01, vec).z);
        glVertex3d(point_func(base_point, 0.01, -0.01, 0.01, vec).x, point_func(base_point, 0.01, -0.01, 0.01, vec).y, point_func(base_point, 0.01, -0.01, 0.01, vec).z);
        glVertex3d(point_func(base_point, 0.01, 0.01, 0.01, vec).x, point_func(base_point, 0.01, 0.01, 0.01, vec).y, point_func(base_point, 0.01, 0.01, 0.01, vec).z);
        glVertex3d(point_func(base_point, 0.01, 0.01, -0.01, vec).x, point_func(base_point, 0.01, 0.01, -0.01, vec).y, point_func(base_point, 0.01, 0.01, -0.01, vec).z);
        //左
        glVertex3d(point_func(base_point, -0.01, -0.01, -0.01, vec).x, point_func(base_point, -0.01, -0.01, -0.01, vec).y, point_func(base_point, -0.01, -0.01, -0.01, vec).z);
        glVertex3d(point_func(base_point, -0.01, -0.01, 0.01, vec).x, point_func(base_point, -0.01, -0.01, 0.01, vec).y, point_func(base_point, -0.01, -0.01, 0.01, vec).z);
        glVertex3d(point_func(base_point, -0.01, 0.01, 0.01, vec).x, point_func(base_point, -0.01, 0.01, 0.01, vec).y, point_func(base_point, -0.01, 0.01, 0.01, vec).z);
        glVertex3d(point_func(base_point, -0.01, 0.01, -0.01, vec).x, point_func(base_point, -0.01, 0.01, -0.01, vec).y, point_func(base_point, -0.01, 0.01, -0.01, vec).z);
        //前
        glVertex3d(point_func(base_point, -0.01, -0.01, 0.01, vec).x, point_func(base_point, -0.01, -0.01, 0.01, vec).y, point_func(base_point, -0.01, -0.01, 0.01, vec).z);
        glVertex3d(point_func(base_point, -0.01, 0.01, 0.01, vec).x, point_func(base_point, -0.01, 0.01, 0.01, vec).y, point_func(base_point, -0.01, 0.01, 0.01, vec).z);
        glVertex3d(point_func(base_point, 0.01, 0.01, 0.01, vec).x, point_func(base_point, 0.01, 0.01, 0.01, vec).y, point_func(base_point, 0.01, 0.01, 0.01, vec).z);
        glVertex3d(point_func(base_point, 0.01, -0.01, 0.01, vec).x, point_func(base_point, 0.01, -0.01, 0.01, vec).y, point_func(base_point, 0.01, -0.01, 0.01, vec).z);
        //後
        glVertex3d(point_func(base_point, -0.01, -0.01, -0.01, vec).x, point_func(base_point, -0.01, -0.01, -0.01, vec).y, point_func(base_point, -0.01, -0.01, -0.01, vec).z);
        glVertex3d(point_func(base_point, -0.01, 0.01, -0.01, vec).x, point_func(base_point, -0.01, 0.01, -0.01, vec).y, point_func(base_point, -0.01, 0.01, -0.01, vec).z);
        glVertex3d(point_func(base_point, 0.01, 0.01, -0.01, vec).x, point_func(base_point, 0.01, 0.01, -0.01, vec).y, point_func(base_point, 0.01, 0.01, -0.01, vec).z);
        glVertex3d(point_func(base_point, 0.01, -0.01, -0.01, vec).x, point_func(base_point, 0.01, -0.01, -0.01, vec).y, point_func(base_point, 0.01, -0.01, -0.01, vec).z);
    glEnd();
}

/*
    ミサイルを描写
*/
void DrawMissoil(xyz base_point, xyz vec){
    glBegin(GL_QUADS); //側面
        // glColor3d(1.0, 0.0, 0.0);
        glColor3d(1.0, 0.49, 0.9);
        //上
        glVertex3d(point_func(base_point, -0.01, 0.01, -0.04, vec).x, point_func(base_point, -0.01, 0.01, -0.04, vec).y, point_func(base_point, -0.01, 0.01, -0.04, vec).z);
        glVertex3d(point_func(base_point, -0.01, 0.01, 0.04, vec).x, point_func(base_point, -0.01, 0.01, 0.04, vec).y, point_func(base_point, -0.01, 0.01, 0.04, vec).z);
        glVertex3d(point_func(base_point, 0.01, 0.01, 0.04, vec).x, point_func(base_point, 0.01, 0.01, 0.04, vec).y, point_func(base_point, 0.01, 0.01, 0.04, vec).z);
        glVertex3d(point_func(base_point, 0.01, 0.01, -0.04, vec).x, point_func(base_point, 0.01, 0.01, -0.04, vec).y, point_func(base_point, 0.01, 0.01, -0.04, vec).z);
        //下
        glVertex3d(point_func(base_point, -0.01, -0.01, -0.04, vec).x, point_func(base_point, -0.01, -0.01, -0.04, vec).y, point_func(base_point, -0.01, -0.01, -0.04, vec).z);
        glVertex3d(point_func(base_point, -0.01, -0.01, 0.04, vec).x, point_func(base_point, -0.01, -0.01, 0.04, vec).y, point_func(base_point, -0.01, -0.01, 0.04, vec).z);
        glVertex3d(point_func(base_point, 0.01, -0.01, 0.04, vec).x, point_func(base_point, 0.01, -0.01, 0.04, vec).y, point_func(base_point, 0.01, -0.01, 0.04, vec).z);
        glVertex3d(point_func(base_point, 0.01, -0.01, -0.04, vec).x, point_func(base_point, 0.01, -0.01, -0.04, vec).y, point_func(base_point, 0.01, -0.01, -0.04, vec).z);
        //右
        glVertex3d(point_func(base_point, 0.01, -0.01, -0.04, vec).x, point_func(base_point, 0.01, -0.01, -0.04, vec).y, point_func(base_point, 0.01, -0.01, -0.04, vec).z);
        glVertex3d(point_func(base_point, 0.01, -0.01, 0.04, vec).x, point_func(base_point, 0.01, -0.01, 0.04, vec).y, point_func(base_point, 0.01, -0.01, 0.04, vec).z);
        glVertex3d(point_func(base_point, 0.01, 0.01, 0.04, vec).x, point_func(base_point, 0.01, 0.01, 0.04, vec).y, point_func(base_point, 0.01, 0.01, 0.04, vec).z);
        glVertex3d(point_func(base_point, 0.01, 0.01, -0.04, vec).x, point_func(base_point, 0.01, 0.01, -0.04, vec).y, point_func(base_point, 0.01, 0.01, -0.04, vec).z);
        //左
        glVertex3d(point_func(base_point, -0.01, -0.01, -0.04, vec).x, point_func(base_point, -0.01, -0.01, -0.04, vec).y, point_func(base_point, -0.01, -0.01, -0.04, vec).z);
        glVertex3d(point_func(base_point, -0.01, -0.01, 0.04, vec).x, point_func(base_point, -0.01, -0.01, 0.04, vec).y, point_func(base_point, -0.01, -0.01, 0.04, vec).z);
        glVertex3d(point_func(base_point, -0.01, 0.01, 0.04, vec).x, point_func(base_point, -0.01, 0.01, 0.04, vec).y, point_func(base_point, -0.01, 0.01, 0.04, vec).z);
        glVertex3d(point_func(base_point, -0.01, 0.01, -0.04, vec).x, point_func(base_point, -0.01, 0.01, -0.04, vec).y, point_func(base_point, -0.01, 0.01, -0.04, vec).z);
        //前
        glVertex3d(point_func(base_point, -0.01, -0.01, 0.04, vec).x, point_func(base_point, -0.01, -0.01, 0.04, vec).y, point_func(base_point, -0.01, -0.01, 0.04, vec).z);
        glVertex3d(point_func(base_point, -0.01, 0.01, 0.04, vec).x, point_func(base_point, -0.01, 0.01, 0.04, vec).y, point_func(base_point, -0.01, 0.01, 0.04, vec).z);
        glVertex3d(point_func(base_point, 0.01, 0.01, 0.04, vec).x, point_func(base_point, 0.01, 0.01, 0.04, vec).y, point_func(base_point, 0.01, 0.01, 0.04, vec).z);
        glVertex3d(point_func(base_point, 0.01, -0.01, 0.04, vec).x, point_func(base_point, 0.01, -0.01, 0.04, vec).y, point_func(base_point, 0.01, -0.01, 0.04, vec).z);
        //後
        glVertex3d(point_func(base_point, -0.01, -0.01, -0.04, vec).x, point_func(base_point, -0.01, -0.01, -0.04, vec).y, point_func(base_point, -0.01, -0.01, -0.04, vec).z);
        glVertex3d(point_func(base_point, -0.01, 0.01, -0.04, vec).x, point_func(base_point, -0.01, 0.01, -0.04, vec).y, point_func(base_point, -0.01, 0.01, -0.04, vec).z);
        glVertex3d(point_func(base_point, 0.01, 0.01, -0.04, vec).x, point_func(base_point, 0.01, 0.01, -0.04, vec).y, point_func(base_point, 0.01, 0.01, -0.04, vec).z);
        glVertex3d(point_func(base_point, 0.01, -0.01, -0.04, vec).x, point_func(base_point, 0.01, -0.01, -0.04, vec).y, point_func(base_point, 0.01, -0.01, -0.04, vec).z);
    glEnd();
}

/*
    地雷を描写
*/
void DrawLandmine(xyz base_point, xyz vec){
    double scale = 0.05;
    glColor3d(0.62, 0.86, 0.0); //ずんだもんカラー
    glBegin(GL_TRIANGLES);
        glVertex3d(point_func(base_point, 2*scale, 0.8*scale, 0*scale, vec).x, point_func(base_point, 2*scale, 0.8*scale, 0*scale, vec).y, point_func(base_point, 2*scale, 0.8*scale, 0*scale, vec).z);
        glVertex3d(point_func(base_point, 1*scale, 0.8*scale, -1.7*scale, vec).x, point_func(base_point, 1*scale, 0.8*scale, -1.7*scale, vec).y, point_func(base_point, 1*scale, 0.8*scale, -1.7*scale, vec).z);
        glVertex3d(point_func(base_point, 0*scale, 0.8*scale, 0*scale, vec).x, point_func(base_point, 0*scale, 0.8*scale, 0*scale, vec).y, point_func(base_point, 0*scale, 0.8*scale, 0*scale, vec).z);
        glVertex3d(point_func(base_point, 1*scale, 0.8*scale, -1.7*scale, vec).x, point_func(base_point, 1*scale, 0.8*scale, -1.7*scale, vec).y, point_func(base_point, 1*scale, 0.8*scale, -1.7*scale, vec).z);
        glVertex3d(point_func(base_point, -1*scale, 0.8*scale, -1.7*scale, vec).x, point_func(base_point, -1*scale, 0.8*scale, -1.7*scale, vec).y, point_func(base_point, -1*scale, 0.8*scale, -1.7*scale, vec).z);
        glVertex3d(point_func(base_point, 0*scale, 0.8*scale, 0*scale, vec).x, point_func(base_point, 0*scale, 0.8*scale, 0*scale, vec).y, point_func(base_point, 0*scale, 0.8*scale, 0*scale, vec).z);
        glVertex3d(point_func(base_point, -1*scale, 0.8*scale, -1.7*scale, vec).x, point_func(base_point, -1*scale, 0.8*scale, -1.7*scale, vec).y, point_func(base_point, -1*scale, 0.8*scale, -1.7*scale, vec).z);
        glVertex3d(point_func(base_point, -2*scale, 0.8*scale, 0*scale, vec).x, point_func(base_point, -2*scale, 0.8*scale, 0*scale, vec).y, point_func(base_point, -2*scale, 0.8*scale, 0*scale, vec).z);
        glVertex3d(point_func(base_point, 0*scale, 0.8*scale, 0*scale, vec).x, point_func(base_point, 0*scale, 0.8*scale, 0*scale, vec).y, point_func(base_point, 0*scale, 0.8*scale, 0*scale, vec).z);
        glVertex3d(point_func(base_point, -2*scale, 0.8*scale, 0*scale, vec).x, point_func(base_point, -2*scale, 0.8*scale, 0*scale, vec).y, point_func(base_point, -2*scale, 0.8*scale, 0*scale, vec).z);
        glVertex3d(point_func(base_point, -1*scale, 0.8*scale, 1.7*scale, vec).x, point_func(base_point, -1*scale, 0.8*scale, 1.7*scale, vec).y, point_func(base_point, -1*scale, 0.8*scale, 1.7*scale, vec).z);
        glVertex3d(point_func(base_point, 0*scale, 0.8*scale, 0*scale, vec).x, point_func(base_point, 0*scale, 0.8*scale, 0*scale, vec).y, point_func(base_point, 0*scale, 0.8*scale, 0*scale, vec).z);
        glVertex3d(point_func(base_point, -1*scale, 0.8*scale, 1.7*scale, vec).x, point_func(base_point, -1*scale, 0.8*scale, 1.7*scale, vec).y, point_func(base_point, -1*scale, 0.8*scale, 1.7*scale, vec).z);
        glVertex3d(point_func(base_point, 1*scale, 0.8*scale, 1.7*scale, vec).x, point_func(base_point, 1*scale, 0.8*scale, 1.7*scale, vec).y, point_func(base_point, 1*scale, 0.8*scale, 1.7*scale, vec).z);
        glVertex3d(point_func(base_point, 0*scale, 0.8*scale, 0*scale, vec).x, point_func(base_point, 0*scale, 0.8*scale, 0*scale, vec).y, point_func(base_point, 0*scale, 0.8*scale, 0*scale, vec).z);
        glVertex3d(point_func(base_point, 1*scale, 0.8*scale, 1.7*scale, vec).x, point_func(base_point, 1*scale, 0.8*scale, 1.7*scale, vec).y, point_func(base_point, 1*scale, 0.8*scale, 1.7*scale, vec).z);
        glVertex3d(point_func(base_point, 2*scale, 0.8*scale, 0*scale, vec).x, point_func(base_point, 2*scale, 0.8*scale, 0*scale, vec).y, point_func(base_point, 2*scale, 0.8*scale, 0*scale, vec).z);
        glVertex3d(point_func(base_point, 0*scale, 0.8*scale, 0*scale, vec).x, point_func(base_point, 0*scale, 0.8*scale, 0*scale, vec).y, point_func(base_point, 0*scale, 0.8*scale, 0*scale, vec).z);

        glVertex3d(point_func(base_point, 2*scale, 0*scale, 0*scale, vec).x, point_func(base_point, 2*scale, 0*scale, 0*scale, vec).y, point_func(base_point, 2*scale, 0*scale, 0*scale, vec).z);
        glVertex3d(point_func(base_point, 1*scale, 0*scale, -1.7*scale, vec).x, point_func(base_point, 1*scale, 0*scale, -1.7*scale, vec).y, point_func(base_point, 1*scale, 0*scale, -1.7*scale, vec).z);
        glVertex3d(point_func(base_point, 0*scale, 0*scale, 0*scale, vec).x, point_func(base_point, 0*scale, 0*scale, 0*scale, vec).y, point_func(base_point, 0*scale, 0*scale, 0*scale, vec).z);
        glVertex3d(point_func(base_point, 1*scale, 0*scale, -1.7*scale, vec).x, point_func(base_point, 1*scale, 0*scale, -1.7*scale, vec).y, point_func(base_point, 1*scale, 0*scale, -1.7*scale, vec).z);
        glVertex3d(point_func(base_point, -1*scale, 0*scale, -1.7*scale, vec).x, point_func(base_point, -1*scale, 0*scale, -1.7*scale, vec).y, point_func(base_point, -1*scale, 0*scale, -1.7*scale, vec).z);
        glVertex3d(point_func(base_point, 0*scale, 0*scale, 0*scale, vec).x, point_func(base_point, 0*scale, 0*scale, 0*scale, vec).y, point_func(base_point, 0*scale, 0*scale, 0*scale, vec).z);
        glVertex3d(point_func(base_point, -1*scale, 0*scale, -1.7*scale, vec).x, point_func(base_point, -1*scale, 0*scale, -1.7*scale, vec).y, point_func(base_point, -1*scale, 0*scale, -1.7*scale, vec).z);
        glVertex3d(point_func(base_point, -2*scale, 0*scale, 0*scale, vec).x, point_func(base_point, -2*scale, 0*scale, 0*scale, vec).y, point_func(base_point, -2*scale, 0*scale, 0*scale, vec).z);
        glVertex3d(point_func(base_point, 0*scale, 0*scale, 0*scale, vec).x, point_func(base_point, 0*scale, 0*scale, 0*scale, vec).y, point_func(base_point, 0*scale, 0*scale, 0*scale, vec).z);
        glVertex3d(point_func(base_point, -2*scale, 0*scale, 0*scale, vec).x, point_func(base_point, -2*scale, 0*scale, 0*scale, vec).y, point_func(base_point, -2*scale, 0*scale, 0*scale, vec).z);
        glVertex3d(point_func(base_point, -1*scale, 0*scale, 1.7*scale, vec).x, point_func(base_point, -1*scale, 0*scale, 1.7*scale, vec).y, point_func(base_point, -1*scale, 0*scale, 1.7*scale, vec).z);
        glVertex3d(point_func(base_point, 0*scale, 0*scale, 0*scale, vec).x, point_func(base_point, 0*scale, 0*scale, 0*scale, vec).y, point_func(base_point, 0*scale, 0*scale, 0*scale, vec).z);
        glVertex3d(point_func(base_point, -1*scale, 0*scale, 1.7*scale, vec).x, point_func(base_point, -1*scale, 0*scale, 1.7*scale, vec).y, point_func(base_point, -1*scale, 0*scale, 1.7*scale, vec).z);
        glVertex3d(point_func(base_point, 1*scale, 0*scale, 1.7*scale, vec).x, point_func(base_point, 1*scale, 0*scale, 1.7*scale, vec).y, point_func(base_point, 1*scale, 0*scale, 1.7*scale, vec).z);
        glVertex3d(point_func(base_point, 0*scale, 0*scale, 0*scale, vec).x, point_func(base_point, 0*scale, 0*scale, 0*scale, vec).y, point_func(base_point, 0*scale, 0*scale, 0*scale, vec).z);
        glVertex3d(point_func(base_point, 1*scale, 0*scale, 1.7*scale, vec).x, point_func(base_point, 1*scale, 0*scale, 1.7*scale, vec).y, point_func(base_point, 1*scale, 0*scale, 1.7*scale, vec).z);
        glVertex3d(point_func(base_point, 2*scale, 0*scale, 0*scale, vec).x, point_func(base_point, 2*scale, 0*scale, 0*scale, vec).y, point_func(base_point, 2*scale, 0*scale, 0*scale, vec).z);
        glVertex3d(point_func(base_point, 0*scale, 0*scale, 0*scale, vec).x, point_func(base_point, 0*scale, 0*scale, 0*scale, vec).y, point_func(base_point, 0*scale, 0*scale, 0*scale, vec).z);
    glEnd();

    glBegin(GL_QUADS);
        glVertex3d(point_func(base_point, 2*scale, 0.8*scale, 0*scale, vec).x, point_func(base_point, 2*scale, 0.8*scale, 0*scale, vec).y, point_func(base_point, 2*scale, 0.8*scale, 0*scale, vec).z);
        glVertex3d(point_func(base_point, 1*scale, 0.8*scale, -1.7*scale, vec).x, point_func(base_point, 1*scale, 0.8*scale, -1.7*scale, vec).y, point_func(base_point, 1*scale, 0.8*scale, -1.7*scale, vec).z);
        glVertex3d(point_func(base_point, 1*scale, 0*scale, -1.7*scale, vec).x, point_func(base_point, 1*scale, 0*scale, -1.7*scale, vec).y, point_func(base_point, 1*scale, 0*scale, -1.7*scale, vec).z);
        glVertex3d(point_func(base_point, 2*scale, 0*scale, 0*scale, vec).x, point_func(base_point, 2*scale, 0*scale, 0*scale, vec).y, point_func(base_point, 2*scale, 0*scale, 0*scale, vec).z);
        glVertex3d(point_func(base_point, 1*scale, 0.8*scale, -1.7*scale, vec).x, point_func(base_point, 1*scale, 0.8*scale, -1.7*scale, vec).y, point_func(base_point, 1*scale, 0.8*scale, -1.7*scale, vec).z);
        glVertex3d(point_func(base_point, -1*scale, 0.8*scale, -1.7*scale, vec).x, point_func(base_point, -1*scale, 0.8*scale, -1.7*scale, vec).y, point_func(base_point, -1*scale, 0.8*scale, -1.7*scale, vec).z);
        glVertex3d(point_func(base_point, -1*scale, 0*scale, -1.7*scale, vec).x, point_func(base_point, -1*scale, 0*scale, -1.7*scale, vec).y, point_func(base_point, -1*scale, 0*scale, -1.7*scale, vec).z);
        glVertex3d(point_func(base_point, 1*scale, 0*scale, -1.7*scale, vec).x, point_func(base_point, 1*scale, 0*scale, -1.7*scale, vec).y, point_func(base_point, 1*scale, 0*scale, -1.7*scale, vec).z);
        glVertex3d(point_func(base_point, -1*scale, 0.8*scale, -1.7*scale, vec).x, point_func(base_point, -1*scale, 0.8*scale, -1.7*scale, vec).y, point_func(base_point, -1*scale, 0.8*scale, -1.7*scale, vec).z);
        glVertex3d(point_func(base_point, -2*scale, 0.8*scale, 0*scale, vec).x, point_func(base_point, -2*scale, 0.8*scale, 0*scale, vec).y, point_func(base_point, -2*scale, 0.8*scale, 0*scale, vec).z);
        glVertex3d(point_func(base_point, -2*scale, 0*scale, 0*scale, vec).x, point_func(base_point, -2*scale, 0*scale, 0*scale, vec).y, point_func(base_point, -2*scale, 0*scale, 0*scale, vec).z);
        glVertex3d(point_func(base_point, -1*scale, 0*scale, -1.7*scale, vec).x, point_func(base_point, -1*scale, 0*scale, -1.7*scale, vec).y, point_func(base_point, -1*scale, 0*scale, -1.7*scale, vec).z);
        glVertex3d(point_func(base_point, -2*scale, 0.8*scale, 0*scale, vec).x, point_func(base_point, -2*scale, 0.8*scale, 0*scale, vec).y, point_func(base_point, -2*scale, 0.8*scale, 0*scale, vec).z);
        glVertex3d(point_func(base_point, -1*scale, 0.8*scale, 1.7*scale, vec).x, point_func(base_point, -1*scale, 0.8*scale, 1.7*scale, vec).y, point_func(base_point, -1*scale, 0.8*scale, 1.7*scale, vec).z);
        glVertex3d(point_func(base_point, -1*scale, 0*scale, 1.7*scale, vec).x, point_func(base_point, -1*scale, 0*scale, 1.7*scale, vec).y, point_func(base_point, -1*scale, 0*scale, 1.7*scale, vec).z);
        glVertex3d(point_func(base_point, -2*scale, 0*scale, 0*scale, vec).x, point_func(base_point, -2*scale, 0*scale, 0*scale, vec).y, point_func(base_point, -2*scale, 0*scale, 0*scale, vec).z);
        glVertex3d(point_func(base_point, -1*scale, 0.8*scale, 1.7*scale, vec).x, point_func(base_point, -1*scale, 0.8*scale, 1.7*scale, vec).y, point_func(base_point, -1*scale, 0.8*scale, 1.7*scale, vec).z);
        glVertex3d(point_func(base_point, 1*scale, 0.8*scale, 1.7*scale, vec).x, point_func(base_point, 1*scale, 0.8*scale, 1.7*scale, vec).y, point_func(base_point, 1*scale, 0.8*scale, 1.7*scale, vec).z);
        glVertex3d(point_func(base_point, 1*scale, 0*scale, 1.7*scale, vec).x, point_func(base_point, 1*scale, 0*scale, 1.7*scale, vec).y, point_func(base_point, 1*scale, 0*scale, 1.7*scale, vec).z);
        glVertex3d(point_func(base_point, -1*scale, 0*scale, 1.7*scale, vec).x, point_func(base_point, -1*scale, 0*scale, 1.7*scale, vec).y, point_func(base_point, -1*scale, 0*scale, 1.7*scale, vec).z);
        glVertex3d(point_func(base_point, 1*scale, 0.8*scale, 1.7*scale, vec).x, point_func(base_point, 1*scale, 0.8*scale, 1.7*scale, vec).y, point_func(base_point, 1*scale, 0.8*scale, 1.7*scale, vec).z);
        glVertex3d(point_func(base_point, 2*scale, 0.8*scale, 0*scale, vec).x, point_func(base_point, 2*scale, 0.8*scale, 0*scale, vec).y, point_func(base_point, 2*scale, 0.8*scale, 0*scale, vec).z);
        glVertex3d(point_func(base_point, 2*scale, 0*scale, 0*scale, vec).x, point_func(base_point, 2*scale, 0*scale, 0*scale, vec).y, point_func(base_point, 2*scale, 0*scale, 0*scale, vec).z);
        glVertex3d(point_func(base_point, 1*scale, 0*scale, 1.7*scale, vec).x, point_func(base_point, 1*scale, 0*scale, 1.7*scale, vec).y, point_func(base_point, 1*scale, 0*scale, 1.7*scale, vec).z);
    glEnd();

    glColor3d(1.0, 1.0, 1.0); //ずんだもんカラー
    glBegin(GL_TRIANGLES);
        glVertex3d(point_func(base_point, 2*scale, -0.8*scale, 0*scale, vec).x, point_func(base_point, 2*scale, -0.8*scale, 0*scale, vec).y, point_func(base_point, 2*scale, -0.8*scale, 0*scale, vec).z);
        glVertex3d(point_func(base_point, 1*scale, -0.8*scale, -1.7*scale, vec).x, point_func(base_point, 1*scale, -0.8*scale, -1.7*scale, vec).y, point_func(base_point, 1*scale, -0.8*scale, -1.7*scale, vec).z);
        glVertex3d(point_func(base_point, 0*scale, -0.8*scale, 0*scale, vec).x, point_func(base_point, 0*scale, -0.8*scale, 0*scale, vec).y, point_func(base_point, 0*scale, -0.8*scale, 0*scale, vec).z);
        glVertex3d(point_func(base_point, 1*scale, -0.8*scale, -1.7*scale, vec).x, point_func(base_point, 1*scale, -0.8*scale, -1.7*scale, vec).y, point_func(base_point, 1*scale, -0.8*scale, -1.7*scale, vec).z);
        glVertex3d(point_func(base_point, -1*scale, -0.8*scale, -1.7*scale, vec).x, point_func(base_point, -1*scale, -0.8*scale, -1.7*scale, vec).y, point_func(base_point, -1*scale, -0.8*scale, -1.7*scale, vec).z);
        glVertex3d(point_func(base_point, 0*scale, -0.8*scale, 0*scale, vec).x, point_func(base_point, 0*scale, -0.8*scale, 0*scale, vec).y, point_func(base_point, 0*scale, -0.8*scale, 0*scale, vec).z);
        glVertex3d(point_func(base_point, -1*scale, -0.8*scale, -1.7*scale, vec).x, point_func(base_point, -1*scale, -0.8*scale, -1.7*scale, vec).y, point_func(base_point, -1*scale, -0.8*scale, -1.7*scale, vec).z);
        glVertex3d(point_func(base_point, -2*scale, -0.8*scale, 0*scale, vec).x, point_func(base_point, -2*scale, -0.8*scale, 0*scale, vec).y, point_func(base_point, -2*scale, -0.8*scale, 0*scale, vec).z);
        glVertex3d(point_func(base_point, 0*scale, -0.8*scale, 0*scale, vec).x, point_func(base_point, 0*scale, -0.8*scale, 0*scale, vec).y, point_func(base_point, 0*scale, -0.8*scale, 0*scale, vec).z);
        glVertex3d(point_func(base_point, -2*scale, -0.8*scale, 0*scale, vec).x, point_func(base_point, -2*scale, -0.8*scale, 0*scale, vec).y, point_func(base_point, -2*scale, -0.8*scale, 0*scale, vec).z);
        glVertex3d(point_func(base_point, -1*scale, -0.8*scale, 1.7*scale, vec).x, point_func(base_point, -1*scale, -0.8*scale, 1.7*scale, vec).y, point_func(base_point, -1*scale, -0.8*scale, 1.7*scale, vec).z);
        glVertex3d(point_func(base_point, 0*scale, -0.8*scale, 0*scale, vec).x, point_func(base_point, 0*scale, -0.8*scale, 0*scale, vec).y, point_func(base_point, 0*scale, -0.8*scale, 0*scale, vec).z);
        glVertex3d(point_func(base_point, -1*scale, -0.8*scale, 1.7*scale, vec).x, point_func(base_point, -1*scale, -0.8*scale, 1.7*scale, vec).y, point_func(base_point, -1*scale, -0.8*scale, 1.7*scale, vec).z);
        glVertex3d(point_func(base_point, 1*scale, -0.8*scale, 1.7*scale, vec).x, point_func(base_point, 1*scale, -0.8*scale, 1.7*scale, vec).y, point_func(base_point, 1*scale, -0.8*scale, 1.7*scale, vec).z);
        glVertex3d(point_func(base_point, 0*scale, -0.8*scale, 0*scale, vec).x, point_func(base_point, 0*scale, -0.8*scale, 0*scale, vec).y, point_func(base_point, 0*scale, -0.8*scale, 0*scale, vec).z);
        glVertex3d(point_func(base_point, 1*scale, -0.8*scale, 1.7*scale, vec).x, point_func(base_point, 1*scale, -0.8*scale, 1.7*scale, vec).y, point_func(base_point, 1*scale, -0.8*scale, 1.7*scale, vec).z);
        glVertex3d(point_func(base_point, 2*scale, -0.8*scale, 0*scale, vec).x, point_func(base_point, 2*scale, -0.8*scale, 0*scale, vec).y, point_func(base_point, 2*scale, -0.8*scale, 0*scale, vec).z);
        glVertex3d(point_func(base_point, 0*scale, -0.8*scale, 0*scale, vec).x, point_func(base_point, 0*scale, -0.8*scale, 0*scale, vec).y, point_func(base_point, 0*scale, -0.8*scale, 0*scale, vec).z);

        glVertex3d(point_func(base_point, 2*scale, 0*scale, 0*scale, vec).x, point_func(base_point, 2*scale, 0*scale, 0*scale, vec).y, point_func(base_point, 2*scale, 0*scale, 0*scale, vec).z);
        glVertex3d(point_func(base_point, 1*scale, 0*scale, -1.7*scale, vec).x, point_func(base_point, 1*scale, 0*scale, -1.7*scale, vec).y, point_func(base_point, 1*scale, 0*scale, -1.7*scale, vec).z);
        glVertex3d(point_func(base_point, 0*scale, 0*scale, 0*scale, vec).x, point_func(base_point, 0*scale, 0*scale, 0*scale, vec).y, point_func(base_point, 0*scale, 0*scale, 0*scale, vec).z);
        glVertex3d(point_func(base_point, 1*scale, 0*scale, -1.7*scale, vec).x, point_func(base_point, 1*scale, 0*scale, -1.7*scale, vec).y, point_func(base_point, 1*scale, 0*scale, -1.7*scale, vec).z);
        glVertex3d(point_func(base_point, -1*scale, 0*scale, -1.7*scale, vec).x, point_func(base_point, -1*scale, 0*scale, -1.7*scale, vec).y, point_func(base_point, -1*scale, 0*scale, -1.7*scale, vec).z);
        glVertex3d(point_func(base_point, 0*scale, 0*scale, 0*scale, vec).x, point_func(base_point, 0*scale, 0*scale, 0*scale, vec).y, point_func(base_point, 0*scale, 0*scale, 0*scale, vec).z);
        glVertex3d(point_func(base_point, -1*scale, 0*scale, -1.7*scale, vec).x, point_func(base_point, -1*scale, 0*scale, -1.7*scale, vec).y, point_func(base_point, -1*scale, 0*scale, -1.7*scale, vec).z);
        glVertex3d(point_func(base_point, -2*scale, 0*scale, 0*scale, vec).x, point_func(base_point, -2*scale, 0*scale, 0*scale, vec).y, point_func(base_point, -2*scale, 0*scale, 0*scale, vec).z);
        glVertex3d(point_func(base_point, 0*scale, 0*scale, 0*scale, vec).x, point_func(base_point, 0*scale, 0*scale, 0*scale, vec).y, point_func(base_point, 0*scale, 0*scale, 0*scale, vec).z);
        glVertex3d(point_func(base_point, -2*scale, 0*scale, 0*scale, vec).x, point_func(base_point, -2*scale, 0*scale, 0*scale, vec).y, point_func(base_point, -2*scale, 0*scale, 0*scale, vec).z);
        glVertex3d(point_func(base_point, -1*scale, 0*scale, 1.7*scale, vec).x, point_func(base_point, -1*scale, 0*scale, 1.7*scale, vec).y, point_func(base_point, -1*scale, 0*scale, 1.7*scale, vec).z);
        glVertex3d(point_func(base_point, 0*scale, 0*scale, 0*scale, vec).x, point_func(base_point, 0*scale, 0*scale, 0*scale, vec).y, point_func(base_point, 0*scale, 0*scale, 0*scale, vec).z);
        glVertex3d(point_func(base_point, -1*scale, 0*scale, 1.7*scale, vec).x, point_func(base_point, -1*scale, 0*scale, 1.7*scale, vec).y, point_func(base_point, -1*scale, 0*scale, 1.7*scale, vec).z);
        glVertex3d(point_func(base_point, 1*scale, 0*scale, 1.7*scale, vec).x, point_func(base_point, 1*scale, 0*scale, 1.7*scale, vec).y, point_func(base_point, 1*scale, 0*scale, 1.7*scale, vec).z);
        glVertex3d(point_func(base_point, 0*scale, 0*scale, 0*scale, vec).x, point_func(base_point, 0*scale, 0*scale, 0*scale, vec).y, point_func(base_point, 0*scale, 0*scale, 0*scale, vec).z);
        glVertex3d(point_func(base_point, 1*scale, 0*scale, 1.7*scale, vec).x, point_func(base_point, 1*scale, 0*scale, 1.7*scale, vec).y, point_func(base_point, 1*scale, 0*scale, 1.7*scale, vec).z);
        glVertex3d(point_func(base_point, 2*scale, 0*scale, 0*scale, vec).x, point_func(base_point, 2*scale, 0*scale, 0*scale, vec).y, point_func(base_point, 2*scale, 0*scale, 0*scale, vec).z);
        glVertex3d(point_func(base_point, 0*scale, 0*scale, 0*scale, vec).x, point_func(base_point, 0*scale, 0*scale, 0*scale, vec).y, point_func(base_point, 0*scale, 0*scale, 0*scale, vec).z);
    glEnd();

    glBegin(GL_QUADS);
        glVertex3d(point_func(base_point, 2*scale, -0.8*scale, 0*scale, vec).x, point_func(base_point, 2*scale, -0.8*scale, 0*scale, vec).y, point_func(base_point, 2*scale, -0.8*scale, 0*scale, vec).z);
        glVertex3d(point_func(base_point, 1*scale, -0.8*scale, -1.7*scale, vec).x, point_func(base_point, 1*scale, -0.8*scale, -1.7*scale, vec).y, point_func(base_point, 1*scale, -0.8*scale, -1.7*scale, vec).z);
        glVertex3d(point_func(base_point, 1*scale, 0*scale, -1.7*scale, vec).x, point_func(base_point, 1*scale, 0*scale, -1.7*scale, vec).y, point_func(base_point, 1*scale, 0*scale, -1.7*scale, vec).z);
        glVertex3d(point_func(base_point, 2*scale, 0*scale, 0*scale, vec).x, point_func(base_point, 2*scale, 0*scale, 0*scale, vec).y, point_func(base_point, 2*scale, 0*scale, 0*scale, vec).z);
        glVertex3d(point_func(base_point, 1*scale, -0.8*scale, -1.7*scale, vec).x, point_func(base_point, 1*scale, -0.8*scale, -1.7*scale, vec).y, point_func(base_point, 1*scale, -0.8*scale, -1.7*scale, vec).z);
        glVertex3d(point_func(base_point, -1*scale, -0.8*scale, -1.7*scale, vec).x, point_func(base_point, -1*scale, -0.8*scale, -1.7*scale, vec).y, point_func(base_point, -1*scale, -0.8*scale, -1.7*scale, vec).z);
        glVertex3d(point_func(base_point, -1*scale, 0*scale, -1.7*scale, vec).x, point_func(base_point, -1*scale, 0*scale, -1.7*scale, vec).y, point_func(base_point, -1*scale, 0*scale, -1.7*scale, vec).z);
        glVertex3d(point_func(base_point, 1*scale, 0*scale, -1.7*scale, vec).x, point_func(base_point, 1*scale, 0*scale, -1.7*scale, vec).y, point_func(base_point, 1*scale, 0*scale, -1.7*scale, vec).z);
        glVertex3d(point_func(base_point, -1*scale, -0.8*scale, -1.7*scale, vec).x, point_func(base_point, -1*scale, -0.8*scale, -1.7*scale, vec).y, point_func(base_point, -1*scale, -0.8*scale, -1.7*scale, vec).z);
        glVertex3d(point_func(base_point, -2*scale, -0.8*scale, 0*scale, vec).x, point_func(base_point, -2*scale, -0.8*scale, 0*scale, vec).y, point_func(base_point, -2*scale, -0.8*scale, 0*scale, vec).z);
        glVertex3d(point_func(base_point, -2*scale, 0*scale, 0*scale, vec).x, point_func(base_point, -2*scale, 0*scale, 0*scale, vec).y, point_func(base_point, -2*scale, 0*scale, 0*scale, vec).z);
        glVertex3d(point_func(base_point, -1*scale, 0*scale, -1.7*scale, vec).x, point_func(base_point, -1*scale, 0*scale, -1.7*scale, vec).y, point_func(base_point, -1*scale, 0*scale, -1.7*scale, vec).z);
        glVertex3d(point_func(base_point, -2*scale, -0.8*scale, 0*scale, vec).x, point_func(base_point, -2*scale, -0.8*scale, 0*scale, vec).y, point_func(base_point, -2*scale, -0.8*scale, 0*scale, vec).z);
        glVertex3d(point_func(base_point, -1*scale, -0.8*scale, 1.7*scale, vec).x, point_func(base_point, -1*scale, -0.8*scale, 1.7*scale, vec).y, point_func(base_point, -1*scale, -0.8*scale, 1.7*scale, vec).z);
        glVertex3d(point_func(base_point, -1*scale, 0*scale, 1.7*scale, vec).x, point_func(base_point, -1*scale, 0*scale, 1.7*scale, vec).y, point_func(base_point, -1*scale, 0*scale, 1.7*scale, vec).z);
        glVertex3d(point_func(base_point, -2*scale, 0*scale, 0*scale, vec).x, point_func(base_point, -2*scale, 0*scale, 0*scale, vec).y, point_func(base_point, -2*scale, 0*scale, 0*scale, vec).z);
        glVertex3d(point_func(base_point, -1*scale, -0.8*scale, 1.7*scale, vec).x, point_func(base_point, -1*scale, -0.8*scale, 1.7*scale, vec).y, point_func(base_point, -1*scale, -0.8*scale, 1.7*scale, vec).z);
        glVertex3d(point_func(base_point, 1*scale, -0.8*scale, 1.7*scale, vec).x, point_func(base_point, 1*scale, -0.8*scale, 1.7*scale, vec).y, point_func(base_point, 1*scale, -0.8*scale, 1.7*scale, vec).z);
        glVertex3d(point_func(base_point, 1*scale, 0*scale, 1.7*scale, vec).x, point_func(base_point, 1*scale, 0*scale, 1.7*scale, vec).y, point_func(base_point, 1*scale, 0*scale, 1.7*scale, vec).z);
        glVertex3d(point_func(base_point, -1*scale, 0*scale, 1.7*scale, vec).x, point_func(base_point, -1*scale, 0*scale, 1.7*scale, vec).y, point_func(base_point, -1*scale, 0*scale, 1.7*scale, vec).z);
        glVertex3d(point_func(base_point, 1*scale, -0.8*scale, 1.7*scale, vec).x, point_func(base_point, 1*scale, -0.8*scale, 1.7*scale, vec).y, point_func(base_point, 1*scale, -0.8*scale, 1.7*scale, vec).z);
        glVertex3d(point_func(base_point, 2*scale, -0.8*scale, 0*scale, vec).x, point_func(base_point, 2*scale, -0.8*scale, 0*scale, vec).y, point_func(base_point, 2*scale, -0.8*scale, 0*scale, vec).z);
        glVertex3d(point_func(base_point, 2*scale, 0*scale, 0*scale, vec).x, point_func(base_point, 2*scale, 0*scale, 0*scale, vec).y, point_func(base_point, 2*scale, 0*scale, 0*scale, vec).z);
        glVertex3d(point_func(base_point, 1*scale, 0*scale, 1.7*scale, vec).x, point_func(base_point, 1*scale, 0*scale, 1.7*scale, vec).y, point_func(base_point, 1*scale, 0*scale, 1.7*scale, vec).z);
    glEnd();
}

/*
    鍵を描写
*/
void DrawKey(xyz base_point, xyz vec, int type){
    double scale;
    if (type == 0){ //持ってる場合
        scale = 0.05;
    }
    if (type == 1){
        scale = 0.3;
    }
    
    glBegin(GL_QUADS);
        //黄色（Yellow）
        glColor3d(1.0, 1.0, 0.0);

        glVertex3d(point_func(base_point, 0*scale, 1.7*scale, 0*scale, vec).x, point_func(base_point, 0*scale, 1.7*scale, 0*scale, vec).y, point_func(base_point, 0*scale, 1.7*scale, 0*scale, vec).z);
        glVertex3d(point_func(base_point, 0.7*scale, 1.2*scale, 0*scale, vec).x, point_func(base_point, 0.7*scale, 1.2*scale, 0*scale, vec).y, point_func(base_point, 0.7*scale, 1.2*scale, 0*scale, vec).z);
        glVertex3d(point_func(base_point, 0.3*scale, 1.1*scale, 0*scale, vec).x, point_func(base_point, 0.3*scale, 1.1*scale, 0*scale, vec).y, point_func(base_point, 0.3*scale, 1.1*scale, 0*scale, vec).z);
        glVertex3d(point_func(base_point, 0*scale, 1.25*scale, 0*scale, vec).x, point_func(base_point, 0*scale, 1.25*scale, 0*scale, vec).y, point_func(base_point, 0*scale, 1.25*scale, 0*scale, vec).z);

        glVertex3d(point_func(base_point, 0.7*scale, 1.2*scale, 0*scale, vec).x, point_func(base_point, 0.7*scale, 1.2*scale, 0*scale, vec).y, point_func(base_point, 0.7*scale, 1.2*scale, 0*scale, vec).z);
        glVertex3d(point_func(base_point, 0.7*scale, 0.3*scale, 0*scale, vec).x, point_func(base_point, 0.7*scale, 0.3*scale, 0*scale, vec).y, point_func(base_point, 0.7*scale, 0.3*scale, 0*scale, vec).z);
        glVertex3d(point_func(base_point, 0.3*scale, 0.6*scale, 0*scale, vec).x, point_func(base_point, 0.3*scale, 0.6*scale, 0*scale, vec).y, point_func(base_point, 0.3*scale, 0.6*scale, 0*scale, vec).z);
        glVertex3d(point_func(base_point, 0.3*scale, 1.1*scale, 0*scale, vec).x, point_func(base_point, 0.3*scale, 1.1*scale, 0*scale, vec).y, point_func(base_point, 0.3*scale, 1.1*scale, 0*scale, vec).z);

        glVertex3d(point_func(base_point, 0.7*scale, 0.3*scale, 0*scale, vec).x, point_func(base_point, 0.7*scale, 0.3*scale, 0*scale, vec).y, point_func(base_point, 0.7*scale, 0.3*scale, 0*scale, vec).z);
        glVertex3d(point_func(base_point, 0.3*scale, 0*scale, 0*scale, vec).x, point_func(base_point, 0.3*scale, 0*scale, 0*scale, vec).y, point_func(base_point, 0.3*scale, 0*scale, 0*scale, vec).z);
        glVertex3d(point_func(base_point, 0.01*scale, 0.55*scale, 0*scale, vec).x, point_func(base_point, 0.01*scale, 0.55*scale, 0*scale, vec).y, point_func(base_point, 0.01*scale, 0.55*scale, 0*scale, vec).z);
        glVertex3d(point_func(base_point, 0.3*scale, 0.7*scale, 0*scale, vec).x, point_func(base_point, 0.3*scale, 0.7*scale, 0*scale, vec).y, point_func(base_point, 0.3*scale, 0.7*scale, 0*scale, vec).z);

        glVertex3d(point_func(base_point, 0.3*scale, 0*scale, 0*scale, vec).x, point_func(base_point, 0.3*scale, 0*scale, 0*scale, vec).y, point_func(base_point, 0.3*scale, 0*scale, 0*scale, vec).z);
        glVertex3d(point_func(base_point, 0.01*scale, 0.55*scale, 0*scale, vec).x, point_func(base_point, 0.01*scale, 0.55*scale, 0*scale, vec).y, point_func(base_point, 0.01*scale, 0.55*scale, 0*scale, vec).z);
        glVertex3d(point_func(base_point, -0.01*scale, 0.55*scale, 0*scale, vec).x, point_func(base_point, -0.01*scale, 0.55*scale, 0*scale, vec).y, point_func(base_point, -0.01*scale, 0.55*scale, 0*scale, vec).z);
        glVertex3d(point_func(base_point, -0.3*scale, 0*scale, 0*scale, vec).x, point_func(base_point, -0.3*scale, 0*scale, 0*scale, vec).y, point_func(base_point, -0.3*scale, 0*scale, 0*scale, vec).z);

        glVertex3d(point_func(base_point, 0*scale, 1.7*scale, 0*scale, vec).x, point_func(base_point, 0*scale, 1.7*scale, 0*scale, vec).y, point_func(base_point, 0*scale, 1.7*scale, 0*scale, vec).z);
        glVertex3d(point_func(base_point, -0.7*scale, 1.2*scale, 0*scale, vec).x, point_func(base_point, -0.7*scale, 1.2*scale, 0*scale, vec).y, point_func(base_point, -0.7*scale, 1.2*scale, 0*scale, vec).z);
        glVertex3d(point_func(base_point, -0.3*scale, 1.1*scale, 0*scale, vec).x, point_func(base_point, -0.3*scale, 1.1*scale, 0*scale, vec).y, point_func(base_point, -0.3*scale, 1.1*scale, 0*scale, vec).z);
        glVertex3d(point_func(base_point, 0*scale, 1.25*scale, 0*scale, vec).x, point_func(base_point, 0*scale, 1.25*scale, 0*scale, vec).y, point_func(base_point, 0*scale, 1.25*scale, 0*scale, vec).z);

        glVertex3d(point_func(base_point, -0.7*scale, 1.2*scale, 0*scale, vec).x, point_func(base_point, -0.7*scale, 1.2*scale, 0*scale, vec).y, point_func(base_point, -0.7*scale, 1.2*scale, 0*scale, vec).z);
        glVertex3d(point_func(base_point, -0.7*scale, 0.3*scale, 0*scale, vec).x, point_func(base_point, -0.7*scale, 0.3*scale, 0*scale, vec).y, point_func(base_point, -0.7*scale, 0.3*scale, 0*scale, vec).z);
        glVertex3d(point_func(base_point, -0.3*scale, 0.6*scale, 0*scale, vec).x, point_func(base_point, -0.3*scale, 0.6*scale, 0*scale, vec).y, point_func(base_point, -0.3*scale, 0.6*scale, 0*scale, vec).z);
        glVertex3d(point_func(base_point, -0.3*scale, 1.1*scale, 0*scale, vec).x, point_func(base_point, -0.3*scale, 1.1*scale, 0*scale, vec).y, point_func(base_point, -0.3*scale, 1.1*scale, 0*scale, vec).z);

        glVertex3d(point_func(base_point, -0.7*scale, 0.3*scale, 0*scale, vec).x, point_func(base_point, -0.7*scale, 0.3*scale, 0*scale, vec).y, point_func(base_point, -0.7*scale, 0.3*scale, 0*scale, vec).z);
        glVertex3d(point_func(base_point, -0.3*scale, 0*scale, 0*scale, vec).x, point_func(base_point, -0.3*scale, 0*scale, 0*scale, vec).y, point_func(base_point, -0.3*scale, 0*scale, 0*scale, vec).z);
        glVertex3d(point_func(base_point, -0.01*scale, 0.55*scale, 0*scale, vec).x, point_func(base_point, -0.01*scale, 0.55*scale, 0*scale, vec).y, point_func(base_point, -0.01*scale, 0.55*scale, 0*scale, vec).z);
        glVertex3d(point_func(base_point, -0.3*scale, 0.7*scale, 0*scale, vec).x, point_func(base_point, -0.3*scale, 0.7*scale, 0*scale, vec).y, point_func(base_point, -0.3*scale, 0.7*scale, 0*scale, vec).z);



        glVertex3d(point_func(base_point, 0.3*scale, 0*scale, 0*scale, vec).x, point_func(base_point, 0.3*scale, 0*scale, 0*scale, vec).y, point_func(base_point, 0.3*scale, 0*scale, 0*scale, vec).z);
        glVertex3d(point_func(base_point, 0.3*scale, -1.9*scale, 0*scale, vec).x, point_func(base_point, 0.3*scale, -1.9*scale, 0*scale, vec).y, point_func(base_point, 0.3*scale, -1.9*scale, 0*scale, vec).z);
        glVertex3d(point_func(base_point, -0.3*scale, -1.9*scale, 0*scale, vec).x, point_func(base_point, -0.3*scale, -1.9*scale, 0*scale, vec).y, point_func(base_point, -0.3*scale, -1.9*scale, 0*scale, vec).z);
        glVertex3d(point_func(base_point, -0.3*scale, 0*scale, 0*scale, vec).x, point_func(base_point, -0.3*scale, 0*scale, 0*scale, vec).y, point_func(base_point, -0.3*scale, 0*scale, 0*scale, vec).z);

        glVertex3d(point_func(base_point, 0.3*scale, -0.6*scale, 0*scale, vec).x, point_func(base_point, 0.3*scale, -0.6*scale, 0*scale, vec).y, point_func(base_point, 0.3*scale, -0.6*scale, 0*scale, vec).z);
        glVertex3d(point_func(base_point, 1*scale, -0.6*scale, 0*scale, vec).x, point_func(base_point, 1*scale, -0.6*scale, 0*scale, vec).y, point_func(base_point, 1*scale, -0.6*scale, 0*scale, vec).z);
        glVertex3d(point_func(base_point, 1*scale, -0.85*scale, 0*scale, vec).x, point_func(base_point, 1*scale, -0.85*scale, 0*scale, vec).y, point_func(base_point, 1*scale, -0.85*scale, 0*scale, vec).z);
        glVertex3d(point_func(base_point, 0.3*scale, -0.85*scale, 0*scale, vec).x, point_func(base_point, 0.3*scale, -0.85*scale, 0*scale, vec).y, point_func(base_point, 0.3*scale, -0.85*scale, 0*scale, vec).z);

        glVertex3d(point_func(base_point, 0.3*scale, -1.05*scale, 0*scale, vec).x, point_func(base_point, 0.3*scale, -1.05*scale, 0*scale, vec).y, point_func(base_point, 0.3*scale, -1.05*scale, 0*scale, vec).z);
        glVertex3d(point_func(base_point, 1*scale, -1.05*scale, 0*scale, vec).x, point_func(base_point, 1*scale, -1.05*scale, 0*scale, vec).y, point_func(base_point, 1*scale, -1.05*scale, 0*scale, vec).z);
        glVertex3d(point_func(base_point, 1*scale, -1.3*scale, 0*scale, vec).x, point_func(base_point, 1*scale, -1.3*scale, 0*scale, vec).y, point_func(base_point, 1*scale, -1.3*scale, 0*scale, vec).z);
        glVertex3d(point_func(base_point, 0.3*scale, -1.3*scale, 0*scale, vec).x, point_func(base_point, 0.3*scale, -1.3*scale, 0*scale, vec).y, point_func(base_point, 0.3*scale, -1.3*scale, 0*scale, vec).z);


        glEnd();
}

/*
    アイテムたちを描写
*/
void DrawItems(void){
    for (int i = 0; i < gGame.player_num * item_num_init; i++){
        if (item_info_all[i].see == SDL_TRUE){
            switch (item_info_all[i].kind){
            case 1: //弾丸
                DrawAmmo(item_info_all[i].point, item_info_all[i].vec);
                break;
            case 2: //ミサイル
                DrawMissoil(item_info_all[i].point, item_info_all[i].vec);
                break;
            case 3: //地雷
                DrawLandmine(item_info_all[i].point, item_info_all[i].vec);
            default:
                break;
            }
        }
    }

    //鍵
    //敵のカギ、ゲットだぜ
    if (gGame.player_ID < player_num_init/2) { //チーム1
		if (item_info_all[player_num_init * item_num_init + 1].target == gGame.player_ID){
			item_info_all[player_num_init * item_num_init + 1].see = SDL_TRUE;
		}
	}
    else { //チーム2
		if (item_info_all[player_num_init * item_num_init + 0].target == gGame.player_ID){
			item_info_all[player_num_init * item_num_init + 0].see = SDL_TRUE;
		}
	}

    //鍵落ちてる
    if (item_info_all[item_num_init * player_num_init + 0].see == SDL_TRUE){
        if (item_info_all[player_num_init * item_num_init + 0].target == gGame.player_ID){ //所持
            DrawKey(point_func(player_info_all[gGame.player_ID].point, 0.0, 0.35, 0.1, (xyz){player_info_all[gGame.player_ID].vector.x, player_info_all[gGame.player_ID].vector.y, 0}), (xyz){player_info_all[gGame.player_ID].vector.x, player_info_all[gGame.player_ID].vector.y, 0}, 0);
        }
        if (item_info_all[player_num_init * item_num_init + 0].target == 100){ //落ちてる
            DrawKey(item_info_all[item_num_init * player_num_init + 0].point, item_info_all[item_num_init * player_num_init + 0].vec, 1);
        }
    }
    if (item_info_all[item_num_init * player_num_init + 1].see == SDL_TRUE){
        if (item_info_all[player_num_init * item_num_init + 1].target == gGame.player_ID){ //所持
            DrawKey(point_func(player_info_all[gGame.player_ID].point, 0.0, 0.35, 0.1, (xyz){player_info_all[gGame.player_ID].vector.x, player_info_all[gGame.player_ID].vector.y, 0}), (xyz){player_info_all[gGame.player_ID].vector.x, player_info_all[gGame.player_ID].vector.y, 0}, 0);
        }
        if (item_info_all[player_num_init * item_num_init + 1].target == 100){ //落ちてる
            DrawKey(item_info_all[item_num_init * player_num_init + 1].point, item_info_all[item_num_init * player_num_init + 1].vec, 1);
        }
    }
}



/*
    各種画面描写ステータスを確認して、必要に応じて画面を更新＆
    カメラの位置設定
*/
int reflesh_window_judge(void){  //判定
    if (gGame.window_reflesh == 1){
        gGame.window_reflesh = 0;
        return 1;
    }



    return 0;
}

/*
   視点の設定
*/
void Camera_set(void){
    glLoadIdentity(); // 単位行列に戻す

    //機体からカメラへの差を設定
    xyz ship_to_camera;
    ship_to_camera.x = 0.0;
    ship_to_camera.y = 0.6;
    ship_to_camera.z = 0.6;



    //カメラの絶対座標
    //機体の真上から視点
    //player_info_all[gGame.player_ID].camera_point = point_func(player_info_all[gGame.player_ID].point, ship_to_camera.x, ship_to_camera.y, ship_to_camera.z, player_info_all[gGame.player_ID].vector);

    //機体のy軸上からの視点
    player_info_all[gGame.player_ID].camera_point = point_func(player_info_all[gGame.player_ID].point, ship_to_camera.x, ship_to_camera.y, ship_to_camera.z, (xyz){player_info_all[gGame.player_ID].vector.x, player_info_all[gGame.player_ID].vector.y, 0});

    if (player_info_all[gGame.player_ID].camera_point.y > gGame.map_tile_size){
        player_info_all[gGame.player_ID].camera_point.y = gGame.map_tile_size;
    }
    if (player_info_all[gGame.player_ID].camera_point.y < 0){
        player_info_all[gGame.player_ID].camera_point.y = 0;
    }


    //がっつり機体に追尾
    if (player_info_all[gGame.player_ID].vector.x < -225.0){  //通常飛行
        gluLookAt(player_info_all[gGame.player_ID].camera_point.x, player_info_all[gGame.player_ID].camera_point.y, player_info_all[gGame.player_ID].camera_point.z,
                player_info_all[gGame.player_ID].point.x, player_info_all[gGame.player_ID].point.y, player_info_all[gGame.player_ID].point.z,
                0.0, 1.0, 0.0);
    }
    else if (player_info_all[gGame.player_ID].vector.x < -45.0){  //背面飛行
        gluLookAt(player_info_all[gGame.player_ID].camera_point.x, player_info_all[gGame.player_ID].camera_point.y, player_info_all[gGame.player_ID].camera_point.z,
                player_info_all[gGame.player_ID].point.x, player_info_all[gGame.player_ID].point.y, player_info_all[gGame.player_ID].point.z,
                0.0, -1.0, 0.0);
    }
    else if (player_info_all[gGame.player_ID].vector.x < 135.0){  //通常飛行
        gluLookAt(player_info_all[gGame.player_ID].camera_point.x, player_info_all[gGame.player_ID].camera_point.y, player_info_all[gGame.player_ID].camera_point.z,
                player_info_all[gGame.player_ID].point.x, player_info_all[gGame.player_ID].point.y, player_info_all[gGame.player_ID].point.z,
                0.0, 1.0, 0.0);
    }
    else if (player_info_all[gGame.player_ID].vector.x < 315.0){  //背面飛行
        gluLookAt(player_info_all[gGame.player_ID].camera_point.x, player_info_all[gGame.player_ID].camera_point.y, player_info_all[gGame.player_ID].camera_point.z,
                player_info_all[gGame.player_ID].point.x, player_info_all[gGame.player_ID].point.y, player_info_all[gGame.player_ID].point.z,
                0.0, -1.0, 0.0);
    }
    else {   //通常飛行
        gluLookAt(player_info_all[gGame.player_ID].camera_point.x, player_info_all[gGame.player_ID].camera_point.y, player_info_all[gGame.player_ID].camera_point.z,
                player_info_all[gGame.player_ID].point.x, player_info_all[gGame.player_ID].point.y, player_info_all[gGame.player_ID].point.z,
                0.0, 1.0, 0.0);
    }



    /*
    //カメラが機体の動きに追尾しないバージョン
    gluLookAt(player_info_all[gGame.player_ID].point.x, player_info_all[gGame.player_ID].point.y + 0.4, player_info_all[gGame.player_ID].point.z + 0.4,
            player_info_all[gGame.player_ID].point.x, player_info_all[gGame.player_ID].point.y, player_info_all[gGame.player_ID].point.z,
            0.0, 1.0, 0.0);
    */


}

void DrawUI(void){
    xyz base_point = player_info_all[gGame.player_ID].point;
    xyz vec = player_info_all[gGame.player_ID].vector;
    double wide = 7.0;
    double scale = 0.03;


    glBegin(GL_QUADS);
        // glColor3d(0.0, 0.8, 0.0);

        glVertex3d(point_func(base_point, (-wide)*scale, 11.8*scale, 0.5*scale, vec).x, point_func(base_point, (-wide)*scale, 11.8*scale, 0.5*scale, vec).y, point_func(base_point, (-wide)*scale, 11.8*scale, 0.5*scale, vec).z);
        glVertex3d(point_func(base_point, (-wide)*scale, 11.8*scale, -0.5*scale, vec).x, point_func(base_point, (-wide)*scale, 11.8*scale, -0.5*scale, vec).y, point_func(base_point, (-wide)*scale, 11.8*scale, -0.5*scale, vec).z);
        glVertex3d(point_func(base_point, (-wide)*scale, 11*scale, -0.5*scale, vec).x, point_func(base_point, (-wide)*scale, 11*scale, -0.5*scale, vec).y, point_func(base_point, (-wide)*scale, 11*scale, -0.5*scale, vec).z);
        glVertex3d(point_func(base_point, (-wide)*scale, 11*scale, 0.5*scale, vec).x, point_func(base_point, (-wide)*scale, 11*scale, 0.5*scale, vec).y, point_func(base_point, (-wide)*scale, 11*scale, 0.5*scale, vec).z);
        glVertex3d(point_func(base_point, ((-wide) + 2 * wide * (double)(player_info_all[gGame.player_ID].HP / 100.0))*scale, 11.8*scale, 0.5*scale, vec).x, point_func(base_point, ((-wide) + 2 * wide * (double)(player_info_all[gGame.player_ID].HP / 100.0))*scale, 11.8*scale, 0.5*scale, vec).y, point_func(base_point, ((-wide) + 2 * wide * (double)(player_info_all[gGame.player_ID].HP / 100.0))*scale, 11.8*scale, 0.5*scale, vec).z);
        glVertex3d(point_func(base_point, ((-wide) + 2 * wide * (double)(player_info_all[gGame.player_ID].HP / 100.0))*scale, 11.8*scale, -0.5*scale, vec).x, point_func(base_point, ((-wide) + 2 * wide * (double)(player_info_all[gGame.player_ID].HP / 100.0))*scale, 11.8*scale, -0.5*scale, vec).y, point_func(base_point, ((-wide) + 2 * wide * (double)(player_info_all[gGame.player_ID].HP / 100.0))*scale, 11.8*scale, -0.5*scale, vec).z);
        glVertex3d(point_func(base_point, ((-wide) + 2 * wide * (double)(player_info_all[gGame.player_ID].HP / 100.0))*scale, 11*scale, -0.5*scale, vec).x, point_func(base_point, ((-wide) + 2 * wide * (double)(player_info_all[gGame.player_ID].HP / 100.0))*scale, 11*scale, -0.5*scale, vec).y, point_func(base_point, ((-wide) + 2 * wide * (double)(player_info_all[gGame.player_ID].HP / 100.0))*scale, 11*scale, -0.5*scale, vec).z);
        glVertex3d(point_func(base_point, ((-wide) + 2 * wide * (double)(player_info_all[gGame.player_ID].HP / 100.0))*scale, 11*scale, 0.5*scale, vec).x, point_func(base_point, ((-wide) + 2 * wide * (double)(player_info_all[gGame.player_ID].HP / 100.0))*scale, 11*scale, 0.5*scale, vec).y, point_func(base_point, ((-wide) + 2 * wide * (double)(player_info_all[gGame.player_ID].HP / 100.0))*scale, 11*scale, 0.5*scale, vec).z);
        glVertex3d(point_func(base_point, (-wide)*scale, 11.8*scale, 0.5*scale, vec).x, point_func(base_point, (-wide)*scale, 11.8*scale, 0.5*scale, vec).y, point_func(base_point, (-wide)*scale, 11.8*scale, 0.5*scale, vec).z);
        glVertex3d(point_func(base_point, (-wide)*scale, 11.8*scale, -0.5*scale, vec).x, point_func(base_point, (-wide)*scale, 11.8*scale, -0.5*scale, vec).y, point_func(base_point, (-wide)*scale, 11.8*scale, -0.5*scale, vec).z);
        glVertex3d(point_func(base_point, ((-wide) + 2 * wide * (double)(player_info_all[gGame.player_ID].HP / 100.0))*scale, 11.8*scale, -0.5*scale, vec).x, point_func(base_point, ((-wide) + 2 * wide * (double)(player_info_all[gGame.player_ID].HP / 100.0))*scale, 11.8*scale, -0.5*scale, vec).y, point_func(base_point, ((-wide) + 2 * wide * (double)(player_info_all[gGame.player_ID].HP / 100.0))*scale, 11.8*scale, -0.5*scale, vec).z);
        glVertex3d(point_func(base_point, ((-wide) + 2 * wide * (double)(player_info_all[gGame.player_ID].HP / 100.0))*scale, 11.8*scale, 0.5*scale, vec).x, point_func(base_point, ((-wide) + 2 * wide * (double)(player_info_all[gGame.player_ID].HP / 100.0))*scale, 11.8*scale, 0.5*scale, vec).y, point_func(base_point, ((-wide) + 2 * wide * (double)(player_info_all[gGame.player_ID].HP / 100.0))*scale, 11.8*scale, 0.5*scale, vec).z);
        glVertex3d(point_func(base_point, (-wide)*scale, 11.8*scale, 0.5*scale, vec).x, point_func(base_point, (-wide)*scale, 11.8*scale, 0.5*scale, vec).y, point_func(base_point, (-wide)*scale, 11.8*scale, 0.5*scale, vec).z);
        glVertex3d(point_func(base_point, (-wide)*scale, 11.8*scale, -0.5*scale, vec).x, point_func(base_point, (-wide)*scale, 11.8*scale, -0.5*scale, vec).y, point_func(base_point, (-wide)*scale, 11.8*scale, -0.5*scale, vec).z);
        glVertex3d(point_func(base_point, ((-wide) + 2 * wide * (double)(player_info_all[gGame.player_ID].HP / 100.0))*scale, 11.8*scale, -0.5*scale, vec).x, point_func(base_point, ((-wide) + 2 * wide * (double)(player_info_all[gGame.player_ID].HP / 100.0))*scale, 11.8*scale, -0.5*scale, vec).y, point_func(base_point, ((-wide) + 2 * wide * (double)(player_info_all[gGame.player_ID].HP / 100.0))*scale, 11.8*scale, -0.5*scale, vec).z);
        glVertex3d(point_func(base_point, ((-wide) + 2 * wide * (double)(player_info_all[gGame.player_ID].HP / 100.0))*scale, 11.8*scale, 0.5*scale, vec).x, point_func(base_point, ((-wide) + 2 * wide * (double)(player_info_all[gGame.player_ID].HP / 100.0))*scale, 11.8*scale, 0.5*scale, vec).y, point_func(base_point, ((-wide) + 2 * wide * (double)(player_info_all[gGame.player_ID].HP / 100.0))*scale, 11.8*scale, 0.5*scale, vec).z);
        glVertex3d(point_func(base_point, (-wide)*scale, 11.8*scale, -0.5*scale, vec).x, point_func(base_point, (-wide)*scale, 11.8*scale, -0.5*scale, vec).y, point_func(base_point, (-wide)*scale, 11.8*scale, -0.5*scale, vec).z);
        glVertex3d(point_func(base_point, (-wide)*scale, 11*scale, -0.5*scale, vec).x, point_func(base_point, (-wide)*scale, 11*scale, -0.5*scale, vec).y, point_func(base_point, (-wide)*scale, 11*scale, -0.5*scale, vec).z);
        glVertex3d(point_func(base_point, ((-wide) + 2 * wide * (double)(player_info_all[gGame.player_ID].HP / 100.0))*scale, 11*scale, -0.5*scale, vec).x, point_func(base_point, ((-wide) + 2 * wide * (double)(player_info_all[gGame.player_ID].HP / 100.0))*scale, 11*scale, -0.5*scale, vec).y, point_func(base_point, ((-wide) + 2 * wide * (double)(player_info_all[gGame.player_ID].HP / 100.0))*scale, 11*scale, -0.5*scale, vec).z);
        glVertex3d(point_func(base_point, ((-wide) + 2 * wide * (double)(player_info_all[gGame.player_ID].HP / 100.0))*scale, 11.8*scale, -0.5*scale, vec).x, point_func(base_point, ((-wide) + 2 * wide * (double)(player_info_all[gGame.player_ID].HP / 100.0))*scale, 11.8*scale, -0.5*scale, vec).y, point_func(base_point, ((-wide) + 2 * wide * (double)(player_info_all[gGame.player_ID].HP / 100.0))*scale, 11.8*scale, -0.5*scale, vec).z);
        glVertex3d(point_func(base_point, (-wide)*scale, 11.8*scale, 0.5*scale, vec).x, point_func(base_point, (-wide)*scale, 11.8*scale, 0.5*scale, vec).y, point_func(base_point, (-wide)*scale, 11.8*scale, 0.5*scale, vec).z);
        glVertex3d(point_func(base_point, (-wide)*scale, 11*scale, 0.5*scale, vec).x, point_func(base_point, (-wide)*scale, 11*scale, 0.5*scale, vec).y, point_func(base_point, (-wide)*scale, 11*scale, 0.5*scale, vec).z);
        glVertex3d(point_func(base_point, ((-wide) + 2 * wide * (double)(player_info_all[gGame.player_ID].HP / 100.0))*scale, 11*scale, 0.5*scale, vec).x, point_func(base_point, ((-wide) + 2 * wide * (double)(player_info_all[gGame.player_ID].HP / 100.0))*scale, 11*scale, 0.5*scale, vec).y, point_func(base_point, ((-wide) + 2 * wide * (double)(player_info_all[gGame.player_ID].HP / 100.0))*scale, 11*scale, 0.5*scale, vec).z);
        glVertex3d(point_func(base_point, ((-wide) + 2 * wide * (double)(player_info_all[gGame.player_ID].HP / 100.0))*scale, 11.8*scale, 0.5*scale, vec).x, point_func(base_point, ((-wide) + 2 * wide * (double)(player_info_all[gGame.player_ID].HP / 100.0))*scale, 11.8*scale, 0.5*scale, vec).y, point_func(base_point, ((-wide) + 2 * wide * (double)(player_info_all[gGame.player_ID].HP / 100.0))*scale, 11.8*scale, 0.5*scale, vec).z);


    glEnd();

    for (int i = 0; i < gGame.item2_nmax; i++){
        glBegin(GL_QUADS);
        glColor3d(1.0, 0.68, 0.9);

        glVertex3d(point_func(base_point, ((-wide + (wide / 10)/2) + i * 2 * (wide / 10) -0.15)*scale, -8*scale, 1*scale, vec).x, point_func(base_point, ((-wide + (wide / 10)/2) + i * 2 * (wide / 10) -0.15)*scale, -8*scale, 1*scale, vec).y, point_func(base_point, ((-wide + (wide / 10)/2) + i * 2 * (wide / 10) -0.15)*scale, -8*scale, 1*scale, vec).z);
        glVertex3d(point_func(base_point, ((-wide + (wide / 10)/2) + i * 2 * (wide / 10) -0.45)*scale, -8.3*scale, 1.3*scale, vec).x, point_func(base_point, ((-wide + (wide / 10)/2) + i * 2 * (wide / 10) -0.45)*scale, -8.3*scale, 1.3*scale, vec).y, point_func(base_point, ((-wide + (wide / 10)/2) + i * 2 * (wide / 10) -0.45)*scale, -8.3*scale, 1.3*scale, vec).z);
        glVertex3d(point_func(base_point, ((-wide + (wide / 10)/2) + i * 2 * (wide / 10) +0.45)*scale, -8.3*scale, 1.3*scale, vec).x, point_func(base_point, ((-wide + (wide / 10)/2) + i * 2 * (wide / 10) +0.45)*scale, -8.3*scale, 1.3*scale, vec).y, point_func(base_point, ((-wide + (wide / 10)/2) + i * 2 * (wide / 10) +0.45)*scale, -8.3*scale, 1.3*scale, vec).z);
        glVertex3d(point_func(base_point, ((-wide + (wide / 10)/2) + i * 2 * (wide / 10) +0.15)*scale, -8*scale, 1*scale, vec).x, point_func(base_point, ((-wide + (wide / 10)/2) + i * 2 * (wide / 10) +0.15)*scale, -8*scale, 1*scale, vec).y, point_func(base_point, ((-wide + (wide / 10)/2) + i * 2 * (wide / 10) +0.15)*scale, -8*scale, 1*scale, vec).z);

        glVertex3d(point_func(base_point, ((-wide + (wide / 10)/2) + i * 2 * (wide / 10) -0.3)*scale, -8.3*scale, 1.3*scale, vec).x, point_func(base_point, ((-wide + (wide / 10)/2) + i * 2 * (wide / 10) -0.3)*scale, -8.3*scale, 1.3*scale, vec).y, point_func(base_point, ((-wide + (wide / 10)/2) + i * 2 * (wide / 10) -0.3)*scale, -8.3*scale, 1.3*scale, vec).z);
        glVertex3d(point_func(base_point, ((-wide + (wide / 10)/2) + i * 2 * (wide / 10) +0.15)*scale, -8.3*scale, 1.3*scale, vec).x, point_func(base_point, ((-wide + (wide / 10)/2) + i * 2 * (wide / 10) +0.15)*scale, -8.3*scale, 1.3*scale, vec).y, point_func(base_point, ((-wide + (wide / 10)/2) + i * 2 * (wide / 10) +0.15)*scale, -8.3*scale, 1.3*scale, vec).z);
        glVertex3d(point_func(base_point, ((-wide + (wide / 10)/2) + i * 2 * (wide / 10) +0.3)*scale, -9*scale, 2*scale, vec).x, point_func(base_point, ((-wide + (wide / 10)/2) + i * 2 * (wide / 10) +0.3)*scale, -9*scale, 2*scale, vec).y, point_func(base_point, ((-wide + (wide / 10)/2) + i * 2 * (wide / 10) +0.3)*scale, -9*scale, 2*scale, vec).z);
        glVertex3d(point_func(base_point, ((-wide + (wide / 10)/2) + i * 2 * (wide / 10) -0.3)*scale, -9*scale, 2*scale, vec).x, point_func(base_point, ((-wide + (wide / 10)/2) + i * 2 * (wide / 10) -0.3)*scale, -9*scale, 2*scale, vec).y, point_func(base_point, ((-wide + (wide / 10)/2) + i * 2 * (wide / 10) -0.3)*scale, -9*scale, 2*scale, vec).z);

        glVertex3d(point_func(base_point, ((-wide + (wide / 10)/2) + i * 2 * (wide / 10) -0.3)*scale, -8.7*scale, 1.7*scale, vec).x, point_func(base_point, ((-wide + (wide / 10)/2) + i * 2 * (wide / 10) -0.3)*scale, -8.7*scale, 1.7*scale, vec).y, point_func(base_point, ((-wide + (wide / 10)/2) + i * 2 * (wide / 10) -0.3)*scale, -8.7*scale, 1.7*scale, vec).z);
        glVertex3d(point_func(base_point, ((-wide + (wide / 10)/2) + i * 2 * (wide / 10) -0.6)*scale, -8.7*scale, 1.7*scale, vec).x, point_func(base_point, ((-wide + (wide / 10)/2) + i * 2 * (wide / 10) -0.6)*scale, -8.7*scale, 1.7*scale, vec).y, point_func(base_point, ((-wide + (wide / 10)/2) + i * 2 * (wide / 10) -0.6)*scale, -8.7*scale, 1.7*scale, vec).z);
        glVertex3d(point_func(base_point, ((-wide + (wide / 10)/2) + i * 2 * (wide / 10) -0.6)*scale, -9*scale, 2*scale, vec).x, point_func(base_point, ((-wide + (wide / 10)/2) + i * 2 * (wide / 10) -0.6)*scale, -9*scale, 2*scale, vec).y, point_func(base_point, ((-wide + (wide / 10)/2) + i * 2 * (wide / 10) -0.6)*scale, -9*scale, 2*scale, vec).z);
        glVertex3d(point_func(base_point, ((-wide + (wide / 10)/2) + i * 2 * (wide / 10) -0.3)*scale, -9*scale, 2*scale, vec).x, point_func(base_point, ((-wide + (wide / 10)/2) + i * 2 * (wide / 10) -0.3)*scale, -9*scale, 2*scale, vec).y, point_func(base_point, ((-wide + (wide / 10)/2) + i * 2 * (wide / 10) -0.3)*scale, -9*scale, 2*scale, vec).z);

        glVertex3d(point_func(base_point, ((-wide + (wide / 10)/2) + i * 2 * (wide / 10) +0.3)*scale, -8.7*scale, 1.7*scale, vec).x, point_func(base_point, ((-wide + (wide / 10)/2) + i * 2 * (wide / 10) +0.3)*scale, -8.7*scale, 1.7*scale, vec).y, point_func(base_point, ((-wide + (wide / 10)/2) + i * 2 * (wide / 10) +0.3)*scale, -8.7*scale, 1.7*scale, vec).z);
        glVertex3d(point_func(base_point, ((-wide + (wide / 10)/2) + i * 2 * (wide / 10) +0.6)*scale, -8.7*scale, 1.7*scale, vec).x, point_func(base_point, ((-wide + (wide / 10)/2) + i * 2 * (wide / 10) +0.6)*scale, -8.7*scale, 1.7*scale, vec).y, point_func(base_point, ((-wide + (wide / 10)/2) + i * 2 * (wide / 10) +0.6)*scale, -8.7*scale, 1.7*scale, vec).z);
        glVertex3d(point_func(base_point, ((-wide + (wide / 10)/2) + i * 2 * (wide / 10) +0.6)*scale, -9*scale, 2*scale, vec).x, point_func(base_point, ((-wide + (wide / 10)/2) + i * 2 * (wide / 10) +0.6)*scale, -9*scale, 2*scale, vec).y, point_func(base_point, ((-wide + (wide / 10)/2) + i * 2 * (wide / 10) +0.6)*scale, -9*scale, 2*scale, vec).z);
        glVertex3d(point_func(base_point, ((-wide + (wide / 10)/2) + i * 2 * (wide / 10) +0.3)*scale, -9*scale, 2*scale, vec).x, point_func(base_point, ((-wide + (wide / 10)/2) + i * 2 * (wide / 10) +0.3)*scale, -9*scale, 2*scale, vec).y, point_func(base_point, ((-wide + (wide / 10)/2) + i * 2 * (wide / 10) +0.3)*scale, -9*scale, 2*scale, vec).z);


        glEnd();

    }


    //敵味方の方向の矢印を表示
    for (int i = 0; i < gGame.player_num; i++){
        if (i != gGame.player_ID){
            if ((i < gGame.player_num/2) ^ (gGame.player_ID < gGame.player_num/2)){  //敵
                if (gGame.distance[i] < 30.0){ //敵は表示距離を制限
                    if (player_info_all[i].see == SDL_TRUE){
                        // glColor3d(1.0, 0.2, 0.2);
                        glColor3d(0.95, 0.53, 0.81); //四国めたんカラー 
                        glBegin(GL_TRIANGLES);
                        glVertex3d(point_func(base_point, 0, 0, 0.45, (xyz){0,gGame.ship2vec[i],0}).x, point_func(base_point, 0, 0, 0.45, (xyz){0,gGame.ship2vec[i],0}).y, point_func(base_point, 0, 0, 0.45, (xyz){0,gGame.ship2vec[i],0}).z);
                        glVertex3d(point_func(base_point, 0.05, 0, 0.4, (xyz){0,gGame.ship2vec[i],0}).x, point_func(base_point, 0.05, 0, 0.4, (xyz){0,gGame.ship2vec[i],0}).y, point_func(base_point, 0.05, 0, 0.4, (xyz){0,gGame.ship2vec[i],0}).z);
                        glVertex3d(point_func(base_point, -0.05, 0, 0.4, (xyz){0,gGame.ship2vec[i],0}).x, point_func(base_point, -0.05, 0, 0.4, (xyz){0,gGame.ship2vec[i],0}).y, point_func(base_point, -0.05, 0, 0.4, (xyz){0,gGame.ship2vec[i],0}).z);
                        glEnd();
                    }
                }
            } else { //味方
                if (player_info_all[i].see == SDL_TRUE){
                    glColor3d(0.62, 0.898, 0.20); //ずんだもんカラー
                    glBegin(GL_TRIANGLES);
                    glVertex3d(point_func(base_point, 0, 0, 0.45, (xyz){0,gGame.ship2vec[i],0}).x, point_func(base_point, 0, 0, 0.45, (xyz){0,gGame.ship2vec[i],0}).y, point_func(base_point, 0, 0, 0.45, (xyz){0,gGame.ship2vec[i],0}).z);
                    glVertex3d(point_func(base_point, 0.05, 0, 0.4, (xyz){0,gGame.ship2vec[i],0}).x, point_func(base_point, 0.05, 0, 0.4, (xyz){0,gGame.ship2vec[i],0}).y, point_func(base_point, 0.05, 0, 0.4, (xyz){0,gGame.ship2vec[i],0}).z);
                    glVertex3d(point_func(base_point, -0.05, 0, 0.4, (xyz){0,gGame.ship2vec[i],0}).x, point_func(base_point, -0.05, 0, 0.4, (xyz){0,gGame.ship2vec[i],0}).y, point_func(base_point, -0.05, 0, 0.4, (xyz){0,gGame.ship2vec[i],0}).z);
                    glEnd();
                }
            }       
        }
    }
    
    //鍵の方向を表示
    if (item_info_all[player_num_init*item_num_init + 0].see == SDL_TRUE && item_info_all[player_num_init*item_num_init + 0].target == 100){
        double vec = acos((-item_info_all[player_num_init*item_num_init+0].point.z + player_info_all[gGame.player_ID].point.z)
             / sqrt((player_info_all[gGame.player_ID].point.x - item_info_all[player_num_init*item_num_init+0].point.x)*(player_info_all[gGame.player_ID].point.x - item_info_all[player_num_init*item_num_init+0].point.x) 
             + (player_info_all[gGame.player_ID].point.z - item_info_all[player_num_init*item_num_init+0].point.z)*(player_info_all[gGame.player_ID].point.z - item_info_all[player_num_init*item_num_init+0].point.z)))
             * 180.0 / M_PI;
        if ((item_info_all[player_num_init*item_num_init+0].point.x - player_info_all[gGame.player_ID].point.x) >= 0){
            vec = 180.0 - vec;
        } else {
            vec = vec - 180.0;
        }
        glColor3d(1.0, 1.0, 0.0);
        glBegin(GL_TRIANGLES);
        glVertex3d(point_func(base_point, 0, 0, 0.45, (xyz){0,vec,0}).x, point_func(base_point, 0, 0, 0.45, (xyz){0,vec,0}).y, point_func(base_point, 0, 0, 0.45, (xyz){0,vec,0}).z);
        glVertex3d(point_func(base_point, 0.05, 0, 0.4, (xyz){0,vec,0}).x, point_func(base_point, 0.05, 0, 0.4, (xyz){0,vec,0}).y, point_func(base_point, 0.05, 0, 0.4, (xyz){0,vec,0}).z);
        glVertex3d(point_func(base_point, -0.05, 0, 0.4, (xyz){0,vec,0}).x, point_func(base_point, -0.05, 0, 0.4, (xyz){0,vec,0}).y, point_func(base_point, -0.05, 0, 0.4, (xyz){0,vec,0}).z);
        glEnd();

    }
    if (item_info_all[player_num_init*item_num_init + 1].see == SDL_TRUE && item_info_all[player_num_init*item_num_init + 1].target == 100){
        double vec = acos((-item_info_all[player_num_init*item_num_init+1].point.z + player_info_all[gGame.player_ID].point.z)
             / sqrt((player_info_all[gGame.player_ID].point.x - item_info_all[player_num_init*item_num_init+1].point.x)*(player_info_all[gGame.player_ID].point.x - item_info_all[player_num_init*item_num_init+1].point.x) 
             + (player_info_all[gGame.player_ID].point.z - item_info_all[player_num_init*item_num_init+1].point.z)*(player_info_all[gGame.player_ID].point.z - item_info_all[player_num_init*item_num_init+1].point.z)))
             * 180.0 / M_PI;
        if ((item_info_all[player_num_init*item_num_init+1].point.x - player_info_all[gGame.player_ID].point.x) >= 0){
            vec = 180.0 - vec;
        } else {
            vec = vec - 180.0;
        }
        glColor3d(1.0, 1.0, 0.0);
        glBegin(GL_TRIANGLES);
        glVertex3d(point_func(base_point, 0, 0, 0.45, (xyz){0,vec,0}).x, point_func(base_point, 0, 0, 0.45, (xyz){0,vec,0}).y, point_func(base_point, 0, 0, 0.45, (xyz){0,vec,0}).z);
        glVertex3d(point_func(base_point, 0.05, 0, 0.4, (xyz){0,vec,0}).x, point_func(base_point, 0.05, 0, 0.4, (xyz){0,vec,0}).y, point_func(base_point, 0.05, 0, 0.4, (xyz){0,vec,0}).z);
        glVertex3d(point_func(base_point, -0.05, 0, 0.4, (xyz){0,vec,0}).x, point_func(base_point, -0.05, 0, 0.4, (xyz){0,vec,0}).y, point_func(base_point, -0.05, 0, 0.4, (xyz){0,vec,0}).z);
        glEnd();

    }
    
}


/*
    文字とかを書くやつ。
*/
void DrawInfo(void){
    DrawUI();
}

/* ゲーム終了画面描画関数 */
void GameEnding(void) {
    SDL_Window* e_window;
    SDL_Renderer *e_renderer;
    e_window = SDL_CreateWindow("Thank you for playing!", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, 1280, 720, SDL_SWSURFACE);
    e_renderer= SDL_CreateRenderer(e_window, -1, 0);

    SDL_Surface *s;
    SDL_Texture *t;

    /* テクスチャ作成 */
    if (gGame.player_ID < player_num_init/2){
        if (gGame.finish_team_ID == 1){
            s = IMG_Load("img/Ending_lose.png");
            t = SDL_CreateTextureFromSurface(e_renderer, s);
        } else {
            s = IMG_Load("img/Ending_win.png");
            t = SDL_CreateTextureFromSurface(e_renderer, s);
        }
    } else {
        if (gGame.finish_team_ID == 2){
            s = IMG_Load("img/Ending_win.png");
            t = SDL_CreateTextureFromSurface(e_renderer, s);
        } else {
            s = IMG_Load("img/Ending_lose.png");
            t = SDL_CreateTextureFromSurface(e_renderer, s);
        }
    }


    /* 画面描画 */
    SDL_RenderCopy(e_renderer, t, NULL, NULL);
    SDL_RenderPresent(e_renderer);
    SDL_Delay(5000);

    /* 終了処理 */
    SDL_FreeSurface(s);
    SDL_DestroyTexture(t);
}