#include <stdio.h>
#include <stdlib.h>
#include <iostream>
#include <opencv2/opencv.hpp>
#include <opencv2/core.hpp>
#include <opencv2/highgui.hpp>
#include <opencv2/imgproc.hpp>
using namespace cv;

int main()
{
    std::cout << "Hello world Richard~" << std::endl;
    printf("hello world!!\n");

    /* 畫布 */
    Mat img(270, 720, CV_8UC3, Scalar(56, 50, 38));
    /* 直線 */
    line(img, Point(20, 40), Point(120, 140), Scalar(255, 0, 0), 3);
    /* 實心方塊 */
    rectangle(img, Point(150, 40), Point(250, 140), Scalar(0, 0, 255), -1);
    /* 實心圓 */
    circle(img, Point(330, 90), 50, Scalar(0, 255, 0), -1);
    /* 空心橢圓 */
    ellipse(img, Point(460, 90), Size(60, 40), 45, 0, 360, Scalar(255, 255, 0), 2);
    /* 不規則圖形 */
    Point points[1][5];
    int x = 40, y = 540;
    points[0][0] = Point(0 + y, 50 + x);
    points[0][1] = Point(40 + y, 0 + x);
    points[0][2] = Point(110 + y, 35 + x);
    points[0][3] = Point(74 + y, 76 + x);
    points[0][4] = Point(28 + y, 96 + x);
    const Point *ppt[1] = {points[0]};
    int npt[] = {5};
    polylines(img, ppt, npt, 1, 1, Scalar(0, 255, 255), 3);
    /* 繪出文字 */
    putText(img, "Test Passed !!", Point(10, 230), 0, 3, Scalar(255, 170, 130), 3);
    /* 開啟畫布 */
    namedWindow("OpenCV Test By:Richard", WINDOW_AUTOSIZE);
    imshow("OpenCV Test By:Richard", img);
    waitKey(0);

    system("pause");
    return 0;
}