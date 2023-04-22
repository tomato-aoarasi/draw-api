/*
 * @File	  : sql_utils.hpp
 * @Coding	  : utf-8
 * @Author    : Bing
 * @Time      : 2023/03/11 14:41
 * @Introduce : 处理SQLite
*/

#pragma once
#ifndef SQL_HANDLE_HPP
#define SQL_HANDLE_HPP
#include <unordered_map>
#include <vector>
#include <any>
#include <optional>
#include "prevent_inject.hpp"
#include "sql_util.hpp"

using valueType = std::pair<std::string, std::string>;
class SQL_Handle : public SQL_Util {
public:
	SQL_Handle(void) = default;
	~SQL_Handle(void) {
	}

#ifdef DEBUG
	template <typename T, typename E >
	class contentValue : public std::pair<T, E> {
	private:
		int _ref;
	public:
		contentValue(T first, E second) : std::pair<T, E>(first, second) {};
		contentValue() = default;
		~contentValue() = default;

		T getValue(void) const {
			return this->second;
		};

		auto toType() {
			std::unordered_map<std::string_view, unsigned char> typeMap{
				{"INTEGER",0}, // int
				{"REAL",1},	   // float
				{"TEXT",2},	   // std::string
				{"BLOB",3},	   // -
				{"DATE",4},	   // unsigned long long
			};
			switch (typeMap[this->first])
			{
			case 0:
				return std::stoi(this->second);
			case 1:
				return std::stof(this->second);
			case 2:
				return this->second;
			case 3:
				return this->second;
			case 4:
				return std::stoull(this->second);
			default:
				return NULL;
			}
		};
	};
#endif


	// 获取单条SQL
	auto& simpleQuery(std::string_view sqlSentence) {
		this->m_content.clear();

		sqlite3pp::database db(m_path.c_str());
		sqlite3pp::query qry(db, sqlSentence.data());

		for (sqlite3pp::query::iterator i = qry.begin(); i != qry.end(); ++i) {
			for (int j = 0; j < qry.column_count(); ++j) {
				std::string type{ qry.column_decltype(j) };
				this->m_content.insert({ qry.column_name(j), valueType(std::move(type),(*i).get<char const*>(j)) });
			}
		}
		return this->m_content;
	}

	// 获取多条SQL
	auto& moreQuery(std::string_view sqlSentence) {
		sqlite3pp::database db(m_path.c_str());
		sqlite3pp::query qry(db, sqlSentence.data());

		for (sqlite3pp::query::iterator i = qry.begin(); i != qry.end(); ++i) {
			decltype(m_content) content{};
			for (int j = 0; j < qry.column_count(); ++j) {
				std::string type{ qry.column_decltype(j) };
				
				content.insert({ qry.column_name(j), valueType(std::move(type),(*i).get<char const*>(j)) });
			}
			this->m_contents.emplace_back(content);
		}
		return this->m_contents;
	}

	// 获取单条SQL
	auto& getContent(void) const{
		return this->m_content;
	}

	// 获取多条SQL
	auto& getContents(void) const{
		return this->m_contents;
	}

#ifdef DEBUG
	void test(void) {
		/*contentValue<std::string, std::string> p;
		p.second;*/
	};
#endif
protected:
	//sqlite3pp::database& m_db{ SQL_Util::ms_db };
private:
	std::unordered_map<std::string, valueType> m_content{};
	std::vector<decltype(m_content)> m_contents{};
	const std::filesystem::path& m_path{ SQL_Util::ms_db_path };
};

#endif // !SQL_HANDLE
