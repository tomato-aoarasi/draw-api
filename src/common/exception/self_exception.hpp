/*
 * @File	  : self_exception.hpp
 * @Coding	  : utf-8
 * @Author    : Bing
 * @Time      : 2023/03/05 21:15
 * @Introduce : 文件异常类
*/

#pragma once

#ifndef SELF_EXCEPTION_HPP
#define SELF_EXCEPTION_HPP  
#include <exception>
#include <stdexcept>
#include <string>
#include <string_view>

namespace self {
    class FileException : public std::exception {
    private:
        const char* msg{ "File Exception" };
    public:
        FileException(const char* msg) {
            this->msg = msg;
        }

        const char* getMessage() {
            return msg;
        }

        virtual const char* what() const throw() {
            return msg;
        }
    };

    class TimeoutException : public std::runtime_error {
    private:
        const char* msg{};
    public:
        TimeoutException(const char* msg = "Timeout Exception") : std::runtime_error(msg) {
            this->msg = msg;
        };

        const char* getMessage() {
            return msg;
        }

        virtual const char* what() const throw() {
            return msg;
        }
    };

    class HTTPException : public std::runtime_error {
    private:
        std::string msg{};
        unsigned short code{ 500 };
    public:
        HTTPException(std::string_view msg = "Severe HTTP Error", unsigned short code = 500) : std::runtime_error(msg.data()) {
            this->msg = msg;
            this->code = code;
        };

        const std::string& getMessage() const {
            return this->msg;
        }

        const unsigned short getCode() const {
            return this->code;
        }

        virtual const char* what() const throw() {
            return this->msg.data();
        }
    };
};

#endif