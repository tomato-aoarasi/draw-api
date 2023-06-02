/*
 * @File	  : bottle_service.hpp
 * @Coding	  : utf-8
 * @Author    : Bing
 * @Time      : 2023/03/17 23:47
 * @Introduce : 漂流瓶接口类
*/

#pragma once

#include <chrono>
#include <bcrypt.h>
#include "dao/user.hpp"
#include "service/bottle_service.hpp"
#include "common/utils/sql_handle.hpp"
#include "jwt-cpp/jwt.h"
#include "common/utils/http_util.hpp"
#include "dao/bottle.hpp"

#ifndef BOTTLE_SERVICE_IMPL_HPP 
#define BOTTLE_SERVICE_IMPL_HPP  

#define OLD  0  

using json = nlohmann::json;
class BottleServiceImpl : public BottleService {
public:
	BottleServiceImpl() {
		++ms_count;
	};
	~BottleServiceImpl() = default;

	std::string getBottle(int id) override {
		SQL_Handle sql_helper;

#if OLD
		int max_id{ std::stoi(sql_helper.simpleQuery("SELECT bottleMainId from bottle order by bottleMainId desc LIMIT 1").at("bottleMainId").second) },
			min_id{ std::stoi(sql_helper.simpleQuery("SELECT bottleMainId from bottle order by bottleMainId asc LIMIT 1").at("bottleMainId").second) };
#else
		int max_id{ },
			min_id{ };

		sql_helper.DB << "SELECT bottleMainId from bottle order by bottleMainId desc LIMIT ?"
			<< 1
			>> [&](int bottleMainId) {
			max_id = bottleMainId;
		};

		sql_helper.DB << "SELECT bottleMainId from bottle order by bottleMainId asc LIMIT ?"
			<< 1
			>> [&](int bottleMainId) {
			min_id = bottleMainId;
		};
#endif
		std::vector<json> contents{};

		json result;
		result["maxId"] = max_id;
		result["minId"] = min_id;
		if (id == -1)
		{

#if OLD
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
#else
			sql_helper.DB << "select id,bottleMainId,thrower,throwGroup,content,filePath,timeStamp,reportCount,available from bottle;"
				>> [&](int id, int bottleMainId, std::string thrower, std::string throwGroup,
					std::string content, std::string filePath, _uint64 timeStamp, int reportCount,
					bool available
					) {
						BottleDao bottle{
						id,bottleMainId,thrower,throwGroup,content,
						filePath,timeStamp,reportCount,available
						};

						contents.emplace_back(bottle.getJson());
			};
#endif
			
			result["content"] = contents;
			return result.dump();
		}
		else {
#if OLD
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
#else
			sql_helper.DB << "select id,bottleMainId,thrower,throwGroup,content,filePath,timeStamp,reportCount,available from bottle where bottleMainId = ?;"
				<< id
				>> [&](int id, int bottleMainId, std::string thrower, std::string throwGroup,
					std::string content, std::string filePath, _uint64 timeStamp, int reportCount,
					bool available
					) {
						BottleDao bottle{
						id,bottleMainId,thrower,throwGroup,content,
						filePath,timeStamp,reportCount,available
						};

						contents.emplace_back(bottle.getJson());
			};
#endif
			result["content"] = contents;
			return result.dump();
		}
	};

	std::string getToken(void) override {
		std::string secret{ Config::getConfig()["server"]["token"]["secret"].as<std::string>() };
		std::string issuer{ Config::getConfig()["server"]["token"]["issuer"].as<std::string>() };

		SQL_Handle sql_helper;

#if OLD
		auto content{ sql_helper.simpleQuery("SELECT * from user where uid = 0") };
		User user{
			static_cast<unsigned int>(std::stoul(content.at("uid").second)),
			content.at("userName").second,
			content.at("password").second,
			static_cast<bool>(std::stoul(content.at("role").second)),
			std::stoull(content.at("registrarDate").second)
	};
#else
		User user;
		sql_helper.DB << "SELECT uid,userName,password,role,registrarDate from user where uid = ?;"
			<< 0 
			>> [&](unsigned int uid, std::string userName, std::string password, bool role, unsigned long long int registrarDate) {
				user = User(uid,userName,password,role,registrarDate);
			};
#endif

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
