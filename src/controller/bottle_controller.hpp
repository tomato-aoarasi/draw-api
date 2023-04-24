/*
 * @File	  : bottle_controller.hpp
 * @Coding	  : utf-8
 * @Author    : Bing
 * @Time      : 2023/03/18 20:23
 * @Introduce : 漂流瓶controller
*/

#pragma once
#ifndef BOTTLE_CONTROLLER
#define BOTTLE_CONTROLLER
#include <future>
#include <memory>
#include <string_view>
#include <chrono>
#include <configuration/config.hpp>
#include "crow.h"
#include "common/utils/http_util.hpp"
#include "nlohmann/json.hpp"
#include "bcrypt.h"
#include <service/impl/bottle_service_impl.hpp>
#include <configuration/config.hpp>

using json = nlohmann::json;
using namespace std::chrono;
using namespace std::chrono_literals;
using StatusCode = HTTPUtil::StatusCodeHandle::status;
using StatusCodeHandle = HTTPUtil::StatusCodeHandle;

class BottleController final {
public:
    explicit BottleController(CrowApp& app, std::string_view secret, std::string_view issuer, std::unique_ptr<BottleService> bottle) :
        m_app{ app }, m_secret{ secret }, m_issuer{ issuer }, m_bottle{ std::move(bottle) } {};
    ~BottleController() = default;
    const inline void controller(void) {
        CROW_ROUTE(m_app, "/hello_test")
            .methods("GET"_method)([](const crow::request& req){ 

            json data;
            data["message"] = "Hello World!";

            // 设置响应头为 application/json
            crow::response resp;
            resp.set_header("Content-Type", "application/json");

            // 将 JSON 数据作为响应体返回
            resp.body = data.dump();
            
            
            return resp;
                });


        CROW_ROUTE(m_app, "/getUser")
            .methods("GET"_method)([&](const crow::request& req) {
            crow::response resp;
            auto sec{ req.get_header_value("secret") };
            if (sec.compare(m_secret) == 0) {
                resp.set_header("Content-Type", "application/json");
                resp.body = m_bottle->getToken();
            }
            else {
                resp.code = 401;
                resp.body = StatusCodeHandle::getSimpleJsonResult(401).dump();
            }
            return resp;
                });

        CROW_ROUTE(m_app, "/getBottle")
            .methods("GET"_method)([&](const crow::request& req) {
                int id{ -1 };
                crow::response resp;
                resp.set_header("Content-Type", "application/json");

                id = std::stoi(req.url_params.get("id"));
                bool verifySQL{ self::CheckParameter(std::to_string(id)) };
                if (verifySQL)
                {
                    resp.body = m_bottle->getBottle(id);
                }
                else {
                    resp.code = 500;
                    resp.body = "error";
                }
                return resp;
                });

        /*CROW_ROUTE(m_app, "/verify")
            .methods("POST"_method)([&](const crow::request& req) {
                constexpr auto timeout{ 200ms };
                crow::response resp;
                resp.set_header("Content-Type", "application/json");
                //auto [pass , msg]  { verifyToken(req, resp) };

                verify_token_result result;
                auto j = json::parse(req.body);
                std::string passwd{ j["password"] };
                auto decode_token{ jwt::decode(req.get_header_value("token")) };
                const auto passwd_jwt{ decode_token.get_payload_claim("password").as_string() };
                bool verify_passwd{ bcrypt::validatePassword(passwd, passwd_jwt) };

                if (!verify_passwd)
                {
                    result.msg = "Password verification failed";
                    result.pass = false;
                    result.code = 401;
                    return result;
                }

                auto verifier = jwt::verify()
                    .allow_algorithm(jwt::algorithm::hs256{ this->m_secret })
                    .with_issuer(this->m_issuer);

                try {
                    verifier.verify(decode_token);
                }
                catch (const std::exception& e) {
                    //std::cout << "except" << std::endl;
                    result.msg = e.what();
                    result.pass = false;
                    result.code = 401;
                    return result;
                }
                result.pass = true;
                result.code = 200;
                //std::cout << "final" << std::endl;
                if (result.pass) {
                    resp.body = "ok";
                }else{
                    resp.code = result.code;
                    resp.body = StatusCodeHandle::getSimpleJsonResult(result.code, result.msg).dump();
                }
            });*/
    }
private:
    struct verify_token_result {
        bool pass;
        std::string msg;
        int code;
    };
    BottleController() = delete;
    CrowApp& m_app;
    const std::string m_secret;
    const std::string m_issuer;
    std::unique_ptr<BottleService> m_bottle;
};

#endif