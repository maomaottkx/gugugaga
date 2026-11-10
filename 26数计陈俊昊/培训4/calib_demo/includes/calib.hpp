#pragma once
#include <opencv2/opencv.hpp>
#include <vector>
#include <string>

// 棋盘格内角点：(列数, 行数)。cv::Size 的顺序是 (width, height)，别写反
const cv::Size kPattern(10, 7);

// 一格的真实边长。填 1.0 表示"以格为单位"：
// 只影响 tvecs 的距离单位，不影响 fx / fy
const float kSquareSize = 1.0f;

// 生成棋盘格"世界坐标"：逐行从左到右排，Z 恒为 0（平面）。
// 输出顺序必须和 findChessboardCorners 的角点顺序一致，否则配对错乱。
void makeObjectPoints(std::vector<cv::Point3f>& obj, cv::Size patternSize, float squareSize);

// 检测一张图的角点并细化到亚像素。
// 返回 false = 这张图没有完整棋盘格，调用方应当跳过它。
bool detectCorners(const cv::Mat& gray, cv::Size patternSize, std::vector<cv::Point2f>& corners);

// 标定：输入每张图的 <世界坐标, 像素坐标> 配对，输出内参和畸变系数。
// 返回值是 RMS 重投影误差（像素），这批图大概 1.45。
double runCalibrate(const std::vector<std::vector<cv::Point3f>>& objectPoints,
                    const std::vector<std::vector<cv::Point2f>>& imagePoints,
                    cv::Size imageSize,
                    cv::Mat& cameraMatrix,
                    cv::Mat& distCoeffs);

// 把结果写进 YAML，交给 undistort_demo 读。
// 用 YAML 不用 txt：行列和类型都保留，读回来直接是 cv::Mat。
bool saveParams(const std::string& path, const cv::Mat& cameraMatrix, const cv::Mat& distCoeffs);
