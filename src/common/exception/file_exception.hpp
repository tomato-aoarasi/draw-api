/*
 * @File	  : file_exception.hpp
 * @Coding	  : utf-8
 * @Author    : Bing
 * @Time      : 2023/03/05 21:15
 * @Introduce : 文件异常类
*/

#pragma once

#ifndef FILE_EXCEPTION_HPP
#define FILE_EXCEPTION_HPP
#include <exception>
#include <stdexcept>

namespace self{
    class file_exception : public std::exception {
    private:
        const char* msg{ "file exception" };
    public:
        file_exception(const char* msg_) {
            msg = msg_;
        }

        const char* getMessage() {
            return msg;
        }

        virtual const char* what() const throw() {
            return msg;
        }
    };
}

#endif