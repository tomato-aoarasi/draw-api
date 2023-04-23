/*
 * @File	  : bottle_service.hpp
 * @Coding	  : utf-8
 * @Author    : Bing
 * @Time      : 2023/03/17 23:47
 * @Introduce : 漂流瓶接口类
*/

#pragma once

#ifndef BOTTLE_SERVICE_IMPL_HPP 
#define BOTTLE_SERVICE_IMPL_HPP  
#include "dao/user.hpp"
#include "service/bottle_service.hpp"
#include "common/utils/sql_handle.hpp"
#include <chrono>
#include "jwt-cpp/jwt.h"
#include "common/utils/http_util.hpp"
#include "dao/bottle.hpp"
using json = nlohmann::json;

class BottleServiceImpl : public BottleService {
public:
	BottleServiceImpl() {
		++ms_count;
	};
	~BottleServiceImpl() = default;

	std::string getBottle(int id) override {
		SQL_Handle sql_helper;
		int max_id{ std::stoi(sql_helper.simpleQuery("SELECT bottleMainId from bottle order by bottleMainId desc LIMIT 1").at("bottleMainId").second) },
			min_id{ std::stoi(sql_helper.simpleQuery("SELECT bottleMainId from bottle order by bottleMainId asc LIMIT 1").at("bottleMainId").second) };

		std::vector<json> contents{};

		json result;
		result["maxId"] = max_id;
		result["minId"] = min_id;
		if (id == -1)
		{
			auto contents_sql{ sql_helper.moreQuery("SELECT * from bottle") };
			for (const auto& content : contents_sql) {
				BottleDao bottle_temp{
					std::stoi(content.at("id").second),
					std::stoi(content.at("bottleMainId").second),
					content.at("thrower").second,
					content.at("throwGroup").second,
					content.at("content").second,
					content.at("filePath").second,
					std::stoull(content.at("timeStamp").second),
					std::stoi(content.at("reportCount").second),
					static_cast<bool>(std::stoi(content.at("available").second))
				};
				contents.emplace_back(bottle_temp.getJson());
			};
			result["content"] = contents;
			return result.dump();

		}
		else {
			auto content{ sql_helper.simpleQuery("SELECT * from bottle where bottleMainId = " + std::to_string(id)) };
			if (content.size() > 0)
			{
				BottleDao bottle{
				std::stoi(content.at("id").second),
				std::stoi(content.at("bottleMainId").second),
				content.at("thrower").second,
				content.at("throwGroup").second,
				content.at("content").second,
				content.at("filePath").second,
				std::stoull(content.at("timeStamp").second),
				std::stoi(content.at("reportCount").second),
				static_cast<bool>(std::stoi(content.at("available").second))
				};
				contents.emplace_back(bottle.getJson());
			}
			result["content"] = contents;
			return result.dump();
		}
	};

	std::string getToken(void) override {
		std::string secret{ Config::getConfig()["server"]["token"]["secret"].as<std::string>() };
		std::string issuer{ Config::getConfig()["server"]["token"]["issuer"].as<std::string>() };

		SQL_Handle handle;
		auto content{ handle.simpleQuery("SELECT * from user where uid = 0") };
		User user{
			static_cast<unsigned int>(std::stoul(content.at("uid").second)),
			content.at("userName").second,
			content.at("password").second,
			static_cast<bool>(std::stoul(content.at("role").second)),
			std::stoull(content.at("registrarDate").second)
		};

		auto time_point{ std::chrono::system_clock::now() };
		auto expires{ time_point + std::chrono::days(365) };

		std::time_t timestamp{ static_cast<std::time_t>(user.getRegistrarDate()) };
		// 转换为本地时间
		std::tm t = *std::localtime(&timestamp);

		// 将时间格式化为字符串
		std::stringstream ss;
		ss << std::put_time(&t, "%Y-%m-%d %H:%M:%S");

		auto token = jwt::create()
			.set_issuer(issuer)
			.set_algorithm("HS256")
			.set_type("JWT")
			.set_expires_at(std::move(expires))
			.set_payload_claim("uid", jwt::claim(std::to_string(user.getUid())))
			.set_payload_claim("userName", jwt::claim(user.getUserName()))
			.set_payload_claim("role", jwt::claim(std::to_string(user.getRole())))
			.set_payload_claim("password", jwt::claim(user.getPassword()))
			.set_payload_claim("registrarDate", jwt::claim(ss.str()))
			.sign(jwt::algorithm::hs256{ secret });

		json j;
		j["token"] = std::move(token);

		return j.dump();
	};
private:
	inline static int ms_count{ 0 };
};
#endif // !BOTTLE_SERVICE_IMPL_HPP
