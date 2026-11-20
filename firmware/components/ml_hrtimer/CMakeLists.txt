if (PLATFORM_ESP32 OR ESP_PLATFORM)
    idf_component_register(
        SRCS "ml_hrtimer.c"
        INCLUDE_DIRS "."      
        REQUIRES ml_common esp_timer 
    )    
	include("${CMAKE_CURRENT_LIST_DIR}/../../cmake/ml_platform.cmake")
elseif (PLATFORM_PICO)
    add_library(ml_hrtimer
                ml_hrtimer.c
                ml_hrtimer.h 
                )
    target_include_directories(ml_hrtimer INTERFACE ${CMAKE_CURRENT_SOURCE_DIR})
    target_link_libraries(ml_hrtimer PUBLIC pico_stdlib ml_common pico_time)
else()
	message(FATAL_ERROR "ml_hrtimer: Platform undefined !")
endif()
