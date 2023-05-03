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
#include <opencv2/opencv.hpp>
#include <exception>
#include <stdexcept>

using StatusCodeHandle = HTTPUtil::StatusCodeHandle;

constexpr int amount_spaces{ 2 };

class PhigrosController final {
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
                constexpr const char* PARAMS[] { "songId","QRcode" };
                int song_id{};
                bool is_qr_code{ false };

                if (OtherUtil::verifyParam(req, PARAMS[0])) {
                    song_id = std::stoi(req.url_params.get(PARAMS[0]));
                } else {
                    response.set_header("Content-Type", "application/json");
                    response.code = 400;
                    response.write(StatusCodeHandle::getSimpleJsonResult(400, "parameter 'songId' required and parameter cannot be empty.").dump(amount_spaces));
                    return response;
                }

                std::string content{ req.get_header_value(PARAMS[1]) };

                if (content.size() != 0)
                    is_qr_code = true;

                cv::Mat result{ m_phigros_service->drawSongInfomation(
                    std::move(song_id),std::move(is_qr_code),std::move(content))};

                std::vector<uchar> data;
                cv::imencode(".png", result, data);
                result.release();
                std::string imgStr(data.begin(), data.end());

                response.set_header("Content-Type", "image/png");
                response.write(imgStr);
                return response;
            } catch (const self::TimeoutException& e) {
                response.write(StatusCodeHandle::getSimpleJsonResult(408, "Data API request timeout").dump(amount_spaces));
            } catch (const std::runtime_error& e) {
                response.write(StatusCodeHandle::getSimpleJsonResult(500, e.what()).dump(amount_spaces));
            } catch (const std::exception& e) {
                response.write(StatusCodeHandle::getSimpleJsonResult(500, e.what()).dump(amount_spaces));
            }
            response.set_header("Content-Type", "application/json");

            response.code = 500;
            return response;
        });


        CROW_ROUTE(m_app, "/phi/drawSingle").methods("GET"_method)([&](const crow::request& req) {
            crow::response response;
            response.add_header("Cache-Control", "no-cache");
            response.add_header("Pragma", "no-cache");
            try { 

                //cv::Mat result{ m_phigros_service->drawSongInfomation(
                //    std::move(song_id),std::move(is_qr_code),std::move(content)) };

                cv::Mat result{ m_phigros_service->drawPlayerSingleInfo(0,"","")};

                std::vector<uchar> data;
                cv::imencode(".png", result, data);
                result.release();
                std::string imgStr(data.begin(), data.end());

                response.set_header("Content-Type", "image/png");
                response.write(imgStr);
                return response;
            }
            catch (const self::TimeoutException& e) {
                response.write(StatusCodeHandle::getSimpleJsonResult(408, "Data API request timeout").dump(amount_spaces));
            }
            catch (const std::runtime_error& e) {
                response.write(StatusCodeHandle::getSimpleJsonResult(500, e.what()).dump(amount_spaces));
            }
            catch (const std::exception& e) {
                response.write(StatusCodeHandle::getSimpleJsonResult(500, e.what()).dump(amount_spaces));
            }
            
            response.set_header("Content-Type", "application/json");
            response.code = 500;
            return response;
            }
        );
	};
private:
	CrowApp& m_app;
	std::unique_ptr<PhigrosService> m_phigros_service;
	PhigrosController() = delete;
};

#endif // !PHIGROS_CONTROLLER_HPP