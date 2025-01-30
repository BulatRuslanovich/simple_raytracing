cmake_minimum_required(VERSION 3.10)
project(simple_raytracing LANGUAGES C)

# Настройка стандарта C
set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS OFF)

# Поиск необходимых пакетов
find_package(SDL2 REQUIRED)
find_package(PkgConfig REQUIRED)

pkg_search_module(SDL2_GFX REQUIRED SDL2_gfx)
pkg_search_module(SDL2_TTF REQUIRED SDL2_ttf)

# Настройка исполняемого файла
add_executable(simple_raytracing raytracing.c)

# Настройка путей для заголовочных файлов
target_include_directories(simple_raytracing PRIVATE
        ${SDL2_INCLUDE_DIRS}
        ${SDL2_GFX_INCLUDE_DIRS}
        ${SDL2_TTF_INCLUDE_DIRS}
)

# Настройка линковки
target_link_libraries(simple_raytracing
        ${SDL2_LIBRARIES}
        ${SDL2_GFX_LIBRARIES}
        ${SDL2_TTF_LIBRARIES}
        m  # для математической библиотеки (-lm)
)

# Дополнительные флаги компилятора
if(CMAKE_C_COMPILER_ID MATCHES "GNU|Clang")
    target_compile_options(simple_raytracing PRIVATE -Wall -Wextra -O2)
endif()