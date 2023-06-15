/*
 * @File	  : phigros_service_impl.hpp
 * @Coding	  : utf-8
 * @Author    : Bing
 * @Time      : 2023/04/23 21:02
 * @Introduce : 肥鸽肉丝
*/

#pragma once

#include <string>
#include <cmath>
#include <vector>
#include <thread>
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

#ifndef PHIGROS_SERVICE_IMPL_HPP
#define PHIGROS_SERVICE_IMPL_HPP  

// O3优化
#pragma GCC optimize(3)
#pragma G++ optimize(3)

class PhigrosServiceImpl : public PhigrosService {
private:
	cv::Mat
		phigros{ cv::imread("draw/phi/Phigros.png", cv::IMREAD_UNCHANGED) },
		shadow{ cv::imread("draw/phi/Shadow.png", cv::IMREAD_UNCHANGED) },
		shadow_corner{ cv::imread("draw/phi/ShadowCorner.png", cv::IMREAD_UNCHANGED) },
		playerRKSBox{ cv::imread("draw/phi/rksBox.png", cv::IMREAD_UNCHANGED) },
		dataFrame{ cv::imread("draw/phi/DataFrame.png", cv::IMREAD_UNCHANGED) },
		personal_single_song_info_shadow{ cv::imread("draw/phi/PersonalSingleSongInfoShadow.png",cv::IMREAD_UNCHANGED) },
		b19_background { cv::imread("draw/phi/background.png",cv::IMREAD_UNCHANGED) },
		b19_profile{ cv::imread("draw/phi/profile.png",cv::IMREAD_UNCHANGED) },
		b19_record{ cv::imread("draw/phi/record.png",cv::IMREAD_UNCHANGED) },
		b19_shadow{ cv::Mat(cv::Size(2048, 1080), CV_8UC4) };
	// base64解码为cv::Mat
	cv::Mat base64ToMat(const std::string& base64Str) {
		// 将base64字符串转换为字节数组
		std::vector<uchar> data(base64Str.begin(), base64Str.end());
		data = OtherUtil::base64Decode(base64Str);

		// 从字节数组中读取图像
		cv::Mat img{ cv::imdecode(data, cv::IMREAD_UNCHANGED) };
		if (img.empty()) {
			img.release();
			return cv::imread("draw/phi/UnknowAvatar.png", cv::IMREAD_UNCHANGED);
		}else if (img.type() == 0){
			cv::cvtColor(img, img, cv::COLOR_GRAY2BGR);
		}else if (img.type() == 24) {
			return std::move(img);
		}
		std::vector<cv::Mat> mv;
		split(img, mv);

		cv::Mat dst(img.size(), CV_8UC4, cv::Scalar(0, 0, 0, 255));

		int from_to[] = { 0,0,1,1,2,2 };  // 0通道换到2通道  1通道换到1通道 2通道换到0通道
		mixChannels(&img, 1, &dst, 1, from_to, 3);
		img.release();

		//std::cout << dst.type() << std::endl;

		return std::move(dst);
	}
public:
	~PhigrosServiceImpl() {
		phigros.release();
		shadow.release();
		shadow_corner.release();
		personal_single_song_info_shadow.release();
		playerRKSBox.release();
	};

	PhigrosServiceImpl() {
		cv::flip(shadow_corner, shadow_corner, -1);
		{
			cv::Mat b19_shadow(cv::Size(2048, 1080), CV_8UC4, cv::Scalar(0, 0, 0, 0));
			// 定义渐变的起始和结束颜色
			cv::Scalar startColor(0, 0, 0, 0);
			cv::Scalar endColor(0, 0, 0, 255);

			constexpr const double init_pos_ratio{ 0.15 };
			// std::cout << "col: " << b19_shadow.cols << ",rows: " << b19_shadow.rows << std::endl;
			for (int y{ (int)(b19_shadow.rows * init_pos_ratio) }; y < b19_shadow.rows; y++)
			{
				double ratio{ ((double)y - b19_shadow.rows * init_pos_ratio) / (b19_shadow.rows - b19_shadow.rows * init_pos_ratio) };
				// 计算当前行的颜色
				cv::Scalar color{ startColor * (1 - ratio) + endColor * (ratio) };

				// std::cout << color << std::endl;

				// 绘制矩形
				cv::rectangle(b19_shadow, cv::Point(0, y), cv::Point(b19_shadow.cols, y), color, -1, cv::LINE_AA);
			}
			b19_shadow.copyTo(this->b19_shadow);
			b19_shadow.release();
		}
	};

	inline cv::Mat drawSongInfomation(int song_id, bool isQRCode, std::string_view QRCodeContent) override {
		//设置超过200ms后API请求超时
		constexpr std::chrono::milliseconds timeout{ 2000ms };
		constexpr const size_t size_w{ 2048 }, size_h{ 1080 };

		Json data;

		//获取API数据到data
		OtherUtil::asyncGetAPI(data, timeout, Global::BingAPI, Global::PhiUri + "?id="s + std::to_string(song_id));
		std::exchange(data, data[0]);

		std::string illustrationPath{ Global::PhiResourcePath + data["song_illustration_url"].get<std::string>() };


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

			diffText["rating"] = OtherUtil::retainDecimalPlaces(data["rating"]["ez"].get<float>(), 1);
			diffText["note"] = std::to_string(data["note"]["ez"].get<int>());
			diffText["design"] = data["design"]["ez"].get<std::string>();

			diffTexts.emplace_back(diffText);
		}
		if (!data["design"]["hd"].is_null()) {
			diff.emplace_back(cv::imread("draw/phi/diff/HD.png", cv::IMREAD_UNCHANGED));
			diffSign.emplace_back(cv::imread("draw/phi/diff/DiifFont/HD.png", cv::IMREAD_UNCHANGED));

			diffText["rating"] = OtherUtil::retainDecimalPlaces(data["rating"]["hd"].get<float>(), 1);
			diffText["note"] = std::to_string(data["note"]["hd"].get<int>());
			diffText["design"] = data["design"]["hd"].get<std::string>();

			diffTexts.emplace_back(diffText);
		}
		if (!data["design"]["in"].is_null()) {
			diff.emplace_back(cv::imread("draw/phi/diff/IN.png", cv::IMREAD_UNCHANGED));
			diffSign.emplace_back(cv::imread("draw/phi/diff/DiifFont/IN.png", cv::IMREAD_UNCHANGED));

			diffText["rating"] = OtherUtil::retainDecimalPlaces(data["rating"]["in"].get<float>(), 1);
			diffText["note"] = std::to_string(data["note"]["in"].get<int>());
			diffText["design"] = data["design"]["in"].get<std::string>();

			diffTexts.emplace_back(diffText);
		}
		if (!data["design"]["at"].is_null()) {
			diff.emplace_back(cv::imread("draw/phi/diff/AT.png", cv::IMREAD_UNCHANGED));
			diffSign.emplace_back(cv::imread("draw/phi/diff/DiifFont/AT.png", cv::IMREAD_UNCHANGED));

			diffText["rating"] = OtherUtil::retainDecimalPlaces(data["rating"]["at"].get<float>(), 1);
			diffText["note"] = std::to_string(data["note"]["at"].get<int>());
			diffText["design"] = data["design"]["at"].get<std::string>();

			diffTexts.emplace_back(diffText);
		}
		if (!data["design"]["lg"].is_null()) {
			diff.emplace_back(cv::imread("draw/phi/diff/LG.png", cv::IMREAD_UNCHANGED));
			diffSign.emplace_back(cv::imread("draw/phi/diff/DiifFont/LG.png", cv::IMREAD_UNCHANGED));

			diffText["rating"] = OtherUtil::retainDecimalPlaces(data["rating"]["lg"].get<float>(), 1);
			diffText["note"] = std::to_string(data["note"]["lg"].get<int>());
			diffText["design"] = data["design"]["lg"].get<std::string>();

			diffTexts.emplace_back(diffText);
		}
		if (!data["design"]["sp"].is_null()) {
			diff.emplace_back(cv::imread("draw/phi/diff/SP.png", cv::IMREAD_UNCHANGED));
			diffSign.emplace_back(cv::imread("draw/phi/diff/DiifFont/SP.png", cv::IMREAD_UNCHANGED));

			std::string rating{ OtherUtil::retainDecimalPlaces(data["rating"]["sp"].get<float>(),1) };
			diffText["rating"] = rating == "-1.0"s ? "  ?"s : rating;
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
		cv::Rect rect{ 0, 0, img.size().width - 1, img.size().height };
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
			cv::Mat QRCode{ DrawTool::DrawQRcode(QRCodeContent.data(),true,2,2,6,6) };
			constexpr const float
				sha_cor_ratio{ 0.4f },
				qr_ratio{ 0.8f };
			cv::Mat backgroundQRCode{ DrawTool::createPureMat(QRCode.cols + 8,QRCode.rows + 8, 255,138,138,138) };
			DrawTool::transparentPaste(backgroundQRCode, dstImg, size_w - backgroundQRCode.cols * qr_ratio, size_h - backgroundQRCode.rows * qr_ratio, qr_ratio, qr_ratio);
			DrawTool::transparentPaste(QRCode, dstImg, size_w - QRCode.cols * qr_ratio - 4, size_h - QRCode.rows * qr_ratio - 4, qr_ratio, qr_ratio);
			DrawTool::transparentPaste(shadow_corner, dstImg, size_w - shadow_corner.cols * sha_cor_ratio, size_h - shadow_corner.rows * sha_cor_ratio, sha_cor_ratio, sha_cor_ratio);
			backgroundQRCode.release();
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
			int offset_x{ }, offset_y{ }, hc{ -39 }, vc{ 228 }, font_offset_x{ 0 };
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
				freetype2->putText(result, diffTexts.at(i).at("rating") + " / " + diffTexts.at(i).at("note"), cv::Point(h, v) + cv::Point(3, 2), 72, cv::Scalar(5, 5, 5), -1, cv::LINE_AA, true);
				freetype2->putText(result, diffTexts.at(i).at("rating") + " / " + diffTexts.at(i).at("note"), cv::Point(h, v), 72, cv::Scalar(215, 215, 215), -1, cv::LINE_AA, true);

				const constexpr int font_size_max{ 620 };

				// TODO 裁剪字符串
				std::string chart{ "Chart:  "s + diffTexts.at(i).at("design") };


				int offset_w{ freetype2->getTextSize(chart, 24, -1, nullptr).width };

				while (offset_w > font_size_max) {
					std::wstring_convert<std::codecvt_utf8<wchar_t>> converter;
					std::wstring wstr = converter.from_bytes(chart);

					wstr.pop_back();

					chart = converter.to_bytes(wstr);
					std::string temp{ chart + "..."s };
					offset_w = freetype2->getTextSize(temp, 24, -1, nullptr).width;
					if (offset_w > font_size_max)
					{
						continue;
					}
					else {
						chart = temp;
						break;
					}
				}

				freetype2->putText(result, chart, cv::Point(h - offset_x - X_OFFSETINIT - 13 + font_offset_x, v + 70) + cv::Point(2, 1), 24, cv::Scalar(0, 0, 0), -1, cv::LINE_AA, true);
				freetype2->putText(result, chart, cv::Point(h - offset_x - X_OFFSETINIT - 13 + font_offset_x, v + 70), 24, cv::Scalar(200, 200, 200), -1, cv::LINE_AA, true);
				h += hc; v += vc;
			}
		};

		freetype2->loadFontData("draw/phi/font/AdobeStdR.otf", 0);

		freetype2->putText(result, "Generated by tomato Team - Phigros", cv::Point(18, 1070) + cv::Point(3, 2), 25, cv::Scalar(5, 5, 5), -1, cv::LINE_AA, true);
		freetype2->putText(result, "Generated by tomato Team - Phigros", cv::Point(18, 1070), 25, cv::Scalar(255, 255, 255), -1, cv::LINE_AA, true);

		freetype2.release();


		return std::move(result);

	};

	// string_view yuhao_token临时测试玩家名称
	inline cv::Mat drawPlayerSingleInfo(std::string_view song_id, Ubyte level,
		std::string_view auth_token, std::string_view player_session_token, std::string_view avatar_base64) override {
		constexpr std::chrono::milliseconds timeout{ 2000ms };
		Json dataId{}, songData{}, playerData{};
		//获取API数据到曲目data
		OtherUtil::asyncGetAPI(songData, timeout, Global::BingAPI, Global::PhiUri + "?id="s + song_id.data());
		std::exchange(songData, songData[0]);
		std::string illustrationPath{ Global::PhiResourcePath + songData["song_illustration_url"].get<std::string>() };

		bool flag{ false };
		self::HTTPException httpexception;

		std::thread apiDataGet([&] {
			try {

				constexpr std::chrono::seconds playerTimeout{ 30s };
				//获取API数据到对应id
				/*
				OtherUtil::asyncGetAPI(dataId, timeout, Global::BingAPI, "/api/pgr/findYuhao7370Id?id="s + song_id.data());


				if (dataId["yuhao7370_song_id"].is_null()) {
					httpexception = self::HTTPException("yuhao7370_song_id is null");
					flag = true;
					return;
				}
				std::string yuhao7370id{ dataId["yuhao7370_song_id"].get<std::string>() };
				*/
				// POST请求测试
				// 创建HTTP客户端
				httplib::Client client(Global::PhiPlayDataAPI, 8299);

				// 创建Authorization头部
				std::string auth_header = "Bearer "s + auth_token.data();

				// 创建Get请求
				httplib::Headers headers = {
					{"Authorization", auth_header},
					{"SessionToken", player_session_token.data()},
				};

				// =================================================

				std::future<Json> future{ std::async(std::launch::async,[&]()->Json {
					httplib::Result res { client.Get("/proxy/phi/best?songid="s + song_id.data() + "&level="s + std::to_string(level), headers)};
					// 到时候加一个超时
					if (res && res->status == 200) return json::parse(res->body);

					auto err{ res.error() };
					if (err != httplib::Error::Success)
					{
						httpexception = self::HTTPException(httplib::to_string(err), 500);
						flag = true;
						return Json();
					}

					try {
						httpexception = self::HTTPException(json::parse(res->body)["detail"].get<std::string>(),res->status);
						flag = true;
						return Json();
					}
					catch (...) {
						httpexception = self::HTTPException("", res->status);
						flag = true;
						return Json();
					}
				}

				) };
				// --------------------
				std::future_status status{ future.wait_for(playerTimeout) };

				if (status == std::future_status::timeout) {
					httpexception = self::HTTPException("请求超时", 408);
					flag = true;
					return;
				}
				playerData = future.get();
			}
			catch (const std::runtime_error& e) {
				httpexception = self::HTTPException(e.what());
			}
			// =================================================
			});;


		// =============================
		constexpr const size_t size_w{ 2048 }, size_h{ 1080 };
		cv::Mat illustration{ cv::imread(std::move(illustrationPath),cv::IMREAD_UNCHANGED) };
		cv::resize(illustration, illustration, cv::Size(size_w, size_h));

		cv::Mat img{ illustration.clone() };

		// 切左右两边图片
		{
			cv::Mat left_slice_illustration{ Mat::zeros(illustration.size(),illustration.type()) },
				// 创建一个新的掩码（mask），并将其全部填充为不透明
				mask(illustration.size(), CV_8UC1, cv::Scalar(255));

			// 创建一组点，将其设置为矩形的四个顶点
			std::vector<cv::Point> points
			{
				cv::Point(0, 0),
				cv::Point(685, 0),
				cv::Point(376, size_h),
				cv::Point(0, size_h)
			};

			// 创建一个新的掩码，并将其填充为完全透明
			cv::Mat polyMask(illustration.size(), CV_8UC1, cv::Scalar(0));

			// 在polyMask上绘制我们想要的多边形
			cv::fillConvexPoly(polyMask, points, cv::Scalar(255));

			// 将mask与polyMask相乘，并将结果存储在mask中
			mask = mask.mul(polyMask);

			// 将被切掉的部分的不透明度设置为0
			cv::Mat channels1[4];
			cv::split(illustration, channels1);
			channels1[3] = mask.clone();
			cv::merge(channels1, 4, left_slice_illustration);

			mask.release();
			polyMask.release();
			channels1[0].release();
			channels1[1].release();
			channels1[2].release();
			channels1[3].release();
			// 高斯模糊
			cv::GaussianBlur(left_slice_illustration, left_slice_illustration, cv::Size(0, 0), 5);
			DrawTool::transparentPaste(left_slice_illustration, img);
			left_slice_illustration.release();
		};
		{
			cv::Mat right_slice_illustration{ Mat::zeros(illustration.size(),illustration.type()) },
				// 创建一个新的掩码（mask），并将其全部填充为不透明
				mask(illustration.size(), CV_8UC1, cv::Scalar(255));

			// 创建一组点，将其设置为矩形的四个顶点
			std::vector<cv::Point> points
			{
				cv::Point(size_w, 334),
				cv::Point(size_w, size_h),
				cv::Point(1836, size_h)
			};

			// 创建一个新的掩码，并将其填充为完全透明
			cv::Mat polyMask(illustration.size(), CV_8UC1, cv::Scalar(0));

			// 在polyMask上绘制我们想要的多边形
			cv::fillConvexPoly(polyMask, points, cv::Scalar(255));

			// 将mask与polyMask相乘，并将结果存储在mask中
			mask = mask.mul(polyMask);

			// 将被切掉的部分的不透明度设置为0
			cv::Mat channels1[4];
			cv::split(illustration, channels1);
			channels1[3] = mask.clone();
			cv::merge(channels1, 4, right_slice_illustration);

			mask.release();
			polyMask.release();
			channels1[0].release();
			channels1[1].release();
			channels1[2].release();
			channels1[3].release();
			// 高斯模糊
			cv::GaussianBlur(right_slice_illustration, right_slice_illustration, cv::Size(0, 0), 5);
			DrawTool::transparentPaste(right_slice_illustration, img);
			right_slice_illustration.release();
		};

		DrawTool::transparentPaste(personal_single_song_info_shadow, img);
		cv::Mat rate{ }, courseRating{ };

		int player_form_offset_x{};


		cv::Ptr<freetype::FreeType2> freetype2{ cv::freetype::createFreeType2() };
		freetype2->loadFontData("draw/phi/font/SourceHanSansCN_SairaCondensed_Hybrid_Medium.ttf", 0);

		// 在这堵塞(能快一秒是一秒)
		apiDataGet.join();
		if (flag) {
			illustration.release();
			img.release();
			rate.release();
			throw httpexception;
		}
		std::exchange(playerData, playerData["content"]);

		std::string
			playerName{ playerData["playerNickname"].get<std::string>() },
			playerRKS{ OtherUtil::retainDecimalPlaces(playerData["rankingScore"].get<float>()) };
		int playerSocre{ playerData["record"]["score"].get<int>() },
			playerCourseRanking{ playerData["challengeModeRank"].get<int>() };
		bool playerIsFC{ playerData["record"]["isfc"].get<bool>() };

		if (playerSocre >= 1000000) {
			rate = cv::imread("draw/phi/rating/uniformSize/phi_old.png", cv::IMREAD_UNCHANGED);
		}
		else if (playerIsFC)
		{
			rate = cv::imread("draw/phi/rating/uniformSize/V_FC.png", cv::IMREAD_UNCHANGED);
		}
		else if (playerSocre >= 960000) {
			rate = cv::imread("draw/phi/rating/uniformSize/V_old.png", cv::IMREAD_UNCHANGED);
		}
		else if (playerSocre >= 920000) {
			rate = cv::imread("draw/phi/rating/uniformSize/s_old.png", cv::IMREAD_UNCHANGED);
		}
		else if (playerSocre >= 880000) {
			rate = cv::imread("draw/phi/rating/uniformSize/A.png", cv::IMREAD_UNCHANGED);
		}
		else if (playerSocre >= 820000) {
			rate = cv::imread("draw/phi/rating/uniformSize/B_old.png", cv::IMREAD_UNCHANGED);
		}
		else if (playerSocre >= 700000) {
			rate = cv::imread("draw/phi/rating/uniformSize/C_old.png", cv::IMREAD_UNCHANGED);
		}
		else {
			rate = cv::imread("draw/phi/rating/uniformSize/F_new.png", cv::IMREAD_UNCHANGED);
		}

		rate = rate(cv::Rect(25, 0, rate.cols - 25, rate.rows));
		cv::resize(rate, rate, cv::Size(), 1.15, 1.15);
		DrawTool::transparentPaste(rate, img, 0, 745);
		// 玩家框
		{
			constexpr const int
				h{ 88 },
				offset_h{ 25 };// 88 * tan15.9 = 25

			// + 292
			int baseLine;
			int offset_w{ 292 + freetype2->getTextSize(playerName, 48, -1, &baseLine).width };

			while (offset_w > 1340) {
				std::wstring_convert<std::codecvt_utf8<wchar_t>> converter;
				std::wstring wstr = converter.from_bytes(playerName);

				wstr.pop_back();

				playerName = converter.to_bytes(wstr);
				std::string temp{ playerName + "..."s };
				offset_w = 292 + freetype2->getTextSize(temp, 48, -1, &baseLine).width;
				if (offset_w > 1400)
				{
					continue;
				}
				else {
					playerName = temp;
					break;
				}
			}

			cv::Mat playerForm(h, offset_w + 40, CV_8UC4, cv::Scalar(0, 0, 0, 0));

			// 定义平行四边形的四个顶点坐标
			std::vector<cv::Point> points{
				cv::Point(0, playerForm.size().height),
				cv::Point(offset_h, 0),
				cv::Point(playerForm.size().width, 0),
				cv::Point(playerForm.size().width, playerForm.size().height)
			};

			// 将顶点坐标存储到一个向量中
			std::vector<std::vector<cv::Point>> contours;
			contours.push_back(points);
			cv::drawContours(playerForm, contours, 0, cv::Scalar(0, 0, 0, 178), -1, LINE_AA);

			player_form_offset_x = size_w - playerForm.cols;
			DrawTool::transparentPaste(playerForm, img, player_form_offset_x, 48);
			playerForm.release();
		};
		DrawTool::transparentPaste(playerRKSBox, img, 1907, 96);

		// 玩家头像
		{
			constexpr const int
				h{ 120 },
				offset_h{ 34 };// 120 * tan15.9 = 34//向下取整

			//cv::Mat playerHead{ cv::imread("draw/test.png", cv::IMREAD_UNCHANGED) };
			cv::Mat playerHead{ !avatar_base64.empty() ? base64ToMat(avatar_base64.data()) : cv::imread("draw/phi/UnknowAvatar.png", cv::IMREAD_UNCHANGED) },
				playerHeadBox(h, 166, CV_8UC4, cv::Scalar(0, 0, 0, 0));

			resize(playerHead, playerHead, cv::Size(150, 150));

			// 定义平行四边形的四个顶点坐标
			std::vector<cv::Point> points{
				cv::Point(0, playerHeadBox.size().height),
				cv::Point(offset_h, 0),
				cv::Point(playerHeadBox.size().width - 2, 0),
				cv::Point(playerHeadBox.size().width - offset_h - 2, playerHeadBox.size().height)
			},
				points1{
				cv::Point(30, 21),
				cv::Point(150 - 2, 21),
				cv::Point(120 - 2, 129),
				cv::Point(0, 129),
				cv::Point(0,150),
				cv::Point(150,150),
				cv::Point(150,0),
				cv::Point(0,0),
				cv::Point(0,129)
			};

			// 将顶点坐标存储到一个向量中
			std::vector<std::vector<cv::Point>> contours{ points };
			std::vector<std::vector<cv::Point>> contours1{ points1 };
			//reverse(contours1[0], contours1[0]);
			//contours.emplace_back(points);
			//contours1.emplace_back(points1);
			cv::drawContours(playerHeadBox, contours, 0, cv::Scalar(77, 77, 77, 255), -1, LINE_AA);

			cv::drawContours(playerHead, contours1, 0, cv::Scalar(0, 0, 0, 0), -1, LINE_AA);

			DrawTool::transparentPaste(playerHeadBox, img, 1794, 33);
			DrawTool::transparentPaste(playerHead, img, 1802, 17);
			playerHeadBox.release();
			playerHead.release();
		};

		if (playerCourseRanking >= 500) {
			courseRating = cv::imread("draw/phi/rating/uniformSize/5.png", cv::IMREAD_UNCHANGED);
		}
		else if (playerCourseRanking >= 400) {
			courseRating = cv::imread("draw/phi/rating/uniformSize/4.png", cv::IMREAD_UNCHANGED);
		}
		else if (playerCourseRanking >= 300) {
			courseRating = cv::imread("draw/phi/rating/uniformSize/3.png", cv::IMREAD_UNCHANGED);
		}
		else if (playerCourseRanking >= 200) {
			courseRating = cv::imread("draw/phi/rating/uniformSize/2.png", cv::IMREAD_UNCHANGED);
		}
		else if (playerCourseRanking >= 100) {
			courseRating = cv::imread("draw/phi/rating/uniformSize/1.png", cv::IMREAD_UNCHANGED);
		}
		else {
			courseRating = cv::imread("draw/phi/rating/uniformSize/0.png", cv::IMREAD_UNCHANGED);
		}
		playerCourseRanking = playerCourseRanking % 100;

		DrawTool::transparentPaste(courseRating, img, 1942, 49, 0.269f, 0.269f, cv::INTER_AREA);
		courseRating.release();
		rate.release();
		illustration.release();

		// 文字处理添加
		cv::Mat result(img.rows, img.cols, CV_8UC3);
		//cv::Mat result{ addFont(dstImg, std::move(data), X_OFFSETINIT, Y_OFFSETINIT) };

		int from_to[] = { 0,0, 1,1, 2,2 };
		mixChannels(&img, 1, &result, 1, from_to, 3);

		img.release();




		freetype2->loadFontData("draw/phi/font/PlayoffProCond.ttf", 0);

		std::string songRating{ playerData["record"]["difficulty"].get<std::string>() + ": "s + OtherUtil::retainDecimalPlaces(playerData["record"]["rating"].get<float>(),1) };

		freetype2->putText(result, songRating, cv::Point(117, 357) + cv::Point(5, 3), 56, cv::Scalar(5, 5, 5), -1, cv::LINE_AA, true);
		freetype2->putText(result, songRating, cv::Point(117, 357), 56, cv::Scalar(255, 255, 255), -1, cv::LINE_AA, true);

		freetype2->putText(result, OtherUtil::digitSupplementHandle(playerSocre), cv::Point(238, 970), 84, cv::Scalar(255, 255, 255), -1, cv::LINE_AA, true);
		freetype2->putText(result, "Rate: "s + OtherUtil::retainDecimalPlaces(playerData["record"]["rks"].get<float>()), cv::Point(584, 937), 40, cv::Scalar(255, 255, 255), -1, cv::LINE_AA, true);
		freetype2->putText(result, "Acc: "s + OtherUtil::retainDecimalPlaces(playerData["record"]["acc"].get<float>()) + "%"s, cv::Point(579, 975), 32, cv::Scalar(255, 255, 255), -1, cv::LINE_AA, true);

		std::string  PlayerCourse{ std::to_string(playerCourseRanking) };
		int font_offset_x{ 1975 };
		if (PlayerCourse.length() == 1)
		{
			font_offset_x = 1985;
		}

		freetype2->putText(result, PlayerCourse, cv::Point(font_offset_x, 78), 36, cv::Scalar(255, 255, 255), -1, cv::LINE_AA, true);

		// 加粗字体
		freetype2->loadFontData("draw/phi/font/SourceHanSans_SairaHybridRegularHot.ttf", 0);

		font_offset_x = 1945;
		if (playerRKS.length() == 4)
		{
			font_offset_x = 1950;
		}
		freetype2->putText(result, playerRKS, cv::Point(font_offset_x, 122), 32, cv::Scalar(0, 0, 0), -1, cv::LINE_AA, true);


		// 中等字体
		freetype2->loadFontData("draw/phi/font/SourceHanSansCN_SairaCondensed_Hybrid_Medium.ttf", 0);
		freetype2->putText(result, playerData["record"]["title"], cv::Point(119, 272) + cv::Point(5, 3), 84, cv::Scalar(5, 5, 5), -1, cv::LINE_AA, true);
		freetype2->putText(result, playerData["record"]["title"], cv::Point(119, 272), 84, cv::Scalar(255, 255, 255), -1, cv::LINE_AA, true);

		freetype2->putText(result, playerName, cv::Point(player_form_offset_x + 52, 107), 48, cv::Scalar(255, 255, 255), -1, cv::LINE_AA, true);

		freetype2->loadFontData("draw/phi/font/AdobeStdR.otf", 0);

		freetype2->putText(result, "Powerd by yuhao7370. Generated by tomato Team - Phigros.", cv::Point(1, 1059), 16, cv::Scalar(255, 255, 255), -1, cv::LINE_AA, false);
		freetype2.release();

		return std::move(result);
	}

	inline cv::Mat drawPlayerSingleInfoModernStyle(std::string_view song_id, Ubyte level,
		std::string_view auth_token, std::string_view player_session_token, std::string_view avatar_base64) override {
		Json api_data{};
		
		// ======================================
		httplib::Client client(Global::PhiPlayDataAPI, 8299);
		// 创建Authorization头部
		std::string auth_header = "Bearer "s + auth_token.data();

		// 创建Get请求
		httplib::Headers headers = {
			{"Authorization", auth_header},
			{"SessionToken", player_session_token.data()},
		};

		bool flag{ false };
		self::HTTPException httpexception;
		constexpr std::chrono::seconds playerTimeout{ 30s };
		std::future<Json> future{ std::async(std::launch::async,[&]()->Json {
			httplib::Result res { client.Get("/proxy/phi/best?songid="s + song_id.data() + "&level="s + std::to_string(level), headers)};
			// 到时候加一个超时
			if (res && res->status == 200) return json::parse(res->body);

			auto err{ res.error() };
			if (err != httplib::Error::Success)
			{
				throw self::HTTPException(httplib::to_string(err), 500);
			}

			try {
				httpexception = self::HTTPException(json::parse(res->body).at("detail").get<std::string>(), res->status);
				flag = true;
				return Json();
			}
			catch (...) {
				httpexception = self::HTTPException("", res->status);
				flag = true;
				return Json();
			} }
		) };
		// --------------------
		std::future_status status{ future.wait_for(playerTimeout) };

		if (status == std::future_status::timeout) {
			httpexception = self::HTTPException("请求超时", 408);
			flag = true;
		}
		api_data = future.get();

		if (flag) {
			throw httpexception;
		}
		std::exchange(api_data, api_data.at("content"));
		// ======================================

		int playerSocre{ api_data.at("record").at("score").get<int>() },
			playerCourseRanking{ api_data.at("challengeModeRank").get<int>()};
		bool playerIsFC{ api_data.at("record").at("isfc").get<bool>() };

		std::string
			playerName{ api_data.at("playerNickname").get<std::string>() },
			updateTimeStr{ "Upload Time: "s + api_data.at("updateTime").get<std::string>() };
		double player_rks{ api_data.at("rankingScore").get<double>() };
		int player_form_offset_x{};

		constexpr const int size_w{ 2048 }, size_h{ 1080 };
		cv::Ptr<freetype::FreeType2> freetype2{ cv::freetype::createFreeType2() };
		cv::Mat illustration{ cv::imread(Global::PhiResourcePath + api_data.at("record").at("illustrationPath").get<std::string>(),cv::IMREAD_UNCHANGED)};

		cv::Mat rate{ }, courseRating{ };
		cv::resize(illustration, illustration, cv::Size(size_w, size_h));

		cv::Mat img{ illustration.clone() };


		// 设置高斯模糊
		cv::Mat blurMat(size_w, size_h, CV_8UC4);
		cv::GaussianBlur(img, blurMat, cv::Size(0, 0), 50);
		cv::resize(blurMat, blurMat, cv::Size(size_w, size_h));

		// 创建一个与img大小相同的矩阵
		cv::Mat blackCover{ cv::Mat::ones(cv::Size(img.cols, img.rows), CV_8UC4) };
		// 设置黑色透明图层,alpha通道为255
		blackCover = cv::Scalar{ 0, 0, 0, 255 };

		cv::Mat dstImg;
		cv::addWeighted(blackCover, 0.45, blurMat, 1 - 0.45, 0.0, dstImg); // 叠加黑色不透明图层


		// 绘制平行四边形矩形
		cv::Rect rect{ 0, 0, img.size().width - 1, img.size().height };
		DrawTool::drawParallelogram(img, rect, 74.1);


		// 创建一个全黑的相同尺寸的Mat，作为处理后的图片 
		cv::Mat blackTransparentMask{ DrawTool::createPureMat(img, 100) };

		constexpr const int img_pos_x{ 760 }, img_pos_y{ 240 };
		DrawTool::transparentPaste(dataFrame, dstImg, 80, 190);
		// DrawTool::transparentPaste(blackTransparentMask, dstImg, img_pos_x, img_pos_y + 4, 0.555f, 0.555f);
		DrawTool::transparentPaste(img, dstImg, img_pos_x, img_pos_y, 0.55f, 0.55f);
		// 裁切图像
		// 左
		// 获取裁切后的区域

		constexpr const int separate_bg_pos{ 1730 };

		cv::Rect roi{ separate_bg_pos, 0, blackTransparentMask.cols - separate_bg_pos, blackTransparentMask.rows };
		cv::Mat leftBlackTransparentMask{ blackTransparentMask(roi) };
		//右
		roi = cv::Rect{ 0, 0, blackTransparentMask.cols - separate_bg_pos, blackTransparentMask.rows };
		cv::Mat rightBlackTransparentMask{ blackTransparentMask(roi) };
		DrawTool::transparentPaste(leftBlackTransparentMask, dstImg, 0, 0, 1.0f, 1.0f);
		DrawTool::transparentPaste(rightBlackTransparentMask, dstImg, separate_bg_pos, 0, 1.0f, 1.0f);

		// ====================================

		constexpr const int avatar_offset_correlation{ 20 };

		// 玩家框
		{
			constexpr const int
				h{ 88 },
				offset_h{ 25 }, max_size_framewk{ 1700 };// 88 * tan15.9 = 25

			freetype2->loadFontData("draw/phi/font/SourceHanSansCN_SairaCondensed_Hybrid_Medium.ttf", 0);
			// + 292
			int offset_w{ 292 + freetype2->getTextSize(playerName, 48, -1, nullptr).width };

			while (offset_w > max_size_framewk) {
				std::wstring_convert<std::codecvt_utf8<wchar_t>> converter;
				std::wstring wstr = converter.from_bytes(playerName);

				wstr.pop_back();

				playerName = converter.to_bytes(wstr);
				std::string temp{ playerName + "..."s };
				offset_w = 292 + freetype2->getTextSize(temp, 48, -1, nullptr).width;
				if (offset_w > max_size_framewk + 60)
				{
					continue;
				}
				else {
					playerName = temp;
					break;
				}
			}

			freetype2->loadFontData("draw/phi/font/SourceHanSans_SairaHybridRegularHot.ttf", 0);
			cv::Mat playerForm(h, offset_w + 40 + avatar_offset_correlation, CV_8UC4, cv::Scalar(0, 0, 0, 0));

			// 定义平行四边形的四个顶点坐标
			std::vector<cv::Point> points{
				cv::Point(0, playerForm.size().height),
				cv::Point(offset_h, 0),
				cv::Point(playerForm.size().width, 0),
				cv::Point(playerForm.size().width, playerForm.size().height)
			};

			// 将顶点坐标存储到一个向量中
			std::vector<std::vector<cv::Point>> contours;
			contours.push_back(points);
			cv::drawContours(playerForm, contours, 0, cv::Scalar(0, 0, 0, 178), -1, LINE_AA);

			player_form_offset_x = size_w - playerForm.cols;
			DrawTool::transparentPaste(playerForm, dstImg, player_form_offset_x, 48);
			playerForm.release();
		}

		DrawTool::transparentPaste(playerRKSBox, dstImg, 1907 - avatar_offset_correlation, 96);

		// 玩家头像
		{
			constexpr const int
				h{ 120 },
				offset_h{ 34 };// 120 * tan15.9 = 34//向下取整

			//cv::Mat playerHead{ cv::imread("draw/test.png", cv::IMREAD_UNCHANGED) };
			cv::Mat playerHead{ !avatar_base64.empty() ? base64ToMat(avatar_base64.data()) : cv::imread("draw/phi/UnknowAvatar.png", cv::IMREAD_UNCHANGED) },
				playerHeadBox(h, 166, CV_8UC4, cv::Scalar(0, 0, 0, 0));

			cv::resize(playerHead, playerHead, cv::Size(150, 150));

			// 定义平行四边形的四个顶点坐标
			std::vector<cv::Point> points{
				cv::Point(0, playerHeadBox.size().height),
				cv::Point(offset_h, 0),
				cv::Point(playerHeadBox.size().width - 2, 0),
				cv::Point(playerHeadBox.size().width - offset_h - 2, playerHeadBox.size().height)
			},
				points1{
				cv::Point(30, 21),
				cv::Point(150 - 2, 21),
				cv::Point(120 - 2, 129),
				cv::Point(0, 129),
				cv::Point(0,150),
				cv::Point(150,150),
				cv::Point(150,0),
				cv::Point(0,0),
				cv::Point(0,129)
			};

			// 将顶点坐标存储到一个向量中
			std::vector<std::vector<cv::Point>> contours{ points };
			std::vector<std::vector<cv::Point>> contours1{ points1 };
			//reverse(contours1[0], contours1[0]);
			//contours.emplace_back(points);
			//contours1.emplace_back(points1);
			cv::drawContours(playerHeadBox, contours, 0, cv::Scalar(77, 77, 77, 255), -1, LINE_AA);

			cv::drawContours(playerHead, contours1, 0, cv::Scalar(0, 0, 0, 0), -1, LINE_AA);
			DrawTool::transparentPaste(playerHeadBox, dstImg, 1794 - avatar_offset_correlation, 33);
			DrawTool::transparentPaste(playerHead, dstImg, 1802 - avatar_offset_correlation, 17);
			playerHeadBox.release();
			playerHead.release();
		};

		int updateTimeSizeWidth{};

		// 上传日期
		{
			updateTimeSizeWidth = freetype2->getTextSize(updateTimeStr, 30, -1, nullptr).width;

			cv::Mat updateTimeBlank(cv::Size(updateTimeSizeWidth + 75, 33), CV_8UC4, cv::Scalar(0, 0, 0, 0));
			std::vector<std::vector<cv::Point>> contours{ std::vector<cv::Point>{
				cv::Point(0, updateTimeBlank.size().height),
				cv::Point(0 + 21, 0),//tan15.9 x 75 = 21.3643
				cv::Point(updateTimeBlank.size().width, 0),
				cv::Point(updateTimeBlank.size().width, updateTimeBlank.size().height)
			} };
			cv::drawContours(updateTimeBlank, contours, 0, cv::Scalar(0, 0, 0, 104), -1, LINE_AA);
			DrawTool::transparentPaste(updateTimeBlank, dstImg, size_w - updateTimeBlank.size().width, size_h - updateTimeBlank.size().height);
			updateTimeBlank.release();
		}
		// ====================================
		if (playerCourseRanking >= 500) {
			courseRating = cv::imread("draw/phi/rating/uniformSize/5.png", cv::IMREAD_UNCHANGED);
		}
		else if (playerCourseRanking >= 400) {
			courseRating = cv::imread("draw/phi/rating/uniformSize/4.png", cv::IMREAD_UNCHANGED);
		}
		else if (playerCourseRanking >= 300) {
			courseRating = cv::imread("draw/phi/rating/uniformSize/3.png", cv::IMREAD_UNCHANGED);
		}
		else if (playerCourseRanking >= 200) {
			courseRating = cv::imread("draw/phi/rating/uniformSize/2.png", cv::IMREAD_UNCHANGED);
		}
		else if (playerCourseRanking >= 100) {
			courseRating = cv::imread("draw/phi/rating/uniformSize/1.png", cv::IMREAD_UNCHANGED);
		}
		else {
			courseRating = cv::imread("draw/phi/rating/uniformSize/0.png", cv::IMREAD_UNCHANGED);
		}
		playerCourseRanking = playerCourseRanking % 100;

		DrawTool::transparentPaste(courseRating, dstImg, 1942 - avatar_offset_correlation, 49, 0.269f, 0.269f, cv::INTER_AREA);

		if (playerSocre >= 1000000) {
			rate = cv::imread("draw/phi/rating/uniformSize/phi_new.png", cv::IMREAD_UNCHANGED);
		}else if (playerIsFC) {
			rate = cv::imread("draw/phi/rating/uniformSize/V_FC.png", cv::IMREAD_UNCHANGED);
		}else if (playerSocre >= 960000) {
			rate = cv::imread("draw/phi/rating/uniformSize/V_new.png", cv::IMREAD_UNCHANGED);
		}else if (playerSocre >= 920000) {
			rate = cv::imread("draw/phi/rating/uniformSize/s_new.png", cv::IMREAD_UNCHANGED);
		}else if (playerSocre >= 880000) {
			rate = cv::imread("draw/phi/rating/uniformSize/a_new.png", cv::IMREAD_UNCHANGED);
		}else if (playerSocre >= 820000) {
			rate = cv::imread("draw/phi/rating/uniformSize/B_new.png", cv::IMREAD_UNCHANGED);
		}else if (playerSocre >= 700000) {
			rate = cv::imread("draw/phi/rating/uniformSize/C_new.png", cv::IMREAD_UNCHANGED);
		}else {
			rate = cv::imread("draw/phi/rating/uniformSize/F_new.png", cv::IMREAD_UNCHANGED);
		}
		DrawTool::transparentPaste(rate, dstImg, 600, 408);

		courseRating.release();
		rate.release();
		courseRating.release();
		illustration.release();
		rightBlackTransparentMask.release();
		leftBlackTransparentMask.release();
		blackTransparentMask.release();
		blackCover.release();
		img.release();
		blurMat.release();

		cv::Mat result(dstImg.rows, dstImg.cols, CV_8UC3);

		int from_to[] = { 0,0, 1,1, 2,2 };
		mixChannels(&dstImg, 1, &result, 1, from_to, 3);

		dstImg.release();

		// updateTime
		freetype2->putText(result, updateTimeStr, cv::Point(size_w - updateTimeSizeWidth - 34, size_h - 36), 30, cv::Scalar(255, 255, 255), -1, cv::LINE_AA, false);

		std::string  PlayerCourse{ std::to_string(playerCourseRanking) };
		int font_offset_x{ 1970 };
		if (PlayerCourse.length() == 1)
		{
			font_offset_x = 1982;
		}
		// course
		freetype2->putText(result, std::move(PlayerCourse), cv::Point(font_offset_x - avatar_offset_correlation, 78), 36, cv::Scalar(255, 255, 255), -1, cv::LINE_AA, true);

		font_offset_x = 1945;
		std::string player_rks_str{ OtherUtil::retainDecimalPlaces(player_rks) };
		if (player_rks_str.length() == 4)
		{
			font_offset_x = 1950;
		}
		// rks
		freetype2->putText(result, std::move(player_rks_str), cv::Point(font_offset_x - avatar_offset_correlation, 122), 32, cv::Scalar(0, 0, 0), -1, cv::LINE_AA, true);

		std::string song_name{ api_data.at("record").at("title").get<std::string>() };
		{
			int offset_w{ freetype2->getTextSize(song_name, 51, -1, nullptr).width };
			while (offset_w > 570) {
				std::wstring_convert<std::codecvt_utf8<wchar_t>> converter;
				std::wstring wstr = converter.from_bytes(song_name);

				wstr.pop_back();

				song_name = converter.to_bytes(wstr);
				std::string temp{ song_name + "..."s };
				offset_w = freetype2->getTextSize(temp, 51, -1, nullptr).width;

				if (offset_w <= 570)
				{
					song_name = temp;
					break;
				}
			}
			// song title
			freetype2->putText(result, song_name, cv::Point(323, 318), 51, cv::Scalar(255, 255, 255), -1, cv::LINE_AA, true);
			std::string artist{ api_data.at("record").at("artist").get<std::string>() };
			offset_w = freetype2->getTextSize(artist, 28, -1, nullptr).width;
			while (offset_w > 390) {
				std::wstring_convert<std::codecvt_utf8<wchar_t>> converter;
				std::wstring wstr = converter.from_bytes(artist);

				wstr.pop_back();

				artist = converter.to_bytes(wstr);
				std::string temp{ artist + "..."s };
				offset_w = freetype2->getTextSize(temp, 28, -1, nullptr).width;

				if (offset_w <= 390)
				{
					artist = temp;
					break;
				}
			}
			// artist
			freetype2->putText(result, artist, cv::Point(321, 352), 28, cv::Scalar(255, 255, 255), -1, cv::LINE_AA, true);


			std::string song_level{ api_data.at("record").at("difficulty").get<std::string>() + " Lv."s + OtherUtil::retainDecimalPlaces(api_data.at("record").at("rating").get<double>(), 1) };
			offset_w = freetype2->getTextSize(song_level, 28, -1, nullptr).width;
			// lv
			freetype2->putText(result, song_level, cv::Point(872 - offset_w, 352), 28, cv::Scalar(255, 255, 255), -1, cv::LINE_AA, true);

			// rate
			freetype2->putText(result, OtherUtil::retainDecimalPlaces(api_data.at("record").at("rks").get<double>()), cv::Point(231, 763), 40, cv::Scalar(255, 255, 255), -1, cv::LINE_AA, true);
			
			std::string acc_str{ OtherUtil::retainDecimalPlaces(api_data.at("record").at("acc").get<double>()) + "%"s };

			offset_w = freetype2->getTextSize(acc_str, 40, -1, nullptr).width;
			// acc
			freetype2->putText(result, acc_str, cv::Point(727 - offset_w, 763), 40, cv::Scalar(255, 255, 255), -1, cv::LINE_AA, true);
		}
		// other symbols
		freetype2->putText(result, "Best Score", cv::Point(268, 548), 28, cv::Scalar(255, 255, 255), -1, cv::LINE_AA, true);
		freetype2->putText(result, "Rate", cv::Point(228, 791), 24, cv::Scalar(255, 255, 255), -1, cv::LINE_AA, true);
		freetype2->putText(result, "Accuracy", cv::Point(623, 791), 24, cv::Scalar(255, 255, 255), -1, cv::LINE_AA, true);

		// score
		freetype2->putText(result, OtherUtil::digitSupplementHandle(playerSocre), cv::Point(256, 538) + cv::Point(3, 2), 84, cv::Scalar(67, 67, 67), -1, cv::LINE_AA, false);
		freetype2->putText(result, OtherUtil::digitSupplementHandle(playerSocre), cv::Point(256, 538), 84, cv::Scalar(255, 255, 255), -1, cv::LINE_AA, false);

		freetype2->loadFontData("draw/phi/font/SourceHanSansCN_SairaCondensed_Hybrid_Medium.ttf", 0);

		// player name
		freetype2->putText(result, playerName, cv::Point(player_form_offset_x + 52, 107), 48, cv::Scalar(255, 255, 255), -1, cv::LINE_AA, true);

		freetype2->loadFontData("draw/phi/font/AdobeStdR.otf", 0);
		freetype2->putText(result, "Powerd by yuhao7370. Generated by tomato Team - Phigros.", cv::Point(20, size_h - 36) + cv::Point(3, 4), 28, cv::Scalar(5, 5, 5), -1, cv::LINE_AA, false);
		freetype2->putText(result, "Powerd by yuhao7370. Generated by tomato Team - Phigros.", cv::Point(20, size_h - 36), 28, cv::Scalar(255, 255, 255), -1, cv::LINE_AA, false);
		freetype2.release();

		return result;
	}
};

#endif // !PHIGROS_SERVICE_HPP