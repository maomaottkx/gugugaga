#include "calib.hpp"

// 逐行扫：外层行，内层列。
// 横坐标用列号、纵坐标用行号——写反了不报错，只是 RMS 会大得离谱。
void makeObjectPoints(std::vector<cv::Point3f>& obj, cv::Size patternSize, float squareSize) {
    obj.clear();
    for (int r = 0; r < patternSize.height; ++r) {
        for (int c = 0; c < patternSize.width; ++c) {
            obj.push_back(cv::Point3f(c * squareSize, r * squareSize, 0.0f));
        }
    }
}

bool detectCorners(const cv::Mat& gray, cv::Size patternSize, std::vector<cv::Point2f>& corners) {
    corners.clear();
    // 先粗找：返回 bool，找不到时 corners 是空的，必须拦住
    bool found = cv::findChessboardCorners(
        gray, patternSize, corners,
        cv::CALIB_CB_ADAPTIVE_THRESH + cv::CALIB_CB_NORMALIZE_IMAGE);
    if (!found) {
        return false;
    }

    // 再细修：把整像素角点推到亚像素。
    // Size(-1,-1) 是 zeroZone 死区，-1 表示不设；gray 必须是单通道
    cv::cornerSubPix(gray, corners, cv::Size(11, 11), cv::Size(-1, -1),
        cv::TermCriteria(cv::TermCriteria::EPS + cv::TermCriteria::COUNT, 30, 0.001));
    return true;
}

double runCalibrate(const std::vector<std::vector<cv::Point3f>>& objectPoints,
                    const std::vector<std::vector<cv::Point2f>>& imagePoints,
                    cv::Size imageSize,
                    cv::Mat& cameraMatrix,
                    cv::Mat& distCoeffs) {
    // rvecs / tvecs 是每张图的外参，C++ 必须给容器，不能像 Python 那样传 None
    std::vector<cv::Mat> rvecs, tvecs;
    // 注意：objectPoints.size() 必须等于 imagePoints.size()，否则这里会抛
    // (-2:Unspecified error) nimages == imagePoints.total()
    return cv::calibrateCamera(objectPoints, imagePoints, imageSize,
                               cameraMatrix, distCoeffs, rvecs, tvecs);
}

bool saveParams(const std::string& path, const cv::Mat& cameraMatrix, const cv::Mat& distCoeffs) {
    cv::FileStorage fs(path, cv::FileStorage::WRITE);
    if (!fs.isOpened()) {
        return false;
    }
    fs << "camera_matrix" << cameraMatrix;
    fs << "dist_coeffs" << distCoeffs;
    // 显式 release：析构也会自动关，但显式写能保证缓冲区立刻落盘
    fs.release();
    return true;
}
