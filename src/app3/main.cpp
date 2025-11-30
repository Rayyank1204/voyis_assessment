#include <ctime>
#include <vector>
#include <string>
#include <chrono>
#include <iostream>

#include <zmq.hpp>
#include <sqlite3.h>
#include "messages.pb.h"


std::string get_timestamp() {
    auto now = std::chrono::system_clock::now();
    std::time_t now_time = std::chrono::system_clock::to_time_t(now);
    char buffer[100];
    std::strftime(buffer, sizeof(buffer), "%Y-%m-%d %H:%M:%S", std::localtime(&now_time));
    return std::string(buffer);
}

int main() {
    
    // Establish connection with app 2
    zmq::context_t context(1);
    zmq::socket_t subscriber(context, ZMQ_SUB);
    try {
        subscriber.connect("tcp://localhost:5556");
        subscriber.set(zmq::sockopt::subscribe, "");
        std::cout << "[App 3] Connected to App 2 (Feature Extractor)" << std::endl;
    } catch (const zmq::error_t& e) {
        std::cerr << "ZMQ Connect Error: " << e.what() << std::endl;
        return 1;
    }

    // Initialize / Open Database
    sqlite3* db;
    int rc = sqlite3_open("voyis_data.db", &db);
    
    if (rc) {
        std::cerr << "Can't open database: " << sqlite3_errmsg(db) << std::endl;
        return 1;
    } 
    
    else {
        std::cout << "[App 3] Opened database 'voyis_data.db'" << std::endl;
    }

    const char* sql_create = 
        "CREATE TABLE IF NOT EXISTS image_logs ("
        "id INTEGER PRIMARY KEY AUTOINCREMENT, "
        "timestamp TEXT, "
        "filename TEXT, "
        "image_width INT, "
        "image_height INT, "
        "keypoint_count INT, "
        "image_data BLOB, "
        "keypoints_blob BLOB);";

    char* errMsg = 0;
    rc = sqlite3_exec(db, sql_create, 0, 0, &errMsg);
    if (rc != SQLITE_OK) {
        std::cerr << "SQL Error: " << errMsg << std::endl;
        sqlite3_free(errMsg);
        return 1;
    }

    while (true) {
    
    	// Receive image and keypoints from app 2
        zmq::message_t z_msg;
        auto res = subscriber.recv(z_msg, zmq::recv_flags::none);
        if (!res) continue;

        std::string received_data(static_cast<char*>(z_msg.data()), z_msg.size());
        voyis::data::FeaturePayload payload;
        if (!payload.ParseFromString(received_data)) {
            std::cerr << "[App 3] Error: Failed to parse payload." << std::endl;
            continue;
        }

        const auto& img = payload.original_image();
        int kp_count = payload.keypoints_size();

        const char* sql_insert = "INSERT INTO image_logs (timestamp, filename, image_width, image_height, keypoint_count, image_data) VALUES (?, ?, ?, ?, ?, ?);";
        sqlite3_stmt* stmt;
        
        // Insert entry into database
        if (sqlite3_prepare_v2(db, sql_insert, -1, &stmt, 0) == SQLITE_OK) {
            std::string ts = get_timestamp();
            sqlite3_bind_text(stmt, 1, ts.c_str(), -1, SQLITE_STATIC);
            sqlite3_bind_text(stmt, 2, img.filename().c_str(), -1, SQLITE_STATIC);
            sqlite3_bind_int(stmt, 3, img.width());
            sqlite3_bind_int(stmt, 4, img.height());
            sqlite3_bind_int(stmt, 5, kp_count);
            sqlite3_bind_blob(stmt, 6, img.data().data(), img.data().size(), SQLITE_STATIC);


            if (sqlite3_step(stmt) != SQLITE_DONE) {
                std::cerr << "SQL Insert Error: " << sqlite3_errmsg(db) << std::endl;
            } 
            
            else {
                std::cout << "[App 3] Saved: " << img.filename() 
                          << " | Keypoints: " << kp_count 
                          << " | DB ID: " << sqlite3_last_insert_rowid(db) << std::endl;
            }
            
            sqlite3_finalize(stmt);
        } 
        
        else {
            std::cerr << "SQL Prepare Error: " << sqlite3_errmsg(db) << std::endl;
        }
    }

    sqlite3_close(db);
    
    return 0;
}


