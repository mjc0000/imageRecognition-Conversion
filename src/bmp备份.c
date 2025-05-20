#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct {
    unsigned char blue;
    unsigned char green;
    unsigned char red;
} RGB;

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

    if (infoHeader.biBitCount != 1 && infoHeader.biBitCount != 4 && 
        infoHeader.biBitCount != 8 && infoHeader.biBitCount != 24 && 
        infoHeader.biBitCount != 32) {
        fclose(inputFile);
        fclose(outputFile);
        printf("不支持的位深度！\n");
        return FALSE;
    }

    // 写入文件头和信息头
    fwrite(&fileHeader, sizeof(BITMAPFILEHEADER), 1, outputFile);
    fwrite(&infoHeader, sizeof(BITMAPINFOHEADER), 1, outputFile);

    int width = infoHeader.biWidth;
    int height = abs(infoHeader.biHeight);
    int paletteSize = (infoHeader.biBitCount <= 8) ? (1 << infoHeader.biBitCount) : 0;

    // 如果有调色板，复制并处理调色板
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

    // 读取整个图像数据
    for (int y = 0; y < height; y++) {
        fread(rowBuffer + y * rowSize, 1, rowSize, inputFile);
    }

    // 二值化处理
    // 阈值范围是0-100，表示二值化的百分比
    // 0 表示全白，100 表示全黑
    int binaryThreshold = 255 * threshold / 100;

    if (infoHeader.biBitCount == 24) {
        for (int y = 0; y < height; y++) {
            for (int x = 0; x < width; x++) {
                RGB *pixel = (RGB *)(rowBuffer + y * rowSize + x * 3);
                unsigned char gray = rgbToGray(pixel->red, pixel->green, pixel->blue);
                // 二值化
                unsigned char binary = (gray <= binaryThreshold) ? 0 : 255;
                pixel->red = binary;
                pixel->green = binary;
                pixel->blue = binary;
            }
        }
    }
    else if (infoHeader.biBitCount == 32) {
        for (int y = 0; y < height; y++) {
            for (int x = 0; x < width; x++) {
                unsigned char *pixel = rowBuffer + y * rowSize + x * 4;
                unsigned char gray = rgbToGray(pixel[2], pixel[1], pixel[0]);
                // 二值化
                unsigned char binary = (gray <= binaryThreshold) ? 0 : 255;
                pixel[0] = binary; // B
                pixel[1] = binary; // G
                pixel[2] = binary; // R
            }
        }
    }

    // 写入处理后的图像数据
    for (int y = 0; y < height; y++) {
        fwrite(rowBuffer + y * rowSize, 1, rowSize, outputFile);
    }

    free(rowBuffer);
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
            char binaryFile[260] = {0};
            int threshold = 50; // 设置默认阈值为50

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
                // 获取二值化阈值
                do {
                    system("cls");
                    printf("请输入二值化阈值 (0-100)%%: \n");
                    printf("0 - 输出全白图像\n");
                    printf("100 - 输出全黑图像\n");
                    printf("默认值: 50 (直接按回车使用默认值)\n");
                    printf("阈值: ");
                    fflush(stdin);
                    
                    // 读取整行输入
                    char input[20] = {0};
                    fgets(input, sizeof(input), stdin);
                    
                    // 去除输入中的换行符
                    size_t len = strlen(input);
                    if (len > 0 && input[len-1] == '\n') {
                        input[len-1] = '\0';
                    }
                    
                    // 检查是否为空输入（直接按回车）
                    if (input[0] == '\0') {
                        threshold = 50;
                        printf("使用默认阈值: 50%%\n");
                    } else {
                        // 尝试将输入解析为整数
                        char* endptr;
                        long value = strtol(input, &endptr, 10);
                        
                        // 检查转换是否成功且在有效范围内
                        if (*endptr != '\0' || value < 0 || value > 100) {
                            printf("输入无效，阈值必须在0-100之间！\n");
                            printf("按任意键重新输入...\n");
                            fflush(stdin);
                            getchar();
                            continue;
                        }
                        
                        threshold = (int)value;
                    }
                    break;
                } while (1);

                strncpy(binaryFile, szFile, sizeof(binaryFile) - 12);
                binaryFile[sizeof(binaryFile) - 12] = '\0';
                char thresholdStr[5];
                snprintf(thresholdStr, sizeof(thresholdStr), "%d", threshold);
                strcat(binaryFile, "_binary_");
                strcat(binaryFile, thresholdStr);
                strcat(binaryFile, ".bmp");

                if (ConvertToBinary(szFile, binaryFile, threshold)) {
                    printf("转换成功！\n");
                    printf("二值图像: %s\n", binaryFile);
                } else {
                    printf("转换失败！\n");
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