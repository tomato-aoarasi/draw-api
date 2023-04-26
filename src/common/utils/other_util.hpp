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
#include <chrono>
#include <future>
#include "configuration/config.hpp"
#include "crow.h"

using namespace std::chrono_literals;

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

    /// <summary>
    /// 效验request参数
    /// </summary>
    /// <param name="req">req丢进去就完事了</param>
    /// <param name="val">是否存在的字符串</param>
    /// <returns>true返回长度不为0的内容</returns>
    inline static bool verifyParam(const crow::request& req, std::string_view val) {
        bool has_value{ hasParam(req.url_params.keys(), val) };
        if (has_value)
        {
            return !std::string(req.url_params.get(val.data())).empty();
        }
        else return false;
    }

    /// <summary>
    /// 异步获取API(超时)
    /// </summary>
    /// <param name="data">传入的JSON</param>
    /// <param name="timeout">超时时间</param>
    /// <param name="domain">域名</param>
    /// <param name="uri">uri</param>
    inline static void asyncGetAPI(Json& data ,const std::chrono::milliseconds& timeout,const std::string& domain, const std::string& uri) {
        std::future<Json> future{ std::async(std::launch::async,[&]()->Json {

            httplib::Client client(domain);
            httplib::Result res = client.Get(uri);

            // 到时候加一个超时
            if (res && res->status == 200) return json::parse(res->body);
            throw std::runtime_error("Failed to get API data");
        }) };

        std::future_status status{ future.wait_for(timeout) };

        if (status == std::future_status::timeout)throw self::TimeoutException();
        data = future.get();
    }
private:
};

#endif // OTHER_UTIL