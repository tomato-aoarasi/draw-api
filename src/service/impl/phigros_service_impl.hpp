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
#include <array>
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
#include <cpprest/http_client.h>
#include <cpprest/json.h>

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
	// 字体定义
	inline static const std::string
		sairaHybridRegularHot{ "draw/phi/font/SourceHanSans_SairaHybridRegularHot.ttf" }, 
		adobeStdR{ "draw/phi/font/AdobeStdR.otf" },
		sairaCondensedHybridMedium{"draw/phi/font/SourceHanSansCN_SairaCondensed_Hybrid_Medium.ttf"},
		playoffProCond { "draw/phi/font/PlayoffProCond.ttf" },
		SourceHanSansCNMedium90Y { "draw/phi/font/SourceHanSansCN-Medium-90Y.ttf" };

	inline static cv::Mat
		phigros{ cv::imread("draw/phi/Phigros.png", cv::IMREAD_UNCHANGED) },
		shadow{ cv::imread("draw/phi/Shadow.png", cv::IMREAD_UNCHANGED) },
		shadow_corner{ cv::imread("draw/phi/ShadowCorner.png", cv::IMREAD_UNCHANGED) },
		playerRKSBox{ cv::imread("draw/phi/rksBox.png", cv::IMREAD_UNCHANGED) },
		dataFrame{ cv::imread("draw/phi/DataFrame.png", cv::IMREAD_UNCHANGED) },
		personal_single_song_info_shadow{ cv::imread("draw/phi/PersonalSingleSongInfoShadow.png",cv::IMREAD_UNCHANGED) },
		b19_record1{ cv::imread("draw/phi/record1.png",cv::IMREAD_UNCHANGED) },
		b19_record_sign{ cv::imread("draw/phi/record_sign.png",cv::IMREAD_UNCHANGED) },
		b19_shadow{ cv::Mat(cv::Size(345, 189), CV_8UC4) },
		info_style2_shadow{ cv::Mat(cv::Size(2048, 1080), CV_8UC4) },
		unknow { cv::imread("draw/phi/Unknow.png",cv::IMREAD_UNCHANGED) },
		overflow { cv::imread("draw/phi/overflow.png",cv::IMREAD_UNCHANGED) },
		b19_background{ cv::imread("draw/phi/background_p.png",cv::IMREAD_UNCHANGED) },
		collect_box{ cv::imread("draw/phi/collect_box.png",cv::IMREAD_UNCHANGED) },
		collect_sign{ cv::imread("draw/phi/button/收集品icon.png",cv::IMREAD_UNCHANGED) },
		info_modern_form_style2{ cv::imread("draw/phi/info_modern_form_style2.png",cv::IMREAD_UNCHANGED) };
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
		}
		else if (img.type() == 0) {
			cv::cvtColor(img, img, cv::COLOR_GRAY2BGR);
		}
		else if (img.type() == 24) {
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
	};
public:
	~PhigrosServiceImpl() {
		phigros.release();
		shadow.release();
		shadow_corner.release();
		personal_single_song_info_shadow.release();
		dataFrame.release();
		playerRKSBox.release();
		b19_record1.release();
		b19_record_sign.release();
		b19_shadow.release();
		b19_background.release();
		unknow.release();
		overflow.release();
		collect_box.release();
		info_modern_form_style2.release();
		info_style2_shadow.release();
		collect_sign.release();
	};

	PhigrosServiceImpl() {
		cv::flip(shadow_corner, shadow_corner, -1);
		{
			cv::Mat b19_shadow(this->b19_shadow.size(), CV_8UC4, cv::Scalar(0, 0, 0, 0));
			// 定义渐变的起始和结束颜色
			cv::Scalar startColor(0, 0, 0, 0);
			cv::Scalar endColor(0, 0, 0, 242);

			constexpr const double init_pos_ratio{ 0.1 };
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
		{
			cv::Mat info_style2_shadow(this->info_style2_shadow.size(), CV_8UC4, cv::Scalar(0, 0, 0, 0));
			// 定义渐变的起始和结束颜色
			cv::Scalar startColor(0, 0, 0, 0);
			cv::Scalar endColor(0, 0, 0, 204);

			constexpr const double init_pos_ratio{ 0.8 };
			for (int y{ (int)(info_style2_shadow.rows * init_pos_ratio) }; y < info_style2_shadow.rows; y++)
			{
				double ratio{ ((double)y - info_style2_shadow.rows * init_pos_ratio) / (info_style2_shadow.rows - info_style2_shadow.rows * init_pos_ratio) };
				// 计算当前行的颜色
				cv::Scalar color{ startColor * (1 - ratio) + endColor * (ratio) };

				// std::cout << color << std::endl;

				// 绘制矩形
				cv::rectangle(info_style2_shadow, cv::Point(0, y), cv::Point(info_style2_shadow.cols, y), color, -1, cv::LINE_AA);
			}
			info_style2_shadow.copyTo(this->info_style2_shadow);
			info_style2_shadow.release();
		}
	};

	inline cv::Mat drawSongInfomation(Json info_param, bool isQRCode, std::string_view QRCodeContent, const std::string& authorization) override {
		constexpr std::chrono::seconds timeout{ 30s }; // 设置超时时间为 20 秒
		constexpr const size_t size_w{ 2048 }, size_h{ 1080 };

		Json data;

		web::http::client::http_client_config config;
		config.set_timeout(timeout);

		web::http::client::http_client client(U(Global::PhiAPI), config);
		// 创建第一个HTTP请求, 添加匹配索引
		web::http::http_request request_add_index(web::http::methods::POST);
		request_add_index.set_request_uri("/phi/song"s);
		request_add_index.headers().add("Content-Type", "application/json");
		request_add_index.headers().add("Authorization", "Bearer "s + authorization);
		request_add_index.set_body(web::json::value::parse(info_param.dump()));

		auto response = client.request(request_add_index).get();

		{
			auto status_code{ response.status_code() };
			if (status_code >= 300 or status_code < 200) {
				data = Json::parse(response.extract_json().get().serialize());
				std::string msg{ (data.count("detail") ? data.at("detail").get<std::string>() : "") };
				uint16_t status{ data.at("status").get<uint16_t>() };
				throw self::HTTPException(msg, status_code, status);
			}
		}

		data = Json::parse(response.extract_json().get().serialize());

		std::string illustrationPath{ Global::PhiResourcePath + data["illustrationPath"].get<std::string>() };


		// 读取素材
		cv::Mat img{ cv::imread(std::move(illustrationPath), cv::IMREAD_UNCHANGED) };
		cv::resize(img, img, cv::Size(size_w, size_h));


		std::vector<cv::Mat> diff;
		std::vector<cv::Mat> diffSign;

		// rating note design
		std::unordered_map<std::string, std::string> diffText;
		std::vector<decltype(diffText)> diffTexts;

		{
			std::array<std::string, 6>difficulties{ "ez", "hd", "in", "at", "lg", "sp" };

			for (auto& difficulty : difficulties) {
				if (data["flag"][difficulty].get<bool>()) {
					auto rating{ OtherUtil::retainDecimalPlaces(data["content"][difficulty]["rating"].get<float>(), 1) };

					diffText["rating"] = rating == "-1.0"s ? "  ?"s : rating;
					diffText["note"] = std::to_string(data["content"][difficulty]["note"].get<int>());
					diffText["design"] = data["content"][difficulty]["design"].get<std::string>();

					std::transform(difficulty.begin(), difficulty.end(), difficulty.begin(), ::toupper);
					diff.emplace_back(cv::imread("draw/phi/diff/"s + difficulty + ".png"s, cv::IMREAD_UNCHANGED));
					diffSign.emplace_back(cv::imread("draw/phi/diff/DiifFont/"s + difficulty + ".png"s, cv::IMREAD_UNCHANGED));
					diffTexts.emplace_back(diffText);
				}
			}
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
		freetype2->loadFontData(this->sairaHybridRegularHot, 0);

		//freetype(freetype2);

		// 添加中文文字
		const std::string
			chapter{ "Chapter:  "s + data["chapter"].get<std::string>() },
			artist{ "Artist:  "s + data["artist"].get<std::string>() },
			illustration{ "Illustration:  "s + data["illustration"].get<std::string>() },
			duration{ "Duration:  "s + data["duration"].get<std::string>() },
			bpm{ "BPM:  "s + data["bpm"].get<std::string>() },
			title{ data["title"].get<std::string>() };

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

		freetype2->loadFontData(this->adobeStdR, 0);

		freetype2->putText(result, "Generated by tomato Team - Phigros", cv::Point(18, 1070) + cv::Point(3, 2), 25, cv::Scalar(5, 5, 5), -1, cv::LINE_AA, true);
		freetype2->putText(result, "Generated by tomato Team - Phigros", cv::Point(18, 1070), 25, cv::Scalar(255, 255, 255), -1, cv::LINE_AA, true);

		freetype2.release();


		return std::move(result);

	};

	inline cv::Mat drawPlayerSingleInfo(std::string_view song_id, Ubyte level,
		std::string_view auth_token, std::string_view player_session_token, std::string_view avatar_base64, bool is_game_avatar) override {
		constexpr std::chrono::seconds timeout{ 30s };
		//Json dataId{}, songData{}, playerData{};
		Json data{};

		web::http::client::http_client_config config;
		config.set_timeout(timeout);

		web::http::client::http_client client(U(Global::PhiAPI), config);

		web::uri_builder builder(U("/phi/best"));
		// 参数添加
		builder.append_query(U("songid"), U(song_id.data()));

		// 创建第一个HTTP请求, 添加匹配索引
		web::http::http_request request_add_index(web::http::methods::GET);
		request_add_index.set_request_uri(builder.to_string());
		request_add_index.headers().add("Content-Type", "application/json");
		request_add_index.headers().add("Authorization", "Bearer "s + auth_token.data());
		request_add_index.headers().add("SessionToken", player_session_token.data());

		auto response = client.request(request_add_index).get();

		{
			auto status_code{ response.status_code() };
			if (status_code >= 300 or status_code < 200) {
				data = Json::parse(response.extract_json().get().serialize());
				std::string msg{ (data.count("detail") ? data.at("detail").get<std::string>() : "") };
				uint16_t status{ data.at("status").get<uint16_t>() };
				throw self::HTTPException(msg, status_code, status);
			}
		}

		data = Json::parse(response.extract_json().get().serialize());

		std::string illustrationPath{ Global::PhiResourcePath + data["content"]["record"]["illustrationPath"].get<std::string>()};

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
		freetype2->loadFontData(this->SourceHanSansCNMedium90Y, 0);

		std::exchange(data, data["content"]);

		std::string
			playerName{ data["playerNickname"].get<std::string>() },
			playerRKS{ OtherUtil::retainDecimalPlaces(data["rankingScore"].get<float>()) };
		int playerSocre{ data["record"]["score"].get<int>() },
			playerCourseRanking{ data["challengeModeRank"].get<int>() };
		bool playerIsFC{ data["record"]["isfc"].get<bool>() };

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
			cv::Mat playerHead,
				playerHeadBox(h, 166, CV_8UC4, cv::Scalar(0, 0, 0, 0));

			const bool avatarHasEnable{ data.at("other").at("avatarHasEnable").get<bool>() };

			//cv::Mat playerHead{ cv::imread("draw/test.png", cv::IMREAD_UNCHANGED) };
			if (is_game_avatar) {
				playerHead = avatarHasEnable
					?
					cv::imread(Global::PhiResourcePath + data.at("other").at("avatarPath").get<std::string>(), cv::IMREAD_UNCHANGED)
					:
					cv::imread("draw/phi/UnknowAvatar.png", cv::IMREAD_UNCHANGED);
			}
			else {
				playerHead = !avatar_base64.empty()
					?
					base64ToMat(avatar_base64.data())
					:
					cv::imread("draw/phi/UnknowAvatar.png", cv::IMREAD_UNCHANGED);
			}

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

		illustration.release();
		img.release();
		rate.release();
		img.release();

		freetype2->loadFontData(this->SourceHanSansCNMedium90Y, 0);

		std::string songRating{ data["record"]["difficulty"].get<std::string>() + ": "s + OtherUtil::retainDecimalPlaces(data["record"]["rating"].get<float>(),1) };

		freetype2->putText(result, songRating, cv::Point(117, 357) + cv::Point(5, 3), 56, cv::Scalar(5, 5, 5), -1, cv::LINE_AA, true);
		freetype2->putText(result, songRating, cv::Point(117, 357), 56, cv::Scalar(255, 255, 255), -1, cv::LINE_AA, true);

		freetype2->putText(result, OtherUtil::digitSupplementHandle(playerSocre), cv::Point(246, 970), 96, cv::Scalar(255, 255, 255), -1, cv::LINE_AA, true);
		freetype2->putText(result, "Rate: "s + OtherUtil::retainDecimalPlaces(data["record"]["rks"].get<float>()), cv::Point(584, 937), 40, cv::Scalar(255, 255, 255), -1, cv::LINE_AA, true);
		freetype2->putText(result, "Acc: "s + OtherUtil::retainDecimalPlaces(data["record"]["acc"].get<float>()) + "%"s, cv::Point(579, 975), 32, cv::Scalar(255, 255, 255), -1, cv::LINE_AA, true);

		std::string  PlayerCourse{ std::to_string(playerCourseRanking) };
		int font_offset_x{ 1975 };
		if (PlayerCourse.length() == 1)
		{
			font_offset_x = 1985;
		}

		freetype2->putText(result, PlayerCourse, cv::Point(font_offset_x, 78), 36, cv::Scalar(255, 255, 255), -1, cv::LINE_AA, true);

		// 加粗字体
		freetype2->loadFontData(this->sairaHybridRegularHot, 0);

		font_offset_x = 1945;
		if (playerRKS.length() == 4)
		{
			font_offset_x = 1950;
		}
		freetype2->putText(result, playerRKS, cv::Point(font_offset_x, 122), 32, cv::Scalar(0, 0, 0), -1, cv::LINE_AA, true);


		// 中等字体
		freetype2->loadFontData(this->SourceHanSansCNMedium90Y, 0);
		freetype2->putText(result, data["record"]["title"], cv::Point(119, 272) + cv::Point(5, 3), 84, cv::Scalar(5, 5, 5), -1, cv::LINE_AA, true);
		freetype2->putText(result, data["record"]["title"], cv::Point(119, 272), 84, cv::Scalar(255, 255, 255), -1, cv::LINE_AA, true);

		freetype2->putText(result, playerName, cv::Point(player_form_offset_x + 52, 107), 48, cv::Scalar(255, 255, 255), -1, cv::LINE_AA, true);

		freetype2->loadFontData(this->adobeStdR, 0);

		freetype2->putText(result, "Generated by tomato Team - Phigros.", cv::Point(1, 1059), 16, cv::Scalar(255, 255, 255), -1, cv::LINE_AA, false);
		freetype2.release();

		return std::move(result);
	}

	inline cv::Mat drawPlayerSingleInfoModernStyle(std::string_view song_id, Ubyte level,
		std::string_view auth_token, std::string_view player_session_token, std::string_view avatar_base64, bool is_game_avatar) override {
		Json api_data{};
		
		// ======================================

		constexpr std::chrono::seconds timeout{ 30s };

		web::http::client::http_client_config config;
		config.set_timeout(timeout);

		web::http::client::http_client client(U(Global::PhiAPI), config);

		web::uri_builder builder(U("/phi/best"));
		// 参数添加
		builder.append_query(U("songid"), U(song_id.data()));

		// 创建第一个HTTP请求, 添加匹配索引
		web::http::http_request request_add_index(web::http::methods::GET);
		request_add_index.set_request_uri(builder.to_string());
		request_add_index.headers().add("Content-Type", "application/json");
		request_add_index.headers().add("Authorization", "Bearer "s + auth_token.data());
		request_add_index.headers().add("SessionToken", player_session_token.data());

		auto response = client.request(request_add_index).get();

		{
			auto status_code{ response.status_code() };
			if (status_code >= 300 or status_code < 200) {
				api_data = Json::parse(response.extract_json().get().serialize());
				std::string msg{ (api_data.count("detail") ? api_data.at("detail").get<std::string>() : "") };
				uint16_t status{ api_data.at("status").get<uint16_t>() };
				throw self::HTTPException(msg, status_code, status);
			}
		}

		api_data = Json::parse(response.extract_json().get().serialize());
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

			freetype2->loadFontData(this->SourceHanSansCNMedium90Y, 0);
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

			freetype2->loadFontData(this->sairaHybridRegularHot, 0);
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

			cv::Mat playerHead,
				playerHeadBox(h, 166, CV_8UC4, cv::Scalar(0, 0, 0, 0));

			const bool avatarHasEnable{ api_data.at("other").at("avatarHasEnable").get<bool>() };

			//cv::Mat playerHead{ cv::imread("draw/test.png", cv::IMREAD_UNCHANGED) };
			if (is_game_avatar) {
				playerHead = avatarHasEnable
					? 
					cv::imread(Global::PhiResourcePath + api_data.at("other").at("avatarPath").get<std::string>(), cv::IMREAD_UNCHANGED)
					: 
					cv::imread("draw/phi/UnknowAvatar.png", cv::IMREAD_UNCHANGED);
			}
			else {
				playerHead = !avatar_base64.empty()
					? 
					base64ToMat(avatar_base64.data())
					: 
					cv::imread("draw/phi/UnknowAvatar.png", cv::IMREAD_UNCHANGED);
			}


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

		freetype2->loadFontData(this->SourceHanSansCNMedium90Y, 0);

		// player name
		freetype2->putText(result, playerName, cv::Point(player_form_offset_x + 52, 107), 48, cv::Scalar(255, 255, 255), -1, cv::LINE_AA, true);

		freetype2->loadFontData(this->adobeStdR, 0);
		freetype2->putText(result, "Generated by tomato Team - Phigros.", cv::Point(20, size_h - 36) + cv::Point(3, 4), 28, cv::Scalar(5, 5, 5), -1, cv::LINE_AA, false);
		freetype2->putText(result, "Generated by tomato Team - Phigros.", cv::Point(20, size_h - 36), 28, cv::Scalar(255, 255, 255), -1, cv::LINE_AA, false);
		freetype2.release();

		return result;
	}
	
	inline cv::Mat drawPlayerSingleInfoModernStyle2(std::string_view song_id, Ubyte level,
		std::string_view auth_token, std::string_view player_session_token, std::string_view avatar_base64, bool is_game_avatar) override {
		Json api_data{};
		
		// ======================================
		
		constexpr std::chrono::seconds timeout{ 30s };

		web::http::client::http_client_config config;
		config.set_timeout(timeout);

		web::http::client::http_client client(U(Global::PhiAPI), config);

		web::uri_builder builder(U("/phi/best"));
		// 参数添加
		builder.append_query(U("songid"), U(song_id.data()));

		// 创建第一个HTTP请求, 添加匹配索引
		web::http::http_request request_add_index(web::http::methods::GET);
		request_add_index.set_request_uri(builder.to_string());
		request_add_index.headers().add("Content-Type", "application/json");
		request_add_index.headers().add("Authorization", "Bearer "s + auth_token.data());
		request_add_index.headers().add("SessionToken", player_session_token.data());

		auto response = client.request(request_add_index).get();

		{
			auto status_code{ response.status_code() };
			if (status_code >= 300 or status_code < 200) {
				api_data = Json::parse(response.extract_json().get().serialize());
				std::string msg{ (api_data.count("detail") ? api_data.at("detail").get<std::string>() : "") };
				uint16_t status{ api_data.at("status").get<uint16_t>() };
				throw self::HTTPException(msg, status_code, status);
			}
		}

		api_data = Json::parse(response.extract_json().get().serialize());
		std::exchange(api_data, api_data.at("content"));

		// ======================================

		int playerSocre{ api_data.at("record").at("score").get<int>() },
			playerCourseRanking{ api_data.at("challengeModeRank").get<int>()};
		bool playerIsFC{ api_data.at("record").at("isfc").get<bool>() };

		std::string
			playerName{ api_data.at("playerNickname").get<std::string>() };
		double player_rks{ api_data.at("rankingScore").get<double>() };
		int player_form_offset_x{};

		constexpr const int size_w{ 2048 }, size_h{ 1080 };
		cv::Ptr<freetype::FreeType2> freetype2{ cv::freetype::createFreeType2() };
		cv::Mat illustration{ cv::imread(Global::PhiResourcePath + api_data.at("record").at("illustrationPath").get<std::string>(),cv::IMREAD_UNCHANGED)};

		cv::Mat rate{ }, courseRating{ };
		cv::resize(illustration, illustration, cv::Size(size_w, size_h));

		cv::Mat img{ illustration.clone() };


		// 设置高斯模糊
		cv::GaussianBlur(illustration, illustration, cv::Size(0, 0), 50);

		DrawTool::transparentPaste(this->info_modern_form_style2, illustration);

		// 绘制平行四边形矩形
		cv::Mat shadow{ info_style2_shadow.clone() };
		cv::Rect rect{ 0, 0, img.size().width - 1, img.size().height };
		DrawTool::drawParallelogram2(img, rect, 74.1);
		DrawTool::drawParallelogram2(shadow, rect, 74.1);

		constexpr const int img_pos_x{ 796 }, img_pos_y{ 275 };
		// DrawTool::transparentPaste(blackTransparentMask, dstImg, img_pos_x, img_pos_y + 4, 0.555f, 0.555f);
		
		constexpr const float scale_rate{ 603.0f / 1024.0f }, phi_scale_rate{ 0.8f }, phi_sign_scale_rate{ 0.5f };
		
		DrawTool::transparentPaste(img, illustration, img_pos_x, img_pos_y, scale_rate, scale_rate);
		DrawTool::transparentPaste(shadow, illustration, img_pos_x, img_pos_y, scale_rate, scale_rate);
		DrawTool::transparentPaste(this->collect_box, illustration, 764, 973);
		DrawTool::transparentPaste(this->collect_sign, illustration, 936, 982, phi_sign_scale_rate, phi_sign_scale_rate);

		DrawTool::transparentPaste(this->phigros, illustration, 64, 56, phi_scale_rate, phi_scale_rate);
		// 裁切图像
		// 左
		// 获取裁切后的区域
		// ====================================

		constexpr const int avatar_offset_correlation{ 20 };

		// 玩家框
		{
			constexpr const int
				h{ 88 },
				offset_h{ 25 }, max_size_framewk{ 1700 };// 88 * tan15.9 = 25

			freetype2->loadFontData(this->SourceHanSansCNMedium90Y, 0);
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

			freetype2->loadFontData(this->sairaHybridRegularHot, 0);
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
			DrawTool::transparentPaste(playerForm, illustration, player_form_offset_x, 48);
			playerForm.release();
		}

		DrawTool::transparentPaste(playerRKSBox, illustration, 1907 - avatar_offset_correlation, 96);

		// 玩家头像
		{
			constexpr const int
				h{ 120 },
				offset_h{ 34 };// 120 * tan15.9 = 34//向下取整

			cv::Mat playerHead,
				playerHeadBox(h, 166, CV_8UC4, cv::Scalar(0, 0, 0, 0));

			const bool avatarHasEnable{ api_data.at("other").at("avatarHasEnable").get<bool>() };

			//cv::Mat playerHead{ cv::imread("draw/test.png", cv::IMREAD_UNCHANGED) };
			if (is_game_avatar) {
				playerHead = avatarHasEnable
					? 
					cv::imread(Global::PhiResourcePath + api_data.at("other").at("avatarPath").get<std::string>(), cv::IMREAD_UNCHANGED)
					: 
					cv::imread("draw/phi/UnknowAvatar.png", cv::IMREAD_UNCHANGED);
			}
			else {
				playerHead = !avatar_base64.empty()
					? 
					base64ToMat(avatar_base64.data())
					: 
					cv::imread("draw/phi/UnknowAvatar.png", cv::IMREAD_UNCHANGED);
			}


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
			DrawTool::transparentPaste(playerHeadBox, illustration, 1794 - avatar_offset_correlation, 33);
			DrawTool::transparentPaste(playerHead, illustration, 1802 - avatar_offset_correlation, 17);
			playerHeadBox.release();
			playerHead.release();
		};

		int updateTimeSizeWidth{};

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

		DrawTool::transparentPaste(courseRating, illustration, 1942 - avatar_offset_correlation, 49, 0.269f, 0.269f, cv::INTER_AREA);

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
		DrawTool::transparentPaste(rate, illustration, 606, 301);

		courseRating.release();
		rate.release();
		img.release();
		shadow.release();

		cv::Mat result(illustration.rows, illustration.cols, CV_8UC3);

		int from_to[] = { 0,0, 1,1, 2,2 };
		mixChannels(&illustration, 1, &result, 1, from_to, 3);

		illustration.release();

		// updateTime
		{
			freetype2->putText(result, "Upload Time", cv::Point(971, 1000), 20, cv::Scalar(255, 255, 255), -1, cv::LINE_AA, true);

			const std::string updateTimeStr{ api_data.at("updateTime").get<std::string>() };
			constexpr const int font_size{ 28 }; 
			const int w_length{ freetype2->getTextSize(updateTimeStr, font_size, -1, nullptr).width };
			
			freetype2->putText(result, updateTimeStr, cv::Point(1009 - (w_length / 2), 1034), font_size, cv::Scalar(255, 255, 255), -1, cv::LINE_AA, true);

		}
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
		freetype2->putText(result, "Best Score", cv::Point(216, 568), 28, cv::Scalar(255, 255, 255), -1, cv::LINE_AA, true);
		freetype2->putText(result, "Rate", cv::Point(228, 791), 24, cv::Scalar(255, 255, 255), -1, cv::LINE_AA, true);
		freetype2->putText(result, "Accuracy", cv::Point(623, 791), 24, cv::Scalar(255, 255, 255), -1, cv::LINE_AA, true);

		// score
		freetype2->putText(result, OtherUtil::digitSupplementHandle(playerSocre), cv::Point(210, 444), 84, cv::Scalar(255, 255, 255), -1, cv::LINE_AA, false);

		freetype2->loadFontData(this->SourceHanSansCNMedium90Y, 0);

		// player name
		freetype2->putText(result, playerName, cv::Point(player_form_offset_x + 52, 107), 48, cv::Scalar(255, 255, 255), -1, cv::LINE_AA, true);

		freetype2->putText(result, "Generated by tomato Team - Phigros.", cv::Point(20, size_h - 36) + cv::Point(3, 4), 28, cv::Scalar(5, 5, 5), -1, cv::LINE_AA, false);
		freetype2->putText(result, "Generated by tomato Team - Phigros.", cv::Point(20, size_h - 36), 28, cv::Scalar(255, 255, 255), -1, cv::LINE_AA, false);
		freetype2.release();

		return result;
	}

	inline cv::Mat drawB19(std::string_view auth_token, std::string_view  player_session_token, std::string_view avatar_base64, bool is_game_avatar) override {
		Json api_data{};
		
		// ======================================
		constexpr std::chrono::seconds timeout{ 30s };

		web::http::client::http_client_config config;
		config.set_timeout(timeout);

		web::http::client::http_client client(U(Global::PhiAPI), config);

		// 创建第一个HTTP请求, 添加匹配索引
		web::http::http_request request_add_index(web::http::methods::GET);
		request_add_index.set_request_uri(U("/phi/all"));
		request_add_index.headers().add("Content-Type", "application/json");
		request_add_index.headers().add("Authorization", "Bearer "s + auth_token.data());
		request_add_index.headers().add("SessionToken", player_session_token.data());

		auto response = client.request(request_add_index).get();

		{
			auto status_code{ response.status_code() };
			if (status_code >= 300 or status_code < 200) {
				api_data = Json::parse(response.extract_json().get().serialize());
				std::string msg{ (api_data.count("detail") ? api_data.at("detail").get<std::string>() : "") };
				uint16_t status{ api_data.at("status").get<uint16_t>() };
				throw self::HTTPException(msg, status_code, status);
			}
		}

		api_data = Json::parse(response.extract_json().get().serialize());
		std::exchange(api_data, api_data.at("content"));

		//std::cout << api_data << std::endl;

		std::vector<Json> player_all_data = api_data.at("best_list").at("best").get<std::vector<Json>>();

		std::size_t all_data_size{ player_all_data.size() };
		bool is_phi{ api_data.at("best_list").at("is_phi").get<bool>() };

		// =======================================================
		cv::Ptr<freetype::FreeType2> freetype2{ cv::freetype::createFreeType2() };
		cv::Mat draw{ b19_background.clone() };
		cv::Mat illustration_shadow{ b19_shadow.clone() };
		cv::Mat unknow{ this->unknow.clone()};
		cv::resize(unknow, unknow, illustration_shadow.size());

		DrawTool::transparentPaste(phigros, draw, 117, 47, 0.8f, 0.8f);
		int playerCourseRanking{ api_data.at("challengeModeRank").get<int>() };

		constexpr const int overflow_line_y{ 2266 };

		cv::Mat courseRating{ };

		switch (playerCourseRanking / 100)
		{
		case 1:
			courseRating = cv::imread("draw/phi/rating/uniformSize/1.png", cv::IMREAD_UNCHANGED);
			break;
		case 2:
			courseRating = cv::imread("draw/phi/rating/uniformSize/2.png", cv::IMREAD_UNCHANGED);
			break;
		case 3:
			courseRating = cv::imread("draw/phi/rating/uniformSize/3.png", cv::IMREAD_UNCHANGED);
			break;
		case 4:
			courseRating = cv::imread("draw/phi/rating/uniformSize/4.png", cv::IMREAD_UNCHANGED);
			break;
		case 5:
			courseRating = cv::imread("draw/phi/rating/uniformSize/5.png", cv::IMREAD_UNCHANGED);
			break;
		default:
			courseRating = cv::imread("draw/phi/rating/uniformSize/0.png", cv::IMREAD_UNCHANGED);
			break;
		}
		playerCourseRanking = playerCourseRanking % 100;

		DrawTool::transparentPaste(courseRating, draw, 600, 220, 0.21f, 0.21f, cv::INTER_AREA);

		// 玩家框
		{
			cv::Mat playerFormRKS(cv::Size(90, 25), CV_8UC4, cv::Scalar(0, 0, 0, 0));

			const int height_rks{ static_cast<int>(playerFormRKS.rows * std::tan(15.9 * std::numbers::pi / 180.0)) }; // 96 * tan(15.9°) = 27.346319361909209215659939610795
			std::vector<cv::Point> points_rks{
				cv::Point(0, playerFormRKS.rows),
				cv::Point(height_rks, 0),
				cv::Point(playerFormRKS.cols - 3, 0),
				cv::Point(playerFormRKS.cols - height_rks - 3, playerFormRKS.rows)
			};
			std::vector<std::vector<cv::Point>> contours_rks;
			contours_rks.emplace_back(points_rks);
			cv::drawContours(playerFormRKS, contours_rks, 0, cv::Scalar(255, 255, 255, 255), -1, LINE_AA);

			DrawTool::transparentPaste(playerFormRKS, draw, 590, 258);
			playerFormRKS.release();

			cv::Mat playerForm(cv::Size(520, 85), CV_8UC4, cv::Scalar(0, 0, 0, 0));
			// 定义平行四边形的四个顶点坐标

			const int height{ static_cast<int>(playerForm.rows * std::tan(15.9 * std::numbers::pi / 180.0 ))}; // 96 * tan(15.9°) = 27.346319361909209215659939610795

			std::vector<cv::Point> points{
				cv::Point(0, playerForm.rows),
				cv::Point(height, 0),
				cv::Point(playerForm.cols - 2, 0),
				cv::Point(playerForm.cols - height - 2, playerForm.rows)
			};

			// 将顶点坐标存储到一个向量中
			std::vector<std::vector<cv::Point>> contours;
			contours.emplace_back(points);
			cv::drawContours(playerForm, contours, 0, cv::Scalar(0, 0, 0, 178), -1, LINE_AA);
			
			DrawTool::transparentPaste(playerForm, draw, 90, 213);

			playerForm.release();
		}

		{
			constexpr const int h{ 108 };

			// cv::Mat player_avatar{ !avatar_base64.empty() ? base64ToMat(avatar_base64.data()) : cv::imread("draw/phi/UnknowAvatar.png", cv::IMREAD_UNCHANGED) };

			// ========================
			cv::Mat player_avatar;

			const bool avatarHasEnable{ api_data.at("other").at("avatarHasEnable").get<bool>() };

			if (is_game_avatar) {
				player_avatar = avatarHasEnable
					?
					cv::imread(Global::PhiResourcePath + api_data.at("other").at("avatarPath").get<std::string>(), cv::IMREAD_UNCHANGED)
					:
					cv::imread("draw/phi/UnknowAvatar.png", cv::IMREAD_UNCHANGED);
			}
			else {
				player_avatar = !avatar_base64.empty()
					?
					base64ToMat(avatar_base64.data())
					:
					cv::imread("draw/phi/UnknowAvatar.png", cv::IMREAD_UNCHANGED);
			}
			// ========================

			cv::resize(player_avatar, player_avatar, cv::Size(135, 135));

			int avatar_height_offset{ static_cast<int>(player_avatar.rows * std::tan(15.9 * std::numbers::pi / 180.0) / 2.0 ) };

			// 定义平行四边形的四个顶点坐标
			std::vector<cv::Point> points{
				cv::Point(player_avatar.cols - h, avatar_height_offset),
				cv::Point(player_avatar.cols, avatar_height_offset),
				cv::Point(h, player_avatar.rows - avatar_height_offset),
				cv::Point(0, player_avatar.rows - avatar_height_offset),
				cv::Point(0,player_avatar.rows),
				cv::Point(player_avatar.cols,player_avatar.rows),
				cv::Point(player_avatar.cols,0),
				cv::Point(0,0),
				cv::Point(0,player_avatar.rows - avatar_height_offset)
			};

			// 将顶点坐标存储到一个向量中
			std::vector<std::vector<cv::Point>> contours{ points };
			cv::drawContours(player_avatar, contours, 0, cv::Scalar(0, 0, 0, 0), -1, LINE_AA);
			DrawTool::transparentPaste(player_avatar, draw, 107, 187);
			player_avatar.release();
		};

		// 曲绘处理相关
		{
			// TODO
			//=================================================
			
			int width_offset{ static_cast<int>(illustration_shadow.rows * std::tan(15.9 * std::numbers::pi / 180.0)) };

			// 定义平行四边形的四个顶点坐标
			std::vector<cv::Point> points{
				cv::Point(0, illustration_shadow.rows),
				cv::Point(0 + width_offset, 0),
				cv::Point(illustration_shadow.cols - 1, 0),
				cv::Point(illustration_shadow.cols - width_offset - 1, illustration_shadow.rows),
				cv::Point(illustration_shadow.cols,illustration_shadow.rows),
				cv::Point(illustration_shadow.cols,0),
				cv::Point(illustration_shadow.cols,0),
				cv::Point(0,0),
				cv::Point(0,illustration_shadow.rows)
			};

			// 将顶点坐标存储到一个向量中
			std::vector<std::vector<cv::Point>> contours{ points };
			cv::drawContours(illustration_shadow, contours, 0, cv::Scalar(0, 0, 0, 0), -1, LINE_AA);
			cv::drawContours(unknow, contours, 0, cv::Scalar(0, 0, 0, 0), -1, LINE_AA);

			//=================================================
			DrawTool::transparentPaste(b19_record1, draw, 45, 613);
			DrawTool::transparentPaste(b19_record_sign, draw, 45, 613);

			DrawTool::transparentPaste(cv::imread("draw/phi/rating/uniformSize/phi_old.png", cv::IMREAD_UNCHANGED), draw, 516, 644, 0.6f, 0.6f);
			DrawTool::transparentPaste(b19_record1, draw, 406, 613);

			constexpr const int rate_offset_x{ 233 }, rate_offset_y{ 122 }, difficulty_box_offset_x{ 60 }, difficulty_box_offset_y{ 10 };

			// 含phi记录
			if (is_phi) {
				int socre{ api_data.at("best_list").at("phi").at("score").get<int>() };
				bool is_fc{ api_data.at("best_list").at("phi").at("isfc").get<bool>() };
				cv::Mat illustration{ cv::imread(Global::PhiResourcePath + api_data.at("best_list").at("phi").at("illustrationPath").get<std::string>(),cv::IMREAD_UNCHANGED) }, rate{ }, difficulty_box(cv::Size(48, 21), CV_8UC4, cv::Scalar(0, 0, 0, 0));

				std::string difficulty{api_data.at("best_list").at("phi").at("difficulty").get<std::string>()};
				int difficulty_width_offset{ static_cast<int>(difficulty_box.rows * std::tan(15.9 * std::numbers::pi / 180.0)) };
				std::vector<std::vector<cv::Point>> difficulty_contours{std::vector<cv::Point>{
					cv::Point(0 + difficulty_width_offset, 0),
					cv::Point(difficulty_box.cols - 1, 0),
					cv::Point(difficulty_box.cols - difficulty_width_offset - 1, difficulty_box.rows),
					cv::Point(0, difficulty_box.rows)}
				};

				// 难度上色
				if (difficulty == "EZ") {
					cv::drawContours(difficulty_box, difficulty_contours, 0, cv::Scalar(68, 195, 38, 255), -1, LINE_AA);
				}else if (difficulty == "HD") {
					cv::drawContours(difficulty_box, difficulty_contours, 0, cv::Scalar(209, 137, 89, 255), -1, LINE_AA);
					//difficulty_box
				}else if (difficulty == "IN") {
					cv::drawContours(difficulty_box, difficulty_contours, 0, cv::Scalar(41, 25, 191, 255), -1, LINE_AA);
					//difficulty_box
				}else if (difficulty == "AT") {
					cv::drawContours(difficulty_box, difficulty_contours, 0, cv::Scalar(74, 74, 74, 255), -1, LINE_AA);
					//difficulty_box
				} 
				/*
				else { // LG (wait...what?)
					cv::drawContours(difficulty_box, difficulty_contours, 0, cv::Scalar(164, 164, 164, 255), -1, LINE_AA);
					//difficulty_box
				}
				*/ 

				if (socre >= 1000000) {
					rate = cv::imread("draw/phi/rating/uniformSize/phi_new.png", cv::IMREAD_UNCHANGED);
				}
				else if (is_fc) {
					rate = cv::imread("draw/phi/rating/uniformSize/V_FC.png", cv::IMREAD_UNCHANGED);
				}
				else if (socre >= 960000) {
					rate = cv::imread("draw/phi/rating/uniformSize/V_new.png", cv::IMREAD_UNCHANGED);
				}
				else if (socre >= 920000) {
					rate = cv::imread("draw/phi/rating/uniformSize/s_new.png", cv::IMREAD_UNCHANGED);
				}
				else if (socre >= 880000) {
					rate = cv::imread("draw/phi/rating/uniformSize/a_new.png", cv::IMREAD_UNCHANGED);
				}
				else if (socre >= 820000) {
					rate = cv::imread("draw/phi/rating/uniformSize/B_new.png", cv::IMREAD_UNCHANGED);
				}
				else if (socre >= 700000) {
					rate = cv::imread("draw/phi/rating/uniformSize/C_new.png", cv::IMREAD_UNCHANGED);
				}
				else {
					rate = cv::imread("draw/phi/rating/uniformSize/F_new.png", cv::IMREAD_UNCHANGED);
				}



				cv::resize(illustration, illustration, illustration_shadow.size(), 0.0, 0.0, InterpolationFlags::INTER_LINEAR);
				cv::drawContours(illustration, contours, 0, cv::Scalar(0, 0, 0, 0), -1, LINE_AA);
				DrawTool::transparentPaste(illustration, draw, 422, 622);
				DrawTool::transparentPaste(illustration_shadow, draw, 422, 622);
				DrawTool::transparentPaste(difficulty_box, draw, 422 + difficulty_box_offset_x, 622 + difficulty_box_offset_y);
				DrawTool::transparentPaste(rate, draw, 422 + rate_offset_x, 622 + rate_offset_y, 0.26f, 0.26f);
				illustration.release();
				difficulty_box.release();
				rate.release();
			}
			// 曲目记录(不含phi)
			for (int item{ 0 }; item < 22; ++item) {
				int column{ (item + 2) % 3 }, row{ (item + 2) / 3 },
					row_temp{ item < 19 ? 622 + 228 * row : overflow_line_y };
				cv::Mat rate{ }, difficulty_box(cv::Size(48, 21), CV_8UC4, cv::Scalar(0, 0, 0, 0));


				if(item <= all_data_size - 1) {
					int socre{ player_all_data.at(item).at("score").get<int>() };
					bool is_fc{ player_all_data.at(item).at("isfc").get<bool>() };
					std::string difficulty{ player_all_data.at(item).at("difficulty").get<std::string>() };
					cv::Mat illustration{ cv::imread(Global::PhiResourcePath + player_all_data.at(item).at("illustrationPath").get<std::string>(), cv::IMREAD_UNCHANGED) };
					
					int difficulty_width_offset{ static_cast<int>(difficulty_box.rows * std::tan(15.9 * std::numbers::pi / 180.0)) };
					std::vector<std::vector<cv::Point>> difficulty_contours{ std::vector<cv::Point>{
						cv::Point(0 + difficulty_width_offset, 0),
						cv::Point(difficulty_box.cols - 1, 0),
						cv::Point(difficulty_box.cols - difficulty_width_offset - 1, difficulty_box.rows),
						cv::Point(0, difficulty_box.rows)}
					};

					// 难度上色
					if (difficulty == "EZ") {
						cv::drawContours(difficulty_box, difficulty_contours, 0, cv::Scalar(68, 195, 38, 255), -1, LINE_AA);
					}
					else if (difficulty == "HD") {
						cv::drawContours(difficulty_box, difficulty_contours, 0, cv::Scalar(209, 137, 89, 255), -1, LINE_AA);
						//difficulty_box
					}
					else if (difficulty == "IN") {
						cv::drawContours(difficulty_box, difficulty_contours, 0, cv::Scalar(41, 25, 191, 255), -1, LINE_AA);
						//difficulty_box
					}
					else if (difficulty == "AT") {
						cv::drawContours(difficulty_box, difficulty_contours, 0, cv::Scalar(74, 74, 74, 255), -1, LINE_AA);
						//difficulty_box
					}
					/*
					else {// LG (wait...what?)
						cv::drawContours(difficulty_box, difficulty_contours, 0, cv::Scalar(164, 164, 164, 255), -1, LINE_AA);
						//difficulty_box
					}
					*/

					if (socre >= 1000000) {
						rate = cv::imread("draw/phi/rating/uniformSize/phi_new.png", cv::IMREAD_UNCHANGED);
					}
					else if (is_fc) {
						rate = cv::imread("draw/phi/rating/uniformSize/V_FC.png", cv::IMREAD_UNCHANGED);
					}
					else if (socre >= 960000) {
						rate = cv::imread("draw/phi/rating/uniformSize/V_new.png", cv::IMREAD_UNCHANGED);
					}
					else if (socre >= 920000) {
						rate = cv::imread("draw/phi/rating/uniformSize/s_new.png", cv::IMREAD_UNCHANGED);
					}
					else if (socre >= 880000) {
						rate = cv::imread("draw/phi/rating/uniformSize/a_new.png", cv::IMREAD_UNCHANGED);
					}
					else if (socre >= 820000) {
						rate = cv::imread("draw/phi/rating/uniformSize/B_new.png", cv::IMREAD_UNCHANGED);
					}
					else if (socre >= 700000) {
						rate = cv::imread("draw/phi/rating/uniformSize/C_new.png", cv::IMREAD_UNCHANGED);
					}
					else {
						rate = cv::imread("draw/phi/rating/uniformSize/F_new.png", cv::IMREAD_UNCHANGED);
					}

					cv::resize(illustration, illustration, illustration_shadow.size(), 0.0, 0.0, InterpolationFlags::INTER_LINEAR);
					cv::drawContours(illustration, contours, 0, cv::Scalar(0, 0, 0, 0), -1, LINE_AA);

					DrawTool::transparentPaste(illustration, draw, 61 + column * 361, row_temp);
					illustration.release();
				} else {
					DrawTool::transparentPaste(unknow, draw, 61 + column * 361, row_temp);
				}
				
				DrawTool::transparentPaste(illustration_shadow, draw, 61 + column * 361, row_temp); 
				if (!rate.empty()) {
					DrawTool::transparentPaste(rate, draw, 61 + column * 361 + rate_offset_x, row_temp + rate_offset_y, 0.26f, 0.26f);
				}
				DrawTool::transparentPaste(difficulty_box, draw, 61 + column * 361 + difficulty_box_offset_x, row_temp + difficulty_box_offset_y);
				
				difficulty_box.release();
				rate.release();
			}
		}

		DrawTool::transparentPaste(overflow, draw, 0, 2200);
		// =======================================================

		illustration_shadow.release();
		courseRating.release();
		unknow.release();
		
		// =======================================================
		cv::Mat result(draw.size(), CV_8UC3);

		int from_to[] = { 0,0, 1,1, 2,2 };
		mixChannels(&draw, 1, &result, 1, from_to, 3);
		draw.release();
		freetype2->loadFontData(this->sairaHybridRegularHot, 0);

		const std::string updateTimeStr{ "Upload Time: "s + api_data.at("updateTime").get<std::string>() };
		constexpr const int updateSize{ 24 };
		const int updateTimeSizeWidth{ freetype2->getTextSize(updateTimeStr, updateSize, -1, nullptr).width };

		freetype2->putText(result, updateTimeStr, cv::Point(1080 - updateTimeSizeWidth, 95), updateSize, cv::Scalar(255, 255, 255), -1, cv::LINE_AA, false);
		
		{
			constexpr const int max_size_w{ 322 },font_size{ 36 };
			std::string player_nickname{ api_data.at("playerNickname").get<std::string>() };
			int offset_w{ freetype2->getTextSize(player_nickname, font_size, -1, nullptr).width };
			while (offset_w > max_size_w) {
				std::wstring_convert<std::codecvt_utf8<wchar_t>> converter;
				std::wstring wstr = converter.from_bytes(player_nickname);

				wstr.pop_back();

				player_nickname = converter.to_bytes(wstr);
				std::string temp{ player_nickname + "..."s };
				offset_w = freetype2->getTextSize(temp, font_size, -1, nullptr).width;

				if (offset_w <= max_size_w){
					player_nickname = temp;
					break;
				}
			}
			freetype2->putText(result, player_nickname, cv::Point(248, 233), font_size, cv::Scalar(255, 255, 255), -1, cv::LINE_AA, false);
			//OtherUtil::Println("width size:", freetype2->getTextSize(player_nickname, font_size, -1, nullptr).width);
		}
		{
			std::string player_course_ranking_rks_str{ OtherUtil::retainDecimalPlaces(api_data.at("rankingScore").get<double>()) };
			constexpr const int rks_font_size { 24 };

			const int player_rks_width_length{ freetype2->getTextSize(player_course_ranking_rks_str, rks_font_size, -1, nullptr).width };
			freetype2->putText(result, player_course_ranking_rks_str, cv::Point(632 - player_rks_width_length / 2, 255), rks_font_size, cv::Scalar(0, 0, 0), -1, cv::LINE_AA, false);
		
			std::string player_course_ranking_str{std::to_string(playerCourseRanking)};
			constexpr const int font_size{ 28 };

			const int player_course_ranking_width_length{ freetype2->getTextSize(player_course_ranking_str, font_size, -1, nullptr).width };
			freetype2->putText(result, player_course_ranking_str, cv::Point(640 - player_course_ranking_width_length / 2, 212), font_size, cv::Scalar(255, 255, 255), -1, cv::LINE_AA, false);
		}

		// profile
		{
			std::string profile { api_data.at("other").at("profile").get<std::string>()};

			if (!profile.empty()){
				//constexpr const std::size_t max_length{ 240 };
				//std::wstring_convert<std::codecvt_utf8<wchar_t>> converter;
				//std::wstring wstr_profile{ converter.from_bytes(profile) };
				//if (wstr_profile.length() > max_length)
				//{
				//	profile = converter.to_bytes(wstr_profile.substr(0, max_length - 1));
				//}

				constexpr const int font_size{ 21 }, max_size_w{ 545 };
				std::vector<std::string> profiles{ };
				// 存在\n\r
				if (profile.find("\r\n") != std::string::npos){
					profiles = OtherUtil::split(profile, "\r\n");
				}// 存在\n
				else {
					profiles = OtherUtil::split(profile, "\n");
				}

				std::vector<std::string> profiles_handled{ };

				// 存放std:string 与 位置
				std::vector<std::string>temps{};

				// 最大容许多少行
				constexpr const uint8_t max_line{ 8 };
				// 当前行
				uint8_t current_line{ 0 };

				// 字符串转换需要
				std::wstring_convert<std::codecvt_utf8<wchar_t>> converter;

				for (const auto& element : profiles) {
					if (current_line >= max_line) { break; }
					// 如果flag为true说明了多行插入
					bool flag{ false };

					int offset_w{ freetype2->getTextSize(element, font_size, -1, nullptr).width };

					std::wstring wstr{ converter.from_bytes(element) };

					for (int i = 0; i < wstr.length(); i++) {
						std::string temp = converter.to_bytes(wstr.substr(0, i + 1));
						cv::Size text_size = freetype2->getTextSize(temp, font_size, -1, nullptr); // 获取文本的大小
						if (text_size.width > max_size_w) { // 如果文本宽度超过最大宽度
							if (current_line >= max_line) { break; }
							temps.emplace_back(converter.to_bytes(wstr.substr(0, i)) + " "s);
							wstr = wstr.substr(i); // 剩余文本
							i = 0; // 重新开始计数
							flag = true;
							++current_line;
						}
					}

					if (current_line < max_line and flag) {
						temps.emplace_back(converter.to_bytes(wstr) + " "s);
						wstr.clear();
						++current_line;
					}

					// 一段vector多行分割
					if (flag) {
						for (const auto& str : temps) {
							profiles_handled.emplace_back(str + " "s);
						}
						temps.clear();
					}
					// 一段vector一行分割
					else {
						profiles_handled.emplace_back(element + " "s);
						++current_line;
					}
				}

				int h{ 0 };

				const std::size_t
					length{ profiles_handled.size() },
					max_rows{ length <= 7 ? length : 7 };

				const int profile_height_length{ freetype2->getTextSize(profile, font_size, -1, nullptr).height };
				constexpr const int magnification{ 2 };
				int init_h{ 460 };
				if (length & 1) {
					init_h -= profile_height_length / 2;
				}
				init_h -= profile_height_length * (max_rows / 2) * magnification;


				//OtherUtil::Println(init_h, profile_height_length);
				for (std::size_t row{ 0 }; row < max_rows; ++row) {
					//奇数
					freetype2->putText(result, profiles_handled.at(row), cv::Point(110, init_h + h), font_size, cv::Scalar(255, 255, 255), -1, cv::LINE_AA, false);
					h += profile_height_length * magnification;
				}
			} else {
				freetype2->putText(result, "Failed data acquisition", cv::Point(110, 435), 42, cv::Scalar(255, 255, 255), -1, cv::LINE_AA, false);
			}

		}

		// record
		{
			freetype2->putText(result, "\\        EZ   HD    IN    AT", cv::Point(151, 633), 22, cv::Scalar(255, 255, 255), -1, cv::LINE_AA, false);
			freetype2->putText(result, "Cleared", cv::Point(119, 679), 18, cv::Scalar(255, 255, 255), -1, cv::LINE_AA, false);
			freetype2->putText(result, "FC", cv::Point(108, 719), 18, cv::Scalar(255, 149, 82), -1, cv::LINE_AA, false); //BGR
			freetype2->putText(result, "Phi", cv::Point(97, 759), 18, cv::Scalar(95, 255, 255), -1, cv::LINE_AA, false);

			// record
			{
				Json record_data{ api_data.at("other").at("records") };

				record_data.swap(record_data.at(0));

				//OtherUtil::Println(record_data);

				// 0:EZ 1:HD 2:IN 3:AT
				// 0:Cleared 1:FC 2:Phi
				std::array<std::array<uint16_t, 4>, 3> records
				{
					std::array<uint16_t, 4>{record_data["EZ"]["clear"].get<uint16_t>(), record_data["HD"]["clear"].get<uint16_t>(), record_data["IN"]["clear"].get<uint16_t>(), record_data["AT"]["clear"].get<uint16_t>()},
					std::array<uint16_t, 4>{record_data["EZ"]["fc"].get<uint16_t>(), record_data["HD"]["fc"].get<uint16_t>(), record_data["IN"]["fc"].get<uint16_t>(), record_data["AT"]["fc"].get<uint16_t>()},
					std::array<uint16_t, 4>{record_data["EZ"]["phi"].get<uint16_t>(), record_data["HD"]["phi"].get<uint16_t>(), record_data["IN"]["phi"].get<uint16_t>(), record_data["AT"]["phi"].get<uint16_t>()}
				};
				// x - l / 2

				constexpr const int font_size{ 16 };

				for (int row{ 0 }; row < records.size(); ++row) {
					for (int column{ 0 }; column < records.at(row).size(); ++column){
						std::string count{ std::to_string(records.at(row).at(column)) };
						int w_length{ freetype2->getTextSize(count, font_size, -1, nullptr).width };

						int pos_x{ (205 + column * 44 - 10 * row) - (w_length / 2) },pos_y{ 683 + row * 39 };

						freetype2->putText(result, count, cv::Point(pos_x, pos_y), font_size, cv::Scalar(255, 255, 255), -1, cv::LINE_AA, false);
					}
				}
				
			}
		}

		// 游戏成绩
		{
			const int graph_width{ b19_shadow.cols };
			// phi
			if (is_phi) {
				// 422, 622  # h - 2

				// title
				{
					constexpr const int max_size_w{ 200 }, font_size{ 18 };
					std::string title{ api_data.at("best_list").at("phi").at("title").get<std::string>() };
					int offset_w{ freetype2->getTextSize(title, font_size, -1, nullptr).width };

					while (offset_w > max_size_w) {
						std::wstring_convert<std::codecvt_utf8<wchar_t>> converter;
						std::wstring wstr = converter.from_bytes(title);

						wstr.pop_back();

						title = converter.to_bytes(wstr);
						std::string temp{ title + "..."s };
						offset_w = freetype2->getTextSize(temp, font_size, -1, nullptr).width;

						if (offset_w <= max_size_w) {
							title = temp;
							break;
						}
					}


					// ranking
					/*{
						constexpr const int font_size{ 12 };
						std::string ranking{ "IN Lv."s + OtherUtil::retainDecimalPlaces(api_data.at("best_list").at("phi").at("level").get<double>(), 1) };
						freetype2->putText(result, ranking, cv::Point(422 + 26 + offset_w + 8, 622 + 124), font_size, cv::Scalar(255, 255, 255), -1, cv::LINE_AA, false);
					}*/

					freetype2->putText(result, title, cv::Point(422 + 26, 622 + 118), font_size, cv::Scalar(255, 255, 255), -1, cv::LINE_AA, false);
				}

				// ranking
				{
					constexpr const int font_size{ 16 }, init_w_offset_x{ 82 }, init_w_offset_y{ 11 };
					std::string ranking{ OtherUtil::retainDecimalPlaces(api_data.at("best_list").at("phi").at("level").get<double>(), 1) };
					int offset_w{ freetype2->getTextSize(ranking, font_size, -1, nullptr).width };
					freetype2->putText(result, ranking, cv::Point(422 + init_w_offset_x - offset_w / 2, 622 + init_w_offset_y), font_size, cv::Scalar(255, 255, 255), -1, cv::LINE_AA, false);
				}

				// score
				{
					constexpr const int font_size{ 21 };
					std::string score{ std::to_string(api_data.at("best_list").at("phi").at("score").get<int>()) };
					const int text_width{ freetype2->getTextSize(score, font_size, -1, nullptr).width };
					freetype2->putText(result, score, cv::Point(422 + 241 - text_width, 622 + 159), font_size, cv::Scalar(255, 255, 255), -1, cv::LINE_AA, false);
				}

				// acc & rks
				{
					constexpr const int font_size{ 18 };
					std::string
						rks{ OtherUtil::retainDecimalPlaces(api_data.at("best_list").at("phi").at("rankingSocre").get<double>()) },
						acc{ OtherUtil::retainDecimalPlaces(api_data.at("best_list").at("phi").at("acc").get<double>()) + "%"s };
					freetype2->putText(result, rks, cv::Point(422 + 21, 622 + 143), font_size, cv::Scalar(255, 255, 255), -1, cv::LINE_AA, false);
					freetype2->putText(result, acc, cv::Point(422 + 76, 622 + 143), font_size, cv::Scalar(255, 255, 255), -1, cv::LINE_AA, false);
				}

				// symbol
				{
					freetype2->putText(result, "Rate", cv::Point(422 + 19, 622 + 163), 12, cv::Scalar(255, 255, 255), -1, cv::LINE_AA, false);
					freetype2->putText(result, "Accuracy", cv::Point(422 + 74, 622 + 165), 10, cv::Scalar(255, 255, 255), -1, cv::LINE_AA, false);
				}
			}

			// 正常
			for (int item{ 0 }; item < 22; ++item) {
				int column{ (item + 2) % 3 }, row{ (item + 2) / 3 },
					row_temp{ item < 19 ? 622 + 228 * row : overflow_line_y };
				// #标记
				/*if (item >= 1)*/ {
					constexpr const int font_size{ 25 };
					std::string text{ "#"s + std::to_string(item + 1) };
					int w_length{ freetype2->getTextSize(text, font_size, -1, nullptr).width };
					freetype2->putText(result, text, cv::Point(column * 361 + graph_width - w_length + 56, row_temp - 26), font_size, cv::Scalar(255, 255, 255), -1, cv::LINE_AA, false);
				}

				// 其他记录
				if (item <= all_data_size - 1) {
					//61 + column * 361, row_temp
					// title
					{
						constexpr const int max_size_w{ 200 }, font_size{ 18 };
						std::string title{ player_all_data.at(item).at("title").get<std::string>() };
						int offset_w{ freetype2->getTextSize(title, font_size, -1, nullptr).width };

						while (offset_w > max_size_w) {
							std::wstring_convert<std::codecvt_utf8<wchar_t>> converter;
							std::wstring wstr = converter.from_bytes(title);

							wstr.pop_back();

							title = converter.to_bytes(wstr);
							std::string temp{ title + "..."s };
							offset_w = freetype2->getTextSize(temp, font_size, -1, nullptr).width;

							if (offset_w <= max_size_w) {
								title = temp;
								break;
							}
						}

						// ranking
						/* {
							constexpr const int font_size{ 12 };
							std::string ranking{ "IN Lv."s + OtherUtil::retainDecimalPlaces(player_all_data.at(item).at("level").get<double>(), 1) };
							freetype2->putText(result, ranking, cv::Point(61 + column * 361 + 26 + offset_w + 8, row_temp + 124), font_size, cv::Scalar(255, 255, 255), -1, cv::LINE_AA, false);
						}
						*/

						freetype2->putText(result, title, cv::Point(61 + column * 361 + 26, row_temp + 118), font_size, cv::Scalar(255, 255, 255), -1, cv::LINE_AA, false);
					}

					// ranking
					{
						constexpr const int font_size{ 15 }, init_w_offset_x{ 82 }, init_w_offset_y{ 11 };
						std::string ranking{ OtherUtil::retainDecimalPlaces(player_all_data.at(item).at("level").get<double>(), 1) };
						int offset_w{ freetype2->getTextSize(ranking, font_size, -1, nullptr).width };
						freetype2->putText(result, ranking, cv::Point(61 + column * 361 + init_w_offset_x - offset_w / 2, row_temp + init_w_offset_y), font_size, cv::Scalar(255, 255, 255), -1, cv::LINE_AA, false);
					}

					// score
					{
						constexpr const int font_size{ 21 };
						std::string score{ OtherUtil::digitSupplementHandle(player_all_data.at(item).at("score").get<int>()) };
						const int text_width{ freetype2->getTextSize(score, font_size, -1, nullptr).width };
						freetype2->putText(result, score, cv::Point(61 + column * 361 + 241 - text_width, row_temp + 159), font_size, cv::Scalar(255, 255, 255), -1, cv::LINE_AA, false);
					}

					// acc & rks
					{
						constexpr const int font_size{ 18 };
						std::string
							rks{ OtherUtil::retainDecimalPlaces(player_all_data.at(item).at("rankingSocre").get<double>()) },
							acc{ OtherUtil::retainDecimalPlaces(player_all_data.at(item).at("acc").get<double>()) + "%"s };
						freetype2->putText(result, rks, cv::Point(61 + column * 361 + 21, row_temp + 143), font_size, cv::Scalar(255, 255, 255), -1, cv::LINE_AA, false);
						freetype2->putText(result, acc, cv::Point(61 + column * 361 + 76, row_temp + 143), font_size, cv::Scalar(255, 255, 255), -1, cv::LINE_AA, false);
					}

					// symbol
					{
						freetype2->putText(result, "Rate", cv::Point(61 + column * 361 + 19, row_temp + 163), 12, cv::Scalar(255, 255, 255), -1, cv::LINE_AA, false);
						freetype2->putText(result, "Accuracy", cv::Point(61 + column * 361 + 74, row_temp + 165), 10, cv::Scalar(255, 255, 255), -1, cv::LINE_AA, false);
					}
				}
			}
		}

		// 签名
		{
			const std::string signature_str{ "Generated by tomato Team - Phigros" };
			constexpr const int font_size{ 28 };
			const int offset_w{ freetype2->getTextSize(signature_str, font_size, -1, nullptr).width };

			freetype2->putText(result, signature_str, cv::Point(result.cols / 2 - offset_w / 2, 2490), font_size, cv::Scalar(255, 255, 255), -1, cv::LINE_AA, false);
		}
		freetype2.reset();
		freetype2.release();
		return result;
	}
};

#endif // !PHIGROS_SERVICE_HPP