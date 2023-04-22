/*
 * @File	  : sql_utils.hpp
 * @Coding	  : utf-8
 * @Author    : Bing
 * @Time      : 2023/03/11 14:35
 * @Introduce : SQLite工具
*/

#pragma once

#ifndef SQL_UTIL_HPP
#define SQL_UTIL_HPP
#include <memory>
#include <regex>
#include <filesystem>
#include "common/utils/log_system.hpp"
#include "configuration/config.hpp"
#include "common/exception/file_exception.hpp"
#include "sqlite3pp.h"

class SQL_Util {
public:
	SQL_Util(void) = default;
	~SQL_Util(void) = default;
	static void initialized(void) {
		const std::filesystem::path path{ Config::getConfig()["db"]["path"].as<std::string>() };

		std::regex reg{ "^(.*)(\\.)(db)$" };

		bool suffix_match{};

		if (std::filesystem::exists(path))
		{
			spdlog::info(std::format("文件{}存在", path.filename().string()));
			if (!std::regex_match(path.string(), std::move(reg))) {
				spdlog::error("文件后缀名错误,非db为后缀");
				throw self::file_exception("suffix match error");
			}
		}
		else {
			spdlog::error(std::format("文件{}不存在", path.filename().string()));
			throw self::file_exception(std::format("not found file:{}", path.filename().string()).c_str());
		}

		spdlog::info("数据库路径获取成功!");
	};

private:
	SQL_Util(const SQL_Util&) = delete;
	SQL_Util(SQL_Util&&) = delete;
	SQL_Util& operator=(const SQL_Util&) = delete;
	SQL_Util& operator=(SQL_Util&&) = delete;
protected:
	inline static const std::filesystem::path ms_db_path{ Config::getConfig()["db"]["path"].as<std::string>() };
};
#endif