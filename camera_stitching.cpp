#include <stdio.h>
#include <stdlib.h>
#include <iostream>
#include <opencv2/core.hpp>
#include <opencv2/videoio.hpp>
#include <opencv2/highgui.hpp>
#include <opencv2/imgcodecs.hpp>
#include <opencv2/stitching.hpp>
#include <opencv2/calib3d.hpp>
#include <chrono>

using namespace cv;
using namespace std;

Stitcher::Mode mode = Stitcher::PANORAMA;
vector<Mat> imgs;

int main()
{
    Mat pano;
    Mat frame;
    Mat frame2;
    vector<Mat> imgs;

    //--- INITIALIZE VIDEOCAPTURE
    int deviceID = 0;        // 0 = open default camera
    int apiID = cv::CAP_ANY; // 0 = autodetect default API
    VideoCapture cap(deviceID, apiID);
    VideoCapture cap2(1, apiID);
    Ptr<Stitcher> stitcher = Stitcher::create(mode);

    // check if we succeeded
    if (!cap.isOpened() || !cap2.isOpened())
    {
        cerr << "ERROR! Unable to open camera\n";
        return -1;
    }
    //--- GRAB AND WRITE LOOP
    cout << "Start grabbing" << endl
         << "Press any key to terminate" << endl;

    cap.read(frame);
    cap2.read(frame2);
    imgs.push_back(frame);
    imgs.push_back(frame2);

    for (;;)
    {
        // wait for a new frame from camera and store it into 'frame'
        cap.read(frame);
        cap2.read(frame2);
        // check if we succeeded
        if (frame.empty() || frame2.empty())
        {
            cerr << "ERROR! blank frame grabbed\n";
            break;
        }
        // show live and wait for a key with timeout long enough to show images
        Stitcher::Status status = stitcher->stitch(imgs, pano);
        imshow("Live", frame);
        imshow("Live2", frame2);
        if (status == cv::Stitcher::Status::OK)
        {
            imshow("stitching", pano);
        }

        if (waitKey(5) >= 0)
            break;
    }
    // the camera will be deinitialized automatically in VideoCapture destructor
    return 0;
}