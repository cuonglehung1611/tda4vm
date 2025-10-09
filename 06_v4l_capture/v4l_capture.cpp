#include <opencv2/opencv.hpp>
#include <linux/videodev2.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <iostream>
#include <string>

#define VIDEO_DEVICE "/dev/video0"
#define WIDTH 640
#define HEIGHT 480
#define FORMAT V4L2_PIX_FMT_YUYV

struct buffer {
    void* start;
    size_t length;
};

// Function to print pixel format for debugging
std::string fourccToString(uint32_t fourcc) {
    char fcc[5] = {
        static_cast<char>(fourcc & 0xFF),
        static_cast<char>((fourcc >> 8) & 0xFF),
        static_cast<char>((fourcc >> 16) & 0xFF),
        static_cast<char>((fourcc >> 24) & 0xFF),
        0
    };
    return std::string(fcc);
}

int main() {
    // Open the webcam device
    int fd = open(VIDEO_DEVICE, O_RDWR);
    if (fd == -1) {
        std::cerr << "Failed to open webcam device: " << VIDEO_DEVICE << std::endl;
        return -1;
    }

    // Query device capabilities
    struct v4l2_capability cap = {0};
    if (ioctl(fd, VIDIOC_QUERYCAP, &cap) == -1) {
        std::cerr << "Failed to query device capabilities!" << std::endl;
        close(fd);
        return -1;
    }
    std::cout << "Device: " << cap.card << ", Driver: " << cap.driver << std::endl;

    // Check supported pixel formats
    struct v4l2_fmtdesc fmt_desc = {0};
    fmt_desc.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    std::cout << "Supported pixel formats:" << std::endl;
    while (ioctl(fd, VIDIOC_ENUM_FMT, &fmt_desc) != -1) {
        std::cout << "  " << fourccToString(fmt_desc.pixelformat) << ": " << fmt_desc.description << std::endl;
        fmt_desc.index++;
    }

    // Set video format
    struct v4l2_format fmt = {0};
    fmt.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    fmt.fmt.pix.width = WIDTH;
    fmt.fmt.pix.height = HEIGHT;
    fmt.fmt.pix.pixelformat = FORMAT;
    fmt.fmt.pix.field = V4L2_FIELD_ANY;

    if (ioctl(fd, VIDIOC_S_FMT, &fmt) == -1) {
        std::cerr << "Failed to set video format!" << std::endl;
        close(fd);
        return -1;
    }

    // Verify the set format
    if (ioctl(fd, VIDIOC_G_FMT, &fmt) == -1) {
        std::cerr << "Failed to get video format!" << std::endl;
        close(fd);
        return -1;
    }
    std::cout << "Set format: " << fourccToString(fmt.fmt.pix.pixelformat)
              << ", " << fmt.fmt.pix.width << "x" << fmt.fmt.pix.height << std::endl;

    // Request buffers
    struct v4l2_requestbuffers req = {0};
    req.count = 4;
    req.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    req.memory = V4L2_MEMORY_MMAP;

    if (ioctl(fd, VIDIOC_REQBUFS, &req) == -1) {
        std::cerr << "Buffer request failed!" << std::endl;
        close(fd);
        return -1;
    }

    // Allocate buffers
    buffer* buffers = new buffer[req.count];
    for (unsigned int i = 0; i < req.count; i++) {
        struct v4l2_buffer buf = {0};
        buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
        buf.memory = V4L2_MEMORY_MMAP;
        buf.index = i;

        if (ioctl(fd, VIDIOC_QUERYBUF, &buf) == -1) {
            std::cerr << "Buffer query failed!" << std::endl;
            delete[] buffers;
            close(fd);
            return -1;
        }

        buffers[i].length = buf.length;
        buffers[i].start = mmap(NULL, buf.length, PROT_READ | PROT_WRITE, MAP_SHARED, fd, buf.m.offset);
        if (buffers[i].start == MAP_FAILED) {
            std::cerr << "MMAP failed!" << std::endl;
            delete[] buffers;
            close(fd);
            return -1;
        }
    }

    // Start streaming
    for (unsigned int i = 0; i < req.count; i++) {
        struct v4l2_buffer buf = {0};
        buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
        buf.memory = V4L2_MEMORY_MMAP;
        buf.index = i;
        if (ioctl(fd, VIDIOC_QBUF, &buf) == -1) {
            std::cerr << "Queue buffer failed!" << std::endl;
            delete[] buffers;
            close(fd);
            return -1;
        }
    }

    enum v4l2_buf_type type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    if (ioctl(fd, VIDIOC_STREAMON, &type) == -1) {
        std::cerr << "Stream ON failed!" << std::endl;
        delete[] buffers;
        close(fd);
        return -1;
    }

    // Create OpenCV window
    cv::namedWindow("Webcam", cv::WINDOW_AUTOSIZE);
    cv::Mat frame(HEIGHT, WIDTH, CV_8UC3);

    while (true) {
        struct v4l2_buffer buf = {0};
        buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
        buf.memory = V4L2_MEMORY_MMAP;

        // Dequeue buffer
        if (ioctl(fd, VIDIOC_DQBUF, &buf) == -1) {
            std::cerr << "Dequeue buffer failed!" << std::endl;
            break;
        }

        // Convert YUYV to BGR
        cv::Mat yuyv(HEIGHT, WIDTH, CV_8UC2, buffers[buf.index].start);
        cv::cvtColor(yuyv, frame, cv::COLOR_YUV2BGR_YUYV); // Use YUYV conversion

        // Display frame
        cv::imshow("Webcam", frame);

        // Requeue buffer
        if (ioctl(fd, VIDIOC_QBUF, &buf) == -1) {
            std::cerr << "Queue buffer failed!" << std::endl;
            break;
        }

        // Exit on ESC key
        if (cv::waitKey(1) == 27) break;
    }

    // Stop streaming
    if (ioctl(fd, VIDIOC_STREAMOFF, &type) == -1) {
        std::cerr << "Stream OFF failed!" << std::endl;
    }

    // Free buffers
    for (unsigned int i = 0; i < req.count; i++) {
        munmap(buffers[i].start, buffers[i].length);
    }
    delete[] buffers;

    // Close device
    close(fd);

    // Close OpenCV window
    cv::destroyAllWindows();

    return 0;
}