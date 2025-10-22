#include <opencv2/opencv.hpp>
#include <opencv2/dnn.hpp>
#include <iostream>
#include <vector>
#include <fstream>

// Danh sách classes từ COCO dataset (80 classes)
const std::vector<std::string> classes = {
    "person", "bicycle", "car", "motorcycle", "airplane", "bus", "train", "truck", "boat", "traffic light",
    "fire hydrant", "stop sign", "parking meter", "bench", "bird", "cat", "dog", "horse", "sheep", "cow",
    "elephant", "bear", "zebra", "giraffe", "backpack", "umbrella", "handbag", "tie", "suitcase", "frisbee",
    "skis", "snowboard", "sports ball", "kite", "baseball bat", "baseball glove", "skateboard", "surfboard",
    "tennis racket", "bottle", "wine glass", "cup", "fork", "knife", "spoon", "bowl", "banana", "apple",
    "sandwich", "orange", "broccoli", "carrot", "hot dog", "pizza", "donut", "cake", "chair", "couch",
    "potted plant", "bed", "dining table", "toilet", "tv", "laptop", "mouse", "remote", "keyboard", "cell phone",
    "microwave", "oven", "toaster", "sink", "refrigerator", "book", "clock", "vase", "scissors", "teddy bear",
    "hair drier", "toothbrush"
};

struct Detection {
    cv::Rect box;
    float conf;
    int class_id;
};

void preprocess(const cv::Mat& image, cv::Mat& processed, int input_size) {
    // Resize với letterboxing để giữ aspect ratio
    int max_dim = std::max(image.cols, image.rows);
    cv::Mat padded = cv::Mat::zeros(max_dim, max_dim, CV_8UC3);
    image.copyTo(padded(cv::Rect(0, 0, image.cols, image.rows)));

    cv::resize(padded, processed, cv::Size(input_size, input_size));
    cv::cvtColor(processed, processed, cv::COLOR_BGR2RGB);  // BGR to RGB
    processed.convertTo(processed, CV_32F, 1.0 / 255.0);   // Normalize [0,1]
}

void postprocess(cv::Mat& output, std::vector<Detection>& detections, const cv::Size& original_size, float conf_threshold = 0.5, float nms_threshold = 0.45) {
    // YOLOv8 output: [1, 84, 8400] -> [batch, (4 box + 1 conf + 80 classes), num_proposals]
    int num_classes = 80;
    int num_proposals = output.size[2];
    std::vector<cv::Rect> boxes;
    std::vector<float> confs;
    std::vector<int> class_ids;

    for (int i = 0; i < num_proposals; ++i) {
        float* data = output.ptr<float>(0, 0, i);  // Row i
        float conf = data[4];  // Confidence
        if (conf > conf_threshold) {
            // Tìm class_id với score cao nhất
            float max_class_score = 0;
            int max_class_id = -1;
            for (int j = 0; j < num_classes; ++j) {
                float score = data[5 + j] * conf;  // Class prob * conf
                if (score > max_class_score) {
                    max_class_score = score;
                    max_class_id = j;
                }
            }
            if (max_class_score > conf_threshold) {
                // Box: center_x, center_y, w, h (normalized)
                float cx = data[0];
                float cy = data[1];
                float w = data[2];
                float h = data[3];
                // Denormalize to original size
                int left = static_cast<int>((cx - w / 2) * original_size.width);
                int top = static_cast<int>((cy - h / 2) * original_size.height);
                int width = static_cast<int>(w * original_size.width);
                int height = static_cast<int>(h * original_size.height);

                boxes.emplace_back(left, top, width, height);
                confs.push_back(max_class_score);
                class_ids.push_back(max_class_id);
            }
        }
    }

    // NMS
    std::vector<int> indices;
    cv::dnn::NMSBoxes(boxes, confs, conf_threshold, nms_threshold, indices);

    for (int idx : indices) {
        Detection det;
        det.box = boxes[idx];
        det.conf = confs[idx];
        det.class_id = class_ids[idx];
        detections.push_back(det);
    }
}

int main(int argc, char** argv) {
    if (argc != 3) {
        std::cerr << "Usage: " << argv[0] << " <model_path> <image_path>" << std::endl;
        return -1;
    }

    std::string model_path = argv[1];
    std::string image_path = argv[2];

    // Load model
    cv::dnn::Net net = cv::dnn::readNetFromONNX(model_path);
    if (net.empty()) {
        std::cerr << "Error loading model from " << model_path << std::endl;
        return -1;
    }
    // Nếu có CUDA: net.setPreferTarget(cv::dnn::DNN_TARGET_CUDA);

    // Load image
    cv::Mat image = cv::imread(image_path);
    if (image.empty()) {
        std::cerr << "Error loading image from " << image_path << std::endl;
        return -1;
    }
    cv::Size original_size = image.size();

    // Preprocess
    cv::Mat processed;
    int input_size = 640;
    preprocess(image, processed, input_size);

    // Blob: [1,3,640,640]
    cv::Mat blob = cv::dnn::blobFromImage(processed);

    // Inference
    auto start = std::chrono::high_resolution_clock::now();
    net.setInput(blob);
    cv::Mat output = net.forward();  // Output [1,84,8400]
    auto end = std::chrono::high_resolution_clock::now();
    double time_ms = std::chrono::duration<double, std::milli>(end - start).count();
    std::cout << "Inference time: " << time_ms << " ms" << std::endl;

    // Postprocess
    std::vector<Detection> detections;
    postprocess(output, detections, original_size);

    // Draw results
    for (const auto& det : detections) {
        cv::rectangle(image, det.box, cv::Scalar(0, 255, 0), 2);
        std::string label = classes[det.class_id] + ": " + cv::format("%.2f", det.conf);
        cv::putText(image, label, cv::Point(det.box.x, det.box.y - 10), cv::FONT_HERSHEY_SIMPLEX, 0.5, cv::Scalar(0, 255, 0), 2);
    }

    // Save output
    cv::imwrite("output.jpg", image);
    std::cout << "Detections saved to output.jpg" << std::endl;

    // In kết quả
    for (const auto& det : detections) {
        std::cout << "Class: " << classes[det.class_id] << ", Conf: " << det.conf << ", Box: (" 
                  << det.box.x << ", " << det.box.y << ", " << det.box.width << ", " << det.box.height << ")" << std::endl;
    }

    return 0;
}