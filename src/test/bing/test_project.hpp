#pragma once

#ifndef TEST_PROJECT_HPP
#define TEST_PROJECT_HPP  
#include <iostream>
#include <fstream>
#include <exception>
#include <stdexcept>
#include <filesystem>
#include <common/utils/sql_handle.hpp>
#include "common/utils/other_util.hpp"
#include <span>
#include <jwt-cpp/jwt.h>
#include <bcrypt.h>
#include "dao/bottle.hpp"
#include <sqlite_modern_cpp.h>
#include "crow.h"
#include "common/utils/draw_tool.hpp"
#include <opencv2/opencv.hpp>
#include <qrencode.h>
#include <hiredis/hiredis.h>
#include <common/utils/http_util.hpp>
#include <controller/phigros_controller.hpp>

using namespace std::string_literals;

//#define WARNING_CONTENT  
using _uint64 = unsigned long long int;
using StatusCodeHandle = HTTPUtil::StatusCodeHandle;

class TestProject final{
public:
	static void TESTMemory(void) {
		crow::SimpleApp app;
		app.bindaddr("0.0.0.0").port(9961);

		CROW_ROUTE(app, "/test")
			.methods("GET"_method)([](const crow::request& req) {
			int song_id{};
			crow::response response;
			if (OtherUtil::verifyParam(req, "songId")) {
				song_id = std::stoi(req.url_params.get("songId"));
			}
			else {
				response.set_header("Content-Type", "application/json");
				response.code = 400;
				response.write(StatusCodeHandle::getSimpleJsonResult(400, "parameter 'songId' required and parameter cannot be empty.").dump(amount_spaces));
				return response;
			}

			response.add_header("Cache-Control", "no-cache");
			response.add_header("Pragma", "no-cache");
			std::string QRCodeContent = "Hello";

			//设置超过200ms后API请求超时
			constexpr std::chrono::milliseconds timeout{ 2000ms };

			Json data;

			//获取API数据到data
			OtherUtil::asyncGetAPI(data, timeout, "http://150.158.89.12:6680"s, "/api/pgr/findBySong?id="s + std::to_string(song_id));

			std::string illustrationPath = { "/home/Soft/NginxWeb/html/main/phigros/" + data[0]["song_illustration_url"].get<std::string>()};
			// 读取素材
			cv::Mat img{ cv::imread(std::move(illustrationPath), cv::IMREAD_UNCHANGED) },
				phigros{ cv::imread("draw/phi/Phigros.png", cv::IMREAD_UNCHANGED) },
				shadow{ cv::imread("draw/phi/Shadow.png", cv::IMREAD_UNCHANGED) },
				shadow_corner{ cv::imread("draw/phi/ShadowCorner.png", cv::IMREAD_UNCHANGED) };
			std::vector<cv::Mat> diff;
			std::vector<cv::Mat> diffSign;

			// 0.EZ 1.HD 2.IN 3.AT 4.LG 5.SP
			std::vector<bool> diffMarker(6, false);

			if (!data[0]["design"]["ez"].is_null()) {
				diff.emplace_back(cv::imread("draw/phi/diff/EZ.png", cv::IMREAD_UNCHANGED));
				diffSign.emplace_back(cv::imread("draw/phi/diff/DiifFont/EZ.png", cv::IMREAD_UNCHANGED));
				diffMarker.at(0) = true;
			}
			if (!data[0]["design"]["hd"].is_null()) {
				diff.emplace_back(cv::imread("draw/phi/diff/HD.png", cv::IMREAD_UNCHANGED));
				diffSign.emplace_back(cv::imread("draw/phi/diff/DiifFont/HD.png", cv::IMREAD_UNCHANGED));
				diffMarker.at(1) = true;
			}
			if (!data[0]["design"]["in"].is_null()) {
				diff.emplace_back(cv::imread("draw/phi/diff/IN.png", cv::IMREAD_UNCHANGED));
				diffSign.emplace_back(cv::imread("draw/phi/diff/DiifFont/IN.png", cv::IMREAD_UNCHANGED));
				diffMarker.at(2) = true;
			}
			if (!data[0]["design"]["at"].is_null()) {
				diff.emplace_back(cv::imread("draw/phi/diff/AT.png", cv::IMREAD_UNCHANGED));
				diffSign.emplace_back(cv::imread("draw/phi/diff/DiifFont/AT.png", cv::IMREAD_UNCHANGED));
				diffMarker.at(3) = true;
			}
			if (!data[0]["design"]["lg"].is_null()) {
				diff.emplace_back(cv::imread("draw/phi/diff/LG.png", cv::IMREAD_UNCHANGED));
				diffSign.emplace_back(cv::imread("draw/phi/diff/DiifFont/LG.png", cv::IMREAD_UNCHANGED));
				diffMarker.at(4) = true;
			}
			if (!data[0]["design"]["sp"].is_null()) {
				diff.emplace_back(cv::imread("draw/phi/diff/SP.png", cv::IMREAD_UNCHANGED));
				diffSign.emplace_back(cv::imread("draw/phi/diff/DiifFont/SP.png", cv::IMREAD_UNCHANGED));
				diffMarker.at(5) = true;
			}
			size_t diffCount{ diff.size() };

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
			if (false) {
				cv::Mat QRCode{ DrawTool::DrawQRcode(QRCodeContent.data(),true,2,2,6,6) };
				DrawTool::transparentPaste(QRCode, dstImg, 0, 0, 0.8, 0.8);
				DrawTool::transparentPaste(shadow_corner, dstImg, 0, 0, 0.2f, 0.2f);
				QRCode.deallocate();
			}

			// 添加难度框的阴影
			cv::Mat diffBoxShadow{ DrawTool::createPureMat(diff.at(0), 15) };

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
			}

			int h{ 1296 + offset_x }, v{ 112 + offset_y };

			// std::cout << data << std::endl;
			for (size_t i{ 0 }; i < diffCount; ++i) {
				DrawTool::transparentPaste(diffBoxShadow, dstImg, h + 1, v, 1.005f, 1.015f);
				DrawTool::transparentPaste(diff.at(i), dstImg, h - 2, v - 5, 1.0f, 1.0f);
				DrawTool::transparentPaste(diffSign.at(i), dstImg, h + sign_offset_x + 1, v + sign_offset_y, 0.4f, 0.4f);
				h += hc; v += vc;
				diff.at(i).deallocate();
				diffSign.at(i).deallocate();
			}
			if (diffCount == 3 || diffCount == 2 || diffCount == 1) {
				DrawTool::transparentPaste(phigros, dstImg, 1240, 800, 1.0f, 1.0f);
			}
			else if (diffCount == 4) {
				DrawTool::transparentPaste(phigros, dstImg, 1280, 900, 0.8f, 0.8f);
			}
			else {
			}


			// 文字处理添加
			cv::Mat result(dstImg.rows, dstImg.cols, CV_8UC3);
			blurMat.deallocate();
			blackCover.deallocate();
			shadow.deallocate();
			img.deallocate();
			blackTransparentMask.deallocate();
			leftBlackTransparentMask.deallocate();
			rightBlackTransparentMask.deallocate();
			shadow_corner.deallocate();
			diffBoxShadow.deallocate();
			phigros.deallocate();
			dstImg.deallocate();
			//cv::Mat result{ addFont(dstImg, std::move(data), X_OFFSETINIT, Y_OFFSETINIT) };

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


			{
				//freetype2->loadFontData("draw/phi/font/AdobeStdR.otf", 0);
				constexpr int HC{ -39 }, VC{ 228 };
				int h{ 1318 + X_OFFSETINIT }, v{ 112 + Y_OFFSETINIT }, offset_x{ 195 }, offset_y{ 108 };
				freetype2->putText(result, "6.0  / 275", Point(h + offset_x, v + offset_y) + cv::Point(3, 2), 72, Scalar(5, 5, 5), -1, cv::LINE_AA, true);
				freetype2->putText(result, "6.0  / 275", Point(h + offset_x, v + offset_y), 72, Scalar(215, 215, 215), -1, cv::LINE_AA, true);

				freetype2->putText(result, "Chart:  .---- ----. ---.. / -. . .-. (百九十八 / NerSAN)", Point(h - 13, v + offset_y + 70) + cv::Point(2, 1), 24, Scalar(0, 0, 0), -1, cv::LINE_AA, true);
				freetype2->putText(result, "Chart:  .---- ----. ---.. / -. . .-. (百九十八 / NerSAN)", Point(h - 13, v + offset_y + 70), 24, Scalar(200, 200, 200), -1, cv::LINE_AA, true);

				h += HC; v += VC;
				freetype2->putText(result, "11.1 / 595", Point(h + offset_x, v + offset_y) + cv::Point(3, 2), 72, Scalar(5, 5, 5), -1, cv::LINE_AA, true);
				freetype2->putText(result, "11.1 / 595", Point(h + offset_x, v + offset_y), 72, Scalar(215, 215, 215), -1, cv::LINE_AA, true);

				freetype2->putText(result, "Chart:  Jαckψ feat. Βαρβαροςαντρας", Point(h - 13, v + offset_y + 70) + cv::Point(2, 1), 24, Scalar(0, 0, 0), -1, cv::LINE_AA, true);
				freetype2->putText(result, "Chart:  Jαckψ feat. Βαρβαροςαντρας", Point(h - 13, v + offset_y + 70), 24, Scalar(200, 200, 200), -1, cv::LINE_AA, true);

				h += HC; v += VC;
				freetype2->putText(result, "14.7 / 7328", Point(h + offset_x, v + offset_y) + cv::Point(3, 2), 72, Scalar(5, 5, 5), -1, cv::LINE_AA, true);
				freetype2->putText(result, "14.7 / 7328", Point(h + offset_x, v + offset_y), 72, Scalar(215, 215, 215), -1, cv::LINE_AA, true);

				freetype2->putText(result, "Chart:  Two skunks (Ctymax and 晨) in Africa shouting WAHHHHHH!", Point(h - 13, v + offset_y + 70) + cv::Point(2, 1), 24, Scalar(0, 0, 0), -1, cv::LINE_AA, true);
				freetype2->putText(result, "Chart:  Two skunks (Ctymax and 晨) in Africa shouting WAHHHHHH!", Point(h - 13, v + offset_y + 70), 24, Scalar(200, 200, 200), -1, cv::LINE_AA, true);
			};
			// 添加日文文字
			//freetype2->putText(result, "こんにちは", Point(10, 100), font_size, font_color, -1, cv::LINE_AA, true);
			freetype2.reset();
			


			std::vector<uchar> cdata;
			cv::imencode(".png", result, cdata);
			result.deallocate();
			std::string imgStr(cdata.begin(), cdata.end());

			response.set_header("Content-Type", "image/png");
			response.write(imgStr);
			return response;
				});
		app.multithreaded().run();
	}

	static void testModernSqlite() {
		try {
			// creates a database file 'dbfile.db' if it does not exists.
			sqlite::database db("localDB.db");
			
			/*
				std::string s;
				std::cin >> s;
			*/

			// executes the query and creates a 'user' table
			db << "select id,bottleMainId,thrower,throwGroup,content,filePath,timeStamp,reportCount,available from bottle;"

			/* 
				where bottleMainId = ? ;"
								<< s
			*/
				>> [&](int id, int bottleMainId, std::string thrower, std::string throwGroup,
					std::string content, std::string filePath, _uint64 timeStamp, int reportCount,
					bool available
					) {
						BottleDao bottle{
						id,bottleMainId,thrower,throwGroup,content,
						filePath,timeStamp,reportCount,available
						};

				std::cout << bottle.getJson().dump() << std::endl;
			};

		}
		catch (const std::exception& e) {
			std::cout << e.what() << std::endl;
		}
	}

	static int testRedis(void) {
		const char* redis_password = "bkhra571";
		redisContext* redis_conn = redisConnectWithTimeout("127.0.0.1", 6379, { 1, 500000 });
		if (redis_conn == nullptr || redis_conn->err) {
			if (redis_conn) {
				fprintf(stderr, "Redis connection error: %s\n", redis_conn->errstr);
			}
			else {
				fprintf(stderr, "Failed to allocate redis context\n");
			}
			redisFree(redis_conn);
			exit(1);
		}

		// 身份验证
		redisReply* redis_reply = static_cast<redisReply*>(redisCommand(redis_conn, "AUTH %s", redis_password));

		if (redis_reply == nullptr) {
			std::cerr << "Failed to authenticate with Redis: " << redis_conn->errstr << std::endl;
			redisFree(redis_conn);
			return 1;
		}

		const char* key = "mykey";
		const char* value = "hello world";

		redisReply* reply = (redisReply*)redisCommand(redis_conn, "SET %s %s", key, value);
		if (reply == NULL) {
			fprintf(stderr, "Failed to execute SET command\n");
			redisFree(redis_conn);
			exit(1);
		}

		printf("SET %s %s\n", key, value);

		freeReplyObject(reply);
		redisFree(redis_conn);
		return 0;
	}

	//检测文件是否存在
	static void testMethod1(void) {
		bool b1{ std::filesystem::exists("config.yaml") };
		bool b2{ std::filesystem::exists("config.yml") };
		std::cout << "ymal -> " << b1 << std::endl;
		std::cout << "yml  -> " << b2 << std::endl;
	}

#ifdef WARNING_CONTENT
	// SQL查询测试
	static void testMethod2(void) {
		std::cout << "More:\n";
		TestProject::testMoreSQLQuery();

		std::cout << "Simple:\n";
		TestProject::testSimpleSQLQuery();
	}
#endif // WARNING_CONTENT
	// jwt测试
	static void testMethod3(void) {
		std::string token = "eyJhbGciOiJIUzI1NiIsInR5cCI6IkpXUyJ9.eyJpc3MiOiJhdXRoMCJ9.AbIJTDMFc7yUa5MhvcP03nJPyCPzZtQcGEp-zWfOkEE";
		auto decoded = jwt::decode(token);

		for (auto& e : decoded.get_payload_json())
			std::cout << e.first << " = " << e.second << std::endl;
	}

	// bcrypt测试
	static void testMethod4(void) {
		std::string password = "250kaijie";

		//生成
		std::string hash = bcrypt::generateHash(password,10);

		std::cout << "250kaijie: " << hash << std::endl;

		//效验
		std::cout << "\"" << password << "\" : " << bcrypt::validatePassword(password, hash) << std::endl;
		std::cout << "\"wrong\" : " << bcrypt::validatePassword("wrong", hash) << std::endl;
		std::cout << "\"database\" : " << bcrypt::validatePassword(password, "$2a$10$2MKm.McjEdc/O.PXArxEeOh4dBROZ1BoLswMP8lG8bwRBDZdqhRoe") << std::endl;
	}

	// 二维码测试
	static void testMethod5(void) {
		cv::Mat QRCode{ DrawTool::DrawQRcode("https://www.bilibili.com/", true) };
		cv::imwrite("QRCode.jpg", QRCode);
	}

	// crow框架测试
	static void testMethod6(void) {
		crow::SimpleApp app;
		app.bindaddr("0.0.0.0").port(9961);

		CROW_ROUTE(app, "/hello_test_wtf")
			.methods("GET"_method)([](const crow::request& req) {

			json data;
			data["message"] = "Hello World!";

			// 设置响应头为 application/json
			crow::response resp;
			resp.set_header("Content-Type", "application/json");

			// 将 JSON 数据作为响应体返回
			resp.body = data.dump();


			return resp;
				});
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

		app.multithreaded().run();
	}
#ifdef WARNING_CONTENT
	static void testMoreSQLQuery(void) {
		StartWatch();
		SQL_Handle handle;
		auto contents{ handle.moreQuery("SELECT * from bottle") };
		printMaps(contents);

		std::vector<BottleDao> result;
		std::string jsonList {"["};
		bool flag{false};
		for (const auto& content : contents) {
			BottleDao bottle{
				std::stoi(content.at("id").second),
				std::stoi(content.at("bottleMainId").second),
				content.at("thrower").second,
				content.at("throwGroup").second,
				content.at("content").second,
				content.at("filePath").second,
				std::stoull(content.at("timeStamp").second),
				std::stoi(content.at("reportCount").second),
				std::stoi(content.at("available").second)
			};
			jsonList += (flag ? "," : "") + std::move(bottle.getJson().dump());
			flag = true;
			std::cout << "json -> " << bottle.getJson() << "\n";
			result.push_back(std::move(bottle));
		};
		jsonList += "]";

		std::cout << "jsons -> " << jsonList << "\n";
		StopWatch();
	}
	static void testSimpleSQLQuery(void) {
		StartWatch();

		SQL_Handle handle;
		auto content{ handle.simpleQuery("SELECT * from bottle where bottleMainId = 1000000") };
		printMap(content);
		
		BottleDao bottle{ 
			std::stoi(content.at("id").second),
			std::stoi(content.at("bottleMainId").second),
			content.at("thrower").second,
			content.at("throwGroup").second,
			content.at("content").second,
			content.at("filePath").second,
			std::stoull(content.at("timeStamp").second),
			std::stoi(content.at("reportCount").second),
			std::stoi(content.at("available").second)
		};

		std::cout << bottle.toString();
		std::cout << bottle.getJson() << "\n";

		StopWatch();
	}
#endif // WARNING_CONTENT
private:
	static void printMap(const auto& m) {
		std::cout << "========================\n";
		for (auto& [key, value] : m) {
			std::cout << std::format("key:{}, type:{}, value:{}\n", key, value.first, value.second);
		}
		std::cout << "========================\n";
		std::cout.flush();
	}
	
	static void printMaps(const auto& vec) {
		std::cout << "========================\n";
		for (const auto& item : vec) {
			for (auto& [key, value] : item) {
				std::cout << std::format("key:{}, type:{}, value:{}\n", key, value.first, value.second);
			}
			std::cout << "========================\n";
		}
		std::cout.flush();
	}

	inline static std::chrono::system_clock::time_point start, end;

	static void StartWatch(void) {
		start = std::chrono::high_resolution_clock::now();
	};

	static void StopWatch(void) {
		end = std::chrono::high_resolution_clock::now();
		std::chrono::duration<double, std::milli> elapsed = end - start;
		std::cout << "\033[42mtime consumed " << elapsed.count() << " ms\033[0m\n";
		std::cout.flush();
	};
};

#endif