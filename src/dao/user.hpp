/*
 * @File	  : user.hpp
 * @Coding	  : utf-8
 * @Author    : Bing
 * @Time      : 2023/03/18 00:18
 * @Introduce : 用户存储对象
*/

#pragma once
#ifndef USER_HPP
#define USER_HPP
#include <string_view>
#include "fmt/format.h"
#include "nlohmann/json.hpp"

using json = nlohmann::json;
using _uint64 = unsigned long long int;
class User final{
public:
	User() = default;
	User(unsigned int uid, std::string_view userName, std::string_view password, unsigned int role, _uint64 registrarDate) :
		m_uid{ uid }, m_userName{ userName }, m_password{ password },
		m_role{ role }, m_registrarDate{ registrarDate } {};
	void setUid(unsigned int uid) {
		this->m_uid = uid;
	}
	unsigned int getUid() const {
		return this->m_uid;
	}
	void setUserName(std::string_view userName) {
		this->m_userName = userName;
	}
	std::string getUserName() const {
		return this->m_userName;
	}
	void setPassword(std::string_view password) {
		this->m_password = password;
	}
	std::string getPassword() const {
		return this->m_password;
	}
	void setRole(unsigned int role) {
		this->m_role = role;
	}
	unsigned int getRole() const {
		return this->m_role;
	}
	void setRegistrarDate(_uint64 registrarDate) {
		this->m_registrarDate = registrarDate;
	}
	_uint64 getRegistrarDate() const {
		return this->m_registrarDate;
	}
	auto getJson() {
		json j;
		j["uid"] = this->m_uid;
		j["userName"] = this->m_userName;
		j["password"] = this->m_password;
		j["role"] = this->m_role;
		j["registrarDate"] = this->m_registrarDate;
		return j;
	};
private:
	unsigned int m_uid, m_role;
	std::string m_userName,m_password;
	_uint64 m_registrarDate;
};
#endif