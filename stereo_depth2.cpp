#include <iostream>
#include <opencv2/core.hpp>
#include <opencv2/calib3d.hpp>
#include <opencv2/highgui.hpp>
#include <opencv2/imgproc.hpp>
#include <opencv2/features2d.hpp>
#include <opencv2/xfeatures2d.hpp>

using namespace cv;
using namespace std;
using namespace cv::xfeatures2d;

void get_keypoints_and_descriptors(
    Mat imgL,
    Mat imgR,
    vector<KeyPoint> &keypoints_L,
    vector<KeyPoint> &keypoints_R,
    Mat &descriptors_L,
    Mat &descriptors_R,
    vector<vector<DMatch>> &knn_matches_result)
{
    //-- Step 1: Detect the keypoints using SURF Detector, compute the descriptors
    int minHessian = 400;
    Ptr<SURF> detector = SURF::create(minHessian);
    detector->detectAndCompute(imgL, noArray(), keypoints_L, descriptors_L);
    detector->detectAndCompute(imgR, noArray(), keypoints_R, descriptors_R);
    //-- Step 2: Matching descriptor vectors with a FLANN based matcher
    // Since SURF is a floating-point descriptor NORM_L2 is used
    Ptr<DescriptorMatcher> matcher = DescriptorMatcher::create(DescriptorMatcher::FLANNBASED);
    matcher->knnMatch(descriptors_L, descriptors_R, knn_matches_result, 2);
}

vector<DMatch> filter_knn_matches(vector<vector<DMatch>> knn_matches,
                                  float ratio_threshold = 0.7f)
{
    vector<DMatch> good_matches;
    for (size_t i = 0; i < knn_matches.size(); i++)
    {
        if (knn_matches[i][0].distance < ratio_threshold * knn_matches[i][1].distance)
        {
            good_matches.push_back(knn_matches[i][0]);
        }
    }
    return good_matches;
}

Mat compute_fundamental_matrix(vector<DMatch> matches,
                               vector<KeyPoint> keypoints_L,
                               vector<KeyPoint> keypoints_R,
                               vector<Point2f> &imgpts_L,
                               vector<Point2f> &imgpts_R)
{
    for (size_t i = 0; i < matches.size(); i++)
    {
        imgpts_L.push_back(keypoints_L[matches[i].queryIdx].pt);
        imgpts_R.push_back(keypoints_L[matches[i].trainIdx].pt);
    }

    Mat fundamental_matrix = findFundamentalMat(imgpts_L, imgpts_R, FM_RANSAC);

    return fundamental_matrix;
}

int main(int argc, char *argv[])
{
    Mat H1, H2;
    Mat descriptors_L;
    Mat descriptors_R;
    vector<Point2f> imgpts_L;
    vector<Point2f> imgpts_R;
    vector<KeyPoint> keypoints_L;
    vector<KeyPoint> keypoints_R;
    vector<vector<DMatch>> knn_matches;

    cout << "start main" << endl;
    cout << argc << argv[1] << endl;
    Mat imgL = imread("./im0.png", IMREAD_GRAYSCALE);
    Mat imgR = imread("./im1.png", IMREAD_GRAYSCALE);

    /* ==== Find good keypoints to use ==== */
    get_keypoints_and_descriptors(
        imgL,
        imgR,
        keypoints_L,
        keypoints_R,
        descriptors_L,
        descriptors_R,
        knn_matches);
    vector<DMatch> good_matches = filter_knn_matches(knn_matches);

    /* ==== Draw matches ==== */
    Mat img_matches;
    drawMatches(imgL, keypoints_L, imgR, keypoints_R, good_matches, img_matches, Scalar::all(-1),
                Scalar::all(-1), vector<char>(), DrawMatchesFlags::NOT_DRAW_SINGLE_POINTS);
    resize(img_matches, img_matches, Size(2142, 750));
    imshow("Good Matches", img_matches);

    /* ==== Compute Fundamental Matrix ==== */
    Mat F = compute_fundamental_matrix(good_matches, keypoints_L, keypoints_R, imgpts_L, imgpts_R);

    /* ==== Stereo rectify uncalibrated ==== */
    stereoRectifyUncalibrated(imgpts_L, imgpts_R, F, imgL.size(), H1, H2);

    /* ====  Undistort (Rectify) ==== */
    Mat imgL_undistorted(imgL.size(), imgL.type());
    Mat imgR_undistorted(imgR.size(), imgR.type());
    warpPerspective(imgL, imgL_undistorted, H1, imgL.size());
    warpPerspective(imgR, imgR_undistorted, H2, imgR.size());
    resize(imgL_undistorted, imgL_undistorted, Size(714, 500));
    resize(imgR_undistorted, imgR_undistorted, Size(714, 500));
    imshow("undistorted_L", imgL_undistorted);
    imshow("undistorted_R", imgR_undistorted);

    /* ====  Calculate Disparity (Depth Map) ==== */
    int win_size = 2;
    int min_disp = -4;
    int max_disp = 9;
    int num_disp = max_disp - min_disp;
    double minVal;
    double maxVal;
    Ptr<StereoBM> sbm = StereoBM::create(16, 9);
    Ptr<StereoSGBM> sgbm = StereoSGBM::create(0, 16, 3);
    Mat dispbm, dispsgbm;
    Mat dispnorm_bm, dispnorm_sgbm;
    int sgbmWinSize = 3;
    int cn = imgL.channels();
    sgbm->setP1(8 * cn * sgbmWinSize * sgbmWinSize);
    sgbm->setP2(32 * cn * sgbmWinSize * sgbmWinSize);
    sgbm->setMinDisparity(0);
    sgbm->setNumDisparities(256);
    sgbm->setUniquenessRatio(10);
    sgbm->setSpeckleWindowSize(100);
    sgbm->setSpeckleRange(32);
    sgbm->setDisp12MaxDiff(1);
    sgbm->setMode(StereoSGBM::MODE_SGBM);

    sbm->compute(imgL, imgR, dispbm);
    minMaxLoc(dispbm, &minVal, &maxVal);
    dispbm.convertTo(dispnorm_bm, CV_8UC1, 255 / (maxVal - minVal));
    sgbm->compute(imgL, imgR, dispsgbm);
    minMaxLoc(dispsgbm, &minVal, &maxVal);
    dispsgbm.convertTo(dispnorm_sgbm, CV_8UC1, 255 / (maxVal - minVal));

    applyColorMap(dispnorm_bm, dispnorm_bm, COLORMAP_TURBO);
    applyColorMap(dispnorm_sgbm, dispnorm_sgbm, COLORMAP_TURBO);

    resize(dispnorm_bm, dispnorm_bm, Size(714, 500));
    resize(dispnorm_sgbm, dispnorm_sgbm, Size(714, 500));

    imshow("BM", dispnorm_bm);
    imshow("CSGBM", dispnorm_sgbm);
    waitKey(0);

    return 0;
}