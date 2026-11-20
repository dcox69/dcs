if (PLATFORM_ESP32 OR ESP_PLATFORM)
    idf_component_register(
        SRCS "ml_datetime.c" "ml_timezone.c"
        INCLUDE_DIRS "."      
        REQUIRES ml_common  
    )    
	include("${CMAKE_CURRENT_LIST_DIR}/../../cmake/ml_platform.cmake")
elseif (PLATFORM_PICO)
    add_library(ml_datetime
                ml_datetome.c
                ml_timezone.c 
                )
    target_include_directories(ml_datetime INTERFACE ${CMAKE_CURRENT_SOURCE_DIR})
    target_link_libraries(ml_datetime PUBLIC pico_stdlib ml_common)
else()
	message(FATAL_ERROR "ml_datetime: Platform undefined !")
endif()
