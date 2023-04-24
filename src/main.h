#pragma once

#ifndef MAIN_H
#define MAIN_H

// 设置1为开启跨域访问(想要性能问题的话建议关闭,使用反向代理)
#define CORS_OPEN  0  

#include <iostream>
#include <memory>

#include "spdlog/spdlog.h"
#include "spdlog/sinks/daily_file_sink.h"
#include "fmt/format.h"

namespace std {
    using fmt::format;
    using fmt::format_error;
    using fmt::formatter;
}

#include <common/utils/sql_util.hpp>
#include <common/utils/log_system.hpp>
#include <configuration/config.hpp>

#include "crow/middlewares/cors.h"
#include "crow.h"

/* =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-= */
#include "service/bottle_service.hpp"
#include "service/impl/bottle_service_impl.hpp"
#include "controller/bottle_controller.hpp"

#include "service/phigros_service.hpp"
#include "service/impl/phigros_service_impl.hpp"
#include "controller/phigros_controller.hpp"
/* =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-= */

// O3优化
#pragma GCC optimize(3)
#pragma G++ optimize(3)

#if OLD 
#define CROW_ENABLE_SSL  
#endif

#define CROW_ENFORCE_WS_SPEC  

//初始化
inline void init(void) {
    Config::initialized();
    LogSystem::initialized();
    SQL_Util::initialized();
}

// 启动项
inline void start(void){
    const std::string
        secret{ Config::Parameter::getSecret()},
        issuer{ Config::Parameter::getIssuer()};
    const ushort port{ Config::Parameter::getPort()};
    const ubyte concurrency{ Config::Parameter::getConcurrency() };

    /* =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-= */

    CrowApp app; 

    // 日志等级
    crow::logger::setLogLevel(crow::LogLevel::DEBUG);

#if CORS_OPEN
    // 跨域访问
    if (has_cors) {
        auto& cors = app.get_middleware<crow::CORSHandler>();
        cors
            .global()
            .headers("origin, x-requested-with, accept, access-control-allow-origin, authorization, content-type")
            .methods("POST"_method, "GET"_method, "PUT"_method, "DELETE"_method, "PATCH"_method, "OPTIONS"_method)
            //.prefix("/cors")
            .origin("*");
    }
#endif
    
    app.bindaddr("0.0.0.0").port(port);
    
    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

    // BottleService
    std::unique_ptr<BottleService> bottle{ new BottleServiceImpl() };
    BottleController bottle_controller(app, secret, issuer, std::move(bottle));
    bottle_controller.controller();

    // PhigrosService
    std::unique_ptr<PhigrosService> phigros_service{ new PhigrosServiceImpl() };
    PhigrosController phigros_controller(app, std::move(phigros_service));
    phigros_controller.controller();

    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

    if (concurrency != 0)
        app.concurrency(concurrency).run();
    else
        app.multithreaded().run();
}

#endif