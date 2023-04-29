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
#include <vector>
#include <numbers>
#include <stdexcept>
#include <chrono>
#include <future>
#include <unordered_map>
#include <utility>
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
private:
    cv::Mat
        phigros{ cv::imread("draw/phi/Phigros.png", cv::IMREAD_UNCHANGED) },
        shadow{ cv::imread("draw/phi/Shadow.png", cv::IMREAD_UNCHANGED) },
        shadow_corner{ cv::imread("draw/phi/ShadowCorner.png", cv::IMREAD_UNCHANGED) };
public:
    ~PhigrosServiceImpl() {
        phigros.release();
        shadow.release();
        shadow_corner.release();
    };
    inline cv::Mat drawSongInfomation(int song_id, bool isQRCode, std::string_view QRCodeContent) {
        //设置超过200ms后API请求超时
        constexpr std::chrono::milliseconds timeout{ 2000ms };
        constexpr const size_t size_w{ 2048 },size_h{1080};

        Json data;

        //获取API数据到data
        OtherUtil::asyncGetAPI(data, timeout, Global::BingAPI, Global::PhiUri + "?id="s + std::to_string(song_id));
        std::exchange(data, data[0]);

        std::string illustrationPath = { Global::PhiResourcePath + data["song_illustration_url"].get<std::string>() };
        

        // 读取素材
        cv::Mat img{ cv::imread(std::move(illustrationPath), cv::IMREAD_UNCHANGED) };
        cv::resize(img, img, cv::Size(size_w, size_h));


        std::vector<cv::Mat> diff;
        std::vector<cv::Mat> diffSign;

        // rating note design
        std::unordered_map<std::string, std::string> diffText;
        std::vector<decltype(diffText)> diffTexts;

        if (!data["design"]["ez"].is_null()) {
            diff.emplace_back(cv::imread("draw/phi/diff/EZ.png", cv::IMREAD_UNCHANGED));
            diffSign.emplace_back(cv::imread("draw/phi/diff/DiifFont/EZ.png", cv::IMREAD_UNCHANGED));
            
            diffText["rating"] = OtherUtil::retainDecimalPlaces(data["rating"]["ez"].get<float>(),1);
            diffText["note"] = std::to_string(data["note"]["ez"].get<int>());
            diffText["design"] = data["design"]["ez"].get<std::string>();

            diffTexts.emplace_back(diffText);
        }
        if (!data["design"]["hd"].is_null()) {
            diff.emplace_back(cv::imread("draw/phi/diff/HD.png", cv::IMREAD_UNCHANGED));
            diffSign.emplace_back(cv::imread("draw/phi/diff/DiifFont/HD.png", cv::IMREAD_UNCHANGED));

            diffText["rating"] = OtherUtil::retainDecimalPlaces(data["rating"]["hd"].get<float>(),1);
            diffText["note"] = std::to_string(data["note"]["hd"].get<int>());
            diffText["design"] = data["design"]["hd"].get<std::string>();

            diffTexts.emplace_back(diffText);
        }
        if (!data["design"]["in"].is_null()) {
            diff.emplace_back(cv::imread("draw/phi/diff/IN.png", cv::IMREAD_UNCHANGED));
            diffSign.emplace_back(cv::imread("draw/phi/diff/DiifFont/IN.png", cv::IMREAD_UNCHANGED));

            diffText["rating"] = OtherUtil::retainDecimalPlaces(data["rating"]["in"].get<float>(),1);
            diffText["note"] = std::to_string(data["note"]["in"].get<int>());
            diffText["design"] = data["design"]["in"].get<std::string>();

            diffTexts.emplace_back(diffText);
        }
        if (!data["design"]["at"].is_null()) {
            diff.emplace_back(cv::imread("draw/phi/diff/AT.png", cv::IMREAD_UNCHANGED));
            diffSign.emplace_back(cv::imread("draw/phi/diff/DiifFont/AT.png", cv::IMREAD_UNCHANGED));

            diffText["rating"] = OtherUtil::retainDecimalPlaces(data["rating"]["at"].get<float>(),1);
            diffText["note"] = std::to_string(data["note"]["at"].get<int>());
            diffText["design"] = data["design"]["at"].get<std::string>();

            diffTexts.emplace_back(diffText);
        }
        if (!data["design"]["lg"].is_null()) {
            diff.emplace_back(cv::imread("draw/phi/diff/LG.png", cv::IMREAD_UNCHANGED));
            diffSign.emplace_back(cv::imread("draw/phi/diff/DiifFont/LG.png", cv::IMREAD_UNCHANGED));

            diffText["rating"] = OtherUtil::retainDecimalPlaces(data["rating"]["lg"].get<float>(),1);
            diffText["note"] = std::to_string(data["note"]["lg"].get<int>());
            diffText["design"] = data["design"]["lg"].get<std::string>();

            diffTexts.emplace_back(diffText);
        }
        if (!data["design"]["sp"].is_null()) {
            diff.emplace_back(cv::imread("draw/phi/diff/SP.png", cv::IMREAD_UNCHANGED));
            diffSign.emplace_back(cv::imread("draw/phi/diff/DiifFont/SP.png", cv::IMREAD_UNCHANGED));

            diffText["rating"] = OtherUtil::retainDecimalPlaces(data["rating"]["sp"].get<float>(),1);
            diffText["note"] = std::to_string(data["note"]["sp"].get<int>());
            diffText["design"] = data["design"]["sp"].get<std::string>();

            diffTexts.emplace_back(diffText);
        }
        size_t diffCount{ diff.size() };

        // 设置高斯模糊
        cv::Mat blurMat(size_w, size_h, CV_8UC4);
        cv::GaussianBlur(img, blurMat, cv::Size(0, 0), 50); 
        cv::resize(blurMat, blurMat, cv::Size(size_w, size_h));
        
        // 创建一个与img大小相同的矩阵
        cv::Mat blackCover{ cv::Mat::ones(cv::Size(img.cols, img.rows), CV_8UC4) }; 
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
            QRCode.release();
        }

        constexpr const int sign_offset_x{ 43 }, sign_offset_y{ 27 };
        constexpr const int X_OFFSETINIT{ 0 }, Y_OFFSETINIT{ 0 };
        int offset_x{ }, offset_y{ }, hc{ -39 }, vc{ 228 };
        if (diffCount == 1) {
            offset_x = -39;
            offset_y = 228;
        }
        else if (diffCount == 2) {
            offset_x = -19;
            offset_y = 57;
            vc = 342;
            hc = -58;
        }
        else if (diffCount == 3) {
            offset_x = 0;
            offset_y = 0;
            vc = 228;
        }
        else if (diffCount == 4) {
            offset_x = 10;
            offset_y = -64;
            vc = 228;
        }
        else if (diffCount == 5) {
            offset_x = 20;
            offset_y = -74;
            vc = 210;
        }
        else {
            throw std::runtime_error("开什么玩笑,完全塞不下好嘛");
        }
        
        int h{ 1296 + offset_x }, v{ 112 + offset_y };

        for (size_t i{ 0 }; i < diffCount; ++i) {
            // 添加难度框的阴影
            cv::Mat diffBoxShadow{ DrawTool::createPureMat(diff.at(i), 15) };
            DrawTool::transparentPaste(diffBoxShadow, dstImg, h + 1, v, 1.005f, 1.015f);
            DrawTool::transparentPaste(diff.at(i), dstImg, h - 2, v - 5, 1.0f, 1.0f);
            DrawTool::transparentPaste(diffSign.at(i), dstImg, h + sign_offset_x + 1, v + sign_offset_y, 0.4f, 0.4f);
            h += hc; v += vc;
            diffBoxShadow.release();
            diff.at(i).release();
            diffSign.at(i).release();
        }
        if (diffCount == 3 || diffCount == 2 || diffCount == 1) {
            DrawTool::transparentPaste(phigros, dstImg, 1240, 800, 1.0f, 1.0f);
        }
        else if (diffCount == 4) {
            DrawTool::transparentPaste(phigros, dstImg, 1280, 920, 0.8f, 0.8f);
        }


        // 文字处理添加
        cv::Mat result(dstImg.rows, dstImg.cols, CV_8UC3);
        //cv::Mat result{ addFont(dstImg, std::move(data), X_OFFSETINIT, Y_OFFSETINIT) };

        int from_to[] = { 0,0, 1,1, 2,2 };
        mixChannels(&dstImg, 1, &result, 1, from_to, 3);
        
        // ===========================================================

        blurMat.release();
        blackCover.release();
        img.release();
        blackTransparentMask.release();
        leftBlackTransparentMask.release();
        rightBlackTransparentMask.release();
        dstImg.release();
        //phigros.release();
        //shadow.release();
        //shadow_corner.release();

        // ===========================================================
        int font_size = 32;

        cv::Ptr<freetype::FreeType2> freetype2;

        freetype2 = cv::freetype::createFreeType2();
        freetype2->loadFontData("draw/phi/font/SourceHanSans_SairaHybridRegularHot.ttf", 0);

        //freetype(freetype2);

        // 添加中文文字
        const std::string
            chapter{ "Chapter:  "s + data["chapter"].get<std::string>() },
            artist{ "Artist:  "s + data["artist"].get<std::string>() },
            illustration{ "Illustration:  "s + data["illustration"].get<std::string>() },
            duration{ "Duration:  "s + data["duration"].get<std::string>() },
            bpm{ "BPM:  "s + data["bpm"].get<std::string>() },
            title{ data["song_name"].get<std::string>() };

        freetype2->putText(result, chapter, Point(285, 75) + cv::Point(4, 4), 42, Scalar(80, 80, 80, 40), -1, cv::LINE_AA, true);
        freetype2->putText(result, chapter, Point(285, 75), 42, Scalar(189, 189, 189), -1, cv::LINE_AA, true);
        constexpr int N = 70, N1 = 12;
        int n = N;
        int n1 = N1;
        freetype2->putText(result, artist, Point(152, 788) + cv::Point(5, 3), 36, Scalar(5, 5, 5), -1, cv::LINE_AA, true);
        freetype2->putText(result, artist, Point(152, 788), 36, Scalar(232, 232, 232), -1, cv::LINE_AA, true);
        // 偏移 6
        freetype2->putText(result, illustration, Point(152 - n1, 788 + n) + cv::Point(5, 3), 36, Scalar(5, 5, 5), -1, cv::LINE_AA, true);
        freetype2->putText(result, illustration, Point(152 - n1, 788 + n), 36, Scalar(232, 232, 232), -1, cv::LINE_AA, true);
        // 13
        // 60
        n += N;
        n1 += N1;
        freetype2->putText(result, duration, Point(152 - n1, 788 + n) + cv::Point(5, 3), 36, Scalar(5, 5, 5), -1, cv::LINE_AA, true);
        freetype2->putText(result, duration, Point(152 - n1, 788 + n), 36, Scalar(232, 232, 232), -1, cv::LINE_AA, true);
        n += N;
        n1 += N1;
        freetype2->putText(result, bpm, Point(152 - n1, 788 + n) + cv::Point(5, 3), 36, Scalar(5, 5, 5), -1, cv::LINE_AA, true);
        freetype2->putText(result, bpm, Point(152 - n1, 788 + n), 36, Scalar(232, 232, 232), -1, cv::LINE_AA, true);

        freetype2->putText(result, title, Point(167, 704) + cv::Point(3, 2), 36, Scalar(20, 20, 20), -1, cv::LINE_AA, true);
        freetype2->putText(result, title, Point(167, 704), 36, Scalar(255, 255, 255), -1, cv::LINE_AA, true);
        {
            constexpr const int X_OFFSETINIT{ 195 }, Y_OFFSETINIT{ 108 };
            constexpr int HC{ 195 }, VC{ 108 };
            int offset_x{ }, offset_y{ }, hc{ -39 }, vc{ 228 },font_offset_x{ 0 };
            if (diffCount == 1) {
                offset_x = -39;
                offset_y = 228;
                font_offset_x = -39;
            }
            else if (diffCount == 2) {
                offset_x = -19;
                offset_y = 57;
                font_offset_x = -20;
                vc = 342;
                hc = -58;
            }
            else if (diffCount == 3) {
                offset_x = 0;
                offset_y = 0;
                vc = 228;
            }
            else if (diffCount == 4) {
                offset_x = 10;
                offset_y = -64;
                vc = 228;
                font_offset_x = 10;
            }
            else if (diffCount == 5) {
                offset_x = 20;
                offset_y = -74;
                vc = 210;
                font_offset_x = 20;
            }
            else {
                throw std::runtime_error("开什么玩笑,完全塞不下好嘛");
            }
            int h{ 1318 + offset_x + X_OFFSETINIT }, v{ 112 + offset_y + Y_OFFSETINIT };
            for (size_t i{ 0 }; i < diffCount; ++i) {
                freetype2->putText(result, diffTexts.at(i).at("rating") + " / " + diffTexts.at(i).at("note"), Point(h, v) + cv::Point(3, 2), 72, Scalar(5, 5, 5), -1, cv::LINE_AA, true);
                freetype2->putText(result, diffTexts.at(i).at("rating") + " / " + diffTexts.at(i).at("note"), Point(h, v), 72, Scalar(215, 215, 215), -1, cv::LINE_AA, true);
                
                freetype2->putText(result, "Chart:  " + diffTexts.at(i).at("design"), Point(h - offset_x - X_OFFSETINIT - 13 + font_offset_x, v + 70) + cv::Point(2, 1), 24, Scalar(0, 0, 0), -1, cv::LINE_AA, true);
                freetype2->putText(result, "Chart:  " + diffTexts.at(i).at("design"), Point(h - offset_x - X_OFFSETINIT - 13 + font_offset_x, v + 70), 24, Scalar(200, 200, 200), -1, cv::LINE_AA, true);
                h += hc; v += vc;
            }
        };
        freetype2.release();


        return std::move(result);

    };
};


#endif // !PHIGROS_SERVICE_HPP