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
#include <configuration/config.hpp>
#include "common/utils/http_util.hpp"
#include "crow.h"
#include "service/phigros_service.hpp"
#include "common/utils/other_util.hpp"
#include <opencv2/opencv.hpp>
#include <exception>
#include <stdexcept>

using StatusCodeHandle = HTTPUtil::StatusCodeHandle;

class PhigrosController final {
public:
	~PhigrosController() = default;

	explicit PhigrosController(CrowApp& app, std::unique_ptr<PhigrosService> phigros_service)
		: m_app{ app }, m_phigros_service{ std::move(phigros_service) } {};

	const inline void controller(void) {
        CROW_ROUTE(m_app, "/test").methods("GET"_method)([&](const crow::request& req) {
            bool is_exception{ false };
            crow::response response;
            try {
                const std::string PARAM{ "songId" };
                int song_id{};

                if (OtherUtil::verifyParam(req, PARAM)) {
                    song_id = std::stoi(req.url_params.get(PARAM));
                } else {
                    response.set_header("Content-Type", "application/json");
                    response.code = 400;
                    response.write(StatusCodeHandle::getSimpleJsonResult(400, "parameter 'songId' required and parameter cannot be empty.").dump());
                    return response;
                }
                cv::Mat result{ m_phigros_service->drawSongInfomation(song_id)};

                std::vector<uchar> data;
                cv::imencode(".png", std::move(result), data);
                std::string imgStr(data.begin(), data.end());

                response.set_header("Content-Type", "image/png");
                response.body = imgStr;
            return response;
            } catch (const std::runtime_error& e) {
                response.write(StatusCodeHandle::getSimpleJsonResult(500, e.what()).dump()); is_exception = true;
            } catch (const std::exception& e) {
                response.write(StatusCodeHandle::getSimpleJsonResult(500, e.what()).dump()); is_exception = true;
            }
            response.set_header("Content-Type", "application/json");
            response.code = 500;
            return response;
        });

	};
private:
	CrowApp& m_app;
	std::unique_ptr<PhigrosService> m_phigros_service;
	PhigrosController() = delete;
};

#endif // !PHIGROS_CONTROLLER_HPP