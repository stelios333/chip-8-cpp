#include <iostream>
#include <zconf.h>
#include <string>
#include <SDL_video.h>
#include <SDL_render.h>
#include <SDL_events.h>
#include <SDL_ttf.h>
#include <SDL.h>
#include "chip8.h"

uint8_t keymap[16] = {
        SDLK_x,
        SDLK_1,
        SDLK_2,
        SDLK_3,
        SDLK_q,
        SDLK_w,
        SDLK_e,
        SDLK_a,
        SDLK_s,
        SDLK_d,
        SDLK_z,
        SDLK_c,
        SDLK_4,
        SDLK_r,
        SDLK_f,
        SDLK_v,
};

void get_text_and_rect(SDL_Renderer *renderer, int x, int y, char *text, 
        TTF_Font *font, SDL_Texture **texture, SDL_Rect *rect, Uint32 WrapLength) {
    int text_width;
    int text_height;
    if (*texture) {
        SDL_DestroyTexture(*texture);
        *texture = nullptr;
    }
    SDL_Surface *surface;
    SDL_Color textColor = {255, 255, 255, 0};

    surface = TTF_RenderText_Blended_Wrapped(font, text, textColor, WrapLength);
    *texture = SDL_CreateTextureFromSurface(renderer, surface);
    text_width = surface->w;
    text_height = surface->h;
    SDL_FreeSurface(surface);
    rect->x = x;
    rect->y = y;
    rect->w = text_width;
    rect->h = text_height;
}

int main(int argc, char *argv[])
{

    if (argc <= 1)
    {
        std::cerr << "Path to ROM to be loaded must be given as argument\nType -help to see usage\n";
        return 1;
    }

    if (strcmp(argv[1], "-h") == 0)
    {
        //print the help menu
        std::cout << "Normal usage: ./Chip8_Emulator <path_to_rom>\n"
                  << "-a \n\tDisable audio\n"
                  << "-t \n\tTrace mode\n"
                  << "-d \n\tDebug mode\n";
        return 0;
    }

    Chip8 chip8;
    if (!chip8.load_rom(argv[1])) //loading ROM provided as argument
    {
        std::cerr << "ROM could not be loaded. Possibly invalid path given\n";
        return 1;
    }

    bool trace_mode = false, audio_on = true, debug_mode = false;
    if (argc > 2) //there are flags
    {
        for (int i = 2; i < argc; ++i)
        {
            if (strcmp(argv[i], "-t") == 0) //turn on trace mode
            {
                trace_mode = true;
            }
            else if (strcmp(argv[i], "-a") == 0)
            {
                audio_on = false;
            }
            else if (strcmp(argv[i], "-d") == 0)
            {
                debug_mode = true;
            }
            else
            {
                std::cerr << "Invalid flags given. Type -h to check usage\n";
                return 1;
            }
        }
    }

    //set up SDL
    SDL_Window *window;
    SDL_Renderer *renderer;
    SDL_Texture *texture, *text_texture, *debug_text_texture, *mem_view_texture;
    SDL_Rect main_rect, help_text_rect, debug_rect, mem_view_rect;
    //SDL_Color White = {255, 255, 255, 255};

    main_rect.x = 0;
    main_rect.y = 0;
    main_rect.w = 640;
    main_rect.h = 320;

    debug_rect.x = main_rect.w + 5;
    debug_rect.y = 0;
    debug_rect.w = 640;
    debug_rect.h = 320;
    
    const int window_height = main_rect.h + 20;
    int window_width = main_rect.w;

    if (debug_mode) {
        window_width += debug_rect.x;
    }

    const char* window_title = "Chip-8 Emulator";
    if (SDL_Init(SDL_INIT_EVERYTHING) < 0)
    {
        std::cerr << "Error in initialising SDL " << SDL_GetError() << std::endl;
        SDL_Quit();
        return 1;
    }

    window = SDL_CreateWindow((char*) window_title, SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, window_width, window_height,
                              SDL_WINDOW_SHOWN);
    
    if (window == nullptr)
    {
        std::cerr << "Error in creating window " << SDL_GetError() << std::endl;
        SDL_Quit();
        return 1;
    }

    renderer = SDL_CreateRenderer(window, -1, 0);
    if (renderer == nullptr)
    {
        std::cerr << "Error in initializing rendering " << SDL_GetError() << std::endl;
        SDL_Quit();
        return 1;
    }

    SDL_RenderSetLogicalSize(renderer, window_width, window_height);

    texture = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_ARGB8888, SDL_TEXTUREACCESS_STREAMING, 64, 32);
    if (texture == nullptr)
    {
        std::cerr << "Error in setting up texture " << SDL_GetError() << std::endl;
        SDL_Quit();
        return 1;
    }

    TTF_Init();
    constexpr const char* myFont = "NotoSansMono.ttf";
    TTF_Font* Sans = TTF_OpenFont(myFont, 16);
    if (Sans == nullptr) {
        std::cerr << "Failed to load font \""<<myFont<<"\"." << std::endl;
        SDL_Quit();
        return 1;
    }
    const char* help_message = "p = pause, o = step, tab = fast forward, r = reset";
    get_text_and_rect(renderer, 5, window_height-23, (char*) help_message, Sans, &text_texture, &help_text_rect, static_cast<Uint32>(main_rect.w));


    bool fast_forward = false;
    uint16_t iter_count = 0;

    int mem_view_offset = 0;
    constexpr int mem_view_lines = 8;
    constexpr const int mem_view_bytes_per_line = 8;
    constexpr const int mem_view_bytes = mem_view_lines * mem_view_bytes_per_line;

    auto start = std::chrono::steady_clock::now();
    while (true)
    {


        if(chip8.single_cycle(trace_mode, audio_on)) {
            std::cout << "Failed to execute last instruction. Exiting...\n";
            SDL_Quit();
            return 1;
        }

        SDL_Event event;
        while (SDL_PollEvent(&event))
        {
            if (event.type == SDL_QUIT)
            {
                return 0;
            }

            if (event.type == SDL_KEYDOWN)
            {
                if (debug_mode){
                    if (event.key.keysym.sym == SDLK_DOWN) {
                        if (mem_view_offset < 4096 - mem_view_bytes) {
                            mem_view_offset += mem_view_lines;
                        }
                    }
                    if (event.key.keysym.sym == SDLK_UP) {
                        if (mem_view_offset > 0) {
                            mem_view_offset -= mem_view_lines;
                        }
                    }
                    if (event.key.keysym.sym == SDLK_PAGEDOWN) {
                        if (mem_view_offset < 4096 - mem_view_bytes) {
                            mem_view_offset += mem_view_bytes;
                        }
                        if (mem_view_offset > 4096 - mem_view_bytes) {
                            mem_view_offset = 4096 - mem_view_bytes;
                        }
                    }
                    if (event.key.keysym.sym == SDLK_PAGEUP) {
                        if (mem_view_offset > 0) {
                            mem_view_offset -= mem_view_bytes;
                        }
                        if (mem_view_offset < 0) {
                            mem_view_offset = 0;
                        }
                    }
                }
                if (event.key.keysym.sym == SDLK_r) {
                    chip8.reset();
                    if (!chip8.load_rom(argv[1]))
                    {
                        std::cerr << "ROM could not be reloaded. File probably deleted or moved.\n";
                        return 1;
                    }
                    chip8.set_draw_flag(true);
                }
                if (event.key.keysym.sym == SDLK_p) {
                    chip8.set_paused(!chip8.get_paused());
                    if (chip8.get_paused()) {

                        std::string new_title = std::format("Chip-8 emulator (PC={:#05x})", *chip8.get_debug_info().pc);
                        SDL_SetWindowTitle(window, new_title.c_str());
                    } else {

                        SDL_SetWindowTitle(window, window_title);
                    }
                }
                if (event.key.keysym.sym == SDLK_o) {
                    chip8.set_paused(false);
                    chip8.single_cycle(trace_mode, audio_on);
                    chip8.set_paused(true);
                    std::string new_title = std::format("Chip-8 emulator (PC={:#05x})", *chip8.get_debug_info().pc);
                    SDL_SetWindowTitle(window, new_title.c_str());
                }
                if (event.key.keysym.sym == SDLK_TAB) {
                    fast_forward = 1;
                }
                if (event.key.keysym.sym == SDLK_ESCAPE)
                {
                    SDL_Quit();
                    return 0;
                }

                for (int i = 0; i < 16; ++i)
                {
                    if (event.key.keysym.sym == keymap[i])
                    {
                        chip8.set_keypad_value(i, 1);
                    }
                }
            }

            if (event.type == SDL_KEYUP)
            {
                if (event.key.keysym.sym == SDLK_TAB) {
                    fast_forward = 0;
                }
                for (int i = 0; i < 16; ++i)
                {
                    if (event.key.keysym.sym == keymap[i])
                    {
                        chip8.set_keypad_value(i, 0);
                    }
                }
            }

        }

        if (chip8.get_draw_flag()||(debug_mode && iter_count%16==0))
        {
            chip8.set_draw_flag(false);
            uint32_t pixels[32 * 64];
            bool* raw_pixels = chip8.get_display_buffer();
            for (int i = 0; i < 32 * 64; ++i)
            {
                if (raw_pixels[i] == 0)
                {
                    pixels[i] = 0xFF000000;
                }
                else
                {
                    pixels[i] = 0xFFFFFFFF;
                }
            }
            if (debug_mode) {
                DebugInfo& dbg = chip8.get_debug_info();
                std::string debug_info_str = "";
                for (uint8_t i = 0; i<16; ++i) {
                    debug_info_str += std::format("V{:01x}={:#04x} ", i, dbg.V[i]);
                }
                debug_info_str+="\n";
                debug_info_str+=std::format("PC={:#05x}, DT={:#04x}, ST={:#04x}, IReg={:#04x}", *dbg.pc, chip8.get_delay_timer(), chip8.get_sound_timer(), *dbg.I);
                debug_info_str+="\nstack: {";
                
                for (uint8_t i = 0; i<16; ++i) {
                    debug_info_str += std::format("{:#05x} ", dbg.stack[i]);
                }
                debug_info_str.pop_back();
                debug_info_str+="}\nkeys: {";
                for (uint8_t i = 0; i<16; ++i) {
                    debug_info_str += std::format("{:01x} ", dbg.keypad[i]);
                }
                debug_info_str.pop_back();
                debug_info_str += "}";

                get_text_and_rect(renderer, debug_rect.x, 0, (char*) debug_info_str.c_str(), Sans, &debug_text_texture, &debug_rect, static_cast<Uint32>(main_rect.w));
                std::string mem_view_text = std::format("{:#05x}: ", mem_view_offset);
                for (uint16_t i = 0;i<mem_view_bytes;++i) {
                    mem_view_text += std::format("{:#04x} ", chip8.get_memory()[i+mem_view_offset]);
                    if ((i+1)%mem_view_lines==0&&!(i>mem_view_bytes-mem_view_bytes_per_line)){
                        mem_view_text.pop_back();
                        mem_view_text += std::format("\n{:#05x}: ", mem_view_offset+i+1);
                    }
                }
                get_text_and_rect(renderer, debug_rect.x, debug_rect.h+5, (char*) mem_view_text.c_str(), Sans, &mem_view_texture, &mem_view_rect, static_cast<Uint32>(main_rect.w));
            }
            SDL_UpdateTexture(texture, NULL, pixels, 64 * sizeof(uint32_t));
            SDL_RenderClear(renderer);
            SDL_RenderCopy(renderer, texture, NULL, &main_rect);
            if (debug_mode) {
                SDL_SetRenderDrawColor(renderer, 255 , 255 , 255, 255);
                SDL_RenderDrawRect(renderer, &mem_view_rect);
                SDL_SetRenderDrawColor(renderer, 0 , 0 , 0, 255);
                SDL_RenderCopy(renderer, debug_text_texture, NULL, &debug_rect);
                SDL_RenderCopy(renderer, mem_view_texture, NULL, &mem_view_rect);
            }
            SDL_RenderCopy(renderer, text_texture, NULL, &help_text_rect);

            SDL_RenderPresent(renderer);
        }

        ++iter_count;
        auto now = std::chrono::steady_clock::now();
        double elapsed =
        std::chrono::duration<double>(now - start).count();
        
        start = now;
        if (!(iter_count%1024) && debug_mode) {
            std::cout << "Cycles/s: "
                    << 1.0/elapsed << '\n';
        }
        if (!fast_forward&&elapsed < 0.0015) {
            
            usleep(1500-(elapsed*1000000));
        } 
            
        

    }

    return 0;
}