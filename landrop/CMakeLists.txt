cmake_minimum_required(VERSION 3.16)

project(landrop LANGUAGES CXX)

# Enable Qt automatic handling of UI files, MOC, and RCC
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

# Set the modern C++ standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Find required Qt6 modules
find_package(Qt6 REQUIRED COMPONENTS Widgets)
find_package(QT NAMES Qt6 Qt5 REQUIRED COMPONENTS Core)
find_package(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS Core)
find_package(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS Network)

# For unit tests (ctest)
enable_testing()

# List source files and headers
set(PROJECT_SOURCES
    main.cpp
    
    config/config.cpp
    config/config.h

    network/sender.cpp
    network/sender.h
    network/receiver.cpp
    network/receiver.h

    ui/mainwindow.cpp
    ui/mainwindow.h
    ui/sendfilewidget.cpp
    ui/sendfilewidget.h
    ui/sharedfileswidget.cpp
    ui/sharedfileswidget.h
    ui/userlistwidget.cpp
    ui/userlistwidget.h
    ui/transferitemwidget.cpp
    ui/transferitemwidget.h
    ui/transferhistorywidget.h
    ui/transferhistorywidget.cpp
    ui/batchrequestdialog.h
    ui/batchrequestdialog.cpp
    ui/configdialog.h
    ui/configdialog.cpp
    ui/aboutdialog.h
    ui/aboutdialog.cpp
)

# Resource file
set(RESOURCES resources.qrc)

# Define the executable target
qt_add_executable(landrop
    MANUAL_FINALIZATION
    ${PROJECT_SOURCES}
    ${RESOURCES}
)

# Link Qt libraries
target_link_libraries(landrop PRIVATE 
    Qt${QT_VERSION_MAJOR}::Widgets
    Qt${QT_VERSION_MAJOR}::Core
    Qt${QT_VERSION_MAJOR}::Network
)

# For WindowsOS, set console creation.
set_target_properties(landrop PROPERTIES
    WIN32_EXECUTABLE TRUE
)

# Finalize Qt6 builds
qt_finalize_target(landrop)
