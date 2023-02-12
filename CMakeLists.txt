cmake_minimum_required(VERSION 3.10)
project(TranslationService)

set(CMAKE_CXX_STANDARD 20)

add_executable(TranslationService main.cpp)

# Link with the Windows API library
target_link_libraries(TranslationService PRIVATE user32)

# Find the curl library
find_package(CURL REQUIRED)
target_link_libraries(TranslationService PRIVATE CURL::libcurl)

# Find the nlohmann/json library
find_package(nlohmann_json 3.2.0 REQUIRED)
target_link_libraries(TranslationService PRIVATE nlohmann_json::nlohmann_json)
