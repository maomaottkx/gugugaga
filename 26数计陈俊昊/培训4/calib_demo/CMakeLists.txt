cmake_minimum_required(VERSION 3.10)
project(calib_demo)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(OpenCV REQUIRED)

# 业务逻辑单独编成静态库
add_library(calib STATIC tools/calib.cpp)
target_include_directories(calib PUBLIC includes ${OpenCV_INCLUDE_DIRS})
target_link_libraries(calib PRIVATE ${OpenCV_LIBS})

add_executable(calib_demo src/main.cpp)
target_link_libraries(calib_demo PRIVATE calib ${OpenCV_LIBS})
