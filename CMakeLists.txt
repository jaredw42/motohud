cmake_minimum_required(VERSION 3.16)
project(motohud LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTORCC ON)

# Find Qt6 packages
find_package(Qt6 REQUIRED COMPONENTS Widgets Network)

# Add source files
set(SOURCES
    motohud.cpp
    main_window.cpp
    devices/gnss_client.cpp
    devices/ublox_parser.cpp
    widgets/speedometer_compass.cpp
    widgets/gnss_status.cpp
)

set(HEADERS
    main_window.h
    devices/gnss_client.h
    devices/ublox_parser.h
    widgets/speedometer_compass.h
    widgets/gnss_status.h
)

# Create executable
add_executable(motohud ${SOURCES} ${HEADERS})

# Enable Qt6 MOC, UIC, and RCC
qt_standard_project_setup()

# Link Qt6 libraries
target_link_libraries(motohud PRIVATE Qt6::Widgets Qt6::Network)

# Include directories
target_include_directories(motohud PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/devices
    ${CMAKE_CURRENT_SOURCE_DIR}/widgets
)