#pragma once

#ifndef TEST_PROJECT_HPP
#define TEST_PROJECT_HPP  
#include <iostream>
#include <fstream>
#include <exception>
#include <stdexcept>
#include <filesystem>
#include <common/utils/sql_handle.hpp>
#include <span>
#include <jwt-cpp/jwt.h>
#include <bcrypt.h>
#include "dao/bottle.hpp"
#include <sqlite_modern_cpp.h>
#include "crow.h"
#include "common/utils/draw_tool.hpp"
#include <opencv2/opencv.hpp>
#include <qrencode.h>

using namespace std::string_literals;

//#define WARNING_CONTENT  
using _uint64 = unsigned long long int;

class TestProject final{
public:
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