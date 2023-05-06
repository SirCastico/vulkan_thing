#include "SDL_events.h"
#include "SDL_keycode.h"
#include <SDL.h>

#include "renderer.hpp"



int main(int argc, char* argv[]){
    
    Renderer engine;
    bool running=true;
    SDL_Event event;
    while(running){
        while(SDL_PollEvent(&event)!=0){
            if(event.type==SDL_QUIT)
                running = false;    
            if(event.type==SDL_KEYDOWN){
                if(event.key.keysym.sym==SDLK_ESCAPE)
                    running=false;
            }
        }
        engine.draw();
    }
    printf("finish\n");

    return 0;
}
