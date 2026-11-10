cmake_minimum_required(VERSION 3.10)
project(calib_demo)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(OpenCV REQUIRED)

# 整个 assets 目录复制进 build，程序从 build 里跑才找得到图。
# 源路径结尾的斜杠不能省：带斜杠=复制目录里的东西，不带=复制目录本身
file(COPY ${CMAKE_CURRENT_SOURCE_DIR}/assets/ DESTINATION ${CMAKE_CURRENT_BINARY_DIR}/assets)

# 业务逻辑单独编成静态库，和 培训2/student_demo 的做法保持一致
add_library(calib STATIC tools/calib.cpp)
# PUBLIC：这个 include 路径要传给链接它的 calib_demo，
# 否则 main.cpp 里的 #include "calib.hpp" 找不到
target_include_directories(calib PUBLIC includes ${OpenCV_INCLUDE_DIRS})
target_link_libraries(calib ${OpenCV_LIBS})

# 入口只编 main.cpp，calib.cpp 已经进库里了，再写一遍会被编两次
add_executable(calib_demo src/main.cpp)
target_link_libraries(calib_demo calib)
