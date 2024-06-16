#include <stdio.h>
#include <SDL2/SDL.h>   //-lSDL2
#include <SDL2/SDL2_gfxPrimitives.h>    //-lSDL2_gfx
#include <SDL2/SDL_ttf.h>   //-lSDL2_ttf
#include <SDL2/SDL_image.h> //-lSDL2_image
#include <SDL2/SDL_mixer.h> //-lSDL2_mixer

    SDL_Window* w_window;
    SDL_Renderer* w_renderer;
    SDL_Texture* w_texture;
    SDL_Surface* w_strings;
    SDL_Event w_event;
    SDL_Color w_green = {0x00, 0xff, 0x00}; // フォントの色,緑
    SDL_Color w_red = {0xff, 0x00, 0x00};   //フォントの色,赤
    TTF_Font* w_font;

//２つの入力されたx座標を元にボタンを表示する関数
void TYPE(int x1,int x2, char button[],int mode){
    boxColor(w_renderer,x1,100,x2,200,0xff000000);
    w_font = TTF_OpenFont("./w_fonts-japanese-gothic.ttf", 12);
    switch (mode){
    case 0:
        w_strings = TTF_RenderUTF8_Blended(w_font, button, w_red);
        break;
    default:
        w_strings = TTF_RenderUTF8_Blended(w_font, button, w_green);
        break;
    }
    w_texture = SDL_CreateTextureFromSurface(w_renderer, w_strings); SDL_Rect src_rect = {0, 0, w_strings->w, w_strings->h}; // 転送元
    SDL_Rect dst_rect = {(x1 + x2) / 2.1, 130, w_strings->w+20, w_strings->h+20};
    SDL_RenderCopy(w_renderer, w_texture, &src_rect, &dst_rect);
}

//ボタンの選択を表示する関数
void choice(int button_num){
        switch (button_num){
        case 0:
            SDL_DestroyRenderer(w_renderer);
            w_renderer = SDL_CreateRenderer(w_window,-1,0);
            SDL_SetRenderDrawColor(w_renderer,255,255,255,255);
            SDL_RenderClear(w_renderer);
            //第4引数が0が選択中
            TYPE(100,250,"START",0);
            TYPE(350,500,"Manual",1);
            TYPE(600,750,"END",1);
            SDL_RenderPresent(w_renderer);
            break;

        case 1:
            SDL_DestroyRenderer(w_renderer);
            w_renderer = SDL_CreateRenderer(w_window,-1,0);
            SDL_SetRenderDrawColor(w_renderer,255,255,255,255);
            SDL_RenderClear(w_renderer);
            //第4引数が0が選択中
            TYPE(100,250,"START",1);
            TYPE(350,500,"Manual",0);
            TYPE(600,750,"END",1);
            SDL_RenderPresent(w_renderer);
            break;

        case 2:
            SDL_DestroyRenderer(w_renderer);
            w_renderer = SDL_CreateRenderer(w_window,-1,0);
            SDL_SetRenderDrawColor(w_renderer,255,255,255,255);
            SDL_RenderClear(w_renderer);
            //第4引数が0が選択中
            TYPE(100,250,"START",1);
            TYPE(350,500,"Manual",1);
            TYPE(600,750,"END",0);
            SDL_RenderPresent(w_renderer);
            break;
        }

    SDL_RenderPresent(w_renderer);
}

int main(int argc, char* argv[])
{
    SDL_Surface* image = IMG_Load("mycat.png");
    SDL_Texture* pngtex;
    Mix_Music *music; // BGMデータ格納用構造体
    int button_num = 0;
    TTF_Init();


    // SDL初期化（初期化失敗の場合は終了）
    if(SDL_Init(SDL_INIT_EVERYTHING) < 0) {
        fprintf(stderr, "failed to initialize SDL.\n");
        exit(-1);
    }


    w_window = SDL_CreateWindow("report2",SDL_WINDOWPOS_CENTERED,SDL_WINDOWPOS_CENTERED,800,640,0);
    w_renderer = SDL_CreateRenderer(w_window,-1,0);
    SDL_SetRenderDrawColor(w_renderer,255,255,255,255);
    SDL_RenderClear(w_renderer);

    //第４引数は選択中か判別
    TYPE(100,250,"START",0);
    TYPE(350,500,"Manual",1);
    TYPE(600,750,"END",1);

    SDL_RenderPresent(w_renderer);

    while (1){
        if (SDL_PollEvent(&w_event)){
            switch(w_event.type){
            case SDL_MOUSEBUTTONDOWN:
                    if (w_event.button.x >= 100 && w_event.button.x <= 250){
                        if (w_event.button.y >= 100 && w_event.button.y <= 200){
                            button_num = 0;
                            choice(button_num);
                            if (button_num == 0){
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
                    if (w_event.button.x >= 350 && w_event.button.x <= 500){
                        if (w_event.button.y >= 100 && w_event.button.y <= 200 ){
                            button_num = 1;
                            choice(button_num);
                            if (button_num == 1){
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
                    if (w_event.button.x >= 600 && w_event.button.x <= 750){
                        if (w_event.button.y >= 100 && w_event.button.y <= 200){
                            button_num = 2;
                            choice(button_num);
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

                        }
                    }

            break;


            case SDL_KEYDOWN:
                switch (w_event.key.keysym.sym){
                case SDLK_RETURN:
                    switch (button_num){
                        case 0:
                            pngtex = SDL_CreateTextureFromSurface(w_renderer,image);
                            SDL_Rect src_rect = {0, 0, image->w, image->h};
                            SDL_Rect dst_rect = {300, 300, 300, 300};
                            SDL_RenderCopy(w_renderer, pngtex, &src_rect, &dst_rect);
                            SDL_RenderPresent(w_renderer);
                            break;
                        case 1:
                            Mix_PlayMusic(music,1);
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
                    }
                    break;
                case SDLK_RIGHT:
                    button_num += 1;
                    if (button_num >= 3)    button_num = 0;
                    choice(button_num);
                    break;
                case SDLK_LEFT:
                    button_num -= 1;
                    if (button_num < 0)    button_num = 2;
                    choice(button_num);
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