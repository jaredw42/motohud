cmake_minimum_required(VERSION 3.16)
project(motohud LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTORCC ON)

# Find Qt6 packages
find_package(Qt6 REQUIRED COMPONENTS Widgets Network)
find_package(PkgConfig REQUIRED)
pkg_check_modules(GPIOD REQUIRED IMPORTED_TARGET libgpiod)

# Add source files
set(SOURCES
    motohud.cpp
    main_window.cpp
    devices/gnss_client.cpp
    devices/rpi_pwm_gpio.cpp
    devices/ublox_parser.cpp
    widgets/speedometer_compass.cpp
    widgets/speedometer_compass_config.cpp
    widgets/gnss_status.cpp
    widgets/lcd_display.cpp
    widgets/retro_speedo_tach.cpp
)

set(HEADERS
    main_window.h
    devices/gnss_client.h
    devices/rpi_pwm_gpio.h
    devices/ublox_parser.h
    widgets/speedometer_compass.h
    widgets/speedometer_compass_config.h
    widgets/gnss_status.h
    widgets/tile.h
    widgets/lcd_display.h
    widgets/retro_speedo_tach.h
)

# Create executable
add_executable(motohud ${SOURCES} ${HEADERS})

# Enable Qt6 MOC, UIC, and RCC
qt_standard_project_setup()

# Link Qt6 libraries
target_link_libraries(motohud PRIVATE Qt6::Widgets Qt6::Network)
target_link_libraries(motohud PRIVATE PkgConfig::GPIOD)

if(GPIOD_VERSION VERSION_GREATER_EQUAL "2.0")
    target_compile_definitions(motohud PRIVATE MOTOHUD_LIBGPIOD_V2)
endif()

# Include directories
target_include_directories(motohud PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/devices
    ${CMAKE_CURRENT_SOURCE_DIR}/widgets
)