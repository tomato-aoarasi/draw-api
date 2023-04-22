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
#include <filesystem>
#include "common/exception/file_exception.hpp"
#include "yaml-cpp/yaml.h"

class Config final {
public:
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

#endif