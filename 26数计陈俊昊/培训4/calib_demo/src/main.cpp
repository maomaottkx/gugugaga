// 相机标定：读目录里的棋盘格图 -> 输出内参 K 和畸变系数 D -> 存矫正后图像
// 用法：./calib_demo <图片目录> [--show]
//   例：./calib_demo /home/mao/cam_mix_img
//   标定图是数据集，不进仓库，所以目录必须从命令行传进来
#include <iostream>
#include <string>
#include <vector>
#include <opencv2/opencv.hpp>

#include "calib.hpp"

int main(int argc, char** argv) {
    // 1. 图片来源：命令行第一个参数；--show 控制要不要弹窗
    std::string dir = (argc > 1) ? argv[1] : "cam_mix_img";
    bool show = false;
    for (int i = 1; i < argc; ++i) {
        if (std::string(argv[i]) == "--show") {
            show = true;
        }
    }

    std::vector<cv::String> files;
    cv::glob(dir + "/*.jpg", files);
    std::cout << "发现图片: " << files.size() << " 张  目录: " << dir << std::endl;
    if (files.empty()) {
        std::cout << "一张图都没找到，检查路径是否写对（要用正斜杠）" << std::endl;
        return 1;
    }

    // 2. 世界坐标：整批图同一块板子，造一份就够
    std::vector<cv::Point3f> obj;
    makeObjectPoints(obj, kPattern, kSquareSize);

    // 3. 逐张检测角点
    std::vector<std::vector<cv::Point3f>> objectPoints;
    std::vector<std::vector<cv::Point2f>> imagePoints;

    int okCount = 0;
    int failCount = 0;
    cv::Size imageSize;
    cv::Mat firstColor;   // 第一张检测成功的彩色图，后面画角点和做矫正都用它
    bool hasFirst = false;

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

        // 第一张成功的图：把 70 个角点画出来存成 corners.jpg，作为检测效果的证据
        if (!hasFirst) {
            cv::Mat canvas = img.clone();
            cv::drawChessboardCorners(canvas, kPattern, corners, true);
            cv::imwrite("corners.jpg", canvas);
            firstColor = img;
            hasFirst = true;
        }

        // 这两个 push 必须成对，缺一半 calibrateCamera 会抛异常
        objectPoints.push_back(obj);
        imagePoints.push_back(corners);
        ++okCount;
    }

    std::cout << "角点检出: " << okCount << " / " << files.size()
              << " 张（" << failCount << " 张检不出来，正常）" << std::endl;
    if (okCount < 3) {
        std::cout << "有效图不足 3 张，没法标定" << std::endl;
        return 1;
    }

    // 4. 标定
    cv::Mat cameraMatrix, distCoeffs;
    double rms = runCalibrate(objectPoints, imagePoints, imageSize, cameraMatrix, distCoeffs);

    std::cout << "\n========== 标定结果 ==========" << std::endl;
    std::cout << "重投影误差 RMS = " << rms << " 像素" << std::endl;
    std::cout << "内参矩阵 K =" << std::endl << cameraMatrix << std::endl;
    std::cout << "畸变系数 D = " << distCoeffs << std::endl;
    std::cout << "  （顺序 k1, k2, p1, p2, k3）" << std::endl;

    // 5. 存 YAML，给 undistort_demo 读
    if (!saveParams("params.yml", cameraMatrix, distCoeffs)) {
        std::cout << "写 params.yml 失败" << std::endl;
        return 1;
    }
    std::cout << "已写入 params.yml" << std::endl;

    // 6. 畸变矫正，存三张图作为交付物
    cv::Mat before = firstColor;
    cv::Mat after = undistortImage(before, cameraMatrix, distCoeffs);

    cv::imwrite("undistort_before.jpg", before);
    cv::imwrite("undistort_after.jpg", after);

    // 左右拼接，中间画一条红线分隔，方便一眼看出差别
    cv::Mat compare;
    cv::hconcat(before, after, compare);
    cv::line(compare, cv::Point(before.cols, 0), cv::Point(before.cols, before.rows),
             cv::Scalar(0, 0, 255), 2);
    cv::putText(compare, "BEFORE", cv::Point(10, 30),
                cv::FONT_HERSHEY_SIMPLEX, 1.0, cv::Scalar(0, 0, 255), 2);
    cv::putText(compare, "AFTER", cv::Point(before.cols + 10, 30),
                cv::FONT_HERSHEY_SIMPLEX, 1.0, cv::Scalar(0, 0, 255), 2);
    cv::imwrite("undistort_compare.jpg", compare);
    cv::imwrite("undistort_result.jpg", after);

    std::cout << "已写入 corners.jpg / undistort_before.jpg / undistort_after.jpg"
              << " / undistort_compare.jpg / undistort_result.jpg" << std::endl;

    if (show) {
        cv::imshow("before", before);
        cv::imshow("after", after);
        cv::imshow("compare", compare);
        cv::waitKey(0);
    }
    return 0;
}
