from merge import merge
from ctypes import *

decoder = cdll.LoadLibrary("./libOpenSVCDecoder.so")
player = cdll.LoadLibrary("./libPlayer.so")

for segment in range(0, 5):
    layerGroup = ["./seg"+str(segment)+"-L0.svc", "./seg"+str(segment)+"-L1.svc"]
    outFile =  "./seg"+str(segment)+".264"
    initLayer = "./init.svc"
    merge(layerGroup, outFile, initLayer)
    
for segment in range(0, 5):
    h264_file = "./seg"+str(segment)+".264"
    out_file = "./seg"+str(segment)+".yuv"
    if segment == 4:
      decoder.decode(h264_file, out_file, 1, 25)
    elif segment % 2 == 1:
      decoder.decode(h264_file, out_file, 0, 48)
    else:
      decoder.decode(h264_file, out_file, 1, 48)


player.player_init()
player.play_segment("./seg0.yuv", 1280, 720, 24, 2)
player.play_segment("./seg1.yuv", 640, 360, 24, 2)
player.play_segment("./seg2.yuv", 1280, 720, 24, 2)
player.play_segment("./seg3.yuv", 640, 360, 24, 2)
player.play_segment("./seg4.yuv", 1280, 720, 24, 2)
player.player_quit()
