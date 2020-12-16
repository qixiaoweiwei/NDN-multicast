#include "SDL2/SDL.h" 
#include "SDL2/SDL_image.h" 
#include "SDL2/SDL_timer.h"
#include "unistd.h"
#include "string.h"
#include "stdio.h"
#include "player.h"

#define window_width 1280
#define window_height 720

SDL_Renderer* renderer;
SDL_Window* window;

void player_init()
{
  SDL_Init(SDL_INIT_VIDEO);
  window = SDL_CreateWindow("SVC Player", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, window_width, window_height, 0);
  renderer = SDL_CreateRenderer(window, -1, 0);
}

void player_quit()
{
  SDL_DestroyRenderer(renderer);
  SDL_DestroyWindow(window);
  SDL_Quit();
}

void play_segment(char* segment_file, int pixel_width, int pixel_height, int fps, int secs)
{
  SDL_Texture* texture = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_IYUV, SDL_TEXTUREACCESS_STREAMING, pixel_width, pixel_height);
  int bpp = 12, pixel_size = pixel_width*pixel_height*bpp/8;
  unsigned char* pixels = (unsigned char*)malloc(sizeof(unsigned char)*pixel_size);
  FILE* fp=fopen(segment_file, "rb+");
  for (int frame = 0; frame < secs * fps; frame++)
  {
    if(fread(pixels, 1, pixel_size, fp) == 0)
      break;
    SDL_UpdateTexture(texture, NULL, pixels, pixel_width);
    SDL_RenderClear(renderer);
    SDL_RenderCopy(renderer, texture, NULL, NULL);
    SDL_RenderPresent(renderer);
    usleep(1000000/fps);
  }
  fclose(fp);
  free(pixels);
  SDL_DestroyTexture(texture);
}
