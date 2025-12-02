// main.cpp
#include <opencv2/opencv.hpp>
#include <iostream>
#include <string>

using namespace cv;
using namespace std;

int main(int argc, char** argv)
{
    // Change this to your video path
    string videoPath = "your_video.avi";

    // If you pass the path as command-line argument (recommended)
    if (argc >= 2) {
        videoPath = argv[1];
    }

    // Open the video file
    VideoCapture cap(videoPath);

    // Check if video opened successfully
    if (!cap.isOpened()) {
        cerr << "ERROR: Could not open video file: " << videoPath << endl;
        cerr << "Possible reasons:" << endl;
        cerr << "  - Wrong file path" << endl;
        cerr << "  - File is corrupted" << endl;
        cerr << "  - Missing codec support in OpenCV build" << endl;
        return -1;
    }

    // Optional: print video info
    double fps = cap.get(CAP_PROP_FPS);
    int width  = (int)cap.get(CAP_PROP_FRAME_WIDTH);
    int height = (int)cap.get(CAP_PROP_FRAME_HEIGHT);
    
    cout << "Video opened successfully!" << endl;
    cout << "Resolution: " << width << "x" << height << endl;
    cout << "FPS: " << fps << endl;
    cout << "Press 'q' or ESC to quit" << endl;

    namedWindow("Video Playback", WINDOW_AUTOSIZE);

    Mat frame;
    while (true)
    {
        cap >> frame;                    // Read next frame
        if (frame.empty()) {             // End of video or error
            cout << "End of video" << endl;
            break;
        }

        imshow("Video Playback", frame);

        // Wait ~1ms; 27 = ESC key, 'q' = quit
        char key = (char)waitKey(1);
        if (key == 'q' || key == 27) {
            break;
        }
    }

    // Cleanup
    cap.release();
    destroyAllWindows();
    cout << "Playback finished." << endl;

    return 0;
}