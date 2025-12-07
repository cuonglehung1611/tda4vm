#include <opencv2/opencv.hpp>
#include <iostream>

int main()
{
	std::string filename = "video.avi";
	cv::VideoCapture cap(filename);

	if (!cap.isOpened()) {
        std::cerr << "Error: Cannot open video file '" << filename << "'\n";
        std::cerr << "Make sure the file exists and the path is correct.\n";
        return -1;
    }
	return 0;

}