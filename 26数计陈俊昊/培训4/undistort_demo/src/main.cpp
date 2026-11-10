// 只做畸变矫正：读 calib_demo 存下来的 params.yml，把一张图拉直
// 用法：./undistort_demo <图片路径> [params.yml路径]
//   例：./undistort_demo /home/mao/cam_mix_img/img0.jpg assets/params.yml
#include <iostream>
#include <string>
#include <opencv2/opencv.hpp>

#include "undistort.hpp"

int main(int argc, char** argv) {
    std::string imgPath = (argc > 1) ? argv[1] : "img.jpg";
    std::string ymlPath = (argc > 2) ? argv[2] : "assets/params.yml";

    // 1. 读标定结果（毫秒级，不用重跑那一分半的标定）
    cv::Mat cameraMatrix, distCoeffs;
    if (!loadParams(ymlPath, cameraMatrix, distCoeffs)) {
        std::cout << "读不到 " << ymlPath << "，先跑一遍 calib_demo" << std::endl;
        return 1;
    }
    std::cout << "相机内参 K =" << std::endl << cameraMatrix << std::endl;
    std::cout << "畸变系数 D = " << distCoeffs << std::endl;

    // 2. 读图
    cv::Mat img = cv::imread(imgPath);
    if (img.empty()) {
        std::cout << "读不到图片: " << imgPath << std::endl;
        return 1;
    }

    // 3. 矫正 + 存图
    cv::Mat undist = undistortImage(img, cameraMatrix, distCoeffs);
    cv::imwrite("undistorted_result.jpg", undist);
    std::cout << "已写入 undistorted_result.jpg" << std::endl;

    cv::imshow("before", img);
    cv::imshow("after", undist);
    cv::waitKey(0);
    return 0;
}
