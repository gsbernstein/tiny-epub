cmake_minimum_required(VERSION 3.20)

project(tiny_epub_tests LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

get_filename_component(TINY_EPUB_ROOT "${CMAKE_CURRENT_LIST_DIR}/.." ABSOLUTE)

enable_testing()

# Vendor'd libraries
add_subdirectory(${TINY_EPUB_ROOT}/tests/Catch2 ${CMAKE_CURRENT_BINARY_DIR}/Catch2)
add_subdirectory(${TINY_EPUB_ROOT}/components/pugixml ${CMAKE_CURRENT_BINARY_DIR}/pugixml)
add_subdirectory(${TINY_EPUB_ROOT}/components/miniz ${CMAKE_CURRENT_BINARY_DIR}/miniz)

# cpp-misc libraries
include(FetchContent)
FetchContent_Declare(cpp_misc
    GIT_REPOSITORY https://github.com/sindarin-inc/cpp-misc
    GIT_TAG main
)
FetchContent_MakeAvailable(cpp_misc)

add_subdirectory("${cpp_misc_SOURCE_DIR}/bmpimage" ${CMAKE_CURRENT_BINARY_DIR}/bmpimage)
add_subdirectory("${cpp_misc_SOURCE_DIR}/idflog" ${CMAKE_CURRENT_BINARY_DIR}/idflog)
add_subdirectory("${cpp_misc_SOURCE_DIR}/unzipper" ${CMAKE_CURRENT_BINARY_DIR}/unzipper)
add_subdirectory("${cpp_misc_SOURCE_DIR}/imagetest" ${CMAKE_CURRENT_BINARY_DIR}/imagetest)
add_subdirectory("${cpp_misc_SOURCE_DIR}/spiram-cpp" ${CMAKE_CURRENT_BINARY_DIR}/spiram-cpp)
add_subdirectory("${cpp_misc_SOURCE_DIR}/stringutil" ${CMAKE_CURRENT_BINARY_DIR}/stringutil)

# Alternately, comment out the above and point to a local cpp-misc

# set(LOCAL_CPP_MISC "${TINY_EPUB_ROOT}/../embedded-app-2/components/cpp-misc")
# add_subdirectory("${LOCAL_CPP_MISC}/bmpimage" ${CMAKE_CURRENT_BINARY_DIR}/bmpimage)
# add_subdirectory("${LOCAL_CPP_MISC}/idflog" ${CMAKE_CURRENT_BINARY_DIR}/idflog)
# add_subdirectory("${LOCAL_CPP_MISC}/unzipper" ${CMAKE_CURRENT_BINARY_DIR}/unzipper)
# add_subdirectory("${LOCAL_CPP_MISC}/imagetest" ${CMAKE_CURRENT_BINARY_DIR}/imagetest)
# add_subdirectory("${LOCAL_CPP_MISC}/spiram-cpp" ${CMAKE_CURRENT_BINARY_DIR}/spiram-cpp)
# add_subdirectory("${LOCAL_CPP_MISC}/stringutil" ${CMAKE_CURRENT_BINARY_DIR}/stringutil)

# Our libraries
add_subdirectory(${TINY_EPUB_ROOT}/epubfile ${CMAKE_CURRENT_BINARY_DIR}/epubfile)
add_subdirectory(${TINY_EPUB_ROOT}/renderer ${CMAKE_CURRENT_BINARY_DIR}/renderer)

# Ensure renderer library itself picks up CONFIG_* values for host builds
target_compile_definitions(renderer PUBLIC
    CONFIG_EPUB_RENDERER_MAX_BYTES_PER_PARAGRAPH=1500
    CONFIG_EPUB_RENDERER_MAX_BYTES_PER_DISPLAY_PAGE=4000
    CONFIG_EPUB_RENDERER_CSTR_POOL_SIZE=8192
    CONFIG_EPUB_RENDERER_TOKENS_SIZE=2048
    CONFIG_EPUB_RENDERER_MAX_BYTES_PER_LINE=1024
    CONFIG_EPUB_RENDERER_LINES_SIZE=256)

add_executable(tests_epub
    ${CMAKE_CURRENT_LIST_DIR}/TestEPubFile.cpp
)
target_compile_definitions(tests_epub PRIVATE TESTS_DIR="${CMAKE_CURRENT_LIST_DIR}")
target_link_libraries(tests_epub PRIVATE catch2 epubfile)

add_executable(tests_renderer
    ${CMAKE_CURRENT_LIST_DIR}/TestRenderer.cpp
)
target_compile_definitions(tests_renderer PRIVATE TESTS_DIR="${CMAKE_CURRENT_LIST_DIR}")
find_package(PkgConfig REQUIRED)
pkg_check_modules(SDL2 REQUIRED sdl2)
pkg_check_modules(SDL2_TTF REQUIRED SDL2_ttf)
target_include_directories(tests_renderer PRIVATE ${SDL2_INCLUDE_DIRS} ${SDL2_TTF_INCLUDE_DIRS})
if(SDL2_LIBRARY_DIRS)
    target_link_directories(tests_renderer PRIVATE ${SDL2_LIBRARY_DIRS})
endif()
if(SDL2_TTF_LIBRARY_DIRS)
    target_link_directories(tests_renderer PRIVATE ${SDL2_TTF_LIBRARY_DIRS})
endif()
if(SDL2_CFLAGS_OTHER)
    target_compile_options(tests_renderer PRIVATE ${SDL2_CFLAGS_OTHER})
endif()
if(SDL2_TTF_CFLAGS_OTHER)
    target_compile_options(tests_renderer PRIVATE ${SDL2_TTF_CFLAGS_OTHER})
endif()
if(SDL2_LDFLAGS_OTHER)
    target_link_options(tests_renderer PRIVATE ${SDL2_LDFLAGS_OTHER})
endif()
if(SDL2_TTF_LDFLAGS_OTHER)
    target_link_options(tests_renderer PRIVATE ${SDL2_TTF_LDFLAGS_OTHER})
endif()
target_link_libraries(tests_renderer PRIVATE ${SDL2_LIBRARIES} ${SDL2_TTF_LIBRARIES})
find_package(PNG REQUIRED)
target_link_libraries(tests_renderer PRIVATE PNG::PNG)
target_link_libraries(tests_renderer PRIVATE catch2 renderer imagetest)

# Sanitizers (opt-in): configure with -DTINY_EPUB_ENABLE_SANITIZERS=ON
option(TINY_EPUB_ENABLE_SANITIZERS "Enable Address/Undefined sanitizers" OFF)
if(TINY_EPUB_ENABLE_SANITIZERS)
    message(STATUS "Sanitizers enabled for tests")
    # Pass flags as a list so CMake doesn't concatenate them into a single argument
    set(SAN_COMPILE_FLAGS -fsanitize=address -fsanitize=undefined -fno-omit-frame-pointer)
    set(SAN_LINK_FLAGS -fsanitize=address -fsanitize=undefined)
    foreach(tgt tests_epub tests_renderer epubfile renderer)
        if(TARGET ${tgt})
            target_compile_options(${tgt} PRIVATE ${SAN_COMPILE_FLAGS})
            target_link_options(${tgt} PRIVATE ${SAN_LINK_FLAGS})
        endif()
    endforeach()
endif()

add_test(NAME epubfile_basic_load COMMAND tests_epub)
add_test(NAME renderer_epub_render COMMAND tests_renderer)
add_test(NAME renderer_txt_file COMMAND tests_renderer "[renderer][txt][file]")
add_test(NAME renderer_txt_string COMMAND tests_renderer "[renderer][txt][string]")
