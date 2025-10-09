file(GLOB SOURCES
    ${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp
)

if(DEFINED ESP_PLATFORM AND ESP_PLATFORM)
    set(compile_options
        -includesdkconfig.h
    )
    idf_component_register(SRCS ${SOURCES}
        INCLUDE_DIRS "include"
        PRIV_INCLUDE_DIRS "include/Renderer" "src"
        REQUIRES epubfile idflog stringutil
    )

    target_compile_options(${COMPONENT_TARGET} PUBLIC ${compile_options})
else()
    add_library(renderer STATIC ${SOURCES})

    target_include_directories(renderer PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/include
    )
    target_include_directories(renderer PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/include/Renderer
        ${CMAKE_CURRENT_SOURCE_DIR}/src
    )
    target_link_libraries(renderer PUBLIC epubfile idflog spiram-cpp stringutil bmpimage)
endif()

project(Renderer)
