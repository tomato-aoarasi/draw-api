/*
 * @File	  : bottle.hpp
 * @Coding	  : utf-8
 * @Author    : Bing
 * @Time      : 2023/03/11 18:41
 * @Introduce : SQLite存储对象
*/

#pragma once
#ifndef BOTTLE_HPP
#define BOTTLE_HPP
#include "fmt/format.h"
#include "nlohmann/json.hpp"
#include <filesystem>

using json = nlohmann::json;
using _uint64 = unsigned long long int;

class BottleDao final{
public:
	BottleDao() = default;
	BottleDao(const int id, const int bottleMainId,
		std::string_view thrower, std::string_view throwGroup, std::string_view content,
		const std::filesystem::path& filePath, _uint64 timeStamp, const int reportCount,bool available) :
		m_id{ id }, m_bottleMainId{ bottleMainId }, m_thrower{ thrower }, 
		m_throwGroup{ throwGroup }, m_content{ content },
		m_filePath {filePath}, m_timeStamp{ timeStamp }, m_report_count {reportCount}, m_available {available}
	{}

	void setId(const int id) {
		this->m_id = id;
	}
	int getId(void) const{
		return this->m_id;
	}
	void setBottleMainId(const int bottleMainId) {
		this->m_bottleMainId = bottleMainId;
	}
	int getBottleMainId(void) const{
		return this->m_bottleMainId;
	}
	void setReportCount(const int reportCount) {
		this->m_report_count = reportCount;
	}
	int getReportCount(void) const{
		return this->m_report_count;
	}
	void setThrower(std::string_view thrower) {
		this->m_thrower = thrower;
	}
	std::string getThrower(void) const{
		return this->m_thrower;
	}
	void setThrowGroup(std::string_view throwGroup) {
		this->m_throwGroup = throwGroup;
	}
	std::string getThrowGroup(void) const{
		return this->m_throwGroup;
	}
	void setContent(std::string_view content) {
		this->m_content = content;
	}
	std::string getContent(void) const{
		return this->m_content;
	}
	void setFilePath(const std::filesystem::path& filePath) {
		this->m_filePath = filePath;
	}
	std::filesystem::path getFilePath(void) const{
		return this->m_filePath;
	}
	void setTimeStamp(const _uint64 timeStamp) {
		this->m_timeStamp = timeStamp;
	}
	_uint64 getTimeStamp(void) const{
		return this->m_timeStamp;
	}
	void setAvailable(const bool available) {
		this->m_available = available;
	}
	bool getAvailable(void) const {
		return this->m_available;
	}

	std::string toString(void) {
		return std::move(fmt::format("[id={},bottleMainId={},\
		thrower={},throwGroup={},content={},\
		filePath={},timeStamp={},reportCount,available]\n",
		this->m_id, this->m_bottleMainId,
		this->m_thrower, this->m_throwGroup, this->m_content,
		this->m_filePath.c_str(), this->m_timeStamp, this->m_report_count, this->m_available));
	};

	auto getJson() {
		json j;
		j["id"] = this->m_id;
		j["bottleMainId"] = this->m_bottleMainId;
		j["thrower"] = this->m_thrower;
		j["throwGroup"] = this->m_throwGroup;
		j["content"] = this->m_content;
		j["filePath"] = this->m_filePath.string();
		j["timeStamp"] = this->m_timeStamp;
		j["reportCount"] = this->m_report_count;
		j["available"] = this->m_available;
		return j;
	};
private:
	int m_id{}, m_bottleMainId{}, m_report_count{};
	std::string m_thrower{}, m_throwGroup{}, m_content{};
	std::filesystem::path m_filePath{};
	_uint64 m_timeStamp{};
	bool m_available{ true };
};

#endif