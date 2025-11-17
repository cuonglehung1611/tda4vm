#include <opencv2/opencv.hpp>
#include <opencv2/dnn.hpp>
#include <iostream>
#include <vector>
#include <fstream>

using namespace cv;
using namespace dnn;
using namespace std;

struct Detection {
    int class_id;
    float confidence;
    Rect box;
};

vector<string> loadClasses(const string& classesFile) {
    vector<string> classes;
    ifstream ifs(classesFile.c_str());
    string line;
    while (getline(ifs, line)) classes.push_back(line);
    return classes;
}

vector<Detection> postprocess(Mat& frame, const vector<Mat>& outs, float confThreshold, float nmsThreshold) {
    vector<Detection> detections;
    vector<int> classIds;
    vector<float> confidences;
    vector<Rect> boxes;

    for (const auto& output : outs) {
        for (int i = 0; i < output.rows; ++i) {
            Mat scores = output.row(i).colRange(5, output.cols);
            Point classIdPoint;
            double confidence;
            minMaxLoc(scores, 0, &confidence, 0, &classIdPoint);

            if (confidence > confThreshold) {
                int centerX = (int)(output.at<float>(i, 0) * frame.cols);
                int centerY = (int)(output.at<float>(i, 1) * frame.rows);
                int width = (int)(output.at<float>(i, 2) * frame.cols);
                int height = (int)(output.at<float>(i, 3) * frame.rows);
                int left = centerX - width / 2;
                int top = centerY - height / 2;

                classIds.push_back(classIdPoint.x);
                confidences.push_back((float)confidence);
                boxes.push_back(Rect(left, top, width, height));
            }
        }
    }

    vector<int> indices;
    NMSBoxes(boxes, confidences, confThreshold, nmsThreshold, indices);
    for (int idx : indices) {
        Detection det;
        det.class_id = classIds[idx];
        det.confidence = confidences[idx];
        det.box = boxes[idx];
        detections.push_back(det);
    }
    return detections;
}

int main(int argc, char** argv) {
    if (argc < 3) {
        cout << "Usage: " << argv[0] << " <image_path> <model.onnx> [classes.txt]" << endl;
        return -1;
    }

    string imagePath = argv[1];
    string modelPath = argv[2];
    string classesFile = (argc > 3) ? argv[3] : "coco.names";  // Download from ultralytics/coco

    // Load classes
    vector<string> classes = loadClasses(classesFile);

    // Load image and model
    Mat frame = imread(imagePath);
    if (frame.empty()) {
        cout << "Could not load image" << endl;
        return -1;
    }

    Net net = readNet(modelPath);
    net.setPreferableBackend(DNN_BACKEND_OPENCV);
    net.setPreferableTarget(DNN_TARGET_CPU);  // Use DNN_TARGET_OPENCL for accel

    // Preprocess
    Mat blob;
    Scalar mean(0, 0, 0);
    blobFromImage(frame, blob, 1./255., Size(640, 640), mean, true, false, CV_32F);
    net.setInput(blob);

    // Forward pass
    vector<Mat> outs;
    vector<String> outNames = net.getUnconnectedOutLayersNames();
    net.forward(outs, outNames);

    // Postprocess
    float confThreshold = 0.5;
    float nmsThreshold = 0.4;
    vector<Detection> detections = postprocess(frame, outs, confThreshold, nmsThreshold);

    // Draw results
    for (const auto& det : detections) {
        rectangle(frame, det.box, Scalar(0, 255, 0), 2);
        string label = classes[det.class_id] + ": " + to_string(det.confidence);
        putText(frame, label, Point(det.box.x, det.box.y - 10), FONT_HERSHEY_SIMPLEX, 0.5, Scalar(0, 255, 0), 2);
    }

    imshow("Detection", frame);
    waitKey(0);
    imwrite("output.jpg", frame);

    return 0;
}