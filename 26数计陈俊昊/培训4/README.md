# 培训4 · OpenCV 相机标定

## 任务要求

1. 完成 OpenCV 的学习，了解相机标定
2. 使用给定的标定图像，自己选择一个方法进行标定
3. 输出**相机内参**
4. 输出**畸变系数**
5. 展示**畸变矫正之后的图像**

## 目录结构

```
培训4/
├── calib_demo/                  # 任务 1：标定，输出内参与畸变系数
│   ├── CMakeLists.txt
│   ├── includes/
│   │   └── calib.hpp            # 结构常量 + 4 个函数声明
│   ├── tools/
│   │   └── calib.cpp            # 核心实现：世界坐标 / 角点检测 / 标定 / 存 YAML
│   ├── src/
│   │   └── main.cpp             # 入口：循环读图 -> 攒点 -> 标定 -> 存 params.yml
│   └── assets/                  # 97 张棋盘格标定图
├── undistort_demo/              # 任务 2：读标定结果，输出矫正图
│   ├── CMakeLists.txt
│   ├── includes/
│   │   └── undistort.hpp
│   ├── tools/
│   │   └── undistort.cpp        # 读 YAML + 畸变矫正
│   ├── src/
│   │   └── main.cpp             # 入口：读 params.yml -> 矫正 -> 存图 + 显示
│   └── assets/
│       ├── img0.jpg             # 样例图（一张就够）
│       └── params.yml           # 标定产物，也是两个 demo 之间的接口
└── screenshots/
    ├── calib_result.png         # 终端输出：RMS + 内参 + 畸变系数
    └── undistort_compare.png    # 原图与矫正后的对比
```

## 编译

依赖：OpenCV 4.x（Ubuntu 下 `sudo apt install libopencv-dev`）

两个 demo **各自独立**，分别编译：

```bash
cd 培训4/calib_demo
mkdir -p build && cd build
cmake ..
make
```

```bash
cd 培训4/undistort_demo
mkdir -p build && cd build
cmake ..
make
```

## 运行

```bash
# 第一步：标定（约 83 秒，97 张图逐张检测 + 迭代优化）
cd 培训4/calib_demo/build
./calib_demo
```

跑完在 `calib_demo/build/` 下生成 `params.yml`。

```bash
# 第二步：畸变矫正（秒出，因为它不需要重新标定）
cd 培训4/undistort_demo/build
./undistort_demo
```

窗口里按任意键退出，同时生成 `undistorted_result.jpg`。

## 实现思路

### 1. 标定图像

- 97 张棋盘格照片，分辨率统一 **640×480**
- 内角点数 **10 × 7**（即 11×8 个格子）

`patternSize` 不是靠肉眼数格子确定的，而是写了个循环把 19 个候选尺寸逐个试，用 `findChessboardCorners` 的命中率挑：

| 候选尺寸 | 命中率（23 张抽样） |
|---|---|
| **10×7** | **21 / 23** |
| 9×7 | 4 / 23 |
| 9×6 | 3 / 23 |
| 其余 16 个候选 | 0 / 23 |

**不能用格子数当 `patternSize`。** 数出来是 11×8 个格子，但 `findChessboardCorners` 要的是**内角点**，也就是 10×7。

原始数据集共 113 张，其中 16 张检测不到完整棋盘格，已剔除，剩 97 张。

### 2. 世界坐标

标定方程要的是"真实世界的点 <-> 图像里的像素点"的配对。只有像素坐标算不出内参——同一张照片，焦距长一点、板子远一点，投影结果是一样的，必须给它一个真实尺寸当参照。

整批图拍的是同一块板子，所以世界坐标**只造一份**：

```
(x, y, z) = (列号 × 边长, 行号 × 边长, 0)
```

- **Z 恒为 0**：棋盘格是平面
- **边长填 1.0**：它只缩放 `tvecs`（相机到板子的距离单位），**不影响 fx / fy**（内参是像素单位）
- **顺序必须逐行从左到右**：要和 `findChessboardCorners` 输出角点的顺序对齐。顺序错了不会报错，只会算出一个大得离谱的 RMS

### 3. 角点检测与亚像素细化

```cpp
findChessboardCorners  // 粗定位，整像素
cornerSubPix           // 细修，亚像素
```

`cornerSubPix` 只接受**单通道灰度图**，传彩色会直接断言失败。

这一步是标定精度的关键：只差一个像素，内参就跟着偏。

### 4. 标定

```cpp
calibrateCamera(objectPoints, imagePoints, imageSize, K, dist, rvecs, tvecs)
```

- `objectPoints` / `imagePoints` 都是 `vector<vector<...>>`：外层是每张图，内层是那张图的 70 个点
- 两个容器的元素个数**必须相等**，否则会抛 `nimages == imagePoints1.total()`
- 返回值是 **RMS 重投影误差**，判断标定质量的唯一指标

### 5. 存 YAML

```cpp
cv::FileStorage fs("params.yml", cv::FileStorage::WRITE);
fs << "camera_matrix" << cameraMatrix;
fs << "dist_coeffs" << distCoeffs;
```

用 YAML 不用 txt：它把**行列和类型**一起记住（`!!opencv-matrix` / `rows: 3` / `dt: d`），`undistort_demo` 读回来直接就是 `cv::Mat`，不用自己 split / stof。

### 6. 畸变矫正

`cv::undistort` 内部等价于 `initUndistortRectifyMap` + `remap`：对输出图每个像素反算它在原图的采样位置。

本项目 `k1 = -0.43`（负值）是**桶形畸变**：

| 畸变类型 | k1 | 矫正后 |
|---|---|---|
| 桶形（本图） | 负 | 画面轻微放大、外鼓的线被拉直，**没有黑边** |
| 枕形 | 正 | 四角被拉到画面外，出现黑边 |

## 结果

**RMS 重投影误差：1.45057 像素**

**相机内参矩阵：**

$$
\begin{bmatrix}
631.583958 & 0 & 314.463315 \\
0 & 635.524490 & 220.363634 \\
0 & 0 & 1
\end{bmatrix}
$$

| 参数 | 值 | 说明 |
|---|---|---|
| fx | 631.58 | 横向焦距（像素） |
| fy | 635.52 | 纵向焦距（像素），与 fx 差 0.6%，像素是方的 |
| cx | 314.46 | 主点 x，理想值 320，偏 5.5 像素 |
| cy | 220.36 | 主点 y，理想值 240，偏 19.6 像素 |

**畸变系数**（顺序 `[k1, k2, p1, p2, k3]`）：

| k1 | k2 | p1 | p2 | k3 |
|---|---|---|---|---|
| -0.426272 | 0.209189 | 1.6436e-05 | 1.6315e-04 | -0.031247 |

k1 为负值，桶形畸变。

## 效果

- `screenshots/calib_result.png` —— 终端里的 RMS、内参矩阵、畸变系数
- `screenshots/undistort_compare.png` —— 左边原图、右边矫正后
- `undistort_demo/build/undistorted_result.jpg` —— 矫正后的成品图

畸变在这批图上**肉眼不太明显**，原因是桶形纠正只是把画面轻微放大、把外鼓的线拉直，属于全局变化。实测原图与矫正图的差异：平均绝对差 28.06 灰度级，47.4% 的像素变化超过 5 灰度级——**变化确实存在，只是人眼对整体位移不敏感**，要对比画面四角的直线才看得出来。

## 为什么拆成两个 demo

| | calib_demo | undistort_demo |
|---|---|---|
| 干什么 | 算内参和畸变系数 | 用系数把图拉直 |
| 要多少图 | 97 张 | 1 张 |
| 耗时 | 约 83 秒 | 秒出 |
| 输出 | `params.yml` | `undistorted_result.jpg` |

两者之间只靠 `params.yml` 对接。拆开的好处：演示矫正效果时不用每次重跑一分半的标定，也方便单独调试其中一环。

## 内参属于哪台相机

属于**拍摄这 97 张照片的那台相机**（学长提供的数据集）。

- 换一台相机 → 这套内参完全不能用
- 同一台相机换分辨率 → fx/fy/cx/cy 要按比例缩放
- cx/cy 不等于图像中心是正常现象，那是镜头装配偏差，正是"这台相机的身份证"

## 踩过的坑

| 坑 | 现象 | 解法 |
|---|---|---|
| `patternSize` 填成格子数 | `findChessboardCorners` 一直返回 false | 填**内角点数**（10×7），不是格子数（11×8） |
| `cv::Size(10, 7)` 顺序写反 | 检测全失败 | `cv::Size` 是 **(width, height)** |
| 彩色图传进 `cornerSubPix` | 断言 `CV_8UC1` 失败 | 先 `cvtColor` 转灰度 |
| 不检查 `findChessboardCorners` 返回值 | 后面崩在别处 | 返回 false 时直接跳过这张图 |
| 只 push 了一边 | 抛 `nimages == imagePoints1.total()`，程序直接崩 | `objectPoints` / `imagePoints` 两个 push 必须成对 |
| 数量对但顺序错 | **不报错**，RMS 大得离谱 | 世界坐标必须逐行从左到右，和角点顺序一致 |
| 不检查 `imread` 返回 | 空 Mat 一路往下传 | `img.empty()` 判断 + `continue` |
| `assets/` 路径 | 在 build 目录外运行时读不到图 | CMake 里 `file(COPY assets/ ...)` 复制进 build |
| `file(COPY)` 源路径少写斜杠 | 变成 `build/assets/assets/` | 源路径结尾**必须带斜杠** |
| `include_directories` 写在 `add_library` 后面 | 库编译时找不到 OpenCV 头文件 | `include_directories` / `target_include_directories` 只对之后创建的 target 生效 |
| `git add 培训4/` 直接把 build 塞进去 | 提交里多出十几 MB 的 .o/.a 和图片副本 | 先建 `.gitignore` 写 `build/` |
