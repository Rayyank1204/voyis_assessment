
## Outline of Software Architecture

### Application 1: Image Generator

When app 1 is started, it:

    1) Verifies that a directory has been provided and that it is a valid directory
    2) Initializes a ZeroMQ "Publisher" socket on TCP port 5555
    3) Creates a list of all images found in the provided directory
    4) Enters an infinite loop where it:
        i) reads each image from the disk
       ii) packages the image data and metadata into a protobuf message
      iii) broadcasts it to the network
       iv) proceeds to next image, loops back to start it all images processed

### Application 2: Feature Extractor

When app 2 is started, it:

    1) Initializes a ZeroMQ "Subscriber" socket and connects to TCP port 5555
    2) Initializes a ZeroMQ "Publisher" socket and binds to TCP port 5556
    3) Creates a the OpenCV SIFT feature detector object
    4) Enters an infinite loop where it:
        i) waits for and receives an image message from the network
       ii) deserializes the binary data into an OpenCV matrix
      iii) executes the SIFT algorithm to identify key features
       iv) packages the original image and the extracted features into a new protobuf message
        v) broadcasts the processed payload to the network

### Application 3: Data Logger

When app 3 is started, it:

    1) Initializes a ZeroMQ "Subscriber" socket and connects to TCP port 5556
    2) Opens (or creates) a SQLite database
    3) Executes an SQL command to ensure the logging table exists
    4) Enters an infinite loop where it:
        i) waits for and receives a processed message from the network
       ii) deserializes the binary payload into a structured message object
      iii) prepares a SQL INSERT statement with the image metadata, keypoint count, and binary blob
       iv) executes the statement to add the entry to the database disk file


### Design decisions made

#### Inter-process communication 
  * I went with ZeroMQ for inter-process communication because its Publisher/Subscriber pattern fit the project perfectly. It lets App 1 broadcast data without needing to know whether Apps 2 or 3 are actually listening, so everything can start in any order and recover from crashes without any complicated handling. ZeroMQ’s High Water Mark also helps by automatically dropping messages if a consumer falls behind, which keeps memory from being wasted. Finally, I added a short sleep as well to simulate a realistic frame rate.

#### Serialization
   * I went with Protocol Buffers for serialization because sending raw C++ structs over a network is dangerous due to memory padding and endianness differences, which can cause data corruption. Protobuf automatically handles these low-level details and efficiently packages complex data types, which this project dealt with a lot, into a compact format with low overhead.

#### Data storage
   * I chose SQLite3 because it is serverless, zero-configuration, and I have experience coding with it in C++.

#### Testing
   * After doing some research, I decided to go with the Oxford Affine Covariant Regions Dataset for testing. It's a popular dataset for computer vision research specifically to benchmark algorithms like SIFT. I also tested using larger images (30+ MB). 

