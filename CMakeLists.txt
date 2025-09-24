cmake_minimum_required(VERSION 3.16)

project(landropApp LANGUAGES CXX)

add_subdirectory(landrop buildLandrop)
add_subdirectory(landrop-plus buildLandropPlus)
add_subdirectory(landrop-test buildLandropTest)
add_subdirectory(landrop-cli buildLandropCLI)
