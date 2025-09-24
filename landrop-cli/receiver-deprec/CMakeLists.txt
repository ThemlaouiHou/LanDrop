cmake_minimum_required(VERSION 3.16)
project(receiver)

set(CMAKE_CXX_STANDARD 17)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)



find_package(Qt6 REQUIRED COMPONENTS Core Network)

add_executable(receiver
    main.cpp
    networkmanager.cpp
    transferhandler.cpp
    transferhandler.h
    networkmanager.h
)

target_link_libraries(receiver Qt6::Core Qt6::Network)