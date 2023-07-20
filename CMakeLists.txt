# CMakeList.txt: 顶层 CMake 项目文件，在此处执行全局配置
# 并包含子项目。
#
cmake_minimum_required (VERSION 3.22)

#vcpkg
# 手动设置到你的vcpkg根目录
set(VCPKG_ROOT "/home/software/vcpkg")
set(CMAKE_TOOLCHAIN_FILE "${VCPKG_ROOT}/scripts/buildsystems/vcpkg.cmake"
CACHE STRING "Vcpkg toolchain file")
# CMAKE默认设置release版
set(CMAKE_BUILD_TYPE"Release" CACHE STRING"" FORCE)

project ("CrowProject")

set(CMAKE_C_COMPILER "/usr/local/bin/gcc")
set(CMAKE_C_COMPILER "/usr/bin/gcc")
set(CATKIN_TOPLEVEL TRUE)
set(CMAKE_BUILD_TYPE "Release")
set(CMAKE_CXX_STANDARD 20)

# 开启C++20 export/module
#set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fmodules-ts")

# 内存泄露检测
#set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fmodules-ts -fsanitize=address -fno-omit-frame-pointer")
# 内存检测
#set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -fsanitize=address")

# 输出项目信息
message(STATUS ${PROJECT_NAME} ${PROJECT_VERSION_MAJOR} ${PROJECT_VERSION_MINOR})

# 三方库开始
# 头文件
# python三方库相关
set(Python_INCLUDE_DIRS ${Python_DIR}/include)  
set(Python_LIB_DIRS ${Python_DIR}/lib)  

set(LOCAL_INCLUDE /usr/local/include)
set(GLOBAL_INCLUDE /usr/include)
include_directories(LOCAL_INCLUDE)
include_directories(GLOBAL_INCLUDE)

# context
set(CONTEXT_INCLUDE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/3rdparty/include)
include_directories(${CONTEXT_INCLUDE_PATH})

# boost
set(BOOST_INCLUDE_PATH /usr/include/boost)
include_directories(${BOOST_INCLUDE_PATH})

# redis
set(HIREDIS_INCLUDE_PATH /usr/include/hiredis)
include_directories(${HIREDIS_INCLUDE_PATH})

# OpenSSL
include_directories(${OPENSSL_INCLUDE_DIR})

# OpenCV
set(OPENCV_INCLUDE_PATH /usr/include/opencv4)
include_directories(${OPENCV_INCLUDE_PATH})

# Freetype
set(FREETYPE_INCLUDE_PATH /usr/local/include/freetype2)
include_directories(${FREETYPE_INCLUDE_PATH})


# SQLite Modern C++
set(SQLITE_MODERN_INCLUDE_PATH ${VCPKG_ROOT}/packages/sqlite-modern-cpp_x64-linux)
include_directories(${SQLITE_MODERN_INCLUDE_PATH}/include)

# libgo
# set(LIBGO_INCLUDE_PATH ${VCPKG_ROOT}/packages/libgo_x64-linux/)
# include_directories(${LIBGO_INCLUDE_PATH}/include/libgo)

# qrencode库
find_path(QRENCODE_INCLUDE_DIR NAMES qrencode.h)
find_library(QRENCODE_LIBRARY_RELEASE qrencode)
find_library(QRENCODE_LIBRARY_DEBUG qrencoded)
set(QRENCODE_LIBRARIES optimized ${QRENCODE_LIBRARY_RELEASE} debug ${QRENCODE_LIBRARY_DEBUG})

# 协程库
find_package(nlohmann_json REQUIRED)
find_package(yaml-cpp REQUIRED)
find_package(spdlog REQUIRED)
find_package(unofficial-sqlite3 CONFIG REQUIRED)
find_package(fmt REQUIRED)
find_package(cpprestsdk REQUIRED)
find_package(PythonInterp REQUIRED)
find_package(OpenSSL REQUIRED)
find_package(OpenCV REQUIRED)
find_package(OpenGL REQUIRED)  
find_package(GLU REQUIRED)  
find_package(GLUT REQUIRED)
find_package(Freetype REQUIRED)
find_package(Crow)
find_package(Boost REQUIRED COMPONENTS coroutine)
find_path(SQLITE_MODERN_CPP_INCLUDE_DIRS "sqlite_modern_cpp.h")
# find_package(libgo CONFIG REQUIRED)
# find_package(Poco REQUIRED Net)
# find_package(Poco REQUIRED JSON)
# find_package(Poco REQUIRED Util)

add_definitions(-DCROW_ENABLE_SSL)
set(CROW_ENABLE_SSL ON)

# INCLUDE三方库
set(JWT_PATH     ${CMAKE_CURRENT_SOURCE_DIR}/3rdparty/jwt-cpp-master)
include_directories(${JWT_PATH}/include)

set(BCRYPT_PATH     ${CMAKE_CURRENT_SOURCE_DIR}/3rdparty/Bcrypt-cpp-master)
include_directories(${BCRYPT_PATH}/include)


set(SQLITE_PP_PATH     ${CMAKE_CURRENT_SOURCE_DIR}/3rdparty/sqlitepp)
include_directories(${SQLITE_PP_PATH}/include)

# 导入a
link_directories(${SQLITE_PP_PATH}/lib)
link_directories(${BCRYPT_PATH}/lib)
link_directories(${GOLIB_PATH}/lib)
# 三方库结束

# 指定最终的可执行文件的位置
set(EXECUTABLE_OUTPUT_PATH ${CMAKE_CURRENT_SOURCE_DIR}/bin)
# 设置第三方库编译文件输出路径
set(LIBRARY_OUTPUT_DIRECTORY ${OPENCV_LIB_SHARED_PATH}/lib)

include_directories(${CMAKE_CURRENT_SOURCE_DIR}/src)
# 将源代码添加到此项目的可执行文件。
aux_source_directory(${CMAKE_CURRENT_SOURCE_DIR}/src SRC_LIST)
add_executable (${PROJECT_NAME} ${SRC_LIST})

# 将指定目录文件所有复制到二进制文件目录
file(GLOB allCopyFiles  "${CMAKE_CURRENT_SOURCE_DIR}/resource/*")
file(COPY ${allCopyFiles} DESTINATION ${CMAKE_CURRENT_SOURCE_DIR}/bin)

#add_subdirectory(openssl)  
#Poco::Net Poco::JSON Poco::Util
#target_include_directories(${PROJECT_NAME} private ${QRENCODE_INCLUDE_DIR})

target_link_libraries(${PROJECT_NAME} PUBLIC ${OpenCV_LIBS} ${OPENSSL_LIBRARIES} ${QRENCODE_LIBRARY_RELEASE} 
${SQLITE_MODERN_CPP_INCLUDE_DIRS}
Crow::Crow cpprestsdk::cpprest OpenGL::OpenGL GLU GLUT::GLUT Freetype::Freetype hiredis
nlohmann_json::nlohmann_json fmt::fmt sqlite3 unofficial::sqlite3::sqlite3 yaml-cpp Boost::coroutine
spdlog::spdlog #libgo::libgo
libbcrypt.a libSQLitePP.a)

message(G:${CMAKE_C_COMPILER_VERSION},GXX:${CMAKE_CXX_COMPILER_VERSION},CXX STANDARD:${CMAKE_CXX_STANDARD})
message(final)