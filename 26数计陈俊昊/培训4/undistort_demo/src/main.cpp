#include <iostream>
#include <opencv2/opencv.hpp>

#include "undistort.hpp"

int main() {
    // 1. 读标定结果（0.1 秒，不用重跑那一分半的标定）
    cv::Mat cameraMatrix, distCoeffs;
    if (!loadParams("assets/params.yml", cameraMatrix, distCoeffs)) {
        std::cout << "=== 读不到 assets/params.yml，先跑一遍 calib_demo ===" << std::endl;
        return 1;
    }
    std::cout << "=== 相机内参 ===" << std::endl << cameraMatrix << std::endl;
    std::cout << "=== 畸变系数 ===" << std::endl << distCoeffs << std::endl;

    // 2. 读一张样例图
    cv::Mat img = cv::imread("assets/img0.jpg");
    if (img.empty()) {
        std::cout << "=== 读不到 assets/img0.jpg ===" << std::endl;
        return 1;
    }

    // 3. 矫正并存图（存图才是交付物）
    cv::Mat undist = undistortImage(img, cameraMatrix, distCoeffs);
    cv::imwrite("undistorted_result.jpg", undist);

    std::cout << "=== 已写入 undistorted_result.jpg ===" << std::endl;

    // 4. 并排显示，方便截图对比
    cv::imshow("原图", img);
    cv::imshow("矫正后", undist);
    cv::waitKey(0);
    return 0;
}
