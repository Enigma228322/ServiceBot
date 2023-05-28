cmake_minimum_required(VERSION 3.15)

project(barbershop)

execute_process (
    COMMAND bash "cp ../configs/configs.json ."
)
execute_process (
    COMMAND bash "cp configs/configs.json build"
)

# set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} --std=c++2a")
set(Boost_USE_MULTITHREADED ON)
set(CMAKE_BUILD_TYPE Debug)

find_package(Threads REQUIRED)
find_package(OpenSSL REQUIRED)
find_package(Boost COMPONENTS system REQUIRED)
find_package(CURL)

set(INCLUDE_DIRS include)
set(BOT_INCLUDE_DIR libs/tgbot-cpp/include)

if (CURL_FOUND)
    include_directories(${CURL_INCLUDE_DIRS})
    add_definitions(-DHAVE_CURL)
endif()

add_executable(bot
    src/main.cpp
    src/utils/base64.cpp
    src/utils/shit.cpp
    src/Admin.cpp
    src/Session.cpp
    src/User.cpp
    src/DBService/DBImpl.cpp
)

include_directories(
    /usr/local/include
    /usr/include/postgresql/internal/libpq
    ${INCLUDE_DIRS}
    ${BOT_INCLUDE_DIR}
    ${OPENSSL_INCLUDE_DIR}
    ${Boost_INCLUDE_DIR}
)

target_link_libraries(bot PRIVATE
    pqxx
    pq
    /usr/local/lib/libTgBot.a
    /usr/lib/x86_64-linux-gnu/libpq.a
    /usr/local/lib/libpqxx.a
    ${CMAKE_THREAD_LIBS_INIT}
    ${OPENSSL_LIBRARIES}
    ${Boost_LIBRARIES}
    ${CURL_LIBRARIES}
)
