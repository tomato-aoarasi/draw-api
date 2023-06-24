/*
 * @File	  : draw_tool.hpp
 * @Coding	  : utf-8
 * @Author    : Bing
 * @Time      : 2023/04/22 14:37
 * @Introduce : 绘制相关的工具
*/


#pragma once
//#define DEBUG  

#include <stdexcept>
#include <memory>
#include <string_view>
#include <opencv2/opencv.hpp>
#include <opencv2/core.hpp>  
#include <opencv2/highgui.hpp>  
#include <opencv2/imgproc.hpp> 
#include <opencv2/imgcodecs.hpp>
#include <opencv2/freetype.hpp>
#include <ft2build.h>
#include <cmath>
#include <numbers>
#include <qrencode.h>

#ifndef DRAW_TOOL
#define DRAW_TOOL  

using namespace cv;

#include FT_FREETYPE_H

// O3优化
#pragma GCC optimize(3)
#pragma G++ optimize(3)

class DrawTool final{
public:
    /// <summary>
    /// 绘制旋转后的图片
    /// </summary>
    /// <param name="img">需要旋转的图片</param>
    /// <param name="angle"></param>
    /// <param name="flip">-1为180°旋转,0为上下翻转,1为水平翻转</param>
    /// <returns>旋转完毕后的图片</returns>
    inline static cv::Mat drawRotate(cv::Mat img, double angle, int flip = 2) {
        // 计算旋转矩阵
        cv::Point2f center(img.cols / 2.0, img.rows / 2.0);
        cv::Mat rot{ cv::getRotationMatrix2D(center, angle, 1.0) };

        // 确定旋转后的图像大小
        cv::Rect bbox{ cv::RotatedRect(center, img.size(), angle).boundingRect() };
        rot.at<double>(0, 2) += bbox.width / 2.0 - center.x;
        rot.at<double>(1, 2) += bbox.height / 2.0 - center.y;

        cv::Mat rotated{ cv::Mat::zeros(bbox.size(), CV_8UC4) };
        cv::warpAffine(img, rotated, rot, bbox.size(), cv::INTER_LINEAR, cv::BORDER_CONSTANT, cv::Scalar(0, 0, 0, 0));

        if (flip >= -1 && flip <= 1)
            cv::flip(rotated, rotated, flip);

        rot.release();
        img.release();
        return std::move(rotated);
    }

    inline static cv::Mat DrawQRcode(const std::string& url,
        bool reserver = false, 
        int mul_size_x = 10, int mul_size_y = 10,
        int bound_size_x = 30, int bound_size_y = 30, bool u8c1 = true
    ) {
        using namespace cv;
        QRcode* code = QRcode_encodeString(url.c_str(), 0, QR_ECLEVEL_H, QR_MODE_8, 1);
        if (code == nullptr) {
            QRcode_free(code);
            throw std::logic_error("code is NULL");
        }

        cv::Mat img = Mat(code->width, code->width, CV_8U);

        for (int i = 0; i < code->width; ++i) {
            for (int j = 0; j < code->width; ++j)
            {
                img.at<uchar>(i, j) = (code->data[i * code->width + j] & 0x01) == 0x01 ? 0 : 255;
            }
        }
        resize(img, img, Size(img.rows * mul_size_x, img.cols * mul_size_y), 0, 0, INTER_NEAREST);

        cv::Mat result{ Mat::zeros(img.rows + bound_size_x, img.cols + bound_size_y, CV_8U) };
        //白底
        result = 255 - result;
        //建立roi
        Rect roi_rect = Rect((result.rows - img.rows) / 2, (result.cols - img.rows) / 2, img.cols, img.rows);
        //roi关联到目标图像，并把源图像复制到指定roi
        img.copyTo(result(roi_rect));
        img.release();
        if (reserver)cv::bitwise_not(result, result);
        if (u8c1)cvtColor(result, result, CV_8UC1);
        QRcode_free(code);
        return std::move(result);
    }


    // 创建一个全单色的特定尺寸的Mat，作为处理后的图片
    inline static cv::Mat createPureMat(int x, int y, const int alpha = 255, const int B = 0, const int G = 0, const int R = 0) {
        Mat processed = Mat::zeros(x, y, CV_8UC4);
        //y 
        for (int r = 0; r < y; r++) {
            // x
            for (int c = 0; c < x; c++) {
                processed.at<Vec4b>(r, c)[0] = B; // B通道
                processed.at<Vec4b>(r, c)[1] = G; // G通道
                processed.at<Vec4b>(r, c)[2] = R; // R通道
                processed.at<Vec4b>(r, c)[3] = alpha; // alpha通道
            }
        }
        return std::move(processed);
    }

    // 创建一个全单色的相同尺寸的Mat，作为处理后的图片
    inline static cv::Mat createPureMat(cv::Mat img, const int alpha = 255, const int B = 0, const int G = 0, const int R = 0) {
        Mat processed = Mat::zeros(img.rows, img.cols, CV_8UC4);

        for (int r = 0; r < img.rows; r++) {
            for (int c = 0; c < img.cols; c++) {
                // 获取当前像素点的RGBA通道值
                Vec4b pixel = img.at<Vec4b>(r, c);
                // 如果alpha通道值不为0，将该像素点设置为黑色
                if (pixel[3] != 0) {
                    processed.at<Vec4b>(r, c)[0] = B; // B通道
                    processed.at<Vec4b>(r, c)[1] = G; // G通道
                    processed.at<Vec4b>(r, c)[2] = R; // R通道
                    processed.at<Vec4b>(r, c)[3] = alpha; // alpha通道
                }
            }
        }
        img.release();
        return std::move(processed);
    }

    // https://www.desmos.com/calculator/kqtxu2xc0j
    inline static void drawParallelogram(cv::Mat img, const cv::Rect& rect, float alpha = 0.0f, float beta = 0.0f, const bool flip = false) {
        //cv::Rect rect(230, 100, img.size().width, img.size().height);
        alpha = convert_angle_to_radians<float>(alpha);
        beta = convert_angle_to_radians<float>(beta);
        float
            x0{ static_cast<float>(rect.x) },
            y0{ static_cast<float>(rect.y) },
            h{ static_cast<float>(rect.height) },
            w{ static_cast<float>(rect.width) },
            x1{ h / tan(alpha) },
            x2{ (-tan(-beta) * x1 + h + tan(alpha) * w - tan(alpha) * x1) / (tan(alpha) - tan(-beta)) },
            y2{ tan(-beta) * (x2 - x1) + h };

#ifdef DEBUG
        std::cout << "Debug:\n"
            << "x0:" << x0 << "\n"
            << "y0:" << y0 << "\n"
            << "x1:" << x1 << "\n"
            << "x2:" << x2 << "\n"
            << "y2:" << y2 << "\n"
            << "h:" << h << "\n"
            << "w:" << w << "\n"

            << std::endl;
#endif // DEBUG

        // 创建一个新的掩码（mask），并将其全部填充为不透明
        cv::Mat mask(img.size(), CV_8UC1, cv::Scalar(255));

        // 创建一组点，将其设置为矩形的四个顶点
        std::vector<cv::Point> points;

        // 是否翻转
        if (!flip) {
            points.push_back(cv::Point(x0, h - y0));
            points.push_back(cv::Point(w - x1, y2));
            points.push_back(cv::Point(w - x0, y0));
            points.push_back(cv::Point(x1, h - y2));
        }
        else
        {
            points.push_back(cv::Point(x1 + x0, h - y0));
            points.push_back(cv::Point(x2, y2));
            points.push_back(cv::Point(w - x1 - x0, y0));
            points.push_back(cv::Point(w - x2, h - y2));
        }

        // 创建一个新的掩码，并将其填充为完全透明
        cv::Mat polyMask(img.size(), CV_8UC1, cv::Scalar(0));

        // 在polyMask上绘制我们想要的多边形
        cv::fillConvexPoly(polyMask, points, cv::Scalar(255));

        // 将mask与polyMask相乘，并将结果存储在mask中
        mask = mask.mul(polyMask);

        // 将被切掉的部分的不透明度设置为0
        cv::Mat channels1[4];
        cv::split(img, channels1);
        channels1[3] = mask.clone();
        cv::merge(channels1, 4, img);

        mask.release();
        polyMask.release();
        channels1[0].release();
        channels1[1].release();
        channels1[2].release();
        channels1[3].release();
        img.release();
    }

    /// <summary>
    /// 将一种图片粘贴到另一张图片上
    /// </summary>
    /// <param name="img2">需要粘贴的图片</param>
    /// <param name="img1">背景图</param>
    /// <param name="left">向→位移多少像素</param>
    /// <param name="top">向↓位移多少像素</param>
    /// <param name="resize_x">缩放比例x</param>
    /// <param name="resize_y">缩放比例y</param>
    /// <param name="interpolation">缩放锯齿</param>
    /// <param name="right">选填</param>
    /// <param name="bottom">选填</param>
    inline static void transparentPaste(
        cv::Mat img2, cv::Mat img1, const int left = 0, const int top = 0,
        const float resize_x = 1.0f, const float resize_y = 1.0f, InterpolationFlags interpolation  = InterpolationFlags::INTER_LINEAR,
        const int right = 0, const int bottom = 0) {
        resize(img2, img2, cv::Size(), resize_x, resize_y, interpolation);
        int ini_x{ img2.size().width }, ini_y{ img2.size().height };
        copyMakeBorder(img2, img2, top, bottom, left, right, cv::BORDER_CONSTANT, cv::Scalar(0, 0, 0, 0));
        cv::Rect roi(0, 0, img2.cols, img2.rows);
        // 限制ROI区域在img1的边界内
        roi &= cv::Rect(0, 0, img2.cols, img2.rows);
        // 将img2复制到img1的指定坐标处
        img2 = img2(roi);

        cv::Mat mat(img1.rows, img1.cols, CV_8UC4);//#define CV_8UC4 CV_MAKETYPE(CV_8U,4)可以创建-----8位无符号的四通道---带透明色的RGB图像 
        
        // 先检验确保不会发生core dumped
        if (img2.cols > img1.cols || img2.rows > img1.rows || left < 0 || top < 0){
            mat.release();
            img2.release();
            img1.release();
            throw std::runtime_error("img2.cols > img1.cols || img2.rows > img1.rows");
        }
        //  i 为 高
        for (int i = top; i < img2.rows; i++) {
            // j 为 宽
            for (int j = left; j < img2.cols; j++) {
                //Mat::at()取值或改变某点的像素值比较耗时，可以采用Mat的模板子类Mat_<T>
                //Mat类中的at方法作用：用于获取图像矩阵某点的值或改变某点的值。
                double temp = img2.at<cv::Vec4b>(i, j)[3] / 255.0;
                mat.at<cv::Vec4b>(i, j)[0] = (1 - temp) * img1.at<cv::Vec4b>(i, j)[0] + temp * img2.at<cv::Vec4b>(i, j)[0];
                mat.at<cv::Vec4b>(i, j)[1] = (1 - temp) * img1.at<cv::Vec4b>(i, j)[1] + temp * img2.at<cv::Vec4b>(i, j)[1];
                mat.at<cv::Vec4b>(i, j)[2] = (1 - temp) * img1.at<cv::Vec4b>(i, j)[2] + temp * img2.at<cv::Vec4b>(i, j)[2];
                //mat.at<cv::Vec4b>(i, j)[3] = (1 - temp) * img1.at<cv::Vec4b>(i, j)[3] + temp * img2.at<cv::Vec4b>(i, j)[3];
            }
        }
        roi = cv::Rect(left, top, ini_x, ini_y);

        // 裁剪新Mat对象
        mat(roi).copyTo(mat);
        roi = cv::Rect(left, top, mat.cols, mat.rows);
        // 限制ROI区域在img1的边界内
        roi &= cv::Rect(0, 0, img1.cols, img1.rows);

        mat(cv::Rect(cv::Point(0, 0), roi.size())).copyTo(img1(roi));
        mat.release();
        img2.release();
        img1.release();
    }

    // img2复制到img1的指定坐标
    inline static void pasteMat(
        cv::Mat img2, cv::Mat img1, const int x = 0, const int y = 0,
        const float resize_fx = 1.0f, const float resize_fy = 1.0f
    ) {
        cv::resize(img2, img2, cv::Size(), resize_fx, resize_fy);

        // 计算ROI区域
        cv::Rect roi(x, y, img2.cols, img2.rows);

        // 限制ROI区域在img1的边界内
        roi &= cv::Rect(0, 0, img1.cols, img1.rows);

        // 将img2复制到img1的指定坐标处
        cv::Mat roi_img1 = img1(roi);
        cv::Mat roi_img2 = img2(cv::Rect(cv::Point(0, 0), roi.size()));
        roi_img2.copyTo(roi_img1);
        roi_img2.release();
        roi_img1.release();
        img1.release();
        img2.release();
    };

private:
    // 角度转弧度
    template <typename T>
    inline static T convert_angle_to_radians(T angle)
    {
        return angle * (T(1) / 180.0) * std::numbers::pi;
    }

};

#endif