file(GLOB SOURCES
    ${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp
)

if(DEFINED ESP_PLATFORM AND ESP_PLATFORM)
    set(compile_options
        -includesdkconfig.h
    )

    idf_component_register(SRCS ${SOURCES}
                        INCLUDE_DIRS "include"
                        PRIV_INCLUDE_DIRS "include/EPubFile" "src"
                        REQUIRES pugixml idflog spiram-cpp stringutil unzipper)

    target_compile_options(${COMPONENT_TARGET} PUBLIC ${compile_options})
else()
    add_library(epubfile STATIC ${SOURCES})

    target_include_directories(epubfile PUBLIC
            ${CMAKE_CURRENT_SOURCE_DIR}/include
    )
    target_include_directories(epubfile PRIVATE
            ${CMAKE_CURRENT_SOURCE_DIR}/include/EPubFile
            ${CMAKE_CURRENT_SOURCE_DIR}/src
    )
    target_link_libraries(epubfile PUBLIC pugixml idflog spiram-cpp stringutil unzipper)
endif()

project(epubfile)
