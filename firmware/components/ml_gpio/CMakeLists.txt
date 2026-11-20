if (PLATFORM_ESP32 OR ESP_PLATFORM)
    idf_component_register(
        SRCS "ml_gpio.c"
        INCLUDE_DIRS "."      # Make ml_gpio.h public
        REQUIRES ml_common driver
    )    
	include("${CMAKE_CURRENT_LIST_DIR}/../../cmake/ml_platform.cmake")
elseif (PLATFORM_PICO)
    add_library(ml_gpio 
                ml_gpio.c
                ml_gpio.h 
                )
    target_include_directories(ml_gpio INTERFACE ${CMAKE_CURRENT_SOURCE_DIR})
    target_link_libraries(ml_gpio PUBLIC pico_stdlib hardware_gpio ml_common)
else()
	message(FATAL_ERROR "ml_gpio: Platform not defined !")
endif()
