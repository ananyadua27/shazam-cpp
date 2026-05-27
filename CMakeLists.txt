cmake_minimum_required(VERSION 3.16)
project(shazam-cpp VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    add_compile_options(
        -Wall -Wextra -Wpedantic
        -O2
        -msse2
    )
elseif(MSVC)
    add_compile_options(/W4 /O2 /arch:SSE2)
endif()

find_package(PkgConfig REQUIRED)

pkg_check_modules(SNDFILE REQUIRED sndfile)

find_package(SQLite3 REQUIRED)

add_library(shazam_lib STATIC
    src/fft.cpp
    src/spectrogram.cpp
    src/fingerprint.cpp
    src/database.cpp
    src/matcher.cpp
    src/audio_reader.cpp
)

target_include_directories(shazam_lib
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/include
        ${SNDFILE_INCLUDE_DIRS}
        ${SQLite3_INCLUDE_DIRS}
)

target_link_libraries(shazam_lib
    PUBLIC
        ${SNDFILE_LIBRARIES}
        SQLite::SQLite3
)

add_executable(shazam src/main.cpp)
target_link_libraries(shazam PRIVATE shazam_lib)

enable_testing()
add_subdirectory(tests)

install(TARGETS shazam DESTINATION bin)
