#include <opencv2/opencv.hpp>
#include <linux/videodev2.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <unistd.h>
#include <iostream>
#include <cstring>

// #define VIDEO_DEVICE "/dev/video0"
#define WIDTH 640
#define HEIGHT 480
#define FORMAT V4L2_PIX_FMT_YUYV 	// Try YUYV first
                                    // Alternative formats:
                                    //	V4L2_PIX_FMT_MJPEG
                                    //	V4L2_PIX_FMT_YUV420
#define CLEAR(x) memset(&(x), 0, sizeof(x))

struct Buffer {
    void *start;
    size_t length;
};

class V4L2Capture {
public:
    V4L2Capture() : fd(-1), n_buffers(0), buffers(nullptr) {}
    ~V4L2Capture() {
        stop();
        if (buffers) delete[] buffers;
    }
    
    int init(char* video_device) {
        // Open camera device
        fd = open(video_device, O_RDWR);
        if (fd == -1) {
            std::cerr << "Failed to open " << video_device << std::endl;
            return -1;
        }
        
        // Query capabilities
        if (ioctl(fd, VIDIOC_QUERYCAP, &cap) == -1){
            std::cerr << "Failed to query capabilities" << std::endl;
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
        CLEAR(fmt);
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
        struct v4l2_requestbuffers req;
        CLEAR(req);
        req.count = 4;
        req.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
#if USE_USER_POINTER
        req.memory = V4L2_MEMORY_USERPTR;
#else
        req.memory = V4L2_MEMORY_MMAP;
#endif

        if (ioctl(fd, VIDIOC_REQBUFS, &req) == -1) {
            std::cerr << "Buffer request failed!" << std::endl;
            close(fd);
            return -1;
        }
        std::cout << "Required Buffers: " << req.count << std::endl;

        // Request stream param
        struct v4l2_streamparm stream_parm;
        stream_parm.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
        if (ioctl(fd, VIDIOC_G_PARM, &stream_parm) != 0) {
            std::cerr << "Video get param error" << std::endl;
            close(fd);
            return -1;
        }
        stream_parm.parm.capture.capability = V4L2_CAP_TIMEPERFRAME;
        stream_parm.parm.capture.timeperframe.numerator = 1;
        //! set the fps value in the denominator for capturing rate
        stream_parm.parm.capture.timeperframe.denominator = 30;
        if (ioctl(fd, VIDIOC_G_PARM, &stream_parm) != 0) {
            std::cerr << "Video set param error" << std::endl;
            close(fd);
            return -1;
        }

        // Allocate buffers
        n_buffers = req.count;
        buffers = new Buffer[n_buffers];
        for (unsigned int i = 0; i < n_buffers; i++) {
            struct v4l2_buffer buf;
            CLEAR(buf);
            buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
            buf.memory = V4L2_MEMORY_MMAP;
            buf.index = i;

            if (ioctl(fd, VIDIOC_QUERYBUF, &buf) == -1) {
                std::cerr << "Buffer query failed!" << std::endl;
                delete[] buffers;
                close(fd);
                return -1;
            }

            // Map buffers
            buffers[i].length = buf.length;
            buffers[i].start = mmap(NULL, buf.length, PROT_READ | PROT_WRITE, MAP_SHARED, fd, buf.m.offset);
            if (buffers[i].start == MAP_FAILED) {
                std::cerr << "MMAP failed!" << std::endl;
                delete[] buffers;
                close(fd);
                return -1;
            }
                
            // Start streaming
            if (ioctl(fd, VIDIOC_QBUF, &buf) == -1) {
                std::cerr << "Queue buffer failed!" << std::endl;
                delete[] buffers;
                close(fd);
                return -1;
            }
        }
        
        return 0;
    }

    int start() {
        // Start capture
        buf_type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
        if (ioctl(fd, VIDIOC_STREAMON, &buf_type) == -1) {
            std::cerr << "Stream ON failed!" << std::endl;
            delete[] buffers;
            close(fd);
            return -1;
        }
        return 0;
    }
    
    void stop() {
        if (ioctl(fd, VIDIOC_STREAMOFF, &buf_type) != 0) {
            std::cerr << "Stream OFF failed!" << std::endl;
        }
        if (fd != -1) {
            close(fd);
            fd = -1;
        }
    }
    
    cv::Mat captureFrame() {
        struct v4l2_buffer buf;
        CLEAR(buf);
        buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
        buf.memory = V4L2_MEMORY_MMAP;
        
        // Dequeue buffer
        if (ioctl(fd, VIDIOC_DQBUF, &buf) == -1) {
            std::cerr << "Dequeue buffer failed!" << std::endl;
            return cv::Mat();
        }
        
        // Convert BGR
        cv::Mat frame;
        if (fmt.fmt.pix.pixelformat == V4L2_PIX_FMT_YUYV) {
            // Convert YUYV to BGR
            cv::Mat yuyv(HEIGHT, WIDTH, CV_8UC2, buffers[buf.index].start);
            cv::cvtColor(yuyv, frame, cv::COLOR_YUV2BGR_YUYV); // Use YUYV conversion
        } else if (fmt.fmt.pix.pixelformat == V4L2_PIX_FMT_MJPEG) {
            // Decode MJPEG
            cv::Mat jpeg_data(fmt.fmt.pix.sizeimage, 1, CV_8UC1, buffers[buf.index].start);
            std::vector<uchar> jpeg_vec(jpeg_data.data, jpeg_data.data + jpeg_data.total());
            // frame = cv::imdecode(jpeg_vec, cv::IMREAD_COLOR);
        } else {
            // Raw format - may need custom conversion
            frame = cv::Mat(fmt.fmt.pix.height, fmt.fmt.pix.width, CV_8UC3, 
                          buffers[buf.index].start);
        }
        
        // Requeue buffer
        if (ioctl(fd, VIDIOC_QBUF, &buf) == -1) {
            std::cerr << "Queue buffer failed!" << std::endl;
            delete[] buffers;
            close(fd);
        }
        
        return frame;
    }
    
private:
    int fd;
    struct v4l2_capability cap;
    struct v4l2_format fmt;
    unsigned int n_buffers;
    Buffer *buffers;
    enum v4l2_buf_type buf_type;
    
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
};

int main(int argc, char** argv)
{
    if(argc < 3)
    {
        std::cout << "USAGE:" << std::endl;
        std::cout << "  v4l_capture <video device> <nframes>" << std::endl;
        std::cout << "      <video device> can be /dev/video1 " << std::endl;
        std::cout << "      <nframes> if 0 runs forever" << std::endl;
        return 0;
    }

    V4L2Capture capture;
    if (capture.init(argv[1]) == -1) {
        return -1;
    }	
    capture.start();
    
    // Create OpenCV window
    cv::namedWindow("V4L2 Capture", cv::WINDOW_AUTOSIZE);
    cv::Mat frame(HEIGHT, WIDTH, CV_8UC3);

    int frameCount = 0;
    int RUN_FOR_FRAMES = atoi(argv[2]);
    while (!RUN_FOR_FRAMES || frameCount < RUN_FOR_FRAMES) {
        frame = capture.captureFrame();
        if(frame.empty()){
            std::cerr << "Failed to capture frame" << std::endl;
            break;
        }
        
        cv::imshow("V4L2 Capture", frame);
        
        // Exit on ESC key
        if (cv::waitKey(1) == 27) break;

        std::cout << "Frame Count: " << frameCount << std::endl;
        frameCount++;
    }
    
    cv::destroyAllWindows();
    return 0;
}
