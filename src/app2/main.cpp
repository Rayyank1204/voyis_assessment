#include <vector>
#include <string>
#include <thread>
#include <chrono>
#include <iostream>

#include <zmq.hpp>
#include "messages.pb.h"
#include <opencv2/opencv.hpp>


int main() {

    //Establish connection with App 1
    zmq::context_t context(1);
    zmq::socket_t subscriber(context, ZMQ_SUB);
    
    try {
        subscriber.connect("tcp://localhost:5555");
        subscriber.set(zmq::sockopt::subscribe, "");
        std::cout << "[App 2] Connected to App 1 (Generator)" << std::endl;
    } catch (const zmq::error_t& e) {
        std::cerr << "ZMQ Connect Error: " << e.what() << std::endl;
        return 1;
    }

    // Set up IPC with app 3
    zmq::socket_t publisher(context, ZMQ_PUB);
    
    try {
        publisher.bind("tcp://*:5556"); 
        std::cout << "[App 2] Publishing processed data on tcp://*:5556" << std::endl;
    } catch (const zmq::error_t& e) {
        std::cerr << "ZMQ Bind Error: " << e.what() << std::endl;
        return 1;
    }
    
    auto sift_detector = cv::SIFT::create();

    while (true) {
        zmq::message_t z_msg;
        
        // Receive image from app 1
        auto res = subscriber.recv(z_msg, zmq::recv_flags::none);
        if (!res) continue;
	
        std::string received_data(static_cast<char*>(z_msg.data()), z_msg.size());
        voyis::data::Image input_proto;
        if (!input_proto.ParseFromString(received_data)) {
            std::cerr << "[App 2] Error: Failed to parse incoming protobuf." << std::endl;
            continue;
        }

        cv::Mat img(
            input_proto.height(),
            input_proto.width(),
            input_proto.type(),
            (void*)input_proto.data().data()
        );

        if (img.empty()) {
            std::cerr << "[App 2] Error: Decoded image is empty." << std::endl;
            continue;
        }

	// Perform SIFT
        std::vector<cv::KeyPoint> keypoints;
        sift_detector->detect(img, keypoints);

        voyis::data::FeaturePayload output_payload;
        
        output_payload.mutable_original_image()->CopyFrom(input_proto);
 	
 	// Load keypoint data via protobuf for serialization
        for (const auto& kp : keypoints) {
            auto* proto_kp = output_payload.add_keypoints();
            proto_kp->set_x(kp.pt.x);
            proto_kp->set_y(kp.pt.y);
            proto_kp->set_size(kp.size);
            proto_kp->set_angle(kp.angle);
            proto_kp->set_response(kp.response);
            proto_kp->set_octave(kp.octave);
            proto_kp->set_class_id(kp.class_id);
        }

        std::string serialized_msg;
        
        // Send package
        if (output_payload.SerializeToString(&serialized_msg)) {
            zmq::message_t out_msg(serialized_msg.size());
	    memcpy(out_msg.data(), serialized_msg.data(), serialized_msg.size());
		
	    publisher.send(out_msg, zmq::send_flags::none);

	    std::cout << "[App 2] Processed " << input_proto.filename() << " | Found " << keypoints.size() << " features." << std::endl;
        }
    }

    return 0;
}


