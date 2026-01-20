cmake_minimum_required(VERSION 4.1)
project(sso_formats_core C)

set(CMAKE_C_STANDARD 99)
set(CMAKE_C_STANDARD_REQUIRED ON)

if(MSVC)
    add_compile_options(/O2 /Oi /Ot /GL)
    add_link_options(/LTCG)
else()
    add_compile_options(-O3 -march=native -mtune=native -fstrict-aliasing)
endif()

add_library(sso_formats_core SHARED
        src/vf.c
        src/text.c
)

target_include_directories(sso_formats_core PUBLIC headers)