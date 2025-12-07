# 1. First time: just build
make

# 2. Play your video (replace name if needed)
make run VIDEO=your_video.avi

# Or if your video is named exactly "your_video.avi", just:
make run

# Other useful commands
make clean          # remove the executable
make debug          # build with debug symbols
make valgrind VIDEO=myvid.avi   # check for memory leaks