# imageRecognition-Conversion
# 基于BMP的图像处理工具

这是一个功能全面的BMP图像处理工具，专注于图像转换、分析和对比。该工具采用C语言开发，结合Windows API实现了直观的用户交互界面，适用于图像处理和简单计算机视觉应用场景。

## 核心功能

1. **灰度图转换**：
   - 将彩色BMP图像转换为灰度图像
   - 自动生成带有中心十字标记的灰度图像
   - 支持多种位深度的BMP图像处理

2. **二值图转换**：
   - 将图像转换为纯黑白二值图像
   - 支持自定义阈值设置

3. **JPG转BMP格式转换**：
   - 通过系统画图工具辅助将JPG图像转换为BMP格式
   - 提供详细的操作指导

4. **物体识别与标记**：
   - 自动检测二值图像中的物体
   - 使用红色边框标记识别到的物体
   - 支持物体大小过滤，忽略噪点
   - 输出物体位置和大小信息

5. **图像对比分析**：
   - 比较两张二值图像的差异
   - 生成差异可视化图像
   - 计算差异百分比
   - 基于阈值判断是否有新物体进入

## 技术特点

- 采用连通区域分析算法识别图像中的独立物体
- 支持多种位深度的BMP图像（1位、4位、8位、24位和32位）
- 使用广度优先搜索(BFS)算法进行连通区域标记
- 针对不同位深度图像优化的处理逻辑
- 内存管理优化，支持处理大型图像
- 完善的错误处理机制

## 应用场景

- 简单的物体检测和计数
- 图像前后对比分析
- 运动检测和变化监测
- 图像格式转换和预处理
- 教学演示和实验

该工具适用于需要进行基础图像处理和简单计算机视觉任务的用户，特别是在教育、实验和原型开发领域。



# BMP Image Processing Tool Advanced Edition

This is a comprehensive BMP image processing tool focused on image conversion, analysis, and comparison. Developed in C language and integrated with Windows API, it provides an intuitive user interface suitable for image processing and simple computer vision application scenarios.

## Core Features

1. **Grayscale Conversion**:
   - Convert color BMP images to grayscale
   - Automatically generate grayscale images with center cross markers
   - Support multiple bit depth BMP image processing

2. **Binary Image Conversion**:
   - Convert images to pure black and white binary images
   - Support custom threshold settings

3. **JPG to BMP Format Conversion**:
   - Convert JPG images to BMP format with system Paint tool assistance
   - Provide detailed operation guidance

4. **Object Detection and Marking**:
   - Automatically detect objects in binary images
   - Mark identified objects with red borders
   - Support object size filtering to ignore noise
   - Output object position and size information

5. **Image Comparison Analysis**:
   - Compare differences between two binary images
   - Generate difference visualization images
   - Calculate difference percentage
   - Determine if new objects have entered based on threshold

## Technical Features

- Connected region analysis algorithm for identifying independent objects in images
- Support for multiple bit depth BMP images (1-bit, 4-bit, 8-bit, 24-bit, and 32-bit)
- Breadth-First Search (BFS) algorithm for connected region marking
- Optimized processing logic for different bit depth images
- Memory management optimization for processing large images
- Comprehensive error handling mechanisms

## Application Scenarios

- Simple object detection and counting
- Before and after image comparison analysis
- Motion detection and change monitoring
- Image format conversion and preprocessing
- Teaching demonstrations and experiments

This tool is suitable for users who need to perform basic image processing and simple computer vision tasks, especially in education, experimentation, and prototype development fields.
