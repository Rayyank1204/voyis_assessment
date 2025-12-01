
## Distributed Image Processing System

Here is my submission for the Distributed Imaging Services assessment. As required, it’s a distributed system split into three parts: an image generator, a feature processor, and a data logger. The components are loosely coupled so that you can start them in any order and they won't crash if one goes down. It's also built to handle larger files reliably and keep running continuously.

### Prerequisites

* Build Tools (Compiler & CMake), required for compiling C++ code.

  ```
  sudo apt-get update
  sudo apt-get install build-essential cmake
  ```

* OpenCV, required for image processing and SIFT feature detection.

  ```
  sudo apt-get install libopencv-dev
  ```

* ZeroMQ, required for interprocess communication.

  ```
  sudo apt-get install libzmq3-dev
  ```

* Protocol Buffers, require for serialization.

  ```
  sudo apt-get install protobuf-compiler libprotobuf-dev
  ```

* SQLite3 (Database).

  ```
  sudo apt install sqlite3
  sudo apt-get install libsqlite3-dev
  ```


### Installation


 * Clone the repository.

   ```
   git clone https://github.com/Rayyank1204/voyis_assessment.git
   ```

* Create a build directory inside the repository, and compile.

   ```
   mkdir build
   cd build
   cmake ..
   make
   ```


### Usage

To run the system manually, open three separate terminal windows and run one of the following commands in each terminal. You can run the applications in any order. This configuration allows you to monitor the individual logs of the each of the applications simultaneously. 

* Image Generator:

   ```
   cd build # if not already in build
   ./app1 ../images
   ```

* Feature Extractor:

   ```
   cd build
   ./app2
   ```

* Data Logger:

   ```
   cd build
   ./app3
   ```

* Viewing Database entries:

   ```
   cd build
   sqlite3 voyis_data.db "SELECT id, filename, keypoint_count, timestamp FROM image_logs;"
   ```

