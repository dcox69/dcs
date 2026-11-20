if (PLATFORM_ESP32 OR ESP_PLATFORM)
    idf_component_register(
        SRCS "ml_ec11.c"
        INCLUDE_DIRS "."
    )    
	include("${CMAKE_CURRENT_LIST_DIR}/../../cmake/ml_platform.cmake")
elseif (PLATFORM_PICO)
	add_library(ml_ec11
            ml_ec11.c
            ml_ec11.h
			)
	target_include_directories(ml_ec11 INTERFACE ${CMAKE_CURRENT_SOURCE_DIR})
    target_link_libraries(ml_ec11 PUBLIC pico_stdlib)
else()
	message(FATAL_ERROR "ml_ec11: Platform undefined !")
endif()
