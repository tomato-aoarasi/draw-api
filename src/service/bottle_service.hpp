/*
 * @File	  : bottle_service_impl.hpp
 * @Coding	  : utf-8
 * @Author    : Bing
 * @Time      : 2023/03/17 23:47
 * @Introduce : 漂流瓶实现类
*/

#ifndef BOTTLE_SERVICE_HPP
#define BOTTLE_SERVICE_HPP

class BottleService {
public:
	virtual ~BottleService() = default;
	virtual std::string getBottle(int) = 0;
	virtual std::string getToken(void) = 0;
private:
};


#endif // !BOTTLE_SERVICE_HPP
