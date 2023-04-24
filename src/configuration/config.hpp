/*
 * @File	  : config.hpp
 * @Coding	  : utf-8
 * @Author    : Bing
 * @Time      : 2023/03/05 21:14
 * @Introduce : 配置类(解析yaml)
*/

#pragma once

#ifndef CONFIG_HPP
#define CONFIG_HPP  
#include <fstream>
#include <string>
#include <filesystem>
#include "common/exception/file_exception.hpp"
#include "yaml-cpp/yaml.h"
#include "crow.h"
#include "crow/middlewares/cors.h"
#include "fmt/format.h"


using namespace std::string_literals;
using ubyte = unsigned char;
using ushort = unsigned short;

namespace std {
	using fmt::format;
	using fmt::format_error;
	using fmt::formatter;
}


#if CORS_OPEN
using CrowApplication = crow::App<crow::CORSHandler>;
#else
using CrowApplication = crow::SimpleApp;
#endif

#define CrowApp CrowApplication
#define Json nlohmann::json

class Config final{
public:
	// 获取参数
	class Parameter final {
		friend class Config;
	protected:
		inline static std::string ms_secret{}, ms_issuer{};
		inline static ushort ms_port{};
		inline static ubyte ms_concurrency{};
	public:
		static const std::string& getSecret(void){
			return ms_secret;
		}

		static const std::string& getIssuer(void){
			return ms_issuer;
		}

		static const ushort& getPort(void){
			return ms_port;
		}

		static const ubyte& getConcurrency(void){
			return ms_concurrency;
		}
	private:
		Parameter(void) = delete;
		~Parameter(void) = delete;
		Parameter(const Parameter&) = delete;
		Parameter(Parameter&&) = delete;
		Parameter& operator=(const Parameter&) = delete;
		Parameter& operator=(Parameter&&) = delete;
	};

	static void initialized(){
		const bool 
			yaml_whether_exists { std::filesystem::exists(yaml_path) },
			yal_whether_exists  { std::filesystem::exists(yml_path)  };

		if (yal_whether_exists) {
			ms_public_config = YAML::LoadFile(yml_path);
		}
		else if (yaml_whether_exists){
			ms_public_config = YAML::LoadFile(yaml_path);
		}
		else{
			throw self::file_exception("YAML file doesn't exist.");
		}
		
		/* =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-= */

		Parameter::ms_secret = getConfig()["server"]["token"]["secret"].as<std::string>();
		Parameter::ms_issuer = getConfig()["server"]["token"]["issuer"].as<std::string>();
		Parameter::ms_port = getConfig()["server"]["port"].as<ushort>();
		Parameter::ms_concurrency = getConfig()["server"]["concurrency"].as<ubyte>();
	}
	//得到一个YAML配置文件
	static const YAML::Node& getConfig(void) {
		return ms_config;
	}

	inline static YAML::Node ms_public_config;
private:
	Config(void) = delete;
	~Config(void) = delete;
	Config(const Config&) = delete;
	Config(Config&&) = delete;
	Config& operator=(const Config&) = delete;
	Config& operator=(Config&&) = delete;

	inline static const std::filesystem::path
		yaml_path{ "config.yaml" },
		yml_path { "config.yml"  };
	inline static YAML::Node ms_config{ YAML::LoadFile(yaml_path) };
};

class Global final {
public:
	inline static const std::string BingAPI{ "http://150.158.89.12:6680"s};
	inline static const std::string PhiUri{ "/api/pgr/findBySong"s };
	inline static const std::string PhiUrl { BingAPI + PhiUri};
};

#endif