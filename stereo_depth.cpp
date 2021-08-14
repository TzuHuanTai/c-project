#include <stdio.h>
#include <stdlib.h>
#include <iostream>
#include <thread>
#include <chrono>
#include <future>
#include <opencv2/core.hpp>
#include <opencv2/videoio.hpp>
#include <opencv2/highgui.hpp>
#include <opencv2/stitching.hpp>
#include <opencv2/calib3d.hpp>
#include <opencv2/video/background_segm.hpp>
#include <opencv2/imgproc.hpp>
#include <opencv2/features2d.hpp>
#include <opencv2/xfeatures2d.hpp>
#include <opencv2/xfeatures2d/nonfree.hpp>

using namespace cv;
using namespace std;
using namespace cv::xfeatures2d;

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
    Mat frame0gray, frame1gray;
    Mat dispbm, dispsgbm, disparity, disparity1;
    Mat dispnorm_bm, dispnorm_sgbm;
    Mat falseColorsMap, sfalseColorsMap, falsemap;
    Mat img_matches;
    Mat H1, H2;

    Mat pano;
    Mat fgMask;
    Mat tempMat;
    Mat left_1d_frame;
    Mat right_1d_frame;
    Mat disp, disp8, disp8_3c;
    Ptr<StereoBM> sbm = StereoBM::create(16, 7);
    Ptr<StereoSGBM> sgbm = StereoSGBM::create(0, 16, 7);
    // Ptr<Stitcher> stitcher = Stitcher::create(Stitcher::PANORAMA);

    //-- Check its extreme values
    double minVal;
    double maxVal;
    double max_dist = 0;
    double min_dist = 100;
    int minHessian = 630;
    // cv::Ptr<cv::xfeatures2d::SURF> f2d = cv::xfeatures2d::SURF::create();
    // cv::Ptr<cv::xfeatures2d::SURF> fd = cv::xfeatures2d::SURF::create();
    Ptr<Feature2D> f2d = cv::SIFT::create();
    vector<KeyPoint> keypoints_1, keypoints_2;
    Ptr<Feature2D> fd = cv::SIFT::create();
    Mat descriptors_1, descriptors_2;
    BFMatcher matcher(NORM_L2, true); //BFMatcher matcher(NORM_L2);
    vector<DMatch> matches;
    vector<DMatch> good_matches;
    vector<Point2f> imgpts1, imgpts2;
    vector<uchar> status;

    //create Background Subtractor objects
    Ptr<BackgroundSubtractor> pBackSub;
    pBackSub = createBackgroundSubtractorKNN();

    cout << "start main" << endl;
    future<int> camera0 = async(launch::async, show_camera, 0, ref(frame0));
    future<int> camera1 = async(launch::async, show_camera, 1, ref(frame1));

    for (;;)
    {
        try
        {
            if (!frame0.empty() && !frame1.empty())
            {
                // // addWeighted(frame0, 0.7, frame1, 0.5, 0, pano);
                // // show live and wait for a key with timeout long enough to show images
                // // Stitcher::Status status = stitcher->stitch(imgs, pano);
                // // if (status == cv::Stitcher::Status::OK)
                // // {

                // // // threshold(frame1, tempMat, 0, 255, THRESH_BINARY);
                // cvtColor(frame1, right_1d_frame, COLOR_BGR2GRAY);
                // // // cvtColor(tempMat, tempMat, COLOR_GRAY2BGR);
                // // // threshold(tempMat, tempMat, 0, 255, THRESH_BINARY);

                // // // pBackSub->apply(frame0, fgMask, 0);

                // cvtColor(frame0, left_1d_frame, COLOR_BGR2GRAY);
                // // cv::hconcat(left_1d_frame, right_1d_frame, pano);
                // // // cvtColor(tempMat, tempMat, COLOR_GRAY2BGR);
                // // // bitwise_and(frame0, frame0, tempMat, fgMask);
                // // // cvtColor(tempMat, tempMat, COLOR_GRAY2BGR);

                // stereo->compute(left_1d_frame, right_1d_frame, disp);

                // // cout << "channels: " << disp.channels() << ", type: " << disp.type() << endl;

                // disp.convertTo(disp8, CV_8U);

                // convertScaleAbs(disp8, tempMat, 0.256);
                // applyColorMap(tempMat, disp8_3c, COLORMAP_JET);

                // imshow("Stitching", disp8_3c);
                // // imshow("fgMask", fgMask);

                // // }

                // if (waitKey(5) >= 0)
                // {
                //     break;
                // }

                cvtColor(frame0, frame0gray, COLOR_BGR2GRAY);
                cvtColor(frame1, frame1gray, COLOR_BGR2GRAY);

                sbm->compute(frame0gray, frame1gray, dispbm);
                minMaxLoc(dispbm, &minVal, &maxVal);
                dispbm.convertTo(dispnorm_bm, CV_8UC1, 255 / (maxVal - minVal));

                sgbm->compute(frame0gray, frame1gray, dispsgbm);
                minMaxLoc(dispsgbm, &minVal, &maxVal);
                dispsgbm.convertTo(dispnorm_sgbm, CV_8UC1, 255 / (maxVal - minVal));
                applyColorMap(dispnorm_bm, falseColorsMap, cv::COLORMAP_JET);
                applyColorMap(dispnorm_sgbm, sfalseColorsMap, cv::COLORMAP_JET);

                f2d->detect(frame0gray, keypoints_1);
                f2d->detect(frame1gray, keypoints_2);

                //-- Step 2: Calculate descriptors (feature vectors)
                fd->compute(frame0gray, keypoints_1, descriptors_1);
                fd->compute(frame1gray, keypoints_2, descriptors_2);

                //-- Step 3: Matching descriptor vectors with a brute force matcher

                matcher.match(descriptors_1, descriptors_2, matches);
                drawMatches(frame0gray, keypoints_1, frame1gray, keypoints_2, matches, img_matches);
                imshow("matches", img_matches);

                //-- Quick calculation of max and min distances between keypoints
                for (int i = 0; i < matches.size(); i++)
                {
                    double dist = matches[i].distance;
                    if (dist < min_dist)
                        min_dist = dist;
                    if (dist > max_dist)
                        max_dist = dist;
                }

                for (int i = 0; i < matches.size(); i++)
                {
                    if (matches[i].distance <= max(4.5 * min_dist, 0.02))
                    {
                        good_matches.push_back(matches[i]);
                        imgpts1.push_back(keypoints_1[matches[i].queryIdx].pt);
                        imgpts2.push_back(keypoints_2[matches[i].trainIdx].pt);
                    }
                }

                Mat F = findFundamentalMat(imgpts1, imgpts2, cv::FM_RANSAC, 3., 0.99, status); //FM_RANSAC
                stereoRectifyUncalibrated(imgpts1, imgpts1, F, frame0gray.size(), H1, H2);
                Mat rectified1(frame0gray.size(), frame0gray.type());
                warpPerspective(frame0gray, rectified1, H1, frame0gray.size());

                Mat rectified2(frame1gray.size(), frame1gray.type());
                warpPerspective(frame1gray, rectified2, H2, frame1gray.size());

                sgbm->compute(rectified1, rectified2, disparity);
                minMaxLoc(disparity, &minVal, &maxVal);
                disparity.convertTo(disparity1, CV_8UC1, 255 / (maxVal - minVal));
                applyColorMap(disparity1, falsemap, cv::COLORMAP_JET);
                imshow("disparity_rectified_color", falsemap);
                imshow("BM", falseColorsMap);
                imshow("CSGBM", sfalseColorsMap);

                //wait for 40 milliseconds
                int c = waitKey(40);

                //exit the loop if user press "Esc" key  (ASCII value of "Esc" is 27)
                if (27 == char(c))
                    break;
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