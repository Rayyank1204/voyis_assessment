cmake_minimum_required(VERSION 3.10)
project(VoyisDistributedImaging)

set(CMAKE_CXX_STANDARD 17) 

find_package(OpenCV REQUIRED)
find_package(Protobuf REQUIRED)
find_package(PkgConfig REQUIRED)

pkg_check_modules(ZMQ REQUIRED libzmq) 


set(PROTO_SRC_DIR ${CMAKE_SOURCE_DIR}/src/common/proto)
set(PROTO_FILE ${PROTO_SRC_DIR}/messages.proto)

protobuf_generate_cpp(PROTO_SRCS PROTO_HDRS ${PROTO_FILE})


add_library(proto_lib ${PROTO_SRCS} ${PROTO_HDRS})
target_link_libraries(proto_lib ${Protobuf_LIBRARIES})
target_include_directories(proto_lib PUBLIC ${CMAKE_CURRENT_BINARY_DIR})


add_executable(app1 src/app1/main.cpp)

target_include_directories(app1 PRIVATE ${ZMQ_INCLUDE_DIRS})

target_link_libraries(app1 
    PRIVATE 
    ${OpenCV_LIBS} 
    proto_lib 
    ${ZMQ_LIBRARIES}
)

add_executable(app2 src/app2/main.cpp)

target_include_directories(app2 PRIVATE ${ZMQ_INCLUDE_DIRS})

target_link_libraries(app2 
    PRIVATE 
    ${OpenCV_LIBS} 
    proto_lib 
    ${ZMQ_LIBRARIES}
)

add_executable(app3 src/app3/main.cpp)

target_include_directories(app3 PRIVATE ${ZMQ_INCLUDE_DIRS})

target_link_libraries(app3 
    PRIVATE 
    proto_lib 
    ${ZMQ_LIBRARIES}
    sqlite3
)
