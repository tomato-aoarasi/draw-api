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

namespace self{
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
        TimeoutException(const char* msg = "Timeout Exception") : std::runtime_error(msg){
            this->msg = msg;
        };

        const char* getMessage() {
            return msg;
        }

        virtual const char* what() const throw() {
            return msg;
        }
    };
}

#endif