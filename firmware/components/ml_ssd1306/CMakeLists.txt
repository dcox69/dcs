if (PLATFORM_ESP32 OR ESP_PLATFORM)
    idf_component_register(
        SRCS "ml_ssd1306.c"
        INCLUDE_DIRS "." 
        REQUIRES ml_common ml_i2c ml_uw
		 PRIV_REQUIRES driver
    )    
	include("${CMAKE_CURRENT_LIST_DIR}/../../cmake/ml_platform.cmake")
elseif (PLATFORM_PICO)
	add_library(ml_ssd1306
            ml_ssd1306.c
            ml_ssd1306.h
			)
	target_include_directories(ml_ssd1306 INTERFACE ${CMAKE_CURRENT_SOURCE_DIR})
    target_link_libraries(ml_ssd1306 PUBLIC ml_common pico_stdlib ml_i2c ml_uw)
else()
	message(FATAL_ERROR "ml_ssd1306: Platform undefined !")
endif()
