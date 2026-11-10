#pragma once
#include <opencv2/opencv.hpp>
#include <string>

// 从 YAML 读内参和畸变系数。
// FileStorage 会把行列和类型一起还原，读出来直接是 cv::Mat。
bool loadParams(const std::string& path, cv::Mat& cameraMatrix, cv::Mat& distCoeffs);

// 用内参 + 畸变把图拉直。桶形畸变（k1 < 0）矫正后画面会轻微放大，没有黑边。
cv::Mat undistortImage(const cv::Mat& src, const cv::Mat& cameraMatrix, const cv::Mat& distCoeffs);
