if (PLATFORM_ESP32 OR ESP_PLATFORM)
    idf_component_register(
        SRCS "ml_button.c"
        INCLUDE_DIRS "."      
    )    
	include("${CMAKE_CURRENT_LIST_DIR}/../../cmake/ml_platform.cmake")
elseif (PLATFORM_PICO)
    add_library(ml_button
                ml_button.c
                ml_button.h 
                )
    target_include_directories(ml_button INTERFACE ${CMAKE_CURRENT_SOURCE_DIR})
    target_link_libraries(ml_button PUBLIC pico_stdlib)
else()
	message(FATAL_ERROR "ml_button: Platform undefined !")
endif()
