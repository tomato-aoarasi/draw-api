/*
 * @File	  : phigros_service.hpp
 * @Coding	  : utf-8
 * @Author    : Bing
 * @Time      : 2023/04/23 21:02
 * @Introduce : 肥鸽肉丝抽象接口类
*/

#pragma once

#include <string_view>
#include <opencv2/opencv.hpp>
#include "configuration/config.hpp"

#ifndef PHIGROS_SERVICE_HPP
#define PHIGROS_SERVICE_HPP  

class PhigrosService {
public:
	virtual ~PhigrosService() = default;
	virtual cv::Mat drawSongInfomation(int,bool,std::string_view) = 0;
	// 曲目id,YuhaoToken,SessionToken
	virtual cv::Mat drawPlayerSingleInfo(std::string_view, Ubyte, std::string_view, std::string_view, std::string_view, bool) = 0;
	virtual cv::Mat drawPlayerSingleInfoModernStyle(std::string_view, Ubyte, std::string_view, std::string_view, std::string_view, bool) = 0;
	virtual cv::Mat drawB19(std::string_view, std::string_view , std::string_view, bool) = 0;
private:
};


#endif // !PHIGROS_SERVICE_HPP
