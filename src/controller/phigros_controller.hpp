/*
 * @File	  : phigros_controller.hpp
 * @Coding	  : utf-8
 * @Author    : Bing
 * @Time      : 2023/04/24 19:27
 * @Introduce : phigros一些东西
*/

#pragma once

#ifndef PHIGROS_CONTROLLER_HPP
#define PHIGROS_CONTROLLER_HPP  

#include <string>
#include <memory>
#include <algorithm>
#include "common/exception/self_exception.hpp"
#include <configuration/config.hpp>
#include "common/utils/http_util.hpp"
#include "crow.h"
#include "service/phigros_service.hpp"
#include "common/utils/other_util.hpp"
#include "common/utils/log_system.hpp"
#include <opencv2/opencv.hpp>
#include <exception>
#include <stdexcept>

using StatusCodeHandle = HTTPUtil::StatusCodeHandle;

constexpr int amount_spaces{ 2 };

class PhigrosController final {
private:
    CrowApp& m_app;
    std::unique_ptr<PhigrosService> m_phigros_service;
    PhigrosController() = delete;
public:
    ~PhigrosController() = default;

    explicit PhigrosController(CrowApp& app, std::unique_ptr<PhigrosService> phigros_service)
        : m_app{ app }, m_phigros_service{ std::move(phigros_service) } {};

    const inline void controller(void) {
        CROW_ROUTE(m_app, "/phi/drawInfo").methods("GET"_method)([&](const crow::request& req) {
            crow::response response;
            response.add_header("Cache-Control", "no-cache");
            response.add_header("Pragma", "no-cache");
            try {
                std::string authorization{ req.get_header_value("Authorization") }, songid;

                bool is_qr_code{ false };
                std::string content{ req.get_header_value("QRcode")};

                if (content.size() != 0)
                    is_qr_code = true;


                if (OtherUtil::verifyParam(req, "songId")) {
                    songid = req.url_params.get("songId");
                }
                else {
                    throw self::HTTPException("songId don't exist", 401, 3);
                }

                LogSystem::logInfo(std::format("[Phigros]绘制曲目信息 ------ SongId: {}", songid));

               cv::Mat result{ m_phigros_service->drawSongInfomation(
                    songid, is_qr_code, content, authorization) };


                std::vector<uchar> data;
                cv::imencode(".png", result, data);
                result.release();
                std::string imgStr(data.begin(), data.end());

                response.set_header("Content-Type", "image/png");
                response.write(imgStr);
                return response;
            }
            catch (const self::HTTPException& e) {
                response.write(StatusCodeHandle::getSimpleJsonResult(e.getCode(), e.getMessage(), e.getStatus()).dump(amount_spaces));
                LogSystem::logError(std::format("[Phigros]绘制曲目信息 ------ msg: {} / code: {} / status: {}", e.what(), e.getCode(), e.getStatus()));
                response.code = e.getCode();
            }
            catch (const self::TimeoutException& e) {
                LogSystem::logError("[Phigros]绘制曲目信息 ------ API请求超时");
                response.code = 408;
                response.write(StatusCodeHandle::getSimpleJsonResult(408, "Data API request timeout", 2).dump(amount_spaces));
            }
            catch (const std::runtime_error& e) {
                LogSystem::logError(std::format("[Phigros]绘制曲目信息 ------ msg: {} / code: {}", e.what(), 500));
                response.code = 500;
                response.write(StatusCodeHandle::getSimpleJsonResult(500, e.what()).dump(amount_spaces));
            }
            catch (const std::exception& e) {
                LogSystem::logError(std::format("[Phigros]绘制曲目信息 ------ msg: {} / code: {}", e.what(), 500));
                response.code = 500;
                response.write(StatusCodeHandle::getSimpleJsonResult(500, e.what()).dump(amount_spaces));
            }
            response.set_header("Content-Type", "application/json");

            return response;
            });

        CROW_ROUTE(m_app, "/phi/drawSingle").methods("POST"_method)([&](const crow::request& req) {

            std::string song_id{}, avatar_base64{};
            bool is_game_avatar{ false };
            /* EZ:0, HD:1, IN:2, AT:3, Auto: 4 */
            Ubyte level{ 4 };
            uint8_t style { 1 };
            crow::response response;
            response.add_header("Cache-Control", "no-cache");
            response.add_header("Pragma", "no-cache");
            try {
                // 测试玩家名
                //std::string playerNameTest{ req.url_params.get("player")};
                //cv::Mat result{ m_phigros_service->drawSongInfomation(
                //    std::move(song_id),std::move(is_qr_code),std::move(content)) };

                Json jsonData{ json::parse(req.body) };

                std::exchange(jsonData, jsonData[0]);

                //std::cout << jsonData.contains("songId"s) << std::endl;
                //std::cout << jsonData.contains("level"s) << std::endl;
                if (jsonData.contains("songId")) {
                    song_id = jsonData["songId"].get<std::string>();
                } else {
                    response.set_header("Content-Type", "application/json");
                    response.code = 400;
                    response.write(StatusCodeHandle::getSimpleJsonResult(400, "parameter 'songId' required and parameter cannot be empty.", 3).dump(amount_spaces));
                    return response;
                }
                if (jsonData.contains("level")) {
                    level = jsonData["level"].get<Ubyte>();
                }
                if (jsonData.contains("avatar_base64")) {
                    avatar_base64 = jsonData["avatar_base64"].get<std::string>();
                }
                if (jsonData.contains("style")) {
                    style = jsonData["style"].get<uint8_t>();
                }
                if (jsonData.contains("is_game_avatar")) {
                    LogSystem::logInfo("[Phigros]绘制玩家BEST ------ 游戏内头像");
                    is_game_avatar = jsonData["is_game_avatar"].get<bool>();
                }

                std::string
                    authorization{ req.get_header_value("Authorization") },
                    sessionToken{ req.get_header_value("SessionToken") };

                LogSystem::logInfo(std::format("[Phigros]绘制玩家BEST ------ SongID:{} / SessionToken:{}", song_id, sessionToken));
                cv::Mat result{ };

                switch (style)
                {
                case 0:
                    LogSystem::logInfo("[Phigros]绘制玩家BEST ------ 风格样式0(OLD)");
                    result = m_phigros_service->drawPlayerSingleInfo(song_id, level, authorization, sessionToken, avatar_base64, is_game_avatar);
                    break;
                case 1:
                    LogSystem::logInfo("[Phigros]绘制玩家BEST ------ 风格样式1(NEW)");
                    result = m_phigros_service->drawPlayerSingleInfoModernStyle(song_id, level, authorization, sessionToken, avatar_base64, is_game_avatar);
                    break;
                case 2:
                    LogSystem::logInfo("[Phigros]绘制玩家BEST ------ 风格样式2(NEW)");
                    result = m_phigros_service->drawPlayerSingleInfoModernStyle2(song_id, level, authorization, sessionToken, avatar_base64, is_game_avatar);
                    break;
                default:
                    throw self::HTTPException("Style does not exist.", 400);
                }

                std::vector<uchar> data;
                cv::imencode(".png", result, data);
                result.release();
                std::string imgStr(data.begin(), data.end());

                response.set_header("Content-Type", "image/png");
                response.write(imgStr);
                return response;
            }
            catch (const self::HTTPException& e) {
                response.write(StatusCodeHandle::getSimpleJsonResult(e.getCode(), e.getMessage(), e.getStatus()).dump(amount_spaces));
                LogSystem::logError(std::format("[Phigros]绘制玩家BEST ------ msg: {} / code: {} / status: {}", e.what(), e.getCode(), e.getStatus()));
                response.code = e.getCode();
            }
            catch (const self::TimeoutException& e) {
                LogSystem::logError("[Phigros]绘制玩家BEST ------ API请求超时");
                response.write(StatusCodeHandle::getSimpleJsonResult(408, "Data API request timeout", 2).dump(amount_spaces));
                response.code = 408;
            }
            catch (const std::runtime_error& e) {
                LogSystem::logError(std::format("[Phigros]绘制玩家BEST ------ msg: {} / code: {}", e.what(), 500));
                response.write(StatusCodeHandle::getSimpleJsonResult(500, e.what(), 1).dump(amount_spaces));
                response.code = 500;
            }
            catch (const std::exception& e) {
                LogSystem::logError(std::format("[Phigros]绘制玩家BEST ------ msg: {} / code: {}", e.what(), 500));
                response.write(StatusCodeHandle::getSimpleJsonResult(500, e.what(), 1).dump(amount_spaces));
                response.code = 500;
            }

            response.set_header("Content-Type", "application/json");
            return response;
            });

        CROW_ROUTE(m_app, "/phi/drawB19").methods("POST"_method)([&](const crow::request& req) {

            std::string avatar_base64{};
            bool is_game_avatar{ false };
            crow::response response;
            response.add_header("Cache-Control", "no-cache");
            response.add_header("Pragma", "no-cache");
            try {
                Json jsonData{ json::parse(req.body) };

                std::exchange(jsonData, jsonData[0]);

                if (jsonData.contains("avatar_base64")) {
                    avatar_base64 = jsonData["avatar_base64"].get<std::string>();
                }
                if (jsonData.contains("is_game_avatar")) {
                    LogSystem::logInfo("[Phigros]绘制玩家Best 19 ------ 游戏内头像");
                    is_game_avatar = jsonData["is_game_avatar"].get<bool>();
                }
                
                std::string
                    authorization{ req.get_header_value("Authorization") },
                    sessionToken{ req.get_header_value("SessionToken") };

                LogSystem::logInfo(std::format("[Phigros]绘制玩家Best 19 ------ SessionToken:{}", sessionToken));

                cv::Mat result{ m_phigros_service->drawB19(authorization, sessionToken, avatar_base64, is_game_avatar) };
                std::vector<uchar> data;
                cv::imencode(".png", result, data);
                result.release();
                std::string imgStr(data.begin(), data.end());

                response.set_header("Content-Type", "image/png");
                response.write(imgStr);
                return response;
            }
            catch (const self::HTTPException& e) {
                response.write(StatusCodeHandle::getSimpleJsonResult(e.getCode(), e.getMessage(), e.getStatus()).dump(amount_spaces));
                LogSystem::logError(std::format("[Phigros]绘制玩家Best 19 ------ msg: {} / code: {} / status: {}", e.what(), e.getCode(), e.getStatus()));
                response.code = e.getCode();
            }
            catch (const self::TimeoutException& e) {
                LogSystem::logError("[Phigros]绘制玩家Best 19 ------ API请求超时");
                response.write(StatusCodeHandle::getSimpleJsonResult(408, "Data API request timeout", 2).dump(amount_spaces));
                response.code = 408;
            }
            catch (const std::runtime_error& e) {
                LogSystem::logError(std::format("[Phigros]绘制玩家Best 19 ------ msg: {} / code: {}", e.what(), 500));
                response.write(StatusCodeHandle::getSimpleJsonResult(500, e.what(), 1).dump(amount_spaces));
                response.code = 500;
            }
            catch (const std::exception& e) {
                LogSystem::logError(std::format("[Phigros]绘制玩家Best 19 ------ msg: {} / code: {}", e.what(), 500));
                response.write(StatusCodeHandle::getSimpleJsonResult(500, e.what(), 1).dump(amount_spaces));
                response.code = 500;
            }

            response.set_header("Content-Type", "application/json");
            return response;
            });

#ifdef DEBUG
        CROW_ROUTE(m_app, "/phi/test").methods("GET"_method)([&](const crow::request& req) {
            crow::response response;
            response.add_header("Cache-Control", "no-cache");
            response.add_header("Pragma", "no-cache");

            try {
                cv::Mat result{ m_phigros_service->drawPlayerSingleInfoModernStyle("190", 2,"GPwXIOrICHgSWB6rUXhSLmWQ","fajloh66ac75y3yg3i0a54k64","") };

                std::vector<uchar> data;
                cv::imencode(".png", result, data);
                result.release();
                std::string imgStr(data.begin(), data.end());

                response.set_header("Content-Type", "image/png");
                response.write(imgStr);
                return response;
            }catch (const self::HTTPException& e) {
                if (e.getMessage().empty())
                {
                    response.write(StatusCodeHandle::getSimpleJsonResult(e.getCode()).dump(amount_spaces));
                }
                else {
                    response.write(StatusCodeHandle::getSimpleJsonResult(e.getCode(), e.getMessage()).dump(amount_spaces));
                }
                LogSystem::logError(std::format("[Phigros]绘制玩家BEST NEW ------ msg: {} / code: {}", e.what(), e.getCode()));
                response.code = e.getCode();
            }
            catch (const self::TimeoutException& e) {
                LogSystem::logError("[Phigros]绘制玩家BEST NEW ------ API请求超时");
                response.write(StatusCodeHandle::getSimpleJsonResult(408, "Data API request timeout").dump(amount_spaces));
                response.code = 408;
            }
            catch (const std::runtime_error& e) {
                LogSystem::logError(std::format("[Phigros]绘制玩家BEST NEW ------ msg: {} / code: {}", e.what(), 500));
                response.write(StatusCodeHandle::getSimpleJsonResult(500, e.what()).dump(amount_spaces));
                response.code = 500;
            }
            catch (const std::exception& e) {
                LogSystem::logError(std::format("[Phigros]绘制玩家BEST NEW ------ msg: {} / code: {}", e.what(), 500));
                response.write(StatusCodeHandle::getSimpleJsonResult(500, e.what()).dump(amount_spaces));
                response.code = 500;
            }

            response.set_header("Content-Type", "application/json");
            return response;
        });
#endif
    };
};

#endif // !PHIGROS_CONTROLLER_HPP