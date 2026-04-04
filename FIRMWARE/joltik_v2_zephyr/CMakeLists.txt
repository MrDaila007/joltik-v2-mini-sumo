cmake_minimum_required(VERSION 3.20.0)
find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(joltik_v2_sumo)

target_sources(app PRIVATE
    src/main.c
    src/motors.c
    src/sensors.c
    src/led.c
    src/servo.c
    src/combat.c
    src/tactics.c
    src/button.c
    src/rc5.c
    src/shell_cmds.c
    src/settings.c
)

target_sources_ifdef(CONFIG_JOLTIK_DISPLAY app PRIVATE
    src/display.c
    src/u8g2_zephyr_hal.c
)

# u8g2 graphics library (vendored subset)
if(CONFIG_JOLTIK_DISPLAY)
    set(U8G2_DIR ${CMAKE_CURRENT_SOURCE_DIR}/lib/u8g2)
    file(GLOB U8G2_SOURCES ${U8G2_DIR}/*.c)
    target_sources(app PRIVATE ${U8G2_SOURCES})
    target_include_directories(app PRIVATE ${U8G2_DIR})
endif()
