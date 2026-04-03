cmake_minimum_required(VERSION 3.20.0)
list(APPEND DTS_ROOT ${CMAKE_CURRENT_SOURCE_DIR})
find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(joltik_v2_tests)

# Application source under test
set(APP_SRC ${CMAKE_CURRENT_SOURCE_DIR}/../src)

target_sources(app PRIVATE
    # Test files
    src/main.c
    src/test_rc5.c
    src/test_motors.c
    src/test_sensors.c
    src/test_servo.c
    src/test_settings.c
    src/test_led.c
    src/test_combat.c

    # Source under test
    ${APP_SRC}/rc5.c
    ${APP_SRC}/motors.c
    ${APP_SRC}/sensors.c
    ${APP_SRC}/servo.c
    ${APP_SRC}/settings.c
    ${APP_SRC}/combat.c
    ${APP_SRC}/tactics.c
    ${APP_SRC}/led.c

    # Fakes
    src/fake_led_strip.c
)

target_include_directories(app PRIVATE ${APP_SRC})
