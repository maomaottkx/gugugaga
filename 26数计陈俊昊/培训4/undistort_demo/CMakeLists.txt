cmake_minimum_required(VERSION 3.10)
project(undistort_demo)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(OpenCV REQUIRED)

# 只复制一张样例图 + 标定结果，不需要那 97 张
file(COPY ${CMAKE_CURRENT_SOURCE_DIR}/assets/ DESTINATION ${CMAKE_CURRENT_BINARY_DIR}/assets)

add_library(undistort STATIC tools/undistort.cpp)
target_include_directories(undistort PUBLIC includes ${OpenCV_INCLUDE_DIRS})
target_link_libraries(undistort ${OpenCV_LIBS})

add_executable(undistort_demo src/main.cpp)
target_link_libraries(undistort_demo undistort)
