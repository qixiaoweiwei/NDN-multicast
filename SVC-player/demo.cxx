#include "player.h"
#include "merge.hpp"
#include "decoder.h"

#include <string>
#include <stdio.h>
#include <iostream>
#include <pthread.h>

using namespace std;

int main(int argc, char* args[])
{
  for (int segment = 0; segment < 5; segment ++)
  {
    int numLayers = 2;
    string layerGroup[numLayers] = {"./seg"+to_string(segment)+"-L0.svc", "./seg"+to_string(segment)+"-L1.svc"};
    string outFile =  "./seg"+to_string(segment)+".264";
    string initLayer = "./init.svc";
    merge(layerGroup, numLayers, outFile, initLayer);
  }
  
  for (int segment = 0; segment < 5; segment++)
  {
    char h264_file[20];
    sprintf(h264_file, "./seg%d.264", segment);
    char out_file[20];
    sprintf(out_file, "./seg%d.yuv", segment);
    if (segment == 4)
      decode(h264_file, out_file, 1, 25);
    else if (segment % 2 == 1)
      decode(h264_file, out_file, 0, 48);
    else
      decode(h264_file, out_file, 1, 48);
  }
  
  SDL_Renderer* renderer = init();
  play_segment(renderer, "./seg0.yuv", 1280, 720, 24, 2);
  play_segment(renderer, "./seg1.yuv", 640, 360, 24, 2);
  play_segment(renderer, "./seg2.yuv", 1280, 720, 24, 2);
  play_segment(renderer, "./seg3.yuv", 640, 360, 24, 2);
  play_segment(renderer, "./seg4.yuv", 1280, 720, 24, 2);
  SDL_Quit();

  return 0;
}
