cmake_minimum_required(VERSION 3.16)

add_executable(test_fft           test_fft.cpp)
add_executable(test_fingerprint   test_fingerprint.cpp)
add_executable(test_roundtrip     test_roundtrip.cpp)

foreach(t test_fft test_fingerprint test_roundtrip)
    target_link_libraries(${t} PRIVATE shazam_lib)
    add_test(NAME ${t} COMMAND ${t})
endforeach()
