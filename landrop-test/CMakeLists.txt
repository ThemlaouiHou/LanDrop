cmake_minimum_required(VERSION 3.16)

project(landropTest LANGUAGES CXX)

enable_testing()

find_package(QT NAMES Qt6 Qt5 REQUIRED COMPONENTS Test)
find_package(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS Test)
find_package(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS Core)
find_package(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS Widgets)
find_package(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS Network)

set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Main test executable
add_executable(landropTest test_main.cpp)

# Individual test executables with source files from landrop-plus
add_executable(testSharedFileManager 
    test_sharedfilemanager.cpp 
    ../landrop-plus/services/sharedfilemanager.cpp
    ../landrop-plus/config/config.cpp
)
target_include_directories(testSharedFileManager PRIVATE ../landrop-plus)

add_executable(testNetworkManager 
    test_networkmanager.cpp 
    ../landrop-plus/services/networkmanager.cpp
    ../landrop-plus/config/config.cpp
)
target_include_directories(testNetworkManager PRIVATE ../landrop-plus)

add_executable(testFileTransferManager 
    test_filetransfermanager.cpp 
    ../landrop-plus/services/filetransfermanager.cpp
    ../landrop-plus/network/sender.cpp
    ../landrop-plus/network/receiver.cpp
    ../landrop-plus/config/config.cpp
)
target_include_directories(testFileTransferManager PRIVATE ../landrop-plus)

add_executable(testSender 
    test_sender.cpp 
    ../landrop-plus/network/sender.cpp
    ../landrop-plus/config/config.cpp
)
target_include_directories(testSender PRIVATE ../landrop-plus)

add_executable(testReceiver 
    test_receiver.cpp 
    ../landrop-plus/network/receiver.cpp
    ../landrop-plus/network/sender.cpp
    ../landrop-plus/config/config.cpp
)
target_include_directories(testReceiver PRIVATE ../landrop-plus)

add_executable(testDiscoveryService 
    test_discoveryservice.cpp 
    ../landrop-plus/services/broadcastdiscoveryservice.cpp
    ../landrop-plus/services/networkmanager.cpp
    ../landrop-plus/services/sharedfilemanager.cpp
    ../landrop-plus/config/config.cpp
)
target_include_directories(testDiscoveryService PRIVATE ../landrop-plus)

# Register tests
add_test(NAME mainTest COMMAND landropTest)
add_test(NAME sharedFileManagerTest COMMAND testSharedFileManager)
add_test(NAME networkManagerTest COMMAND testNetworkManager)
add_test(NAME fileTransferManagerTest COMMAND testFileTransferManager)
add_test(NAME senderTest COMMAND testSender)
add_test(NAME receiverTest COMMAND testReceiver)
add_test(NAME discoveryServiceTest COMMAND testDiscoveryService)

# Link libraries
target_link_libraries(landropTest PRIVATE Qt${QT_VERSION_MAJOR}::Test)
target_link_libraries(testSharedFileManager PRIVATE Qt${QT_VERSION_MAJOR}::Test Qt6::Core Qt6::Network)
target_link_libraries(testNetworkManager PRIVATE Qt${QT_VERSION_MAJOR}::Test Qt6::Core Qt6::Network)
target_link_libraries(testFileTransferManager PRIVATE Qt${QT_VERSION_MAJOR}::Test Qt6::Core Qt6::Network)
target_link_libraries(testSender PRIVATE Qt${QT_VERSION_MAJOR}::Test Qt6::Core Qt6::Network)
target_link_libraries(testReceiver PRIVATE Qt${QT_VERSION_MAJOR}::Test Qt6::Core Qt6::Network)
target_link_libraries(testDiscoveryService PRIVATE Qt${QT_VERSION_MAJOR}::Test Qt6::Core Qt6::Network)

