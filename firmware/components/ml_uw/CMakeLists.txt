if (PLATFORM_ESP32 OR ESP_PLATFORM)
    idf_component_register(
        SRCS "ml_uw.c"
        INCLUDE_DIRS "." 
        REQUIRES "ml_common"
    )    
	include("${CMAKE_CURRENT_LIST_DIR}/../../cmake/ml_platform.cmake")
elseif (PLATFORM_PICO)
    add_library(ml_uw 
                ml_uw.c
                )
	target_include_directories(ml_sb INTERFACE ${CMAKE_CURRENT_SOURCE_DIR})
    target_link_libraries(ml_uw PUBLIC ml_common pico_stdlib)
else()
	message(FATAL_ERROR "ml_uw: Platform undefined !")
endif()
