APPNAME=v4l_capture
SOURCES=v4l_capture.cpp
INCLUDES= -I/usr/include/opencv4
LDFLAGS+= -lstdc++ -lpthread -lopencv_highgui -lopencv_core -lopencv_imgproc -lopencv_video -lopencv_videoio

all: $(APPNAME)

$(APPNAME):
	$(CXX) $(CXXFLAGS) $(INCLUDES) $(SOURCES) -o $(APPNAME) $(LDFLAGS)

clean:
	$(RM) *.o
	$(RM) $(APPNAME)