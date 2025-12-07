# Configuration
APP       ?= cam_capture
BUILD_DIR := out
SRC       := $(APP).cpp
OBJ       := $(BUILD_DIR)/$(APP).o
DEP       := $(OBJ:.o=.d)
TARGET    := $(BUILD_DIR)/$(APP)

# Compiler and flags
CXX      := g++
CXXFLAGS := -std=c++17 -O2 -march=native -pipe      # or -O0 -g for debugging
CXXFLAGS += -Wall -Wextra -Wpedantic

# OpenCV paths (adjust only if your installation differs)
OPENCV_INC := /usr/local/include/opencv4
OPENCV_LIB := /usr/local/lib

# Includes and libraries
INCLUDES  := -I$(OPENCV_INC)
LDFLAGS   := -L$(OPENCV_LIB)
LIBS      := -lopencv_core                                              \
			 -lopencv_imgproc                                           \
			 -lopencv_imgcodecs                                         \
			 -lopencv_highgui                                           \
			 -lopencv_videoio                                           \
			 -lopencv_video                                             \
			 -lopencv_calib3d                                           \
			 -lyaml-cpp                                           \
			 -lpthread                                                  \
			 -lstdc++``

# ===============================================================
.PHONY: all clean

all: $(TARGET)

# Link the final executable
$(TARGET): $(OBJ)
	$(CXX) $(OBJ) -o $@ $(LDFLAGS) $(LIBS)

# Compile .cpp → .o and automatically generate dependencies
$(OBJ): $(SRC) | $(BUILD_DIR)
	$(CXX) $(CXXFLAGS) $(INCLUDES) -MMD -MP -c $< -o $@

# Create build directory if it doesn't exist
$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)

# Include auto-generated dependency files (speeds up incremental builds)
-include $(DEP)

clean:
	$(RM) -r $(BUILD_DIR)

# Optional: nice help message
help:
	@echo "Usage:"
	@echo "  make              # build default app: cam_capture → out/cam_capture"
	@echo "  make video_reader # build video_reader → out/video_reader"
	@echo "  make clean        # remove entire out/ folder"
	@echo ""
	@echo "All binaries and temporary files are stored in ./out/"