# Makefile for OpenCV video player
CXX      := g++
CXXFLAGS := -std=c++17 -Wall -O2
TARGET   := video_player
SOURCES  := main.cpp

# Automatic detection of OpenCV (works on Ubuntu, macOS with pkg-config, etc.)
OPENCV_LIBS := $(shell pkg-config --cflags --libs opencv4 2>/dev/null || \
                        pkg-config --cflags --libs opencv  2>/dev/null)

FIXED_PATH := /usr/lib/x86_64-linux-gnu:/lib/x86_64-linux-gnu

# Fallback if pkg-config is missing (common on some macOS or conda setups)
ifeq ($(OPENCV_LIBS),)
    $(warning pkg-config did not find OpenCV - using common fallback flags)
    OPENCV_LIBS = -I/usr/local/include/opencv4 \
                  -L/usr/local/lib \
                  -lopencv_core -lopencv_imgproc -lopencv_imgcodecs -lopencv_videoio -lopencv_highgui
endif

# Final linker flags
LDFLAGS  := $(OPENCV_LIBS)

# Default video file (you can override from command line: make run VIDEO=myvideo.avi)
VIDEO    ?= your_video.avi

.PHONY: all clean run debug

all: $(TARGET)

$(TARGET): $(SOURCES)
	$(CXX) $(CXXFLAGS) $< -o $@ $(LDFLAGS)

# Run the player
run: $(TARGET)
	@echo "Playing: $(VIDEO)"
	@LD_LIBRARY_PATH="$(FIXED_PATH):$$LD_LIBRARY_PATH"
	@./$(TARGET) "$(VIDEO)"

# Run with valgrind (memory leak check)
valgrind: $(TARGET)
	valgrind --leak-check=full --show-leak-kinds=all ./$(TARGET) "$(VIDEO)"

# Compile with debug symbols
debug: CXXFLAGS += -g -O0
debug: $(TARGET)

# Clean build files
clean:
	rm -f $(TARGET)

# Show help
help:
	@echo "Available targets:"
	@echo "  make          - build the player"
	@echo "  make run      - build & play (default video: $(VIDEO))"
	@echo "  make run VIDEO=myfile.avi - play a different video"
	@echo "  make debug    - build with debug flags"
	@echo "  make clean    - remove executable"