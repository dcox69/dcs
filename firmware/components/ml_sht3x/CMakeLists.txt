if (PLATFORM_ESP32 OR ESP_PLATFORM)
    idf_component_register(
        SRCS "ml_sht3x.c"
        INCLUDE_DIRS "." 
        REQUIRES ml_common ml_i2c driver
    )    
	include("${CMAKE_CURRENT_LIST_DIR}/../../cmake/ml_platform.cmake")
elseif (PLATFORM_PICO)
	add_library(ml_sht3x
            ml_sht3x.c
            ml_sht3x.h
			)
	target_include_directories(ml_sht3x INTERFACE ${CMAKE_CURRENT_SOURCE_DIR})
    target_link_libraries(ml_sht3x PUBLIC ml_common pico_stdlib ml_i2c)
else()
	message(FATAL_ERROR "ml_sht3x: Platform undefined !")
endif()
