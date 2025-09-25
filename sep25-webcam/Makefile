all: clean build run

build:
	g++ -o webcam webcam.cpp `pkg-config --cflags --libs opencv4`

run:
	./webcam

clean:
	rm webcam
