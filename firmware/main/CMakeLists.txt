idf_component_register( SRC_DIRS "."
                        INCLUDE_DIRS "."
                        REQUIRES    
                        nvs_flash
                        ml_common
                        ml_i2c
                        ml_gpio 
                        ml_sht3x 
                        ml_ssd1306                 
                        ml_hrtimer
                        ml_ec11  
                        ml_uw   
                        ml_button      
                        ml_pid_heater
                    )

target_compile_options(${COMPONENT_LIB} PRIVATE
    -Wall
    -Wextra
    -Wformat
    -Werror      
    -Wno-unused-parameter 
)