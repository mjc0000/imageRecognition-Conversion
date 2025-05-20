#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// 链接通用对话框库
#pragma comment(lib, "comdlg32.lib")

/*
1. 对于灰度图转换功能:
   - 当输入已是灰度图时，代码仍会将其处理一遍，但输出结果会保持灰度不变
   - 24位或32位色深的灰度图(即RGB值相等的图像)会正确处理
   - 8位或更低色深的图像会直接复制其调色板，如果调色板已是灰度的，则保持不变

2. 对于二值化功能:
   - 当输入已是二值图时，根据设置的阈值，可能会将灰色像素进一步二值化为纯黑或纯白
   - 如果输入图像只有纯黑(0)和纯白(255)像素，那么:
     - 阈值 < 100 时：黑色区域保持黑色，白色区域保持白色
     - 阈值 = 100 时：所有区域变为黑色

代码能够正确处理各种位深度的BMP文件，包括已经是灰度或二值的图像，但处理逻辑会应用于所有输入，没有检测跳过已处理过的图像。
*/

typedef struct {
    unsigned char blue;
    unsigned char green;
    unsigned char red;
} RGB;

// 位置信息
typedef struct {
    int minX;
    int minY;
    int maxX;
    int maxY;
} BoundingBox;

typedef struct {
    int x;
    int y;
} Point;

// 标记是否已访问过
typedef struct {
    int width;
    int height;
    unsigned char* data;
} VisitedMap;

// 创建访问标记数组
VisitedMap* createVisitedMap(int width, int height) {
    VisitedMap* map = (VisitedMap*)malloc(sizeof(VisitedMap));
    if (map == NULL) return NULL;
    
    map->width = width;
    map->height = height;
    map->data = (unsigned char*)calloc(width * height, sizeof(unsigned char));
    
    if (map->data == NULL) {
        free(map);
        return NULL;
    }
    
    return map;
}

// 释放访问标记数组
void freeVisitedMap(VisitedMap* map) {
    if (map) {
        if (map->data) free(map->data);
        free(map);
    }
}

// 标记为已访问
void markVisited(VisitedMap* map, int x, int y) {
    if (x >= 0 && x < map->width && y >= 0 && y < map->height) {
        map->data[y * map->width + x] = 1;
    }
}

// 检查是否已访问
int isVisited(VisitedMap* map, int x, int y) {
    if (x >= 0 && x < map->width && y >= 0 && y < map->height) {
        return map->data[y * map->width + x];
    }
    return 1; // 越界视为已访问
}

// 计算灰度值
unsigned char rgbToGray(unsigned char r, unsigned char g, unsigned char b) {
    return (unsigned char)(0.299 * r + 0.587 * g + 0.114 * b);
}

// 绘制十字的函数
void drawCross(unsigned char *buffer, int width, int height, int bitCount, int rowSize) {
    int centerX = width / 2;
    int centerY = height / 2;
    int crossSize = 10;

    if (bitCount == 24) {
        for (int y = 0; y < height; y++) {
            for (int x = 0; x < width; x++) {
                RGB *pixel = (RGB *)(buffer + y * rowSize + x * 3);
                if ((y == centerY && abs(x - centerX) <= crossSize) ||
                    (x == centerX && abs(y - centerY) <= crossSize)) {
                    pixel->red = 255;
                    pixel->green = 0;
                    pixel->blue = 0;
                }
            }
        }
    }
    else if (bitCount == 32) {
        for (int y = 0; y < height; y++) {
            for (int x = 0; x < width; x++) {
                unsigned char *pixel = buffer + y * rowSize + x * 4;
                if ((y == centerY && abs(x - centerX) <= crossSize) ||
                    (x == centerX && abs(y - centerY) <= crossSize)) {
                    pixel[2] = 255; // R
                    pixel[1] = 0;   // G
                    pixel[0] = 0;   // B
                }
            }
        }
    }
}

// 绘制红色边框的函数
void drawRedRectangle(unsigned char *buffer, int width, int height, int bitCount, int rowSize) {
    // 寻找黑色区域的边界
    int minX = width, maxX = 0, minY = height, maxY = 0;
    int foundBlackPixel = 0;
    
    // 查找黑色像素的边界
    if (bitCount == 24) {
        for (int y = 0; y < height; y++) {
            for (int x = 0; x < width; x++) {
                RGB *pixel = (RGB *)(buffer + y * rowSize + x * 3);
                // 检查是否为黑色像素 (RGB值都很低)
                if (pixel->red < 50 && pixel->green < 50 && pixel->blue < 50) {
                    foundBlackPixel = 1;
                    if (x < minX) minX = x;
                    if (x > maxX) maxX = x;
                    if (y < minY) minY = y;
                    if (y > maxY) maxY = y;
                }
            }
        }
    }
    else if (bitCount == 32) {
        for (int y = 0; y < height; y++) {
            for (int x = 0; x < width; x++) {
                unsigned char *pixel = buffer + y * rowSize + x * 4;
                // 检查是否为黑色像素 (RGB值都很低)
                if (pixel[0] < 50 && pixel[1] < 50 && pixel[2] < 50) {
                    foundBlackPixel = 1;
                    if (x < minX) minX = x;
                    if (x > maxX) maxX = x;
                    if (y < minY) minY = y;
                    if (y > maxY) maxY = y;
                }
            }
        }
    }
    else if (bitCount == 8) {
        for (int y = 0; y < height; y++) {
            for (int x = 0; x < width; x++) {
                unsigned char pixel = buffer[y * rowSize + x];
                // 检查是否为黑色像素 (灰度值很低)
                if (pixel < 50) {
                    foundBlackPixel = 1;
                    if (x < minX) minX = x;
                    if (x > maxX) maxX = x;
                    if (y < minY) minY = y;
                    if (y > maxY) maxY = y;
                }
            }
        }
    }
    
    // 如果找到黑色像素，则绘制红色边框
    if (foundBlackPixel) {
        // 添加一点边距
        int margin = 5;
        minX = (minX - margin) > 0 ? (minX - margin) : 0;
        maxX = (maxX + margin) < width ? (maxX + margin) : (width - 1);
        minY = (minY - margin) > 0 ? (minY - margin) : 0;
        maxY = (maxY + margin) < height ? (maxY + margin) : (height - 1);
        
        // 绘制矩形边框
        if (bitCount == 24) {
            for (int y = minY; y <= maxY; y++) {
                for (int x = minX; x <= maxX; x++) {
                    // 只绘制边框
                    if (x == minX || x == maxX || y == minY || y == maxY) {
                        RGB *pixel = (RGB *)(buffer + y * rowSize + x * 3);
                        pixel->red = 255;
                        pixel->green = 0;
                        pixel->blue = 0;
                    }
                }
            }
        }
        else if (bitCount == 32) {
            for (int y = minY; y <= maxY; y++) {
                for (int x = minX; x <= maxX; x++) {
                    // 只绘制边框
                    if (x == minX || x == maxX || y == minY || y == maxY) {
                        unsigned char *pixel = buffer + y * rowSize + x * 4;
                        pixel[2] = 255; // R
                        pixel[1] = 0;   // G
                        pixel[0] = 0;   // B
                    }
                }
            }
        }
        else if (bitCount == 8) {
            // 对于8位图像，我们需要改变颜色索引
            // 这里简化处理，将边框像素设置为一个较亮的值
            for (int y = minY; y <= maxY; y++) {
                for (int x = minX; x <= maxX; x++) {
                    // 只绘制边框
                    if (x == minX || x == maxX || y == minY || y == maxY) {
                        buffer[y * rowSize + x] = 200; // 使用较亮的灰度值
                    }
                }
            }
        }
    }
}

BOOL ConvertToGrayScale(const char *inputPath, const char *grayPath, const char *crossPath) {
    FILE *inputFile = fopen(inputPath, "rb");
    if (!inputFile) {
        printf("无法打开输入文件！\n");
        return FALSE;
    }

    FILE *grayFile = fopen(grayPath, "wb");
    if (!grayFile) {
        fclose(inputFile);
        printf("无法创建灰度输出文件！\n");
        return FALSE;
    }

    FILE *crossFile = fopen(crossPath, "wb");
    if (!crossFile) {
        fclose(inputFile);
        fclose(grayFile);
        printf("无法创建带十字输出文件！\n");
        return FALSE;
    }

    BITMAPFILEHEADER fileHeader;
    BITMAPINFOHEADER infoHeader;

    fread(&fileHeader, sizeof(BITMAPFILEHEADER), 1, inputFile);
    fread(&infoHeader, sizeof(BITMAPINFOHEADER), 1, inputFile);

    if (fileHeader.bfType != 0x4D42) {
        fclose(inputFile);
        fclose(grayFile);
        fclose(crossFile);
        printf("不是有效的BMP文件！\n");
        return FALSE;
    }

    if (infoHeader.biBitCount != 1 && infoHeader.biBitCount != 4 && 
        infoHeader.biBitCount != 8 && infoHeader.biBitCount != 24 && 
        infoHeader.biBitCount != 32) {
        fclose(inputFile);
        fclose(grayFile);
        fclose(crossFile);
        printf("不支持的位深度！\n");
        return FALSE;
    }

    fwrite(&fileHeader, sizeof(BITMAPFILEHEADER), 1, grayFile);
    fwrite(&infoHeader, sizeof(BITMAPINFOHEADER), 1, grayFile);
    fwrite(&fileHeader, sizeof(BITMAPFILEHEADER), 1, crossFile);
    fwrite(&infoHeader, sizeof(BITMAPINFOHEADER), 1, crossFile);

    int width = infoHeader.biWidth;
    int height = abs(infoHeader.biHeight);
    int paletteSize = (infoHeader.biBitCount <= 8) ? (1 << infoHeader.biBitCount) : 0;

    if (paletteSize > 0) {
        RGBQUAD *palette = (RGBQUAD *)malloc(paletteSize * sizeof(RGBQUAD));
        if (!palette) {
            fclose(inputFile);
            fclose(grayFile);
            fclose(crossFile);
            printf("内存分配失败！\n");
            return FALSE;
        }

        fread(palette, sizeof(RGBQUAD), paletteSize, inputFile);
        for (int i = 0; i < paletteSize; i++) {
            unsigned char gray = rgbToGray(palette[i].rgbRed, palette[i].rgbGreen, palette[i].rgbBlue);
            palette[i].rgbRed = gray;
            palette[i].rgbGreen = gray;
            palette[i].rgbBlue = gray;
            palette[i].rgbReserved = 0;
        }
        fwrite(palette, sizeof(RGBQUAD), paletteSize, grayFile);
        fwrite(palette, sizeof(RGBQUAD), paletteSize, crossFile);
        free(palette);
    }

    int rowSize = ((width * infoHeader.biBitCount + 31) / 32) * 4;
    unsigned char *rowBuffer = (unsigned char *)malloc(rowSize * height);
    if (!rowBuffer) {
        fclose(inputFile);
        fclose(grayFile);
        fclose(crossFile);
        printf("内存分配失败！\n");
        return FALSE;
    }

    for (int y = 0; y < height; y++) {
        fread(rowBuffer + y * rowSize, 1, rowSize, inputFile);
    }

    if (infoHeader.biBitCount == 24) {
        for (int y = 0; y < height; y++) {
            for (int x = 0; x < width; x++) {
                RGB *pixel = (RGB *)(rowBuffer + y * rowSize + x * 3);
                unsigned char gray = rgbToGray(pixel->red, pixel->green, pixel->blue);
                pixel->red = gray;
                pixel->green = gray;
                pixel->blue = gray;
            }
            fwrite(rowBuffer + y * rowSize, 1, rowSize, grayFile);
        }
    }
    else if (infoHeader.biBitCount == 32) {
        for (int y = 0; y < height; y++) {
            for (int x = 0; x < width; x++) {
                unsigned char *pixel = rowBuffer + y * rowSize + x * 4;
                unsigned char gray = rgbToGray(pixel[2], pixel[1], pixel[0]);
                pixel[0] = gray; // B
                pixel[1] = gray; // G
                pixel[2] = gray; // R
            }
            fwrite(rowBuffer + y * rowSize, 1, rowSize, grayFile);
        }
    }
    else {
        for (int y = 0; y < height; y++) {
            fwrite(rowBuffer + y * rowSize, 1, rowSize, grayFile);
        }
    }

    if (infoHeader.biBitCount == 24 || infoHeader.biBitCount == 32) {
        drawCross(rowBuffer, width, height, infoHeader.biBitCount, rowSize);
    }
    for (int y = 0; y < height; y++) {
        fwrite(rowBuffer + y * rowSize, 1, rowSize, crossFile);
    }

    free(rowBuffer);
    fclose(inputFile);
    fclose(grayFile);
    fclose(crossFile);
    return TRUE;
}

BOOL DetectAndDrawRectangle(const char *inputPath, const char *outputPath) {
    FILE *inputFile = fopen(inputPath, "rb");
    if (!inputFile) {
        printf("无法打开输入文件！\n");
        return FALSE;
    }

    FILE *outputFile = fopen(outputPath, "wb");
    if (!outputFile) {
        fclose(inputFile);
        printf("无法创建输出文件！\n");
        return FALSE;
    }

    BITMAPFILEHEADER fileHeader;
    BITMAPINFOHEADER infoHeader;

    fread(&fileHeader, sizeof(BITMAPFILEHEADER), 1, inputFile);
    fread(&infoHeader, sizeof(BITMAPINFOHEADER), 1, inputFile);

    if (fileHeader.bfType != 0x4D42) {
        fclose(inputFile);
        fclose(outputFile);
        printf("不是有效的BMP文件！\n");
        return FALSE;
    }

    fwrite(&fileHeader, sizeof(BITMAPFILEHEADER), 1, outputFile);
    fwrite(&infoHeader, sizeof(BITMAPINFOHEADER), 1, outputFile);

    int width = infoHeader.biWidth;
    int height = abs(infoHeader.biHeight);
    int paletteSize = (infoHeader.biBitCount <= 8) ? (1 << infoHeader.biBitCount) : 0;

    // 处理调色板
    if (paletteSize > 0) {
        RGBQUAD *palette = (RGBQUAD *)malloc(paletteSize * sizeof(RGBQUAD));
        if (!palette) {
            fclose(inputFile);
            fclose(outputFile);
            printf("内存分配失败！\n");
            return FALSE;
        }

        fread(palette, sizeof(RGBQUAD), paletteSize, inputFile);
        fwrite(palette, sizeof(RGBQUAD), paletteSize, outputFile);
        free(palette);
    }

    int rowSize = ((width * infoHeader.biBitCount + 31) / 32) * 4;
    unsigned char *rowBuffer = (unsigned char *)malloc(rowSize * height);
    if (!rowBuffer) {
        fclose(inputFile);
        fclose(outputFile);
        printf("内存分配失败！\n");
        return FALSE;
    }

    for (int y = 0; y < height; y++) {
        fread(rowBuffer + y * rowSize, 1, rowSize, inputFile);
    }

    // 绘制红色边框
    drawRedRectangle(rowBuffer, width, height, infoHeader.biBitCount, rowSize);

    // 写入输出文件
    for (int y = 0; y < height; y++) {
        fwrite(rowBuffer + y * rowSize, 1, rowSize, outputFile);
    }

    free(rowBuffer);
    fclose(inputFile);
    fclose(outputFile);
    return TRUE;
}

// 比较两张二值图像并生成差异图
BOOL CompareBinaryImages(const char *firstImagePath, const char *secondImagePath, const char *outputPath, int threshold) {
    FILE *firstFile = fopen(firstImagePath, "rb");
    if (!firstFile) {
        printf("无法打开第一个输入文件！\n");
        return FALSE;
    }

    FILE *secondFile = fopen(secondImagePath, "rb");
    if (!secondFile) {
        fclose(firstFile);
        printf("无法打开第二个输入文件！\n");
        return FALSE;
    }

    FILE *outputFile = fopen(outputPath, "wb");
    if (!outputFile) {
        fclose(firstFile);
        fclose(secondFile);
        printf("无法创建输出文件！\n");
        return FALSE;
    }

    // 读取第一个文件的头信息
    BITMAPFILEHEADER fileHeader1;
    BITMAPINFOHEADER infoHeader1;
    fread(&fileHeader1, sizeof(BITMAPFILEHEADER), 1, firstFile);
    fread(&infoHeader1, sizeof(BITMAPINFOHEADER), 1, firstFile);

    // 读取第二个文件的头信息
    BITMAPFILEHEADER fileHeader2;
    BITMAPINFOHEADER infoHeader2;
    fread(&fileHeader2, sizeof(BITMAPFILEHEADER), 1, secondFile);
    fread(&infoHeader2, sizeof(BITMAPINFOHEADER), 1, secondFile);

    // 检查文件格式
    if (fileHeader1.bfType != 0x4D42 || fileHeader2.bfType != 0x4D42) {
        fclose(firstFile);
        fclose(secondFile);
        fclose(outputFile);
        printf("输入的不是有效的BMP文件！\n");
        return FALSE;
    }

    // 检查图像尺寸是否一致
    if (infoHeader1.biWidth != infoHeader2.biWidth || 
        abs(infoHeader1.biHeight) != abs(infoHeader2.biHeight) ||
        infoHeader1.biBitCount != infoHeader2.biBitCount) {
        fclose(firstFile);
        fclose(secondFile);
        fclose(outputFile);
        printf("两张图像的尺寸或位深度不一致！\n");
        return FALSE;
    }

    // 检查位深度
    if (infoHeader1.biBitCount != 24 && infoHeader1.biBitCount != 32) {
        fclose(firstFile);
        fclose(secondFile);
        fclose(outputFile);
        printf("只支持24位和32位BMP图像！\n");
        return FALSE;
    }

    // 写入文件头和信息头到输出文件
    fwrite(&fileHeader1, sizeof(BITMAPFILEHEADER), 1, outputFile);
    fwrite(&infoHeader1, sizeof(BITMAPINFOHEADER), 1, outputFile);

    int width = infoHeader1.biWidth;
    int height = abs(infoHeader1.biHeight);
    int rowSize = ((width * infoHeader1.biBitCount + 31) / 32) * 4;
    int bytesPerPixel = infoHeader1.biBitCount / 8;

    // 分配内存
    unsigned char *buffer1 = (unsigned char *)malloc(rowSize * height);
    unsigned char *buffer2 = (unsigned char *)malloc(rowSize * height);
    unsigned char *outputBuffer = (unsigned char *)malloc(rowSize * height);

    if (!buffer1 || !buffer2 || !outputBuffer) {
        if (buffer1) free(buffer1);
        if (buffer2) free(buffer2);
        if (outputBuffer) free(outputBuffer);
        fclose(firstFile);
        fclose(secondFile);
        fclose(outputFile);
        printf("内存分配失败！\n");
        return FALSE;
    }

    // 读取图像数据
    fseek(firstFile, fileHeader1.bfOffBits, SEEK_SET);
    fseek(secondFile, fileHeader2.bfOffBits, SEEK_SET);

    fread(buffer1, 1, rowSize * height, firstFile);
    fread(buffer2, 1, rowSize * height, secondFile);

    // 计算差异
    int diffPixelCount = 0;
    int totalPixels = width * height;

    for (int y = 0; y < height; y++) {
        for (int x = 0; x < width; x++) {
            // 获取两个图像对应位置的像素值
            unsigned char pixel1Value, pixel2Value;
            
            if (infoHeader1.biBitCount == 24) {
                RGB* pixel1 = (RGB*)(buffer1 + y * rowSize + x * 3);
                RGB* pixel2 = (RGB*)(buffer2 + y * rowSize + x * 3);
                RGB* outputPixel = (RGB*)(outputBuffer + y * rowSize + x * 3);
                
                // 在二值图中，RGB通常相等，所以只比较一个通道
                pixel1Value = pixel1->red;
                pixel2Value = pixel2->red;
                
                // 计算差异
                if (pixel1Value != pixel2Value) {
                    // 差异像素标记为红色
                    outputPixel->red = 255;
                    outputPixel->green = 0;
                    outputPixel->blue = 0;
                    diffPixelCount++;
                } else {
                    // 相同像素保持原值（白或黑）
                    outputPixel->red = pixel1Value;
                    outputPixel->green = pixel1Value;
                    outputPixel->blue = pixel1Value;
                }
            } else { // 32位
                unsigned char* pixel1 = buffer1 + y * rowSize + x * 4;
                unsigned char* pixel2 = buffer2 + y * rowSize + x * 4;
                unsigned char* outputPixel = outputBuffer + y * rowSize + x * 4;
                
                pixel1Value = pixel1[2]; // R值
                pixel2Value = pixel2[2]; // R值
                
                if (pixel1Value != pixel2Value) {
                    // 差异像素标记为红色
                    outputPixel[0] = 0;    // B
                    outputPixel[1] = 0;    // G
                    outputPixel[2] = 255;  // R
                    outputPixel[3] = 255;  // A
                    diffPixelCount++;
                } else {
                    // 相同像素保持原值
                    outputPixel[0] = pixel1Value; // B
                    outputPixel[1] = pixel1Value; // G
                    outputPixel[2] = pixel1Value; // R
                    outputPixel[3] = 255;         // A
                }
            }
        }
    }

    // 写入差异图像
    fwrite(outputBuffer, 1, rowSize * height, outputFile);

    // 计算差异百分比
    double diffPercentage = (double)diffPixelCount / totalPixels * 100.0;
    printf("差异像素数量: %d (%.2f%%)\n", diffPixelCount, diffPercentage);

    // 根据阈值判断是否有新物品
    if (diffPercentage > threshold) {
        printf("检测到新物品进入！差异超过阈值 %.2f%%\n", (double)threshold);
    } else {
        printf("未检测到明显变化，差异低于阈值 %.2f%%\n", (double)threshold);
    }

    // 释放资源
    free(buffer1);
    free(buffer2);
    free(outputBuffer);
    fclose(firstFile);
    fclose(secondFile);
    fclose(outputFile);
    return TRUE;
}

// 将BMP转换为二值图像
BOOL ConvertToBinary(const char *inputPath, const char *outputPath, int threshold) {
    FILE *inputFile = fopen(inputPath, "rb");
    if (!inputFile) {
        printf("无法打开输入文件！\n");
        return FALSE;
    }

    FILE *outputFile = fopen(outputPath, "wb");
    if (!outputFile) {
        fclose(inputFile);
        printf("无法创建输出文件！\n");
        return FALSE;
    }

    BITMAPFILEHEADER fileHeader;
    BITMAPINFOHEADER infoHeader;

    fread(&fileHeader, sizeof(BITMAPFILEHEADER), 1, inputFile);
    fread(&infoHeader, sizeof(BITMAPINFOHEADER), 1, inputFile);

    if (fileHeader.bfType != 0x4D42) {
        fclose(inputFile);
        fclose(outputFile);
        printf("不是有效的BMP文件！\n");
        return FALSE;
    }

    if (infoHeader.biBitCount != 24 && infoHeader.biBitCount != 32) {
        fclose(inputFile);
        fclose(outputFile);
        printf("只支持24位和32位BMP图像！\n");
        return FALSE;
    }

    fwrite(&fileHeader, sizeof(BITMAPFILEHEADER), 1, outputFile);
    fwrite(&infoHeader, sizeof(BITMAPINFOHEADER), 1, outputFile);

    int width = infoHeader.biWidth;
    int height = abs(infoHeader.biHeight);
    int rowSize = ((width * infoHeader.biBitCount + 31) / 32) * 4;
    int bytesPerPixel = infoHeader.biBitCount / 8;

    unsigned char *buffer = (unsigned char *)malloc(rowSize * height);
    if (!buffer) {
        fclose(inputFile);
        fclose(outputFile);
        printf("内存分配失败！\n");
        return FALSE;
    }

    // 读取图像数据
    for (int y = 0; y < height; y++) {
        fread(buffer + y * rowSize, 1, rowSize, inputFile);
    }

    // 处理图像
    for (int y = 0; y < height; y++) {
        for (int x = 0; x < width; x++) {
            unsigned char pixelValue;
            if (infoHeader.biBitCount == 24) {
                RGB* pixel = (RGB*)(buffer + y * rowSize + x * 3);
                pixelValue = pixel->red;
            } else { // 32位
                unsigned char* pixel = buffer + y * rowSize + x * 4;
                pixelValue = pixel[2]; // R值
            }
            
            if (pixelValue < threshold) {
                if (infoHeader.biBitCount == 24) {
                    RGB* pixel = (RGB*)(buffer + y * rowSize + x * 3);
                    pixel->red = 0;
                    pixel->green = 0;
                    pixel->blue = 0;
                } else { // 32位
                    unsigned char* pixel = buffer + y * rowSize + x * 4;
                    pixel[0] = 0;
                    pixel[1] = 0;
                    pixel[2] = 0;
                }
            } else {
                if (infoHeader.biBitCount == 24) {
                    RGB* pixel = (RGB*)(buffer + y * rowSize + x * 3);
                    pixel->red = 255;
                    pixel->green = 255;
                    pixel->blue = 255;
                } else { // 32位
                    unsigned char* pixel = buffer + y * rowSize + x * 4;
                    pixel[0] = 255;
                    pixel[1] = 255;
                    pixel[2] = 255;
                }
            }
        }
    }

    // 写入处理后的图像数据
    for (int y = 0; y < height; y++) {
        fwrite(buffer + y * rowSize, 1, rowSize, outputFile);
    }

    free(buffer);
    fclose(inputFile);
    fclose(outputFile);
    return TRUE;
}

// 将JPG转换为BMP - 使用命令行工具
BOOL ConvertJpgToBmp(const char *jpgPath, const char *bmpPath) {
    FILE *originalFile = fopen(jpgPath, "rb");
    if (!originalFile) {
        printf("无法打开JPG文件！\n");
        return FALSE;
    }
    fclose(originalFile);
    
    // 使用系统画图工具，但不使用start命令，避免创建新进程导致闪烁
    char command[600] = {0};
    sprintf(command, "mspaint \"%s\"", jpgPath);
    
    printf("系统将打开图片 %s\n", jpgPath);
    printf("请在打开的画图程序中执行:\n");
    printf("1. 点击'文件'>'另存为'\n");
    printf("2. 选择保存类型为'24位位图(*.bmp;*.dib)'\n");
    printf("3. 文件名输入: %s\n", bmpPath);
    printf("4. 点击'保存'\n");
    printf("5. 关闭画图程序\n\n");
    printf("按任意键打开图片...\n");
    getchar();
    
    // 执行命令
    system(command);
    
    // 检查文件是否已创建
    FILE *outFile = fopen(bmpPath, "rb");
    if (outFile) {
        fclose(outFile);
        printf("检测到BMP文件已创建: %s\n", bmpPath);
        return TRUE;
    } else {
        printf("未检测到BMP文件，可能转换失败\n");
        return FALSE;
    }
}

// 使用广度优先搜索查找连通区域
void findConnectedComponent(unsigned char* buffer, int width, int height, int bitCount, int rowSize, 
                            int startX, int startY, VisitedMap* visited, BoundingBox* bbox, int* pixelCount) {
    
    // 只支持24位和32位图
    if (bitCount != 24 && bitCount != 32) return;
    
    int pixelsPerByte = 8 / bitCount;
    int bytesPerPixel = bitCount / 8;
    
    // 初始化
    *pixelCount = 0;
    bbox->minX = width;
    bbox->minY = height;
    bbox->maxX = 0;
    bbox->maxY = 0;
    
    // 创建队列
    Point* queue = (Point*)malloc(width * height * sizeof(Point));
    if (!queue) return;
    
    int front = 0;
    int rear = 0;
    
    // 添加起始点
    queue[rear].x = startX;
    queue[rear].y = startY;
    rear++;
    markVisited(visited, startX, startY);
    
    // 方向数组，表示8个方向
    int dx[8] = {-1, -1, -1, 0, 0, 1, 1, 1};
    int dy[8] = {-1, 0, 1, -1, 1, -1, 0, 1};
    
    while (front < rear) {
        Point current = queue[front++];
        int x = current.x;
        int y = current.y;
        
        // 更新边界框
        if (x < bbox->minX) bbox->minX = x;
        if (y < bbox->minY) bbox->minY = y;
        if (x > bbox->maxX) bbox->maxX = x;
        if (y > bbox->maxY) bbox->maxY = y;
        
        (*pixelCount)++;
        
        // 检查周围8个方向
        for (int i = 0; i < 8; i++) {
            int newX = x + dx[i];
            int newY = y + dy[i];
            
            // 检查边界
            if (newX < 0 || newX >= width || newY < 0 || newY >= height) continue;
            
            // 检查是否已访问
            if (isVisited(visited, newX, newY)) continue;
            
            // 检查是否是黑色像素（二值图中物体通常为黑色）
            unsigned char pixelValue;
            if (bitCount == 24) {
                RGB* pixel = (RGB*)(buffer + newY * rowSize + newX * 3);
                pixelValue = pixel->red; // 在二值图中，r=g=b
            } else { // bitCount == 32
                unsigned char* pixel = buffer + newY * rowSize + newX * 4;
                pixelValue = pixel[2]; // R值
            }
            
            // 如果是黑色像素（值小于128，二值图中通常为0）
            if (pixelValue < 128) {
                queue[rear].x = newX;
                queue[rear].y = newY;
                rear++;
                markVisited(visited, newX, newY);
            }
        }
    }
    
    free(queue);
}

// 分析并标记二值图中的物体
BOOL MarkObjectsInBinaryImage(const char *inputPath, const char *outputPath) {
    FILE *inputFile = fopen(inputPath, "rb");
    if (!inputFile) {
        printf("无法打开输入文件！\n");
        return FALSE;
    }

    // 读取文件头和信息头
    BITMAPFILEHEADER fileHeader;
    BITMAPINFOHEADER infoHeader;
    size_t readSize;
    
    readSize = fread(&fileHeader, sizeof(BITMAPFILEHEADER), 1, inputFile);
    if (readSize != 1) {
        printf("读取文件头失败！\n");
        fclose(inputFile);
        return FALSE;
    }
    
    readSize = fread(&infoHeader, sizeof(BITMAPINFOHEADER), 1, inputFile);
    if (readSize != 1) {
        printf("读取信息头失败！\n");
        fclose(inputFile);
        return FALSE;
    }

    if (fileHeader.bfType != 0x4D42) {
        fclose(inputFile);
        printf("不是有效的BMP文件！\n");
        return FALSE;
    }

    // 获取图像信息
    int width = infoHeader.biWidth;
    int height = abs(infoHeader.biHeight);
    int bitCount = infoHeader.biBitCount;
    int rowSize = ((width * bitCount + 31) / 32) * 4;
    
    printf("图片信息: 宽度=%d, 高度=%d, 位深=%d\n", width, height, bitCount);
    
    // 创建输出文件
    FILE *outputFile = fopen(outputPath, "wb");
    if (!outputFile) {
        fclose(inputFile);
        printf("无法创建输出文件！\n");
        return FALSE;
    }
    
    // 写入文件头和信息头
    fwrite(&fileHeader, sizeof(BITMAPFILEHEADER), 1, outputFile);
    fwrite(&infoHeader, sizeof(BITMAPINFOHEADER), 1, outputFile);
    
    // 处理调色板（如果有）
    unsigned char *palette = NULL;
    int paletteSize = 0;
    if (bitCount <= 8) {
        paletteSize = (1 << bitCount) * sizeof(RGBQUAD);
        palette = (unsigned char*)malloc(paletteSize);
        if (!palette) {
            fclose(inputFile);
            fclose(outputFile);
            printf("内存分配失败！\n");
            return FALSE;
        }
        
        readSize = fread(palette, 1, paletteSize, inputFile);
        if (readSize != paletteSize) {
            free(palette);
            fclose(inputFile);
            fclose(outputFile);
            printf("读取调色板失败！\n");
            return FALSE;
        }
        
        // 对于8位图像，我们需要修改调色板以支持红色边框
        if (bitCount == 8) {
            // 保留一个调色板索引用于红色边框（选择最后一个索引240）
            int redIndex = 240;
            RGBQUAD* palEntries = (RGBQUAD*)palette;
            palEntries[redIndex].rgbRed = 255;     // 设置为红色
            palEntries[redIndex].rgbGreen = 0;
            palEntries[redIndex].rgbBlue = 0;
            palEntries[redIndex].rgbReserved = 0;
            
            printf("为8位图像预留调色板索引 %d 用于红色边框\n", redIndex);
        }
        
        fwrite(palette, 1, paletteSize, outputFile);
    }
    
    // 创建缓冲区
    unsigned char *buffer = (unsigned char*)malloc(rowSize * height);
    if (!buffer) {
        fclose(inputFile);
        fclose(outputFile);
        printf("内存分配失败！\n");
        return FALSE;
    }
    
    // 读取图像数据
    fseek(inputFile, fileHeader.bfOffBits, SEEK_SET);
    readSize = fread(buffer, 1, rowSize * height, inputFile);
    if (readSize != rowSize * height) {
        free(buffer);
        fclose(inputFile);
        fclose(outputFile);
        printf("读取图像数据失败！实际读取 %zu 字节，期望 %d 字节\n", readSize, rowSize * height);
        return FALSE;
    }
    
    // 创建访问标记地图
    VisitedMap* visited = createVisitedMap(width, height);
    if (!visited) {
        free(buffer);
        fclose(inputFile);
        fclose(outputFile);
        printf("内存分配失败！\n");
        return FALSE;
    }

    // 查找并标记物体
    int objectCount = 0;
    int minObjectSize = 50;     // 最小物体像素数量，降低以检测更小的物体
    int maxObjects = 50;        // 增加最大物体数量
    BoundingBox* objects = (BoundingBox*)malloc(maxObjects * sizeof(BoundingBox));
    if (!objects) {
        freeVisitedMap(visited);
        free(buffer);
        fclose(inputFile);
        fclose(outputFile);
        printf("内存分配失败！\n");
        return FALSE;
    }
    
    printf("开始分析图像...\n");
    
    // 根据位深度处理图像
    if (bitCount == 24 || bitCount == 32) {
        // 直接处理24位或32位图像
        int bytesPerPixel = bitCount / 8;
        
        for (int y = 0; y < height; y++) {
            for (int x = 0; x < width; x++) {
                // 检查像素是否已访问
                if (isVisited(visited, x, y)) continue;
                
                // 获取像素值
                unsigned char pixelValue;
                if (bitCount == 24) {
                    RGB* pixel = (RGB*)(buffer + y * rowSize + x * 3);
                    pixelValue = pixel->red; // 二值图中RGB通常相等
                } else { // 32位
                    unsigned char* pixel = buffer + y * rowSize + x * 4;
                    pixelValue = pixel[2]; // R值
                }
                
                // 如果是黑色像素(值小于128)
                if (pixelValue < 128) {
                    BoundingBox bbox;
                    int pixelCount = 0;
                    
                    // 查找连通区域
                    findConnectedComponent(buffer, width, height, bitCount, rowSize, 
                                          x, y, visited, &bbox, &pixelCount);
                    
                    // 过滤小区域
                    if (pixelCount >= minObjectSize) {
                        printf("找到物体 #%d: 位置(%d,%d)-(%d,%d), 大小: %d像素\n", 
                               objectCount+1, bbox.minX, bbox.minY, bbox.maxX, bbox.maxY, pixelCount);
                        
                        if (objectCount < maxObjects) {
                            objects[objectCount++] = bbox;
                        }
                    }
                } else {
                    // 标记白色区域为已访问，加速处理
                    markVisited(visited, x, y);
                }
            }
        }
    } else if (bitCount == 8) {
        // 处理8位图像
        for (int y = 0; y < height; y++) {
            for (int x = 0; x < width; x++) {
                if (isVisited(visited, x, y)) continue;
                
                unsigned char* pixel = buffer + y * rowSize + x;
                if (*pixel < 128) {
                    BoundingBox bbox;
                    int pixelCount = 0;
                    
                    // 特殊处理8位图像的连通区域
                    // 创建一个临时队列
                    Point* queue = (Point*)malloc(width * height * sizeof(Point));
                    if (!queue) {
                        printf("内存分配失败！\n");
                        continue;
                    }
                    
                    int front = 0, rear = 0;
                    queue[rear].x = x;
                    queue[rear].y = y;
                    rear++;
                    markVisited(visited, x, y);
                    
                    // 初始化边界框
                    bbox.minX = x;
                    bbox.minY = y;
                    bbox.maxX = x;
                    bbox.maxY = y;
                    pixelCount = 1;
                    
                    // 方向数组
                    int dx[8] = {-1, -1, -1, 0, 0, 1, 1, 1};
                    int dy[8] = {-1, 0, 1, -1, 1, -1, 0, 1};
                    
                    while (front < rear) {
                        Point current = queue[front++];
                        int cx = current.x;
                        int cy = current.y;
                        
                        for (int i = 0; i < 8; i++) {
                            int nx = cx + dx[i];
                            int ny = cy + dy[i];
                            
                            if (nx < 0 || nx >= width || ny < 0 || ny >= height) continue;
                            if (isVisited(visited, nx, ny)) continue;
                            
                            unsigned char* p = buffer + ny * rowSize + nx;
                            if (*p < 128) {
                                queue[rear].x = nx;
                                queue[rear].y = ny;
                                rear++;
                                markVisited(visited, nx, ny);
                                pixelCount++;
                                
                                // 更新边界框
                                if (nx < bbox.minX) bbox.minX = nx;
                                if (ny < bbox.minY) bbox.minY = ny;
                                if (nx > bbox.maxX) bbox.maxX = nx;
                                if (ny > bbox.maxY) bbox.maxY = ny;
                            }
                        }
                    }
                    
                    free(queue);
                    
                    if (pixelCount >= minObjectSize) {
                        printf("找到物体 #%d: 位置(%d,%d)-(%d,%d), 大小: %d像素\n", 
                               objectCount+1, bbox.minX, bbox.minY, bbox.maxX, bbox.maxY, pixelCount);
                        
                        if (objectCount < maxObjects) {
                            objects[objectCount++] = bbox;
                        }
                    }
                } else {
                    markVisited(visited, x, y);
                }
            }
        }
    } else if (bitCount == 1 || bitCount == 4) {
        printf("目前不支持 %d 位深度的图像自动检测物体\n", bitCount);
        // 用户可以先转换为24位再处理
    }
    
    printf("找到 %d 个物体\n", objectCount);
    
    // 用红色框标记物体
    for (int i = 0; i < objectCount; i++) {
        BoundingBox bbox = objects[i];
        
        // 边界框扩展
        int padding = 2;
        bbox.minX = max(0, bbox.minX - padding);
        bbox.minY = max(0, bbox.minY - padding);
        bbox.maxX = min(width - 1, bbox.maxX + padding);
        bbox.maxY = min(height - 1, bbox.maxY + padding);
        
        // 画红框，根据不同位深度处理
        if (bitCount == 24) {
            // 绘制水平线
            for (int x = bbox.minX; x <= bbox.maxX; x++) {
                // 上边框
                RGB* pixel = (RGB*)(buffer + bbox.minY * rowSize + x * 3);
                pixel->red = 255;   // 红色
                pixel->green = 0;
                pixel->blue = 0;
                
                // 下边框
                pixel = (RGB*)(buffer + bbox.maxY * rowSize + x * 3);
                pixel->red = 255;
                pixel->green = 0;
                pixel->blue = 0;
            }
            
            // 绘制垂直线
            for (int y = bbox.minY; y <= bbox.maxY; y++) {
                // 左边框
                RGB* pixel = (RGB*)(buffer + y * rowSize + bbox.minX * 3);
                pixel->red = 255;
                pixel->green = 0;
                pixel->blue = 0;
                
                // 右边框
                pixel = (RGB*)(buffer + y * rowSize + bbox.maxX * 3);
                pixel->red = 255;
                pixel->green = 0;
                pixel->blue = 0;
            }
        } else if (bitCount == 32) {
            // 绘制水平线
            for (int x = bbox.minX; x <= bbox.maxX; x++) {
                // 上边框
                unsigned char* pixel = buffer + bbox.minY * rowSize + x * 4;
                pixel[2] = 255; // R
                pixel[1] = 0;   // G
                pixel[0] = 0;   // B
                
                // 下边框
                pixel = buffer + bbox.maxY * rowSize + x * 4;
                pixel[2] = 255;
                pixel[1] = 0;
                pixel[0] = 0;
            }
            
            // 绘制垂直线
            for (int y = bbox.minY; y <= bbox.maxY; y++) {
                // 左边框
                unsigned char* pixel = buffer + y * rowSize + bbox.minX * 4;
                pixel[2] = 255;
                pixel[1] = 0;
                pixel[0] = 0;
                
                // 右边框
                pixel = buffer + y * rowSize + bbox.maxX * 4;
                pixel[2] = 255;
                pixel[1] = 0;
                pixel[0] = 0;
            }
        } else if (bitCount == 8) {
            // 对于8位图像，使用我们在调色板中预留的红色索引
            unsigned char frameColor = 240; // 调色板中预留的红色索引
            
            // 绘制水平线
            for (int x = bbox.minX; x <= bbox.maxX; x++) {
                buffer[bbox.minY * rowSize + x] = frameColor;
                buffer[bbox.maxY * rowSize + x] = frameColor;
            }
            
            // 绘制垂直线
            for (int y = bbox.minY; y <= bbox.maxY; y++) {
                buffer[y * rowSize + bbox.minX] = frameColor;
                buffer[y * rowSize + bbox.maxX] = frameColor;
            }
        }
    }
    
    // 写入处理后的图像数据
    fwrite(buffer, 1, rowSize * height, outputFile);
    
    // 释放资源
    free(objects);
    freeVisitedMap(visited);
    free(buffer);
    fclose(inputFile);
    fclose(outputFile);
    
    return TRUE;
}

int main() {
    int choice;
    while (1) { // 添加循环以保持程序运行
        system("cls"); // 清屏以保持界面整洁
        printf("欢迎使用BMP图像处理工具\n");
        printf("请输入选项:\n");
        printf("1 - 转换BMP为灰度图\n");
        printf("2 - 转换BMP为二值图\n");
        printf("3 - 转换JPG为BMP\n");
        printf("4 - 标记二值图中的物体\n");
        printf("5 - 比较两张二值图像\n");
        printf("0 - 退出程序\n");
        printf("选项: ");
        
        // 清空输入缓冲区
        fflush(stdin);
        if (scanf("%d", &choice) != 1) {
            printf("输入无效，请输入数字！\n");
            printf("按任意键继续...\n");
            getchar();
            continue;
        }

        if (choice == 1) {
            OPENFILENAME ofn;
            char szFile[260] = {0};
            char grayFile[260] = {0};
            char crossFile[260] = {0};

            ZeroMemory(&ofn, sizeof(ofn));
            ofn.lStructSize = sizeof(OPENFILENAME);
            ofn.hwndOwner = NULL;
            ofn.lpstrFile = szFile;
            ofn.nMaxFile = sizeof(szFile);
            ofn.lpstrFilter = "BMP Files (*.bmp)\0*.bmp\0All Files (*.*)\0*.*\0";
            ofn.nFilterIndex = 1;
            ofn.lpstrFileTitle = NULL;
            ofn.nMaxFileTitle = 0;
            ofn.lpstrInitialDir = NULL;
            ofn.lpstrTitle = "选择BMP文件";
            ofn.Flags = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST | OFN_HIDEREADONLY;

            if (!GetOpenFileName(&ofn)) {
                DWORD error = CommDlgExtendedError();
                if (error) {
                    printf("打开文件对话框失败，错误代码: %lu\n", error);
                } else {
                    printf("用户取消了选择\n");
                }
            } else {
                strncpy(grayFile, szFile, sizeof(grayFile) - 6);
                grayFile[sizeof(grayFile) - 6] = '\0';
                strcat(grayFile, "_gray.bmp");

                strncpy(crossFile, szFile, sizeof(crossFile) - 11);
                crossFile[sizeof(crossFile) - 11] = '\0';
                strcat(crossFile, "_gray_cross.bmp");

                if (ConvertToGrayScale(szFile, grayFile, crossFile)) {
                    printf("转换成功！\n");
                    printf("灰度图: %s\n", grayFile);
                    printf("带十字灰度图: %s\n", crossFile);
                } else {
                    printf("转换失败！\n");
                }
            }
            printf("按任意键继续...\n");
            fflush(stdin); // 清空输入缓冲区
            getchar();
        }
        else if (choice == 2) {
            OPENFILENAME ofn;
            char szFile[260] = {0};
            char outputFile[260] = {0};

            ZeroMemory(&ofn, sizeof(ofn));
            ofn.lStructSize = sizeof(OPENFILENAME);
            ofn.hwndOwner = NULL;
            ofn.lpstrFile = szFile;
            ofn.nMaxFile = sizeof(szFile);
            ofn.lpstrFilter = "BMP Files (*.bmp)\0*.bmp\0All Files (*.*)\0*.*\0";
            ofn.nFilterIndex = 1;
            ofn.lpstrFileTitle = NULL;
            ofn.nMaxFileTitle = 0;
            ofn.lpstrInitialDir = NULL;
            ofn.lpstrTitle = "选择二值图BMP文件";
            ofn.Flags = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST | OFN_HIDEREADONLY;

            if (!GetOpenFileName(&ofn)) {
                DWORD error = CommDlgExtendedError();
                if (error) {
                    printf("打开文件对话框失败，错误代码: %lu\n", error);
                } else {
                    printf("用户取消了选择\n");
                }
            } else {
                // 创建输出文件名
                strncpy(outputFile, szFile, sizeof(outputFile) - 12);
                outputFile[sizeof(outputFile) - 12] = '\0';
                strcat(outputFile, "_binary.bmp");

                if (ConvertToBinary(szFile, outputFile, 100)) {
                    printf("处理完成！\n");
                    printf("二值化后的图像: %s\n", outputFile);
                } else {
                    printf("处理失败！\n");
                }
            }
            printf("按任意键继续...\n");
            fflush(stdin);
            getchar();
        }
        else if (choice == 3) {
            OPENFILENAME ofn;
            char szFile[260] = {0};
            char bmpFile[260] = {0};

            ZeroMemory(&ofn, sizeof(ofn));
            ofn.lStructSize = sizeof(OPENFILENAME);
            ofn.hwndOwner = NULL;
            ofn.lpstrFile = szFile;
            ofn.nMaxFile = sizeof(szFile);
            ofn.lpstrFilter = "JPG Files (*.jpg;*.jpeg)\0*.jpg;*.jpeg\0All Files (*.*)\0*.*\0";
            ofn.nFilterIndex = 1;
            ofn.lpstrFileTitle = NULL;
            ofn.nMaxFileTitle = 0;
            ofn.lpstrInitialDir = NULL;
            ofn.lpstrTitle = "选择JPG文件";
            ofn.Flags = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST | OFN_HIDEREADONLY;

            if (!GetOpenFileName(&ofn)) {
                DWORD error = CommDlgExtendedError();
                if (error) {
                    printf("打开文件对话框失败，错误代码: %lu\n", error);
                } else {
                    printf("用户取消了选择\n");
                }
            } else {
                // 创建输出BMP文件名
                strncpy(bmpFile, szFile, sizeof(bmpFile) - 5);
                
                // 找到最后一个点的位置
                char *dot = strrchr(bmpFile, '.');
                if (dot != NULL) {
                    *dot = '\0'; // 截断文件名，移除原扩展名
                }
                
                strcat(bmpFile, ".bmp");

                if (ConvertJpgToBmp(szFile, bmpFile)) {
                    printf("转换完成！\n");
                    printf("BMP图像: %s\n", bmpFile);
                } else {
                    printf("转换失败！\n");
                }
            }
            printf("按任意键继续...\n");
            fflush(stdin);
            getchar();
        }
        else if (choice == 4) {
            OPENFILENAME ofn;
            char szFile[260] = {0};
            char outFile[260] = {0};

            ZeroMemory(&ofn, sizeof(ofn));
            ofn.lStructSize = sizeof(OPENFILENAME);
            ofn.hwndOwner = NULL;
            ofn.lpstrFile = szFile;
            ofn.nMaxFile = sizeof(szFile);
            ofn.lpstrFilter = "BMP Files (*.bmp)\0*.bmp\0All Files (*.*)\0*.*\0";
            ofn.nFilterIndex = 1;
            ofn.lpstrFileTitle = NULL;
            ofn.nMaxFileTitle = 0;
            ofn.lpstrInitialDir = NULL;
            ofn.lpstrTitle = "选择二值图BMP文件";
            ofn.Flags = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST | OFN_HIDEREADONLY;

            if (!GetOpenFileName(&ofn)) {
                DWORD error = CommDlgExtendedError();
                if (error) {
                    printf("打开文件对话框失败，错误代码: %lu\n", error);
                } else {
                    printf("用户取消了选择\n");
                }
            } else {
                // 创建输出文件名
                strncpy(outFile, szFile, sizeof(outFile) - 12);
                outFile[sizeof(outFile) - 12] = '\0';
                strcat(outFile, "_objects.bmp");

                printf("开始识别物体...\n");
                if (MarkObjectsInBinaryImage(szFile, outFile)) {
                    printf("识别完成！\n");
                    printf("标记物体后的图像: %s\n", outFile);
                } else {
                    printf("识别失败！\n");
                }
            }
            printf("按任意键继续...\n");
            fflush(stdin);
            getchar();
        }
        else if (choice == 5) {
            OPENFILENAME ofn1, ofn2;
            char szFile1[260] = {0};
            char szFile2[260] = {0};
            char outFile[260] = {0};
            int diffThreshold;

            // 选择第一个图像文件
            ZeroMemory(&ofn1, sizeof(ofn1));
            ofn1.lStructSize = sizeof(OPENFILENAME);
            ofn1.hwndOwner = NULL;
            ofn1.lpstrFile = szFile1;
            ofn1.nMaxFile = sizeof(szFile1);
            ofn1.lpstrFilter = "BMP Files (*.bmp)\0*.bmp\0All Files (*.*)\0*.*\0";
            ofn1.nFilterIndex = 1;
            ofn1.lpstrFileTitle = NULL;
            ofn1.nMaxFileTitle = 0;
            ofn1.lpstrInitialDir = NULL;
            ofn1.lpstrTitle = "选择第一个二值图BMP文件";
            ofn1.Flags = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST | OFN_HIDEREADONLY;

            if (!GetOpenFileName(&ofn1)) {
                DWORD error = CommDlgExtendedError();
                if (error) {
                    printf("打开文件对话框失败，错误代码: %lu\n", error);
                } else {
                    printf("用户取消了选择\n");
                }
            } else {
                // 选择第二个图像文件
                ZeroMemory(&ofn2, sizeof(ofn2));
                ofn2.lStructSize = sizeof(OPENFILENAME);
                ofn2.hwndOwner = NULL;
                ofn2.lpstrFile = szFile2;
                ofn2.nMaxFile = sizeof(szFile2);
                ofn2.lpstrFilter = "BMP Files (*.bmp)\0*.bmp\0All Files (*.*)\0*.*\0";
                ofn2.nFilterIndex = 1;
                ofn2.lpstrFileTitle = NULL;
                ofn2.nMaxFileTitle = 0;
                ofn2.lpstrInitialDir = NULL;
                ofn2.lpstrTitle = "选择第二个二值图BMP文件";
                ofn2.Flags = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST | OFN_HIDEREADONLY;

                if (!GetOpenFileName(&ofn2)) {
                    DWORD error = CommDlgExtendedError();
                    if (error) {
                        printf("打开文件对话框失败，错误代码: %lu\n", error);
                    } else {
                        printf("用户取消了选择\n");
                    }
                } else {
                    // 创建输出文件名
                    strncpy(outFile, szFile1, sizeof(outFile) - 10);
                    outFile[sizeof(outFile) - 10] = '\0';
                    strcat(outFile, "_diff.bmp");

                    // 获取差异阈值
                    printf("请输入差异阈值(百分比，1-100): ");
                    fflush(stdin);
                    if (scanf("%d", &diffThreshold) != 1 || diffThreshold < 1 || diffThreshold > 100) {
                        printf("无效的阈值，使用默认值5%%\n");
                        diffThreshold = 5;
                    }

                    printf("开始比较图像...\n");
                    if (CompareBinaryImages(szFile1, szFile2, outFile, diffThreshold)) {
                        printf("比较完成！\n");
                        printf("差异图像: %s\n", outFile);
                    } else {
                        printf("比较失败！\n");
                    }
                }
            }
            printf("按任意键继续...\n");
            fflush(stdin);
            getchar();
        }
        else if (choice == 0) {
            printf("程序退出\n");
            printf("按任意键关闭...\n");
            fflush(stdin);
            getchar();
            break;
        }
        else {
            printf("无效选项！\n");
            printf("按任意键继续...\n");
            fflush(stdin);
            getchar();
        }
    }
    return 0;
}