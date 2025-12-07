#include <iostream>
#include <iomanip>
#include <vector>
#include <yaml-cpp/yaml.h>

struct TopViewCamera {
    std::string name;
    int width, height;
    double cx, cy;
    std::vector<double> direct_map;  // exactly 5 coefficients
};

int main() {
    try {
        YAML::Node root = YAML::LoadFile("/home/hungc/workspace/video/calib.yml");

        int fps = root["FPS"].as<int>();
        std::cout << "FPS: " << fps << "\n\n";

        const auto& omnicam = root["CamCalib"]["OmniCam"];
        std::vector<std::string> cams = {"FV", "RV", "ML", "MR"};

        std::vector<TopViewCamera> topview_cams;

        for (const auto& name : cams) {
            if (!omnicam[name]) {
                std::cerr << "Missing camera: " << name << "\n";
                continue;
            }

            const auto& cam = omnicam[name];

            TopViewCamera c;
            c.name   = name;
            c.width  = cam["ImageSize"]["Width"].as<int>();
            c.height = cam["ImageSize"]["Height"].as<int>();
            c.cx     = cam["Center"]["Xc"].as<double>();
            c.cy     = cam["Center"]["Yc"].as<double>();
            c.direct_map = cam["DirectMap"].as<std::vector<double>>();

            if (c.direct_map.size() != 5) {
                std::cerr << name << ": DirectMap must have exactly 5 values!\n";
                continue;
            }

            topview_cams.push_back(c);
        }

        // Print all loaded top-view cameras
        std::cout << "Loaded " << topview_cams.size() << " cameras for top-view generation:\n\n";

        for (const auto& cam : topview_cams) {
            std::cout << "=== " << cam.name << " (Top-View Ready) ===\n";
            std::cout << "Resolution : " << cam.width << "x" << cam.height << "\n";
            std::cout << "Center     : (" << std::fixed << std::setprecision(3)
                      << cam.cx << ", " << cam.cy << ")\n";
            std::cout << "DirectMap  : [";
            for (size_t i = 0; i < 5; ++i) {
                std::cout << std::scientific << std::setprecision(6) << cam.direct_map[i];
                if (i < 4) std::cout << ", ";
            }
            std::cout << "]\n\n";
        }

        std::cout << "You now have everything needed to generate 4 top-view images!\n";
        std::cout << "Next step: Use OpenCV remap() with a map generated from DirectMap polynomial.\n";

    }
    catch (const YAML::Exception& e) {
        std::cerr << "YAML Error: " << e.what() << "\n";
        return 1;
    }

    return 0;
}