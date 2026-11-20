   idf_component_register(
        SRCS "ml_pid_heater.c"
        INCLUDE_DIRS "."      
    )    
	include("${CMAKE_CURRENT_LIST_DIR}/../../cmake/ml_platform.cmake")
