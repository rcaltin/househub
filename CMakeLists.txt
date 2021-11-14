cmake_minimum_required (VERSION 3.16)

# project name
project (househub)

# set cmake params
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
# set(CMAKE_CXX_STANDARD 17)
# target_compile_features(${PROJECT_NAME} PRIVATE cxx_std_17)
add_compile_options(-std=c++17)

# conan build information
if(NOT EXISTS "${CMAKE_BINARY_DIR}/conan.cmake")
   message(STATUS "Downloading conan.cmake from https://github.com/conan-io/cmake-conan")
   file(DOWNLOAD "https://raw.githubusercontent.com/conan-io/cmake-conan/master/conan.cmake"
                  "${CMAKE_BINARY_DIR}/conan.cmake")
endif()
include(${CMAKE_BINARY_DIR}/conan.cmake)
conan_cmake_run(CONANFILE conanfile.txt BASIC_SETUP BUILD missing CMAKE_TARGETS)

# project includes
include_directories(
    include
    thirdParty/inih/include
)

# project sources
set(SOURCES
	src/main.cpp
    src/app.cpp
    src/capturer.cpp
    src/capturer_factory.cpp
    src/file_manager.cpp
	src/video_out_stream.cpp
)

# add executable
add_executable (${PROJECT_NAME} 
    ${SOURCES}
)

# includes
target_include_directories(${PROJECT_NAME} 
    PRIVATE 
    ${PROJECT_SOURCE_DIR}/include
)

# link against conan packages
target_link_libraries(${PROJECT_NAME}
    PRIVATE
    stdc++fs
    CONAN_PKG::opencv
)