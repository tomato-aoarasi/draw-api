/*
 * @File	  : phigros_service.hpp
 * @Coding	  : utf-8
 * @Author    : Bing
 * @Time      : 2023/04/23 21:02
 * @Introduce : 肥鸽肉丝抽象接口类
*/

#pragma once

#ifndef PHIGROS_SERVICE_HPP
#define PHIGROS_SERVICE_HPP  

#include <string>
#include <opencv2/opencv.hpp>

class PhigrosService {
public:
	virtual ~PhigrosService() = default;
	virtual cv::Mat drawSongInfomation(int) = 0;
private:
};


#endif // !PHIGROS_SERVICE_HPP
