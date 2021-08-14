#include <stdio.h>
#include <stdlib.h>
#include <iostream>
#include <thread>
#include <chrono>
#include <future>
#include <opencv2/core.hpp>
#include <opencv2/videoio.hpp>
#include <opencv2/highgui.hpp>
#include <opencv2/imgproc.hpp>

using namespace cv;
using namespace std;

int show_camera(int deviceID, Mat &frame)
{
    try
    {
        //--- INITIALIZE VIDEOCAPTURE
        int apiID = cv::CAP_ANY; // 0 = autodetect default API
        VideoCapture cap(deviceID, apiID);
        cap.set(CAP_PROP_FRAME_WIDTH, 640);
        cap.set(CAP_PROP_FRAME_HEIGHT, 480);
        // check if we succeeded
        if (!cap.isOpened())
        {
            cerr << "ERROR! Unable to open camera\n";
            return -1;
        }

        //--- GRAB AND WRITE LOOP
        // cout << cv::getBuildInformation() << endl;
        // cout << "opened video capure device at idx " << 0 + cv::CAP_DSHOW << endl;
        cout << "start reading" << endl;
        cout << "Start grabbing" << endl
             << "Press any key to terminate" << endl;

        for (;;)
        {
            // wait for a new frame from camera and store it into 'frame'
            cap.read(frame);
            // check if we succeeded
            if (frame.empty())
            {
                cerr << "ERROR! blank frame grabbed\n";
                break;
            }
            // cvtColor(frame, frame, COLOR_BGR2GRAY);
            // show live and wait for a key with timeout long enough to show images
            imshow("Live " + to_string(deviceID), frame);

            if (waitKey(5) >= 0)
                break;
        }
        // the camera will be deinitialized automatically in VideoCapture destructor
    }
    catch (exception &exp)
    {
        cerr << exp.what() << endl;
    }
    return 0;
}

int main()
{
    Mat frame0, frame1;
    Mat pano;
    Mat left_1d_frame;
    Mat right_1d_frame;
    // Ptr<Stitcher> stitcher = Stitcher::create(Stitcher::PANORAMA);

    cout << "start main" << endl;
    future<int> camera0 = async(launch::async, show_camera, 0, ref(frame0));
    future<int> camera1 = async(launch::async, show_camera, 1, ref(frame1));

    for (;;)
    {
        try
        {
            if (!frame0.empty() && !frame1.empty())
            {
                // addWeighted(frame0, 0.7, frame1, 0.5, 0, pano);
                // show live and wait for a key with timeout long enough to show images
                // Stitcher::Status status = stitcher->stitch(imgs, pano);
                // if (status == cv::Stitcher::Status::OK)
                // {}

                cvtColor(frame1, right_1d_frame, COLOR_BGR2GRAY);
                cvtColor(frame0, left_1d_frame, COLOR_BGR2GRAY);
                cv::hconcat(left_1d_frame, right_1d_frame, pano);

                imshow("Stitching", pano);

                if (waitKey(5) >= 0)
                {
                    break;
                }
            }
        }
        catch (exception &exp)
        {
            cerr << exp.what() << endl;
        }
    }

    cout << "end main" << endl;

    return 0;
}