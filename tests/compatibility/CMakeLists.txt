cmake_minimum_required(VERSION 3.20)
project(classify_scalar_charconv_smoke LANGUAGES CXX)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CLASSIFY_SCALAR_BUILD_TESTS OFF CACHE BOOL "" FORCE)
add_subdirectory(../.. classify-scalar)
enable_testing()

foreach(mode configured standalone negative optout)
    add_executable(${mode} ../charconv_smoke.cpp)
    target_include_directories(${mode} PRIVATE ../../include)
    add_test(NAME ${mode} COMMAND ${mode})
endforeach()
target_link_libraries(configured PRIVATE classify_scalar::classify_scalar)
target_compile_definitions(configured PRIVATE
    EXPECT_FLOAT_FROM_CHARS=$<BOOL:${CLASSIFY_SCALAR_CMAKE_FLOAT_FROM_CHARS}>)
target_compile_definitions(negative PRIVATE
    CLASSIFY_SCALAR_USE_STD_FLOAT_FROM_CHARS=0 EXPECT_FLOAT_FROM_CHARS=0)
# Even an explicitly positive result must lose to the user's opt-out.
target_compile_definitions(optout PRIVATE CLASSIFY_SCALAR_USE_STD_FLOAT_FROM_CHARS=1
    CLASSIFY_SCALAR_DISABLE_STD_FLOAT_FROM_CHARS EXPECT_FLOAT_FROM_CHARS=0)
