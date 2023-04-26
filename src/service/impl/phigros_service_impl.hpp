/*
 * @File	  : phigros_service_impl.hpp
 * @Coding	  : utf-8
 * @Author    : Bing
 * @Time      : 2023/04/23 21:02
 * @Introduce : 肥鸽肉丝
*/

#pragma once

#ifndef PHIGROS_SERVICE_IMPL_HPP
#define PHIGROS_SERVICE_IMPL_HPP  

#include <string>
#include <cmath>
#include <numbers>
#include <stdexcept>
#include <chrono>
#include <future>
#include <httplib.h>
#include <bits/types.h>
#include "configuration/config.hpp"
#include "common/exception/self_exception.hpp"
#include "common/utils/other_util.hpp"
#include "service/phigros_service.hpp"
#include <opencv2/opencv.hpp>
#include <opencv2/core.hpp>  
#include <opencv2/highgui.hpp>  
#include <opencv2/imgproc.hpp> 
#include <opencv2/imgcodecs.hpp>
#include <opencv2/freetype.hpp>
#include <ft2build.h>
#include <common/utils/draw_tool.hpp>

#include FT_FREETYPE_H
#define CPPHTTPLIB_OPENSSL_SUPPORT

using namespace std::chrono_literals;

// O3优化
#pragma GCC optimize(3)
#pragma G++ optimize(3)

class PhigrosServiceImpl : public PhigrosService {
public:
    ~PhigrosServiceImpl() = default;
    inline cv::Mat drawSongInfomation(int song_id, bool isQRCode, std::string_view QRCodeContent) {
        //设置超过200ms后API请求超时
        constexpr std::chrono::milliseconds timeout{ 2000ms };

        Json data;

        //获取API数据到data
        OtherUtil::asyncGetAPI(data, timeout, Global::BingAPI, Global::PhiUri + "?id="s + std::to_string(song_id));

        std::string illustrationPath = { Global::PhiResourcePath + data[0]["song_illustration_url"].get<std::string>() };
        // 读取素材
        cv::Mat img{ cv::imread(illustrationPath, cv::IMREAD_UNCHANGED)},
            phigros{ cv::imread("draw/phi/Phigros.png", cv::IMREAD_UNCHANGED) },
            shadow{ cv::imread("draw/phi/Shadow.png", cv::IMREAD_UNCHANGED) },
            shadow_corner{ cv::imread("draw/phi/ShadowCorner.png", cv::IMREAD_UNCHANGED) },
            EZ{ cv::imread("draw/phi/diff/EZ.png", cv::IMREAD_UNCHANGED) },
            HD{ cv::imread("draw/phi/diff/HD.png", cv::IMREAD_UNCHANGED) },
            IN{ cv::imread("draw/phi/diff/IN.png", cv::IMREAD_UNCHANGED) },
            AT{ cv::imread("draw/phi/diff/AT.png", cv::IMREAD_UNCHANGED) },
            LG{ cv::imread("draw/phi/diff/LG.png", cv::IMREAD_UNCHANGED) },
            SP{ cv::imread("draw/phi/diff/SP.png", cv::IMREAD_UNCHANGED) },
            EZ_Sign{ cv::imread("draw/phi/diff/DiifFont/EZ.png", cv::IMREAD_UNCHANGED) },
            HD_Sign{ cv::imread("draw/phi/diff/DiifFont/HD.png", cv::IMREAD_UNCHANGED) },
            IN_Sign{ cv::imread("draw/phi/diff/DiifFont/IN.png", cv::IMREAD_UNCHANGED) },
            AT_Sign{ cv::imread("draw/phi/diff/DiifFont/AT.png", cv::IMREAD_UNCHANGED) },
            SP_Sign{ cv::imread("draw/phi/diff/DiifFont/LG.png", cv::IMREAD_UNCHANGED) },
            LG_Sign{ cv::imread("draw/phi/diff/DiifFont/SP.png", cv::IMREAD_UNCHANGED) };

        // 设置高斯模糊
        cv::Mat blurMat;
        cv::GaussianBlur(img, blurMat, cv::Size(0, 0), 50); 
        cv::resize(blurMat, blurMat, cv::Size(), 2048.0f / blurMat.cols, 1080.0f / blurMat.rows);
        
        // 创建一个与img大小相同的矩阵
        cv::Mat blackCover{ cv::Mat::ones(cv::Size{img.cols, img.rows}, CV_8UC4) }; 
        // 设置黑色透明图层,alpha通道为100
        blackCover = cv::Scalar{ 0, 0, 0, 100 }; 

        cv::Mat dstImg;
        cv::addWeighted(blackCover, 0.6, blurMat, 1 - 0.6, 0.0, dstImg); // 叠加黑色不透明图层
        // 绘制平行四边形矩形
        cv::Rect rect{ 0, 0, img.size().width, img.size().height };
        DrawTool::drawParallelogram(img, rect, 79);


        // 创建一个全黑的相同尺寸的Mat，作为处理后的图片 
        cv::Mat blackTransparentMask{ DrawTool::createPureMat(img, 100) };

        DrawTool::transparentPaste(blackTransparentMask, dstImg, 159, 117 + 4, 0.555f, 0.555f);
        DrawTool::transparentPaste(img, dstImg, 159, 117, 0.55f, 0.55f);
        DrawTool::transparentPaste(shadow, dstImg, 159, 117, 0.55f, 0.55f);

        // 裁切图像
        //左
        // 获取裁切后的区域
        cv::Rect roi{ 1830, 0, blackTransparentMask.cols - 1830, blackTransparentMask.rows };
        cv::Mat leftBlackTransparentMask{ blackTransparentMask(roi) };
        //右
        roi = cv::Rect{ 0, 0, blackTransparentMask.cols - 1830, blackTransparentMask.rows };
        cv::Mat rightBlackTransparentMask{ blackTransparentMask(roi) };

        DrawTool::transparentPaste(leftBlackTransparentMask, dstImg, 0, 0, 1.0f, 1.0f);
        DrawTool::transparentPaste(rightBlackTransparentMask, dstImg, 1830, 0, 1.0f, 1.0f);

        // 二维码追加
        if (isQRCode) {
            cv::Mat QRCode{ DrawTool::DrawQRcode(QRCodeContent.data(),true,2,2,6,6)};
            DrawTool::transparentPaste(QRCode, dstImg,0,0,0.8,0.8);
            DrawTool::transparentPaste(shadow_corner, dstImg, 0, 0, 0.2f, 0.2f);
        }

        // 添加难度框的阴影
        cv::Mat diffBoxShadow{ DrawTool::createPureMat(EZ, 15) };

        constexpr int sign_offset_x{ 43 }, sign_offset_y{ 27 };

        constexpr int X_OFFSETINIT{ 0 }, Y_OFFSETINIT{ 0 }, HC{ -39 }, VC{ 228 };
        int h{ 1296 + X_OFFSETINIT }, v{ 112 + Y_OFFSETINIT }, offset_x{ 195 }, offset_y{ 108 };

        DrawTool::transparentPaste(diffBoxShadow, dstImg, h + 1, v, 1.005f, 1.015f);
        DrawTool::transparentPaste(EZ, dstImg, h - 2, v - 5, 1.0f, 1.0f);
        DrawTool::transparentPaste(EZ_Sign, dstImg, h + sign_offset_x + 1, v + sign_offset_y, 0.4f, 0.4f);

        h += HC; v += VC;

        DrawTool::transparentPaste(diffBoxShadow, dstImg, h + 1, v, 1.005f, 1.015f);
        DrawTool::transparentPaste(HD, dstImg, h - 2, v - 5, 1.0f, 1.0f);
        DrawTool::transparentPaste(HD_Sign, dstImg, h + sign_offset_x + 3, v + sign_offset_y, 0.4f, 0.4f);

        h += HC; v += VC;

        DrawTool::transparentPaste(diffBoxShadow, dstImg, h + 1, v, 1.005f, 1.015f);
        DrawTool::transparentPaste(IN, dstImg, h - 2, v - 5, 1.0f, 1.0f);
        DrawTool::transparentPaste(IN_Sign, dstImg, h + sign_offset_x, v + sign_offset_y, 0.4f, 0.4f);
        DrawTool::transparentPaste(phigros, dstImg, 1186, 800, 1.0f, 1.0f);

        // 文字处理添加
        cv::Mat result{ addFont(dstImg, std::move(data), X_OFFSETINIT, Y_OFFSETINIT) };

        return result;

	};
private:
    inline cv::Mat addFont(cv::Mat& dstImg, Json&& data, const int X_OFFSETINIT, const int Y_OFFSETINIT) {
        using namespace cv;
        Mat result(dstImg.rows, dstImg.cols, CV_8UC3);
        int from_to[] = { 0,0, 1,1, 2,2 };
        mixChannels(&dstImg, 1, &result, 1, from_to, 3);
        int font_size = 32;
        cv::Ptr<freetype::FreeType2> freetype2;

        freetype2 = cv::freetype::createFreeType2();
        //freetype2->loadFontData("draw/phi/font/SOURCEHANSANSCN_MEDIUM.OTF", 0);
        freetype2->loadFontData("draw/phi/font/SourceHanSans_SairaHybridRegularHot.ttf", 0);

        //freetype(freetype2);

        // 添加中文文字
        freetype2->putText(result, "Chapter:  Single Collection", Point(285, 75) + cv::Point(4, 4), 42, Scalar(80, 80, 80, 40), -1, cv::LINE_AA, true);
        freetype2->putText(result, "Chapter:  Single Collection", Point(285, 75), 42, Scalar(189, 189, 189), -1, cv::LINE_AA, true);

        constexpr int N = 70, N1 = 12;
        int n = N;
        int n1 = N1;
        freetype2->putText(result, "Artist:  アリスシャッハと魔法の楽団", Point(152, 788) + cv::Point(5, 3), 36, Scalar(5, 5, 5), -1, cv::LINE_AA, true);
        freetype2->putText(result, "Artist:  アリスシャッハと魔法の楽団", Point(152, 788), 36, Scalar(232, 232, 232), -1, cv::LINE_AA, true);
        // 偏移 6
        freetype2->putText(result, "Illustration:  昔璃☆暴雨 ~ ♫危難聖帝♫", Point(152 - n1, 788 + n) + cv::Point(5, 3), 36, Scalar(5, 5, 5), -1, cv::LINE_AA, true);
        freetype2->putText(result, "Illustration:  昔璃☆暴雨 ~ ♫危難聖帝♫", Point(152 - n1, 788 + n), 36, Scalar(232, 232, 232), -1, cv::LINE_AA, true);
        // 13
        // 60
        n += N;
        n1 += N1;
        freetype2->putText(result, "Duration:  2:16", Point(152 - n1, 788 + n) + cv::Point(5, 3), 36, Scalar(5, 5, 5), -1, cv::LINE_AA, true);
        freetype2->putText(result, "Duration:  2:16", Point(152 - n1, 788 + n), 36, Scalar(232, 232, 232), -1, cv::LINE_AA, true);
        n += N;
        n1 += N1;
        freetype2->putText(result, "BPM:  132", Point(152 - n1, 788 + n) + cv::Point(5, 3), 36, Scalar(5, 5, 5), -1, cv::LINE_AA, true);
        freetype2->putText(result, "BPM:  132", Point(152 - n1, 788 + n), 36, Scalar(232, 232, 232), -1, cv::LINE_AA, true);

        // 214
        // 1363
        freetype2->putText(result, "BPM:  132", Point(152 - n1, 788 + n) + cv::Point(5, 3), 36, Scalar(5, 5, 5), -1, cv::LINE_AA, true);
        freetype2->putText(result, "BPM:  132", Point(152 - n1, 788 + n), 36, Scalar(232, 232, 232), -1, cv::LINE_AA, true);


        // 切换字体
        // 下面简单喵两句
        freetype2->putText(result, "Colorful Days♪", Point(167, 704) + cv::Point(3, 2), 36, Scalar(20, 20, 20), -1, cv::LINE_AA, true);
        freetype2->putText(result, "Colorful Days♪", Point(167, 704), 36, Scalar(255, 255, 255), -1, cv::LINE_AA, true);



        //freetype2->loadFontData("draw/phi/font/AdobeStdR.otf", 0);
        constexpr int HC{ -39 }, VC{ 228 };
        int h{ 1296 + X_OFFSETINIT }, v{ 112 + Y_OFFSETINIT }, offset_x{ 195 }, offset_y{ 108 };
        freetype2->putText(result, "6.0  / 275", Point(h + offset_x, v + offset_y) + cv::Point(3, 2), 72, Scalar(5, 5, 5), -1, cv::LINE_AA, true);
        freetype2->putText(result, "6.0  / 275", Point(h + offset_x, v + offset_y), 72, Scalar(215, 215, 215), -1, cv::LINE_AA, true);

        freetype2->putText(result, "Chart:  .---- ----. ---.. / -. . .-. (百九十八 / NerSAN)", Point(h + 7, v + offset_y + 70) + cv::Point(2, 1), 24, Scalar(0, 0, 0), -1, cv::LINE_AA, true);
        freetype2->putText(result, "Chart:  .---- ----. ---.. / -. . .-. (百九十八 / NerSAN)", Point(h + 7, v + offset_y + 70), 24, Scalar(200, 200, 200), -1, cv::LINE_AA, true);

        h += HC; v += VC;
        freetype2->putText(result, "11.1 / 595", Point(h + offset_x, v + offset_y) + cv::Point(3, 2), 72, Scalar(5, 5, 5), -1, cv::LINE_AA, true);
        freetype2->putText(result, "11.1 / 595", Point(h + offset_x, v + offset_y), 72, Scalar(215, 215, 215), -1, cv::LINE_AA, true);

        freetype2->putText(result, "Chart:  Jαckψ feat. Βαρβαροςαντρας", Point(h + 7, v + offset_y + 70) + cv::Point(2, 1), 24, Scalar(0, 0, 0), -1, cv::LINE_AA, true);
        freetype2->putText(result, "Chart:  Jαckψ feat. Βαρβαροςαντρας", Point(h + 7, v + offset_y + 70), 24, Scalar(200, 200, 200), -1, cv::LINE_AA, true);

        h += HC; v += VC;
        freetype2->putText(result, "14.7 / 7328", Point(h + offset_x, v + offset_y) + cv::Point(3, 2), 72, Scalar(5, 5, 5), -1, cv::LINE_AA, true);
        freetype2->putText(result, "14.7 / 7328", Point(h + offset_x, v + offset_y), 72, Scalar(215, 215, 215), -1, cv::LINE_AA, true);

        freetype2->putText(result, "Chart:  Two skunks (Ctymax and 晨) in Africa shouting WAHHHHHH!", Point(h + 7, v + offset_y + 70) + cv::Point(2, 1), 24, Scalar(0, 0, 0), -1, cv::LINE_AA, true);
        freetype2->putText(result, "Chart:  Two skunks (Ctymax and 晨) in Africa shouting WAHHHHHH!", Point(h + 7, v + offset_y + 70), 24, Scalar(200, 200, 200), -1, cv::LINE_AA, true);


        // 添加日文文字
        //freetype2->putText(result, "こんにちは", Point(10, 100), font_size, font_color, -1, cv::LINE_AA, true);
        freetype2.reset();


        return result;
    }
};


#endif // !PHIGROS_SERVICE_HPP