#include "SDL2/SDL.h"

#ifdef __cplusplus
extern "C"{
#endif

SDL_Renderer* init();
void play_segment(SDL_Renderer* renderer, char* segment_file, int pixel_width, int pixel_height, int fps, int secs);

#ifdef __cplusplus
}
#endif
