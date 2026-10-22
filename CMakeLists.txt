cmake_minimum_required(VERSION 3.16)
project(imgs361_mode_key_demo LANGUAGES CXX)

find_package(OpenCV REQUIRED COMPONENTS core highgui imgproc)

add_executable(mode_key_demo mode_key_demo.cpp)
target_compile_features(mode_key_demo PRIVATE cxx_std_20)
target_include_directories(mode_key_demo PRIVATE ${OpenCV_INCLUDE_DIRS})
target_link_libraries(mode_key_demo PRIVATE ${OpenCV_LIBS})
