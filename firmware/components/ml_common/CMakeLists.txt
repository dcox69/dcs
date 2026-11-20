if (PLATFORM_ESP32 OR ESP_PLATFORM)
    idf_component_register(
        SRCS "ml_common.c"
        INCLUDE_DIRS "include" 
        REQUIRES log freertos esp_timer ml_datetime
    )    
	include("${CMAKE_CURRENT_LIST_DIR}/../../cmake/ml_platform.cmake")
elseif (PLATFORM_PICO)
    add_library(ml_common 
                ml_common.c
                ml_mem.c
                )
	target_include_directories(ml_common INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/include)
    target_link_libraries(ml_common PUBLIC pico_stdlib pico_time ml_datetime)
else()
	message(FATAL_ERROR "ml_common: Platform undefined !")
endif()
