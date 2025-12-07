# Configuration
APP       := cam_capture
SRC       := $(APP).cpp
OBJS      := $(SRC:.cpp=.o)
DEPS      := $(OBJS:.o=.d)

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
			 -lpthread                                                  \
			 -lstdc++``

# ===============================================================
.PHONY: all clean run

all: $(APP)

# Link the final executable
$(APP): $(OBJS)
	$(CXX) $(CXXFLAGS) $(OBJS) -o $@ $(LDFLAGS) $(LIBS)

# Compile .cpp → .o and automatically generate dependencies
%.o: %.cpp
	$(CXX) $(CXXFLAGS) $(INCLUDES) -MMD -MP -c $< -o $@

# Include auto-generated dependency files (speeds up incremental builds)
-include $(DEPS)

clean:
	$(RM) $(OBJS) $(DEPS) $(APP)

run: $(APP)
	./$(APP)
