cmake_minimum_required(VERSION 3.10)
project(ExchangeProject)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include_directories(include)

set(DBMS_CORE_SRC
    src/storage.cpp
    src/schema.cpp
    src/engine.cpp
)

set(EXCHANGE_SRC
    src/exchange.cpp
    src/http_server.cpp
)

add_executable(DBMS ${DBMS_CORE_SRC} src/dbms_main.cpp)
target_link_libraries(DBMS pthread)

add_executable(Exchange ${DBMS_CORE_SRC} ${EXCHANGE_SRC} src/exchange_main.cpp)
target_link_libraries(Exchange pthread)
