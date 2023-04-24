/*
 * @File	  : other_util.hpp
 * @Coding	  : utf-8
 * @Author    : Bing
 * @Time      : 2023/04/22 15:33
 * @Introduce : 其他工具
*/

#pragma once

#ifndef OTHER_UTIL
#define OTHER_UTIL  

#include <string>
#include <string_view>
#include <vector>
#include "crow.h"

class OtherUtil final {
public:
    /// <summary>
    /// 判断vector<string>是否存在特定值
    /// </summary>
    /// <param name="keys">crow param的列表</param>
    /// <param name="val">是否含有特定字符串</param>
    /// <returns>true为存在,false为不存在</returns>
    inline static bool hasParam(const std::vector<std::string>& keys, std::string_view val) {
        return std::find(keys.cbegin(), keys.cend(), val) != keys.cend();
    }

    inline static bool verifyParam(const crow::request& req, std::string_view val) {
        bool has_value{ hasParam(req.url_params.keys(), val) };
        if (has_value)
        {
            return !std::string(req.url_params.get(val.data())).empty();
        }
        else return false;
    }
private:
};

#endif // OTHER_UTIL