# cmake/ml_platform.cmake
# Platform and family detection (Pico / ESP-IDF),
# and definition of C compilation macros:
#   PLATFORM_RP2040, PLATFORM_RP2350
#   PLATFORM_ESP32, PLATFORM_ESP32S2, PLATFORM_ESP32S3, PLATFORM_ESP32C2,
#   PLATFORM_ESP32C3, PLATFORM_ESP32C6, PLATFORM_ESP32H2, PLATFORM_ESP32P4
#   DEBUG_MODE (defined for debug builds: !NDEBUG on Pico, optimization level 0 on ESP)

if(DEFINED ML_PLATFORM_CONFIGURED)
    return()
endif()
set(ML_PLATFORM_CONFIGURED ON)

# --- PICO SDK -------------------------------------------------------
# PICO_PLATFORM is defined by pico-sdk (e.g. rp2040, rp2350)
# See pico_pre_load_platform.cmake.
if(DEFINED PICO_PLATFORM)
    message(STATUS "ml_platform: PICO_PLATFORM='${PICO_PLATFORM}'")

    if(PICO_PLATFORM STREQUAL "rp2040")
        set(PLATFORM_RP2040 ON)
        add_compile_definitions(PLATFORM_RP2040)
    elseif(PICO_PLATFORM MATCHES "^rp2350")
        set(PLATFORM_RP2350 ON)
        add_compile_definitions(PLATFORM_RP2350)
    else()
        message(FATAL_ERROR
            "ml_platform.cmake: PICO_PLATFORM='${PICO_PLATFORM}' unhandled")
    endif()

    # "PICO" family alias
    set(PLATFORM_PICO ON)
    add_compile_definitions(PLATFORM_PICO)

    # Detect Pico W (Wi-Fi) variants: RP2040 and RP2350
    if(PICO_BOARD STREQUAL "pico_w")
        set(PLATFORM_PICOW ON)
        add_compile_definitions(PLATFORM_PICOW)
        message(STATUS "ml_platform: Pico W (RP2040) detected (PICO_BOARD=pico_w)")
    elseif(PICO_BOARD STREQUAL "pico2_w")
        set(PLATFORM_PICOW ON)
        add_compile_definitions(PLATFORM_PICOW)
        message(STATUS "ml_platform: Pico 2 W (RP2350) detected (PICO_BOARD=pico2_w)")
    endif()

    # DEBUG_MODE detection: defined if NDEBUG is NOT defined (Debug build)
    if(NOT CMAKE_BUILD_TYPE STREQUAL "Release")
        add_compile_definitions(DEBUG_MODE)
        message(STATUS "ml_platform: Pico Debug mode detected (CMAKE_BUILD_TYPE != Release)")
    else()
        message(STATUS "ml_platform: Pico Release mode detected (implicitly defines NDEBUG)")
    endif()

# --- ESP-IDF -------------------------------------------------------
# IDF_TARGET and ESP_PLATFORM are managed by ESP-IDF.
elseif(DEFINED IDF_TARGET OR DEFINED ENV{IDF_PATH})
    # ESP_PLATFORM is a C define, not a CMake variable,
    # but IDF_TARGET is always present in the CMake cache.
    if(NOT DEFINED IDF_TARGET)
        # Fallback if someone only set ESP_PLATFORM (custom case).
        message(FATAL_ERROR
            "ml_platform.cmake: ESP-IDF project detected but IDF_TARGET is not defined.")
    endif()

    message(STATUS "ml_platform: ESP-IDF target '${IDF_TARGET}'")

    # Global ESP family
    set(PLATFORM_ESP32 ON)
    add_compile_definitions(PLATFORM_ESP32)

    # Per-variant details
    if(IDF_TARGET STREQUAL "esp32")
        set(PLATFORM_ESP32_ORIG ON)
        add_compile_definitions(PLATFORM_ESP32_ORIG)
    elseif(IDF_TARGET STREQUAL "esp32s2")
        set(PLATFORM_ESP32S2 ON)
        add_compile_definitions(PLATFORM_ESP32S2)
    elseif(IDF_TARGET STREQUAL "esp32s3")
        set(PLATFORM_ESP32S3 ON)
        add_compile_definitions(PLATFORM_ESP32S3)
    elseif(IDF_TARGET STREQUAL "esp32c2")
        set(PLATFORM_ESP32C2 ON)
        add_compile_definitions(PLATFORM_ESP32C2)
    elseif(IDF_TARGET STREQUAL "esp32c3")
        set(PLATFORM_ESP32C3 ON)
        add_compile_definitions(PLATFORM_ESP32C3)
    elseif(IDF_TARGET STREQUAL "esp32c6")
        set(PLATFORM_ESP32C6 ON)
        add_compile_definitions(PLATFORM_ESP32C6)
    elseif(IDF_TARGET STREQUAL "esp32h2")
        set(PLATFORM_ESP32H2 ON)
        add_compile_definitions(PLATFORM_ESP32H2)
    elseif(IDF_TARGET STREQUAL "esp32p4")
        set(PLATFORM_ESP32P4 ON)
        add_compile_definitions(PLATFORM_ESP32P4)
    else()
        message(FATAL_ERROR
            "ml_platform.cmake: IDF_TARGET='${IDF_TARGET}' unhandled")
    endif()

    # ESP-IDF: define DEBUG_MODE for debug optimization (-Og) or no optimization (-O0).
    if(CONFIG_COMPILER_OPTIMIZATION_DEBUG OR CONFIG_COMPILER_OPTIMIZATION_NONE)
        add_compile_definitions(DEBUG_MODE)
        message(STATUS "ml_platform: ESP-IDF Debug mode (-Og/-O0)")
    elseif(CONFIG_COMPILER_OPTIMIZATION_SIZE)
        message(STATUS "ml_platform: ESP-IDF Size mode (-Os/-Oz)")
    elseif(CONFIG_COMPILER_OPTIMIZATION_PERF)
        message(STATUS "ml_platform: ESP-IDF Performance mode (-O2)")
    else()
        message(WARNING "ml_platform: ESP-IDF optimization mode unknown")
    endif()
    # Previous DEBUG_MODE detection: defined for optimization level 0 (Debug/-Og).
    #if(CONFIG_COMPILER_OPTIMIZATION_LEVEL STREQUAL "0")
    #    add_compile_definitions(DEBUG_MODE)
    #    message(STATUS "ml_platform: ESP-IDF Debug mode (CONFIG_COMPILER_OPTIMIZATION_LEVEL=0)")
    #else()
    #    message(STATUS "ml_platform: ESP-IDF Release-like (opt=${CONFIG_COMPILER_OPTIMIZATION_LEVEL})")
    #endif()

else()
    message(FATAL_ERROR
        "ml_platform.cmake : unable to detect the platform (Pico/ESP32). "
        "PICO_PLATFORM or IDF_TARGET must be defined.")
endif()
