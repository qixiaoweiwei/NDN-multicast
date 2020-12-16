#ifdef __cplusplus
extern "C"{
#endif

void player_init();
void player_quit();
void play_segment(char* segment_file, int pixel_width, int pixel_height, int fps, int secs);

#ifdef __cplusplus
}
#endif
