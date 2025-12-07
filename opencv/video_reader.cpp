#include <opencv2/opencv.hpp>
#include <iostream>

int main()
{
    std::string filename = "/home/hungc/workspace/video/cam_front.avi";
    cv::VideoCapture cap(filename);

    if (!cap.isOpened()) {
        std::cerr << "Error: Cannot open video file '" << filename << "'\n";
        std::cerr << "Make sure the file exists and the path is correct.\n";
        return -1;
    }

    // print video info
    double fps = cap.get(cv::CAP_PROP_FPS);
    int width  = static_cast<int>(cap.get(cv::CAP_PROP_FRAME_WIDTH));
    int height = static_cast<int>(cap.get(cv::CAP_PROP_FRAME_HEIGHT));
    std::cout << "Playing: " << filename << "\n";
    std::cout << "Resolution: " << width << "x" << height << "\n";
    std::cout << "FPS: " << fps << "\n";
    std::cout << "Controls:\n";
    std::cout << "   'q' or ESC  → Quit\n";
    std::cout << "   Space       → Pause / Resume\n\n";

    cv::Mat frame;
    bool paused = false;

    while (true) {
        if (!paused) {
            if (!cap.read(frame)) {  // cap.read() returns false at end of video
                std::cout << "End of video.\n";
                break;
            }
        }

        cv::imshow("Video Player", frame);

        int key = cv::waitKey(static_cast<int>(1000.0 / fps)) & 0xFF;

        if (key == 'q' || key == 27) {        // 'q' or ESC
            break;
        }
        else if (key == ' ') {             // Space → pause/unpause
            paused = !paused;
            std::cout << (paused ? "Paused (press Space to resume)" : "Resumed") << "\n";
        }
    }

    cap.release();
    cv::destroyAllWindows();
    std::cout << "Playback finished.\n";

    return 0;
}