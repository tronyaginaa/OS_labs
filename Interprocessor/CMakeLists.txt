cmake_minimum_required(VERSION 3.16)
project(lab2)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
add_compile_options(-Wall -Werror)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

if(CMAKE_VERSION VERSION_LESS "3.7.0")
    set(CMAKE_INCLUDE_CURRENT_DIR ON)
endif()

find_package (Threads)
find_library (LIBRT rt)

file(GLOB CONNECTIONS Connections/conn_*.cpp)
file(GLOB HOST queue.hpp Host/*.cpp Host/*.hpp)
file(GLOB CLIENT queue.hpp Client/*.cpp Client/*.hpp)

foreach(PATH_TO_CON ${CONNECTIONS})
    get_filename_component(FILE_NAME ${PATH_TO_CON} NAME_WLE )
    string(REGEX REPLACE "conn_" "" CONN_NAME ${FILE_NAME})

    add_executable(host_${CONN_NAME} ${HOST}
            Connections/connection.hpp Connections/conn_${CONN_NAME}.cpp)
    add_executable(client_${CONN_NAME} ${CLIENT}
            Connections/connection.hpp Connections/conn_${CONN_NAME}.cpp Host/host.hpp)
endforeach()
