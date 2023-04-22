// PocoWebsite.h: 标准系统包含文件的包含文件
// 或项目特定的包含文件。

#pragma once
#include <iostream>

#include "spdlog/spdlog.h"
#include "spdlog/sinks/daily_file_sink.h"
#include "fmt/format.h"


namespace std {
    using fmt::format;
    using fmt::format_error;
    using fmt::formatter;
}

#include <common/utils/sql_util.hpp>
#include <common/utils/log_system.hpp>
#include <configuration/config.hpp>

#include "crow/middlewares/cors.h"
#include "crow.h"
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
#include <openssl/sha.h>  
#include <hiredis/hiredis.h>
#include <boost/coroutine2/all.hpp>
#include <thread>
#include <bits/types.h>
#include <jwt-cpp/jwt.h>
#include <bcrypt.h>
#include <httplib.h>
#include "test/bing/test_project.hpp"
#include <common/utils/draw_tool.hpp>
#include "service/bottle_service.hpp"
#include "service/impl/bottle_service_impl.hpp"
#include "controller/bottle_controller.hpp"

// O3优化
#pragma GCC optimize(3)

#include FT_FREETYPE_H

//#define CROW_ENABLE_SSL
using namespace std::string_literals;
#define CROW_ENFORCE_WS_SPEC

inline cv::Mat addFont(cv::Mat& dstImg, const int X_OFFSETINIT, const int Y_OFFSETINIT);

//初始化
inline void init(void) {
    Config::initialized();
    LogSystem::initialized();
    SQL_Util::initialized();
}

inline void start(void){
    const std::string
        secret{ Config::getConfig()["server"]["token"]["secret"].as<std::string>() },
        issuer{ Config::getConfig()["server"]["token"]["issuer"].as<std::string>() };
    const unsigned short port{ Config::getConfig()["server"]["port"].as<unsigned short>() };

    crow::App<crow::CORSHandler> app; //define your crow application
    // 日志等级
    crow::logger::setLogLevel(crow::LogLevel::CRITICAL);
    //app.loglevel(crow::LogLevel::Info);

    // 跨域访问
    auto& cors = app.get_middleware<crow::CORSHandler>();
    cors
        .global()
        .headers("origin, x-requested-with, accept, access-control-allow-origin, authorization, content-type")
        .methods("POST"_method, "GET"_method, "PUT"_method, "DELETE"_method, "PATCH"_method, "OPTIONS"_method)
        //.prefix("/cors")
        .origin("*");

    app.bindaddr("0.0.0.0").port(port);


    BottleService* bottle{ new BottleServiceImpl() };
    BottleController bottle_controller(app, secret, issuer, bottle);
    bottle_controller.controller();

    CROW_ROUTE(app, "/testPage/<string>")([](std::string name) { // 
        auto page = crow::mustache::load("index.html"); // 
        crow::mustache::context ctx({ {"person", name} }); // 
        return page.render(ctx); //
        });

    CROW_ROUTE(app, "/hello")
        .methods("GET"_method)
        ([](const crow::request& req) {
        std::string par = req.url_params.get("par");
        return "Hello, World!"s + par;
            });

    CROW_ROUTE(app, "/postTest")
        .methods("POST"_method)
        ([](const crow::request& req) {
        std::string data = req.body;
#if 0
        for (const auto& item : req.headers) {
            std::cout << item.first <<
                "\t" << item.second << std::endl;
        }
#endif
        std::cout << "token : " << req.get_header_value("token") << std::endl;
        return "result -> "s + data;
            });

    //define your endpoint at the root directory
    CROW_ROUTE(app, "/test").methods("GET"_method)([]() {
        cv::Mat img{ cv::imread("draw/Random.png", cv::IMREAD_UNCHANGED) },
        phigros{ cv::imread("draw/Phigros.png", cv::IMREAD_UNCHANGED) },
        shadow{ cv::imread("draw/Shadow.png", cv::IMREAD_UNCHANGED) },
        EZ{ cv::imread("draw/diff/EZ.png", cv::IMREAD_UNCHANGED) },
        HD{ cv::imread("draw/diff/HD.png", cv::IMREAD_UNCHANGED) },
        IN{ cv::imread("draw/diff/IN.png", cv::IMREAD_UNCHANGED) },
        AT{ cv::imread("draw/diff/AT.png", cv::IMREAD_UNCHANGED) },
        LG{ cv::imread("draw/diff/LG.png", cv::IMREAD_UNCHANGED) },
        SP{ cv::imread("draw/diff/SP.png", cv::IMREAD_UNCHANGED) },
        EZ_Sign{ cv::imread("draw/diff/DiifFont/EZ.png", cv::IMREAD_UNCHANGED) },
        HD_Sign{ cv::imread("draw/diff/DiifFont/HD.png", cv::IMREAD_UNCHANGED) },
        IN_Sign{ cv::imread("draw/diff/DiifFont/IN.png", cv::IMREAD_UNCHANGED) },
        AT_Sign{ cv::imread("draw/diff/DiifFont/AT.png", cv::IMREAD_UNCHANGED) },
        SP_Sign{ cv::imread("draw/diff/DiifFont/LG.png", cv::IMREAD_UNCHANGED) },
        LG_Sign{ cv::imread("draw/diff/DiifFont/SP.png", cv::IMREAD_UNCHANGED) };

    if (img.empty())
    {
        crow::response response{ crow::status::INTERNAL_SERVER_ERROR };
        response.body = "SERVER ERROR";
        std::cout << "HTTP500 INTERNAL_SERVER_ERROR" << std::endl;
        return response;
    }

    cv::Mat blurMat;
    cv::GaussianBlur(img, blurMat, cv::Size(0, 0), 50); // 设置高斯模糊

    float img_weight_scale{ 2048.0f / blurMat.cols };
    float img_height_scale{ 1080.0f / blurMat.rows };

    cv::resize(blurMat, blurMat, cv::Size(), img_weight_scale, img_height_scale);

    cv::Mat blackCover{ cv::Mat::ones(cv::Size{img.cols, img.rows}, CV_8UC4) }; // 创建一个与img大小相同的矩阵
    blackCover = cv::Scalar{ 0, 0, 0, 100 }; // 设置黑色不透明图层，即所有像素值都为0，第四个通道为153

    cv::Mat dstImg;
    cv::addWeighted(blackCover, 0.6, blurMat, 1 - 0.6, 0.0, dstImg); // 叠加黑色不透明图层

    //DrawTool::copyToPointAlpha(HD, dstImg, 320, 450);
    //DrawTool::copyToPointAlpha(IN, dstImg, 480, 300, 0.5f, 0.5f);
    //DrawTool::copyToPointAlpha(EZ, dstImg, 120, 160, 1.5f,1.5f);

    // 绘制平行四边形矩形
    cv::Rect rect{ 0, 0, img.size().width, img.size().height };
    DrawTool::drawParallelogram(img, rect, 79);
    //cv::Mat imgCopy = img.clone();
    //DrawTool::drawParallelogram(imgCopy, rect, 77, 0.0f, true);
    cv::Mat alphaPassGraph{ DrawTool::createPureMat(img, 100) };
    // 创建一个全黑的相同尺寸的Mat，作为处理后的图片
//#ifdef DEBUG
    //cv::imwrite("PureMat.png", DrawTool::createPureMat(img, 100));
//#endif // 
    DrawTool::copyToPointAlpha(alphaPassGraph, dstImg, 159, 117 + 4, 0.555f, 0.555f);
    DrawTool::copyToPointAlpha(img, dstImg, 159, 117, 0.55f, 0.55f);
    DrawTool::copyToPointAlpha(shadow, dstImg, 159, 117, 0.55f, 0.55f);


    // 裁切图像
    //左
    // 获取裁切后的区域
    cv::Rect roi{ 1830, 0, alphaPassGraph.cols - 1830, alphaPassGraph.rows };
    cv::Mat leftAlphaPassGraph{ alphaPassGraph(roi) };
    //右
    roi = cv::Rect{ 0, 0, alphaPassGraph.cols - 1830, alphaPassGraph.rows };
    cv::Mat rightAlphaPassGraph{ alphaPassGraph(roi) };

    DrawTool::copyToPointAlpha(leftAlphaPassGraph, dstImg, 0, 0, 1.0f, 1.0f);
    DrawTool::copyToPointAlpha(rightAlphaPassGraph, dstImg, 1830, 0, 1.0f, 1.0f);
    cv::Mat alphaPassGraphDiff{ DrawTool::createPureMat(EZ, 15) };

    constexpr int sign_offset_x{ 43 }, sign_offset_y{ 27 };

    constexpr int X_OFFSETINIT{ 0 }, Y_OFFSETINIT{ 0 }, HC{ -39 }, VC{ 228 };
    int h{ 1296 + X_OFFSETINIT }, v{ 112 + Y_OFFSETINIT }, offset_x{ 195 }, offset_y{ 108 };

    DrawTool::copyToPointAlpha(alphaPassGraphDiff, dstImg, h + 1, v, 1.005f, 1.015f);
    DrawTool::copyToPointAlpha(EZ, dstImg, h - 2, v - 5, 1.0f, 1.0f);
    DrawTool::copyToPointAlpha(EZ_Sign, dstImg, h + sign_offset_x + 1, v + sign_offset_y, 0.4f, 0.4f);

    h += HC; v += VC;

    DrawTool::copyToPointAlpha(alphaPassGraphDiff, dstImg, h + 1, v, 1.005f, 1.015f);
    DrawTool::copyToPointAlpha(HD, dstImg, h - 2, v - 5, 1.0f, 1.0f);
    DrawTool::copyToPointAlpha(HD_Sign, dstImg, h + sign_offset_x + 3, v + sign_offset_y, 0.4f, 0.4f);

    h += HC; v += VC;

    DrawTool::copyToPointAlpha(alphaPassGraphDiff, dstImg, h + 1, v, 1.005f, 1.015f);
    DrawTool::copyToPointAlpha(IN, dstImg, h - 2, v - 5, 1.0f, 1.0f);
    DrawTool::copyToPointAlpha(IN_Sign, dstImg, h + sign_offset_x, v + sign_offset_y, 0.4f, 0.4f);
    DrawTool::copyToPointAlpha(phigros, dstImg, 1186, 800, 1.0f, 1.0f);
    //cv::imwrite("output.png", addFont(dstImg, X_OFFSETINIT, Y_OFFSETINIT));

    cv::Mat result{ addFont(dstImg, X_OFFSETINIT, Y_OFFSETINIT) };

    std::vector<uchar> data;
    cv::imencode(".png", std::move(result), data);
    std::string imgStr(data.begin(), data.end());

    crow::response response(crow::status::OK);
    response.set_header("Content-Type", "image/png");
    response.body = imgStr;
    return response;
        });


    // websocket
    CROW_ROUTE(app, "/ws")
        .websocket()
        .onopen([&](crow::websocket::connection& conn) {
        std::cout << "建立ws连接" << std::endl;
            })
        .onclose([&](crow::websocket::connection& conn, const std::string& reason) {
                std::cout << "关闭ws连接" << std::endl;
            })
                .onmessage([&](crow::websocket::connection& conn, const std::string& data, bool is_binary) {
                if (is_binary)
                    std::cout << "binary -> ";
                else
                    std::cout << "normal -> ";
                std::cout << "receive: " << data << std::endl;
                conn.send_text("HelloTo");
                    });

            // 全局异常处理
            CROW_ROUTE(app, "/")([]() {
                crow::response res("404 Not Found has been changed to this message.");
                res.code = 404;
                return res;
                });
            //set the port, set the app to run on multiple threads, and run the app
            app.concurrency(8).run();
            // 下面只能4个线程
            //app.multithreaded().run();

            delete bottle;
}


// 添加完后会生成全新的文字生成图
inline cv::Mat addFont(cv::Mat& dstImg, const int X_OFFSETINIT, const int Y_OFFSETINIT) {
    using namespace cv;
    Mat result(dstImg.rows, dstImg.cols, CV_8UC3);
    int from_to[] = { 0,0, 1,1, 2,2 };
    mixChannels(&dstImg, 1, &result, 1, from_to, 3);
    int font_size = 32;
    cv::Ptr<freetype::FreeType2> freetype2;

    freetype2 = cv::freetype::createFreeType2();
    //freetype2->loadFontData("draw/font/SOURCEHANSANSCN_MEDIUM.OTF", 0);
    freetype2->loadFontData("draw/font/SourceHanSans_SairaHybridRegularHot.ttf", 0);

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



    //freetype2->loadFontData("draw/font/AdobeStdR.otf", 0);
    constexpr int HC{ -39 }, VC{ 228 };
    int h{ 1296 + X_OFFSETINIT }, v{ 112 + Y_OFFSETINIT }, offset_x{ 195 }, offset_y{ 108 };
    freetype2->putText(result, "6.0  / 275", Point(h + offset_x, v + offset_y) + cv::Point(3, 2), 72, Scalar(5, 5, 5), -1, cv::LINE_AA, true);
    freetype2->putText(result, "6.0  / 275", Point(h + offset_x, v + offset_y), 72, Scalar(215, 215, 215), -1, cv::LINE_AA, true);

    freetype2->putText(result, "Design:  .---- ----. ---.. / -. . .-. (百九十八 / NerSAN)", Point(h + 7, v + offset_y + 70) + cv::Point(2, 1), 24, Scalar(0, 0, 0), -1, cv::LINE_AA, true);
    freetype2->putText(result, "Design:  .---- ----. ---.. / -. . .-. (百九十八 / NerSAN)", Point(h + 7, v + offset_y + 70), 24, Scalar(200, 200, 200), -1, cv::LINE_AA, true);

    h += HC; v += VC;
    freetype2->putText(result, "11.1 / 595", Point(h + offset_x, v + offset_y) + cv::Point(3, 2), 72, Scalar(5, 5, 5), -1, cv::LINE_AA, true);
    freetype2->putText(result, "11.1 / 595", Point(h + offset_x, v + offset_y), 72, Scalar(215, 215, 215), -1, cv::LINE_AA, true);

    freetype2->putText(result, "Design:  Jαckψ feat. Βαρβαροςαντρας", Point(h + 7, v + offset_y + 70) + cv::Point(2, 1), 24, Scalar(0, 0, 0), -1, cv::LINE_AA, true);
    freetype2->putText(result, "Design:  Jαckψ feat. Βαρβαροςαντρας", Point(h + 7, v + offset_y + 70), 24, Scalar(200, 200, 200), -1, cv::LINE_AA, true);

    h += HC; v += VC;
    freetype2->putText(result, "14.7 / 7328", Point(h + offset_x, v + offset_y) + cv::Point(3, 2), 72, Scalar(5, 5, 5), -1, cv::LINE_AA, true);
    freetype2->putText(result, "14.7 / 7328", Point(h + offset_x, v + offset_y), 72, Scalar(215, 215, 215), -1, cv::LINE_AA, true);

    freetype2->putText(result, "Design:  Two skunks (Ctymax and 晨) in Africa shouting WAHHHHHH!", Point(h + 7, v + offset_y + 70) + cv::Point(2, 1), 24, Scalar(0, 0, 0), -1, cv::LINE_AA, true);
    freetype2->putText(result, "Design:  Two skunks (Ctymax and 晨) in Africa shouting WAHHHHHH!", Point(h + 7, v + offset_y + 70), 24, Scalar(200, 200, 200), -1, cv::LINE_AA, true);


    // 添加日文文字
    //freetype2->putText(result, "こんにちは", Point(10, 100), font_size, font_color, -1, cv::LINE_AA, true);
    freetype2.reset();


    return result;
}

// TODO: 在此处引用程序需要的其他标头。