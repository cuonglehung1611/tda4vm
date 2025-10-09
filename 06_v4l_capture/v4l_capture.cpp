#include <stdio.h>
#include <iostream>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <getopt.h>             //! getopt_long()
#include <fcntl.h>              //! low-level i/o
#include <unistd.h>
#include <errno.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/time.h>
#include <sys/mman.h>
#include <sys/ioctl.h>
#include <linux/videodev2.h>
#include <sys/time.h>
#include <chrono>
#include <thread>
#include <mutex>
#include "opencv2/opencv.hpp"

#define CLEAR(x) memset(&(x), 0, sizeof(x))
//! modify width and height to change capture resolution
#define MAX_CAP_WIDTH                  1280
#define MAX_CAP_HEIGHT                 720
// to run for indefinite time make this to 0

//!To update and display capture rate after every REFRESH_RATE frames
#define MAX_PORT_PER_CAM               3
#define MAX_FMT_PER_PORT               4

#define USE_USER_POINTER               0
#define ENABLE_RENDERING               1
// possible values: displayGrey or displayRGB
#define DISPLAY_THREAD                 displayRGB
// below macro reverse the bits - for LVDS OV10635 camera
#define ENABLE_BIT_REVERSE             1

//! Buffer used by Driver
#define RING_BUF_COUNT                 5
//! Buffer used across threads
#define PING_PONG_SIZE                 3
//! Skip frames in display thread
#define SKIP_FRAME_COUNT               30

int RUN_FOR_FRAMES = 0;

using namespace std;
using namespace cv;

struct port{
    //! variable to hold the start address of camera
    unsigned int ui32PortStartAddr;
    //! variable to hold no of ports acquired by a camera
    unsigned int ui32NoOfPorts;
    //! variable to hold the supported formats per each port
    unsigned int aui32FmtSupport[MAX_PORT_PER_CAM][MAX_FMT_PER_PORT];
};

struct buffer_ {
    void   *start;
    size_t  length;
};

//! Extension of V4L API in a loop until it succeeds
int xioctl(int si32Fd, int si32Request, void *parg)
{
    int si32ReturnVal = 0;
    do
    {
        //! v4l2 API to si32Request for an operation with camera
        si32ReturnVal = ioctl(si32Fd, si32Request, parg);
    } while (-1 == si32ReturnVal);// && EINTR == errno);
    if(si32ReturnVal)
        cout << si32ReturnVal << endl;
    //! loop continue only on interrupt error
    return si32ReturnVal;
}

static const unsigned char BitReverseTable256[] =
{
  0x00, 0x80, 0x40, 0xC0, 0x20, 0xA0, 0x60, 0xE0, 0x10, 0x90, 0x50, 0xD0, 0x30, 0xB0, 0x70, 0xF0, 
  0x08, 0x88, 0x48, 0xC8, 0x28, 0xA8, 0x68, 0xE8, 0x18, 0x98, 0x58, 0xD8, 0x38, 0xB8, 0x78, 0xF8, 
  0x04, 0x84, 0x44, 0xC4, 0x24, 0xA4, 0x64, 0xE4, 0x14, 0x94, 0x54, 0xD4, 0x34, 0xB4, 0x74, 0xF4, 
  0x0C, 0x8C, 0x4C, 0xCC, 0x2C, 0xAC, 0x6C, 0xEC, 0x1C, 0x9C, 0x5C, 0xDC, 0x3C, 0xBC, 0x7C, 0xFC, 
  0x02, 0x82, 0x42, 0xC2, 0x22, 0xA2, 0x62, 0xE2, 0x12, 0x92, 0x52, 0xD2, 0x32, 0xB2, 0x72, 0xF2, 
  0x0A, 0x8A, 0x4A, 0xCA, 0x2A, 0xAA, 0x6A, 0xEA, 0x1A, 0x9A, 0x5A, 0xDA, 0x3A, 0xBA, 0x7A, 0xFA,
  0x06, 0x86, 0x46, 0xC6, 0x26, 0xA6, 0x66, 0xE6, 0x16, 0x96, 0x56, 0xD6, 0x36, 0xB6, 0x76, 0xF6, 
  0x0E, 0x8E, 0x4E, 0xCE, 0x2E, 0xAE, 0x6E, 0xEE, 0x1E, 0x9E, 0x5E, 0xDE, 0x3E, 0xBE, 0x7E, 0xFE,
  0x01, 0x81, 0x41, 0xC1, 0x21, 0xA1, 0x61, 0xE1, 0x11, 0x91, 0x51, 0xD1, 0x31, 0xB1, 0x71, 0xF1,
  0x09, 0x89, 0x49, 0xC9, 0x29, 0xA9, 0x69, 0xE9, 0x19, 0x99, 0x59, 0xD9, 0x39, 0xB9, 0x79, 0xF9, 
  0x05, 0x85, 0x45, 0xC5, 0x25, 0xA5, 0x65, 0xE5, 0x15, 0x95, 0x55, 0xD5, 0x35, 0xB5, 0x75, 0xF5,
  0x0D, 0x8D, 0x4D, 0xCD, 0x2D, 0xAD, 0x6D, 0xED, 0x1D, 0x9D, 0x5D, 0xDD, 0x3D, 0xBD, 0x7D, 0xFD,
  0x03, 0x83, 0x43, 0xC3, 0x23, 0xA3, 0x63, 0xE3, 0x13, 0x93, 0x53, 0xD3, 0x33, 0xB3, 0x73, 0xF3, 
  0x0B, 0x8B, 0x4B, 0xCB, 0x2B, 0xAB, 0x6B, 0xEB, 0x1B, 0x9B, 0x5B, 0xDB, 0x3B, 0xBB, 0x7B, 0xFB,
  0x07, 0x87, 0x47, 0xC7, 0x27, 0xA7, 0x67, 0xE7, 0x17, 0x97, 0x57, 0xD7, 0x37, 0xB7, 0x77, 0xF7, 
  0x0F, 0x8F, 0x4F, 0xCF, 0x2F, 0xAF, 0x6F, 0xEF, 0x1F, 0x9F, 0x5F, 0xDF, 0x3F, 0xBF, 0x7F, 0xFF
};

#if ENABLE_BIT_REVERSE
void do_reverse(unsigned char* src, unsigned int len)
{
    for (int i = 0; i < len; i++)
	{
		src[i] =  BitReverseTable256[src[i]];
	}
}
#endif

void uyvytogrey(int width, int height, unsigned char* src, unsigned char * dstPtrGrey, unsigned char * dstPtrRGB)
{
    unsigned char currY, nxtY, currU, currV;
    unsigned char * pBuffer = src;
    unsigned char * outputRGB = dstPtrRGB;
    unsigned char * outputGrey = dstPtrGrey;
    unsigned int bufferSize = width * height * 2;

    for (int i = 0; i < bufferSize ; i += 4) {
        *outputGrey++ = pBuffer[i + 1];
        *outputGrey++ = pBuffer[i + 3];
    }
}

void uyvytoRGB(int width, int height, unsigned char* src, unsigned char * dstPtrGrey, unsigned char * dstPtrRGB)
{
    unsigned char currY, nxtY, currU, currV;
    unsigned char * pBuffer = src;
    unsigned char * outputRGB = dstPtrRGB;
    unsigned char * outputGrey = dstPtrGrey;
    unsigned int bufferSize = width * height * 2;

    for (int i = 0; i < bufferSize ; i += 4) {
        currU = pBuffer[i];
        currY = pBuffer[i + 1];
        currV = pBuffer[i + 2];
        nxtY = pBuffer[i + 3];

        //RGB
        unsigned char b0 = (uchar) (currY + 1.772 * (currU - 128));
        unsigned char g0 = (uchar) (currY - 0.344 * (currU - 128) - 0.714 * (currV - 128));
        unsigned char r0 = (uchar) (currY + 1.402 * (currV - 128));

        *outputRGB++ = b0;
        *outputRGB++ = g0;
        *outputRGB++ = r0;

        unsigned char b1 = (uchar) (nxtY + 1.772 * (currU - 128));
        unsigned char g1 = (uchar) (nxtY - 0.344 * (currU - 128) - 0.714 * (currV - 128));
        unsigned char r1 = (uchar) (nxtY + 1.402 * (currV - 128));

        *outputRGB++ = b1;
        *outputRGB++ = g1;
        *outputRGB++ = r1;

        //Grey
        *outputGrey++ = currY;
        *outputGrey++ = nxtY;
    }
}

volatile bool do_run = true;
volatile bool buf_ready = false;
Mutex buffer_access;
unsigned char *buff[PING_PONG_SIZE];
unsigned char *bufferQ[RING_BUF_COUNT];
volatile int buffId = 0;
volatile int frameNo = 0;

void displayGrey(int width, int height)
{
    Mat rgbFrame(height, width, CV_8UC3);
    Mat greyFrame(height, width, CV_8UC1);
#if ENABLE_RENDERING
    imshow("captured frame", rgbFrame);
#endif
    int buffSize = height * width * 2; //YUV422 size
    int frameCount = 1;
    bool newData = false;
    unsigned char *buffer = (unsigned char*)malloc(buffSize);
    while(1) {
        auto b4conv = std::chrono::time_point_cast<std::chrono::milliseconds>(std::chrono::system_clock::now());
        buffer_access.lock();
        if(buf_ready && (frameCount * SKIP_FRAME_COUNT < frameNo))
        {
            memcpy(buffer, buff[buffId], buffSize);
            buf_ready = false;
            newData = true;
            buffId = (buffId + 1) % PING_PONG_SIZE;
            frameCount = frameNo / SKIP_FRAME_COUNT;
            if(!do_run)
            {
                buffer_access.unlock();
                break;
            }
        }
        buffer_access.unlock();
        if(newData)
        {
            newData = false;
            uyvytogrey(width, height, buffer, greyFrame.data, rgbFrame.data);
            auto afconv = std::chrono::time_point_cast<std::chrono::milliseconds>(std::chrono::system_clock::now());
#if ENABLE_RENDERING
            imshow("captured frame", greyFrame);
            waitKey(1);
#endif
            auto afdisp = std::chrono::time_point_cast<std::chrono::milliseconds>(std::chrono::system_clock::now());
            //cout << (afconv - b4conv).count() << "/" << (afdisp - afconv).count() << " ms" << endl;
        }
    }
}

void displayRGB(int width, int height)
{
    Mat rgbFrame(height, width, CV_8UC3);
    Mat greyFrame(height, width, CV_8UC1);
#if ENABLE_RENDERING
    imshow("captured frame", rgbFrame);
#endif
    int buffSize = height * width * 2; //YUV422 size 
    int frameCount = 1;
    bool newData = false;
    unsigned char *buffer = (unsigned char*)malloc(buffSize); 
    while(1) {
        auto b4conv = std::chrono::time_point_cast<std::chrono::milliseconds>(std::chrono::system_clock::now());
        buffer_access.lock();
        if(buf_ready && (frameCount * SKIP_FRAME_COUNT < frameNo))
        {
            //cout << "reading frame " << buffId << endl;
            memcpy(buffer, buff[buffId], buffSize);
            buf_ready = false;
            newData = true;
            buffId = (buffId + 1) % PING_PONG_SIZE;
            frameCount = frameNo / SKIP_FRAME_COUNT;
            if(!do_run)
            {
                buffer_access.unlock();
                break;
            }
        }
        buffer_access.unlock();
        if(newData)
        {
            newData = false;
#if ENABLE_BIT_REVERSE
            do_reverse(buffer, buffSize);
#endif
            uyvytoRGB(width, height, buffer, greyFrame.data, rgbFrame.data);
            auto afconv = std::chrono::time_point_cast<std::chrono::milliseconds>(std::chrono::system_clock::now());
#if ENABLE_RENDERING
            imshow("captured frame", rgbFrame);
            waitKey(1);
#endif
            auto afdisp = std::chrono::time_point_cast<std::chrono::milliseconds>(std::chrono::system_clock::now());
            //cout << (afconv - b4conv).count() << "/" << (afdisp - afconv).count() << " ms" << endl;
        }
    }
}

int main(int argc, char** argv)
{
    unsigned int si32RecentPortNumber = 0;
    char achDevName[256] = "";
    int i32FileDes = -1;
    //! Struct to identify cam serial number
    struct v4l2_capability stDevCapability = {0};
    //! Struct to identify format
    struct v4l2_fmtdesc stFmtDesc = {0};

    //! Finding all devices in the host and detailing its capabilities
    while(1)
    {
        //! Creating char file address of device
        sprintf(achDevName,"/dev/video%d", si32RecentPortNumber);
        //! open the device
        i32FileDes = open(achDevName,O_RDWR /*| O_NONBLOCK*/, 0);
        //! Check for successfull file opening
        if (-1 != i32FileDes)
        {
            cout << "Came file " << achDevName << " opened" << endl;
            //! Query and check the capturing capability of camera
            if(-1 != xioctl(i32FileDes, VIDIOC_QUERYCAP, &stDevCapability))
            {
                cout << "\t Capabilities of Camera:" << endl;
                cout << "\t Driver: " << stDevCapability.driver << endl;
                cout << "\t Card: " << stDevCapability.card << endl;
                cout << "\t Bus info: " << stDevCapability.bus_info << endl;
                cout << "\t Capability: " << stDevCapability.capabilities << endl;
                cout << "\t Device Capability: " << stDevCapability.device_caps << endl << endl;

                //! Setting values to query the supported capturing formats
                stFmtDesc.index = 0;
                stFmtDesc.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
                //! Query the supported formats
                while(0 == ioctl(i32FileDes, VIDIOC_ENUM_FMT, &stFmtDesc))
                {
                    cout << "\t\t Pixel format: " << stFmtDesc.pixelformat << endl;
                    cout << "\t\t Type: " << stFmtDesc.type << endl;
                    cout << "\t\t Flags: " << stFmtDesc.flags << endl;
                    cout << "\t\t Description: " << stFmtDesc.description << endl << endl;
                    stFmtDesc.index++;
                }
                si32RecentPortNumber++;
                close(i32FileDes);
            }
            else
            {
                cout << "\t Capabilities read error" << endl;
                close(i32FileDes);
                break;
            }
        }
        else
        {
            cout << "Came file " << achDevName << " couldn't open" << endl;
            break;
        }
    }

	if(argc < 3)
	{
		std::cout << "USAGE:" << std::endl;
		std::cout << "  v4l_capture <video device> <nframes>" << std::endl;
		std::cout << "      <video device> can be /dev/video1 " << std::endl;
		std::cout << "      <nframes> if 0 runs for ever" << std::endl;
		return 0;
	}
    std::string devicename(argv[1]);
    int RUN_FOR_FRAMES = atoi(argv[2]);

    //! Accessing our required device
    i32FileDes = open(argv[1], O_RDWR | O_NONBLOCK, 0);
    if (0 > i32FileDes)
    {
        cout << "File open error" << endl;
        return 1;
    }

    //! initialise the opened device
    struct v4l2_format stDevfmt;
    unsigned int ui3Four4CC = v4l2_fourcc('U', 'Y', 'V', 'Y');
    CLEAR(stDevfmt);
    stDevfmt.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    stDevfmt.fmt.pix.width       = MAX_CAP_WIDTH;
    stDevfmt.fmt.pix.height      = MAX_CAP_HEIGHT;
    stDevfmt.fmt.pix.pixelformat = ui3Four4CC;
    stDevfmt.fmt.pix.field       = V4L2_FIELD_NONE;
    if (0 != xioctl(i32FileDes, VIDIOC_S_FMT, &stDevfmt))
    {
        cout << "Format Set Error" << endl;
        close(i32FileDes);
        return 2;
    }

    if (0 != xioctl(i32FileDes, VIDIOC_G_FMT, &stDevfmt))
    {
        cout << "Format Set Error" << endl;
        close(i32FileDes);
        return 2;
    }
    int width = stDevfmt.fmt.pix.width;
    int height = stDevfmt.fmt.pix.height;
    int buffSize = height * width * 2; //YUV422 size 
    cout << "Resolution: " << width << "x" << height << endl;

    //! perform memory allocation
    struct v4l2_requestbuffers reqBuf;
    CLEAR(reqBuf);
    reqBuf.count = RING_BUF_COUNT;
    reqBuf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
#if USE_USER_POINTER
    reqBuf.memory = V4L2_MEMORY_USERPTR;
#else
    reqBuf.memory = V4L2_MEMORY_MMAP;
#endif

    if (0 != xioctl(i32FileDes, VIDIOC_REQBUFS, &reqBuf))
    {
        cout << "Required Buffer Query Error" << endl;
        close(i32FileDes);
        return 2;
    }
    cout << "Required Buffers: " << reqBuf.count << endl;

    struct v4l2_streamparm stStreamParam;
    stStreamParam.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    if (0 != xioctl(i32FileDes, VIDIOC_G_PARM, &stStreamParam))
    {
        cout << "Video Get Param Error" << endl;
        close(i32FileDes);
        return 3;
    }
    stStreamParam.parm.capture.capability = V4L2_CAP_TIMEPERFRAME;
    stStreamParam.parm.capture.timeperframe.numerator = 1;
    //! set the fps value in the denominator for capturing rate
    stStreamParam.parm.capture.timeperframe.denominator = 30;
    if (0 != xioctl(i32FileDes, VIDIOC_S_PARM, &stStreamParam))
    {
        cout << "Video Set Param Error" << endl;
        close(i32FileDes);
        return 4;
    }

    struct v4l2_buffer stMapBuf;
    buffer_* pstCamBuffers = (buffer_*)calloc(reqBuf.count,sizeof(buffer_));
#if USE_USER_POINTER
    memset (&stMapBuf, 0, sizeof (stMapBuf));
    stMapBuf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    stMapBuf.memory = V4L2_MEMORY_USERPTR;
    if(0 != xioctl(i32FileDes, VIDIOC_QUERYBUF, &stMapBuf))
    {
        cout << "Buffer Length Query Error" << endl;
        free(pstCamBuffers);
        close(i32FileDes);
        return 5;
    }
    for (int si32LoopVar = 0; si32LoopVar < reqBuf.count;si32LoopVar++)
    {
        CLEAR(stMapBuf);
        stMapBuf.type        = V4L2_BUF_TYPE_VIDEO_CAPTURE;
        stMapBuf.memory      = V4L2_MEMORY_USERPTR;
        stMapBuf.index       = si32LoopVar;
        bufferQ[si32LoopVar] = (unsigned char*)malloc(buffSize); //for uvvy data
        stMapBuf.m.userptr = pstCamBuffers[si32LoopVar].start = bufferQ[si32LoopVar];
        pstCamBuffers[si32LoopVar].length = stMapBuf.length = buffSize;
 
        if(0 != xioctl(i32FileDes, VIDIOC_QBUF, &stMapBuf))
        {
            cout << "Queue Buffer Error" << endl;
            free(pstCamBuffers);
            close(i32FileDes);
            return 7;
        }
    }
#else
    for (int si32LoopVar = 0; si32LoopVar < reqBuf.count;si32LoopVar++)
    {
        CLEAR(stMapBuf);
        stMapBuf.type        = V4L2_BUF_TYPE_VIDEO_CAPTURE;
        stMapBuf.memory      = V4L2_MEMORY_MMAP;
        stMapBuf.index       = si32LoopVar;
        if(0 != xioctl(i32FileDes, VIDIOC_QUERYBUF, &stMapBuf))
        {
            cout << "Buffer Length Query Error" << endl;
            free(pstCamBuffers);
            close(i32FileDes);
            return 5;
        }

        pstCamBuffers[si32LoopVar].start =
                    mmap(NULL,stMapBuf.length,PROT_READ | PROT_WRITE,MAP_SHARED,
                    i32FileDes,stMapBuf.m.offset);
        if(MAP_FAILED == pstCamBuffers[si32LoopVar].start)
        {
            cout << "Memory Mapping Error" << endl;
            free(pstCamBuffers);
            close(i32FileDes);
            return 6;
        }
        pstCamBuffers[si32LoopVar].length = stMapBuf.length;

        if(0 != xioctl(i32FileDes, VIDIOC_QBUF, &stMapBuf))
        {
            cout << "Queue Buffer Error" << endl;
            free(pstCamBuffers);
            close(i32FileDes);
            return 7;
        }
    }
#endif
    for(int i = 0; i < PING_PONG_SIZE; i++)
    {
        buff[i] = (unsigned char*)malloc(buffSize); //for uvvy data
    }

    enum v4l2_buf_type stBufType = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    if(0 != xioctl(i32FileDes, VIDIOC_STREAMON, &stBufType))
    {
        cout << "Stream On Error" << endl;
        free(pstCamBuffers);
        close(i32FileDes);
        return 8;
    }

    //! variable for setting the FILE DESCRIPTOR
    fd_set fds;
    FD_ZERO(&fds);
    FD_SET(i32FileDes, &fds);

    //! time out limit setting variable
    struct timeval stTimeOutVal;
    //! Timeout.
    stTimeOutVal.tv_sec = 10;
    stTimeOutVal.tv_usec = 0;
    int si32FDselectStatus = select(i32FileDes + 1, &fds, NULL,
            NULL, &stTimeOutVal);
    if (0 == si32FDselectStatus)
    {
        cout << "select timeout" << endl;
        free(pstCamBuffers);
        close(i32FileDes);
        return 9;
    }
    else if (0 > si32FDselectStatus)
    {
        cout << "select error: " << si32FDselectStatus << endl;
        free(pstCamBuffers);
        close(i32FileDes);
        return 10;
    }

    struct timespec now;
    int frameCount = 0;
    // display thread creation to do the rendering
    std::thread display(DISPLAY_THREAD, width, height);
    while((!RUN_FOR_FRAMES || frameCount < RUN_FOR_FRAMES))
    {
        CLEAR(stMapBuf);
        stMapBuf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
        stMapBuf.memory = V4L2_MEMORY_MMAP;
        if(0 != xioctl(i32FileDes, VIDIOC_DQBUF, &stMapBuf))
        {
            cout << "Dequeue Buffer Error" << endl;
            break;
        }
        clock_gettime(CLOCK_MONOTONIC, &now);
        buffer_access.lock();
        memcpy(buff[buffId], pstCamBuffers[stMapBuf.index].start, stMapBuf.bytesused);
        //cout << "writing frame " << buffId << endl;
        frameNo = frameCount = stMapBuf.sequence;
        buf_ready = true;
        buffer_access.unlock();
        cout << "Buffer " << stMapBuf.sequence << " ready at: " << pstCamBuffers[stMapBuf.index].start << " at "
             << ((stMapBuf.timestamp.tv_sec * 1000) + (stMapBuf.timestamp.tv_usec / 1000))
             << " ms displayed at " << ((now.tv_sec * 1000) + (now.tv_nsec / 1000000)) << " ms" << endl;
        if(0 != xioctl(i32FileDes, VIDIOC_QBUF, &stMapBuf))
        {
            cout << "Re-queue Buffer Error" << endl;
            break;
        }
    }
    do_run = false;

    if(0 != xioctl(i32FileDes, VIDIOC_STREAMOFF, &stBufType))
    {
        cout << "Stream Off Error" << endl;
        free(pstCamBuffers);
        close(i32FileDes);
        return 13;
    }

    for(int si32LoopVar = 0; si32LoopVar < reqBuf.count; si32LoopVar++)
    {
        if (-1 == munmap(pstCamBuffers[si32LoopVar].start,
                         pstCamBuffers[si32LoopVar].length))
        {
            cout << "Memory Un-Mapping Error" << endl;
            free(pstCamBuffers);
            close(i32FileDes);
            return 14;
        }
    }

    display.join();
    for(int i = 0; i < PING_PONG_SIZE; i++)
    {
        free(buff[i]);
    }

    free(pstCamBuffers);
    close(i32FileDes);

    return 0;
}
