#include "undistort.hpp"

bool loadParams(const std::string& path, cv::Mat& cameraMatrix, cv::Mat& distCoeffs) {
    cv::FileStorage fs(path, cv::FileStorage::READ);
    if (!fs.isOpened()) {
        return false;
    }
    // 键名必须和 calib_demo 里写进去的完全一致
    fs["camera_matrix"] >> cameraMatrix;
    fs["dist_coeffs"] >> distCoeffs;
    fs.release();

    // 读到空矩阵说明键名写错了或文件是坏的
    if (cameraMatrix.empty() || distCoeffs.empty()) {
        return false;
    }
    return true;
}

cv::Mat undistortImage(const cv::Mat& src, const cv::Mat& cameraMatrix, const cv::Mat& distCoeffs) {
    cv::Mat dst;
    // 内部等价于 initUndistortRectifyMap + remap：
    // 对每个输出像素反算它在原图的采样位置，那里没有数据就留黑
    cv::undistort(src, dst, cameraMatrix, distCoeffs);
    return dst;
}
