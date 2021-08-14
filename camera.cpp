#include <stdio.h>
#include <stdlib.h>
#include <iostream>
#include <thread>
#include <chrono>
#include <opencv2/core.hpp>
#include <opencv2/videoio.hpp>
#include <opencv2/highgui.hpp>

using namespace cv;
using namespace std;

void thread_function()
{
    int i = 0;
    for (;;)
    {
        this_thread::sleep_for(chrono::seconds(2));
        cout << "I am a new thread " << i++ << endl;
    }
}

int show_camera()
{
    Mat frame;
    // Mat frame2;
    //--- INITIALIZE VIDEOCAPTURE
    int deviceID = 0;        // 0 = open default camera
    int apiID = cv::CAP_ANY; // 0 = autodetect default API
    VideoCapture cap(deviceID, apiID);
    // VideoCapture cap2(1, apiID);

    // check if we succeeded
    if (!cap.isOpened()) // || !cap2.isOpened())
    {
        cerr << "ERROR! Unable to open camera\n";
        return -1;
    }
    //--- GRAB AND WRITE LOOP
    cout << cv::getBuildInformation() << endl;
    cout << "opened video capure device at idx " << 0 + cv::CAP_DSHOW << endl;
    cout << "start reading" << endl;
    cout << "Start grabbing" << endl
         << "Press any key to terminate" << endl;
    for (;;)
    {
        // wait for a new frame from camera and store it into 'frame'
        cap.read(frame);
        // cap2.read(frame2);
        // check if we succeeded
        if (frame.empty()) // || frame2.empty())
        {
            cerr << "ERROR! blank frame grabbed\n";
            break;
        }
        // show live and wait for a key with timeout long enough to show images
        imshow("Live", frame);
        // imshow("Live2", frame2);
        if (waitKey(5) >= 0)
            break;
    }
    // the camera will be deinitialized automatically in VideoCapture destructor
}

int main()
{
    thread t1(thread_function);
    thread t2(show_camera);

    t1.join();
    t2.join();

    return 0;
}