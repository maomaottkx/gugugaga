cmake_minimum_required(VERSION 3.10)
project(undistort_demo)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(OpenCV REQUIRED)

add_library(undistort STATIC tools/undistort.cpp)
target_include_directories(undistort PUBLIC includes ${OpenCV_INCLUDE_DIRS})
target_link_libraries(undistort PRIVATE ${OpenCV_LIBS})

add_executable(undistort_demo src/main.cpp)
target_link_libraries(undistort_demo PRIVATE undistort ${OpenCV_LIBS})
