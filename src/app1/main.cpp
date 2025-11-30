#include <vector>
#include <string>
#include <thread>
#include <chrono>
#include <iostream>
#include <filesystem>

#include <zmq.hpp>
#include "messages.pb.h"
#include <opencv2/opencv.hpp>

namespace fs = std::filesystem;

const int GLOBAL_HWM = 10;


bool is_image_file(const fs::path& path) {

    std::string ext = path.extension().string();
    std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);
    return (ext == ".jpg" || ext == ".jpeg" || ext == ".png" || ext == ".bmp" || ext == ".ppm");
}

int main(int argc, char* argv[]) {
 
    // Verify input
    if (argc < 2) {
        std::cerr << "Usage: " << argv[0] << " <image_folder_path>" << std::endl;
        return 1;
    }

    std::string folder_path = argv[1];
    if (!fs::exists(folder_path) || !fs::is_directory(folder_path)) {
        std::cerr << "Error: Invalid directory path." << std::endl;
        return 1;
    }

    // Establish IPC 
    zmq::context_t context(1);
    zmq::socket_t publisher(context, ZMQ_PUB);

    publisher.set(zmq::sockopt::sndhwm, GLOBAL_HWM);

    try {
        publisher.bind("tcp://*:5555");
        std::cout << "[App 1] Publisher bound to tcp://*:5555" << std::endl;
    } catch (const zmq::error_t& e) {
        std::cerr << "ZMQ Bind Error: " << e.what() << std::endl;
        return 1;
    }

    // Queue image files
    std::vector<std::string> image_files_names;
    for (const auto& entry : fs::directory_iterator(folder_path)) {
        if (entry.is_regular_file() && is_image_file(entry.path())) {
            image_files_names.push_back(entry.path().string());
        }
    }

    if (image_files_names.empty()) {
        std::cerr << "Error: No images found in " << folder_path << std::endl;
        return 1;
    }

    std::cout << "[App 1] Found " << image_files_names.size() << " images. Starting publication loop..." << std::endl;

    size_t index = 0;
    while (true) {
        const std::string& current_file = image_files_names[index];
        
        cv::Mat image = cv::imread(current_file, cv::IMREAD_COLOR);
        
        if (!image.empty()) {
        
            // Load protobuf object to serialize 
            voyis::data::Image proto_img;
            proto_img.set_width(image.cols);
            proto_img.set_height(image.rows);
            proto_img.set_channels(image.channels());
            proto_img.set_type(image.type());
            proto_img.set_filename(current_file);

            size_t data_size = image.total() * image.elemSize();

            proto_img.set_data(image.data, data_size);

            std::string serialized_msg;
            
            // Send package 
            if (proto_img.SerializeToString(&serialized_msg)) {
                zmq::message_t z_msg(serialized_msg.size());
                memcpy(z_msg.data(), serialized_msg.data(), serialized_msg.size());
                
                publisher.send(z_msg, zmq::send_flags::none);
                
                std::cout << "[App 1] Sent: " << current_file << " (" << data_size / 1024 << " KB)" << std::endl;
            }
        } 
        
        else {
            std::cerr << "[App 1] Warning: Failed to load " << current_file << std::endl;
        }

        index++;
        
        if (index >= image_files_names.size()) {
            index = 0;
        }

        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }

    return 0;
}


