CXX = g++
CXXFLAGS = -std=c++17 -Wall
INCLUDES = `pkg-config --cflags opencv4`
LIBS = `pkg-config --libs opencv4`
TARGET = detector
SOURCE = object_detection.cpp

$(TARGET): $(SOURCE)
	$(CXX) $(CXXFLAGS) $(INCLUDES) -o $(TARGET) $(SOURCE) $(LIBS)

clean:
	rm -f $(TARGET)

test: $(TARGET)
	./$(TARGET) ../yolov8n.onnx bus.jpg

.PHONY: clean test