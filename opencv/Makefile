APPNAME=v4l_app
# SOURCES=v4l_view.cpp
SOURCES=v4l_capture.cpp
INCLUDES= -I/usr/local/include/opencv4
LDFLAGS+= -lstdc++ -lpthread -lopencv_highgui -lopencv_core -lopencv_imgproc -lopencv_video -lopencv_videoio
LDFLAGS+= -L/usr/local/lib

all: clean $(APPNAME) run

$(APPNAME):
	$(CXX) $(CXXFLAGS) $(INCLUDES) $(SOURCES) -o $(APPNAME) $(LDFLAGS)

clean:
	$(RM) *.o
	$(RM) $(APPNAME)

run:
	./$(APPNAME) /dev/video0 10
