cmake_minimum_required(VERSION 3.16)
project(CPUScheduler VERSION 1.0 LANGUAGES CXX)

# Set C++ standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Enable automatic MOC, UIC, RCC for Qt
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

# Find Qt6 packages
find_package(Qt6 REQUIRED COMPONENTS Core Widgets)

# Include directories
include_directories(
    ${CMAKE_SOURCE_DIR}/src
    ${CMAKE_SOURCE_DIR}/src/kernel
    ${CMAKE_SOURCE_DIR}/src/gui
    ${CMAKE_SOURCE_DIR}/src/utils
)

# Collect source files
set(KERNEL_SOURCES
    src/kernel/process.cpp
    src/kernel/ready_queue.cpp
    src/kernel/scheduler.cpp
)

set(GUI_SOURCES
    src/gui/mainwindow.cpp
    src/gui/process_table_widget.cpp
    src/gui/stats_widget.cpp
)

set(UTILS_SOURCES
    src/utils/logger.cpp
)

set(MAIN_SOURCE
    src/main.cpp
)

# Collect header files (for IDE support)
set(KERNEL_HEADERS
    src/kernel/process.h
    src/kernel/ready_queue.h
    src/kernel/scheduler.h
    src/kernel/spinlock.h
)

set(GUI_HEADERS
    src/gui/mainwindow.h
    src/gui/process_table_widget.h
    src/gui/stats_widget.h
)

set(UTILS_HEADERS
    src/utils/logger.h
)

# Create executable
add_executable(cpu_scheduler
    ${MAIN_SOURCE}
    ${KERNEL_SOURCES}
    ${GUI_SOURCES}
    ${UTILS_SOURCES}
    ${KERNEL_HEADERS}
    ${GUI_HEADERS}
    ${UTILS_HEADERS}
)

# Link Qt libraries
target_link_libraries(cpu_scheduler
    Qt6::Core
    Qt6::Widgets
)

# Compiler flags
target_compile_options(cpu_scheduler PRIVATE
    -Wall
    -Wextra
    -pthread
)

# Link threading library
find_package(Threads REQUIRED)
target_link_libraries(cpu_scheduler Threads::Threads)

# Platform-specific settings
if(APPLE)
    set_target_properties(cpu_scheduler PROPERTIES
        MACOSX_BUNDLE FALSE
    )
endif()

# Installation
install(TARGETS cpu_scheduler
    RUNTIME DESTINATION bin
)

# Print configuration summary
message(STATUS "=== CPU Scheduler Build Configuration ===")
message(STATUS "Qt6 version: ${Qt6_VERSION}")
message(STATUS "C++ standard: ${CMAKE_CXX_STANDARD}")
message(STATUS "Build type: ${CMAKE_BUILD_TYPE}")
message(STATUS "========================================")
