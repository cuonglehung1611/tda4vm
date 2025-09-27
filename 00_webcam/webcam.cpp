#include <opencv2/opencv.hpp>
#include <iostream>

int main() {
    // Open the default webcam (index 0)
    cv::VideoCapture cap(0);

    // Check if the webcam opened successfully
    if (!cap.isOpened()) {
        std::cerr << "Error: Could not open the webcam!" << std::endl;
        return -1;
    }

    // Optional: Set resolution (common for webcams)
    cap.set(cv::CAP_PROP_FRAME_WIDTH, 640);
    cap.set(cv::CAP_PROP_FRAME_HEIGHT, 480);

    // Retrieve and display webcam properties
    std::cout << "Webcam Details:" << std::endl;
    std::cout << "----------------" << std::endl;
    std::cout << "Width: " << cap.get(cv::CAP_PROP_FRAME_WIDTH) << " pixels" << std::endl;
    std::cout << "Height: " << cap.get(cv::CAP_PROP_FRAME_HEIGHT) << " pixels" << std::endl;
    std::cout << "Frame Rate: " << cap.get(cv::CAP_PROP_FPS) << " fps" << std::endl;
    std::cout << "Backend API: " << cap.get(cv::CAP_PROP_BACKEND) << " (See OpenCV backend codes)" << std::endl;
    std::cout << "Format: " << cap.get(cv::CAP_PROP_FORMAT) << " (Pixel format)" << std::endl;
    std::cout << "Brightness: " << cap.get(cv::CAP_PROP_BRIGHTNESS) << std::endl;
    std::cout << "Contrast: " << cap.get(cv::CAP_PROP_CONTRAST) << std::endl;
    std::cout << "Saturation: " << cap.get(cv::CAP_PROP_SATURATION) << std::endl;
    std::cout << "Hue: " << cap.get(cv::CAP_PROP_HUE) << std::endl;
    std::cout << "Gain: " << cap.get(cv::CAP_PROP_GAIN) << std::endl;
    std::cout << "Exposure: " << cap.get(cv::CAP_PROP_EXPOSURE) << std::endl;

    while (true) {
        cv::Mat frame;
        // Capture a frame from the webcam
        cap >> frame;

        // Check if the frame is empty
        if (frame.empty()) {
            std::cerr << "Error: Failed to capture a frame!" << std::endl;
            break;
        }

        // Display the frame
        cv::imshow("Cuong's Webcam", frame);

        // Exit on 'q' key press
        if (cv::waitKey(1) == 'q') {
            break;
        }
    }

    // Release the webcam and close windows
    cap.release();
    cv::destroyAllWindows();

    return 0;
}
