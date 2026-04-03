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

target_sources_ifdef(CONFIG_JOLTIK_DISPLAY app PRIVATE src/display.c)
