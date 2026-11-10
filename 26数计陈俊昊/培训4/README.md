# 培训4 · 相机标定

## 任务要求

1. 用群里发的 `cam_mix_img.zip` 里的标定图，自己选一种方法做标定
2. 输出**相机内参矩阵 K**（3×3，含 fx / fy / cx / cy）
3. 输出**畸变系数 D**（5 个：k1 k2 p1 p2 k3）
4. 输出**畸变矫正之后的图像**（`cv::undistort` 存成文件并展示）

## 目录结构

```
培训4/
├── README.md
├── calib_demo/                     # 标定 + 矫正，输出全部交付物
│   ├── CMakeLists.txt
│   ├── includes/
│   │   └── calib.hpp               # 常量 + 5 个函数声明
│   ├── tools/
│   │   └── calib.cpp               # 世界坐标 / 角点检测 / 标定 / 存 YAML / 矫正
│   ├── src/
│   │   └── main.cpp                # 入口：读目录 -> 检测 -> 标定 -> 出图
│   ├── corners.jpg                 # 角点检测结果（70 个点）
│   ├── undistort_before.jpg        # 矫正前
│   ├── undistort_after.jpg         # 矫正后
│   ├── undistort_result.jpg        # 矫正后（交付物）
│   ├── undistort_compare.jpg       # 左右对比
│   ├── params.yml                  # 标定结果（K 和 D）
│   └── shots/                      # 运行截图
│       ├── calib_result.png        # 终端输出的 RMS / K / D
│       └── undistort_compare.png   # 窗口里的原图与矫正后
└── undistort_demo/                 # 单独演示：只读 params.yml 做矫正
    ├── CMakeLists.txt
    ├── includes/
    │   └── undistort.hpp
    ├── tools/
    │   └── undistort.cpp
    └── src/
        └── main.cpp
```

> ⚠️ **标定原图（113 张 / 7.7 MB）不在仓库里**，按提交说明放在仓库外，用命令行参数传目录。

## 编译

依赖：OpenCV 4.x（Ubuntu：`sudo apt install libopencv-dev`）

```bash
cd 培训4/calib_demo
cmake -S . -B build
cmake --build build -j
```

```bash
cd 培训4/undistort_demo
cmake -S . -B build
cmake --build build -j
```

## 运行

**第一步：把标定图解压到仓库外面**（例如 `~/cam_mix_img/`），确认里面是 `img0.jpg` ~ `img112.jpg`。

**第二步：标定**（约 83 秒）

```bash
cd 培训4/calib_demo
./build/calib_demo ~/cam_mix_img            # 只出结果文件
./build/calib_demo ~/cam_mix_img --show     # 顺便弹窗看矫正效果
```

跑完在 `calib_demo/` 下生成 `corners.jpg`、`undistort_before.jpg`、`undistort_after.jpg`、`undistort_compare.jpg`、`undistort_result.jpg`、`params.yml`。

**第三步：单独演示矫正**（毫秒级，不用重跑标定）

```bash
cd 培训4/undistort_demo
./build/undistort_demo ~/cam_mix_img/img0.jpg assets/params.yml
```

## 实现思路

### 1. 数据集

- 113 张 640×480 jpg，`img0.jpg` ~ `img112.jpg`
- 棋盘格内角点 **10 × 7**（11 列 × 8 行方格）
- 每格 15 mm —— 但代码里填的是 `1.0`，因为**边长只影响 `tvecs` 的物理尺度，不影响 K 和 D**

`patternSize` 不是靠肉眼数格子定的，而是写了个循环把 19 个候选尺寸逐个试，看 `findChessboardCorners` 的命中率：

| 候选尺寸 | 命中率（23 张抽样） |
|---|---|
| **10×7** | **21 / 23** |
| 9×7 | 4 / 23 |
| 9×6 | 3 / 23 |
| 其余 16 个 | 0 / 23 |

**不能用格子数当 `patternSize`。** 数出来是 11×8 个格子，但它要的是**内角点**，即 10×7。

### 2. 世界坐标

标定方程要的是「真实世界的点 ↔ 图像里的像素点」的配对。只有像素坐标算不出内参——同一张照片，焦距长一点、板子远一点，投影结果可以完全一样，必须给它一个真实尺寸当参照。

整批图拍的是同一块板子，所以世界坐标**只造一份**：

```
(x, y, z) = (列号 × 边长, 行号 × 边长, 0)
```

- **Z 恒为 0**：棋盘格是平面
- **顺序必须逐行从左到右**：要和 `findChessboardCorners` 输出角点的顺序对齐。顺序错了**不会报错**，只会算出一个大得离谱的 RMS

### 3. 角点检测与亚像素细化

```cpp
findChessboardCorners   // 粗定位，整像素
cornerSubPix            // 细修，亚像素
```

`cornerSubPix` 只接受**单通道灰度图**，传彩色会断言失败。少了这一步，RMS 会明显偏大。

### 4. 标定

```cpp
calibrateCamera(objectPoints, imagePoints, imageSize, K, D, rvecs, tvecs)
```

- 两个容器都是 `vector<vector<...>>`：外层每张图，内层那张图的 70 个点
- **元素个数必须相等**，否则抛 `nimages == imagePoints1.total()`，程序直接崩
- 返回值是 **RMS 重投影误差**，判断标定质量的唯一指标

### 5. 存 YAML

```cpp
cv::FileStorage fs("params.yml", cv::FileStorage::WRITE);
fs << "camera_matrix" << cameraMatrix;
fs << "dist_coeffs" << distCoeffs;
```

用它不用 txt：行列和类型一起记住（`!!opencv-matrix` / `rows: 3` / `dt: d`），读回来直接是 `cv::Mat`。

### 6. 畸变矫正

`cv::undistort` 内部等价于 `initUndistortRectifyMap` + `remap`：对输出图每个像素反算它在原图的采样位置。

本项目 `k1 = -0.43`（负值）是**桶形畸变**：

| 类型 | k1 | 矫正后 |
|---|---|---|
| 桶形（本图） | 负 | 画面轻微放大、外鼓的线被拉直，**没有黑边** |
| 枕形 | 正 | 四角被拉到画面外，出现黑边 |

## 结果

**重投影误差 RMS = 1.45057 像素**

**内参矩阵 K：**

$$
K=\begin{bmatrix}
631.583958 & 0 & 314.463315 \\
0 & 635.524490 & 220.363634 \\
0 & 0 & 1
\end{bmatrix}
$$

| 参数 | 值 | 说明 |
|---|---|---|
| fx | 631.58 | 横向焦距（像素） |
| fy | 635.52 | 纵向焦距，与 fx 差 0.6%，像素是方的 |
| cx | 314.46 | 主点 x，理想 320，偏 5.5 像素 |
| cy | 220.36 | 主点 y，理想 240，偏 19.6 像素 |

**畸变系数 D**（顺序 `[k1, k2, p1, p2, k3]`）：

| k1 | k2 | p1 | p2 | k3 |
|---|---|---|---|---|
| -0.426272 | 0.209189 | 1.6436e-05 | 1.6315e-04 | -0.031247 |

k1 为负 → 桶形畸变。

**自检对照（按提交说明给的四条）**

| 指标 | 期望 | 本结果 |
|---|---|---|
| RMS | 1 ~ 2 像素 | 1.45057 ✅ |
| fx / fy | 500 ~ 800 | 631.6 / 635.5 ✅ |
| cx / cy | 接近 320 / 240 | 314.5 / 220.4 ✅ |
| k1 | 一般为负 | -0.4263 ✅ |

## 效果

| 文件 | 内容 |
|---|---|
| `corners.jpg` | 70 个角点按行列画出并标号 |
| `undistort_before.jpg` | 矫正前 |
| `undistort_after.jpg` | 矫正后 |
| `undistort_compare.jpg` | 左原图 / 右矫正，中间红线分隔 |
| `undistort_result.jpg` | 矫正后的成品图（交付物） |
| `shots/calib_result.png` | 终端打印的 RMS、K、D |
| `shots/undistort_compare.png` | 运行窗口截图 |

畸变在这批图上**肉眼不太明显**，因为桶形纠正只是把画面轻微放大、把外鼓的线拉直，属于全局变化。实测原图与矫正图的差异：平均绝对差 **28.06** 灰度级，**47.4%** 的像素变化超过 5 灰度级——变化确实存在，只是人眼对整体位移不敏感，要对比画面四角的直线才看得出来。

## 为什么分两个 demo

| | calib_demo | undistort_demo |
|---|---|---|
| 干什么 | 算 K 和 D，并存全部交付图 | 只读 `params.yml` 做矫正 |
| 要多少图 | 97 张 | 1 张 |
| 耗时 | 约 83 秒 | 毫秒级 |
| 输入 | 图片目录（命令行参数） | 一张图 + `params.yml` |

两者之间只靠 `params.yml` 对接。拆开的好处：演示矫正效果时不用每次重跑一分半的标定，也方便单独调试其中一环。

## 内参属于哪台相机

属于**拍摄这 113 张照片的那台相机**。

- 换一台相机 → 这套内参完全不能用
- 同一台相机换分辨率 → fx / fy / cx / cy 要按比例缩放
- cx / cy 不等于图像中心是正常的，那是镜头装配偏差，正是"这台相机的身份证"

## 踩过的坑

| 坑 | 现象 | 解法 |
|---|---|---|
| `patternSize` 填成格子数 | `findChessboardCorners` 一直 false | 填内角点数 10×7，不是 11×8 |
| `cv::Size(10, 7)` 顺序写反 | 检测全失败 | `cv::Size` 是 **(width, height)** |
| 彩色图传进 `cornerSubPix` | 断言 `CV_8UC1` 失败 | 先 `cvtColor` 转灰度 |
| 不检查 `findChessboardCorners` 返回值 | 后面崩在别处 | 返回 false 就 `continue` 跳过 |
| 只 push 了一边 | 抛 `nimages == imagePoints1.total()` | 两个 push 必须成对 |
| 数量对但顺序错 | **不报错**，RMS 大得离谱 | 世界坐标逐行从左到右 |
| 不检查 `imread` 返回 | 空 Mat 一路往下传 | `img.empty()` 判断 |
| 把 97 张原图提交进仓库 | 仓库被撑爆（7.7 MB × 每人一份） | 图放仓库外，路径用命令行参数 |
| `git add` 把 build 塞进去 | 提交里多十几 MB 的 .o / .a | 根目录 `.gitignore` 已忽略 `build/`，别改它 |
| `include_directories` 写在 `add_library` 后面 | 库编译找不到 OpenCV 头 | 只对之后创建的 target 生效 |

## 和官方示例的对比

仓库 `示例/培训4相机标定/` 用全量 113 张跑出来是：RMS **1.4278**、fx **629.48**、k1 **-0.42297**。

本作业用 97 张（剔掉 16 张检不出角点的）：RMS **1.45057**、fx **631.58**、k1 **-0.42627**。

两边几乎一致，说明算法路径等价。
