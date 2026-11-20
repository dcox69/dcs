if (PLATFORM_ESP32 OR ESP_PLATFORM)
    idf_component_register(
        SRCS "ml_i2c.c"
        INCLUDE_DIRS "." 
        REQUIRES ml_common driver
    )    
	include("${CMAKE_CURRENT_LIST_DIR}/../../cmake/ml_platform.cmake")
elseif (PLATFORM_PICO)
	add_library(ml_i2c
   	 	ml_i2c.c
    	ml_i2c.h
	)
	target_include_directories(ml_i2c INTERFACE ${CMAKE_CURRENT_SOURCE_DIR})
    target_link_libraries(ml_i2c PUBLIC ml_common pico_stdlib hardware_i2c hardware_gpio)
else()
	message(FATAL_ERROR "ml_i2c: Plaform undefined !")
endif()
