#include <iostream>
#include <vector>
#include <opencv2/opencv.hpp>

#include "calib.hpp"

int main() {
    // 1. 收集素材（必须在 calib_demo 目录下运行，assets 是相对路径）
    std::vector<cv::String> files;
    cv::glob("assets/*.jpg", files);
    std::cout << "=== 图片数: " << files.size() << " ===" << std::endl;
    if (files.empty()) {
        std::cout << "=== assets/ 里没有 jpg，检查当前工作目录 ===" << std::endl;
        return 1;
    }

    // 2. 世界坐标：整批图同一块板子，造一份就够
    std::vector<cv::Point3f> obj;
    makeObjectPoints(obj, kPattern, kSquareSize);
    std::cout << "=== 世界坐标点数: " << obj.size() << " ===" << std::endl;

    // 3. 逐张检测，攒成二维结构
    std::vector<std::vector<cv::Point3f>> objectPoints;
    std::vector<std::vector<cv::Point2f>> imagePoints;

    int okCount = 0;
    int failCount = 0;
    cv::Size imageSize;

    for (size_t i = 0; i < files.size(); ++i) {
        cv::Mat img = cv::imread(files[i]);
        if (img.empty()) {
            ++failCount;
            continue;
        }
        imageSize = img.size();

        cv::Mat gray;
        cv::cvtColor(img, gray, cv::COLOR_BGR2GRAY);

        std::vector<cv::Point2f> corners;
        if (!detectCorners(gray, kPattern, corners)) {
            ++failCount;
            continue;
        }

        // 这两个 push 必须成对，缺一半 calibrateCamera 直接抛异常
        objectPoints.push_back(obj);
        imagePoints.push_back(corners);
        ++okCount;
    }

    std::cout << "=== 找到 " << okCount << " 张, 失败 " << failCount << " 张 ===" << std::endl;
    if (okCount < 3) {
        std::cout << "=== 有效图不足 3 张，标定没有意义 ===" << std::endl;
        return 1;
    }

    // 4. 标定
    cv::Mat cameraMatrix, distCoeffs;
    double rms = runCalibrate(objectPoints, imagePoints, imageSize, cameraMatrix, distCoeffs);

    std::cout << "=== RMS 重投影误差: " << rms << " ===" << std::endl;
    std::cout << "=== 相机内参 ===" << std::endl << cameraMatrix << std::endl;
    std::cout << "=== 畸变系数 ===" << std::endl << distCoeffs << std::endl;

    // 5. 存给 undistort_demo 用；它读这份文件就不用再跑一分半的标定
    if (!saveParams("params.yml", cameraMatrix, distCoeffs)) {
        std::cout << "=== 写 params.yml 失败 ===" << std::endl;
        return 1;
    }
    std::cout << "=== 已写入 params.yml ===" << std::endl;
    return 0;
}
