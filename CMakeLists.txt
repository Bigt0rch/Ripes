cmake_minimum_required(VERSION 3.13)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

message(status "QT_HOST_PATH = ${QT_HOST_PATH}")

find_program(CCACHE_PROGRAM ccache)
if(CCACHE_PROGRAM)
    message(STATUS "CCache enabled")
    set_property(GLOBAL PROPERTY RULE_LAUNCH_COMPILE "${CCACHE_PROGRAM}")
endif()

project(Ripes
        LANGUAGES CXX
        DESCRIPTION "A graphical processor simulator and assembly editor for the RISC-V ISA"
        HOMEPAGE_URL "https://github.com/mortbopet/Ripes"
)

if(NOT ${CMAKE_SYSTEM_NAME} STREQUAL "Windows")
    include(GNUInstallDirs) #Must exist after declaring a project
endif()

# Error flags on everything but MSVC
if(NOT MSVC)
    set(CMAKE_CXX_FLAGS "-Wextra -Wall \
        -Werror=switch -Werror=return-type \
        -Werror=unreachable-code")
endif()

if(MSVC)
    add_definitions(/bigobj) # Allow big object
elseif(MINGW)
   set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -mbig-obj") # Allow big object
endif()

######################################################################
## GUI setup
######################################################################

# Instruct CMake to run moc automatically when needed.
set(CMAKE_AUTOMOC ON)
set(CMAKE_INCLUDE_CURRENT_DIR ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTORCC ON)

set(RIPES_WITH_QPROCESS ON CACHE BOOL "Build with QProcess-related utilities")
# Add as a C++ definition
if(RIPES_WITH_QPROCESS)
    add_definitions(-DRIPES_WITH_QPROCESS)
endif()

# Find required Qt packages
find_package(Qt6 COMPONENTS Core Widgets Svg Charts REQUIRED)

# Finding Qt includes
include_directories(${Qt6Widgets_INCLUDE_DIRS})
include_directories(${Qt6Core_INCLUDE_DIRS})
include_directories(${Qt6Charts_INCLUDE_DIRS})

######################################################################
## Resources setup
######################################################################
qt6_add_resources(ICONS_SRC ${CMAKE_SOURCE_DIR}/resources/icons/icons.qrc)
qt6_add_resources(EXAMPLES_SRC ${CMAKE_SOURCE_DIR}/examples/examples.qrc)
qt6_add_resources(LAYOUTS_SRC ${CMAKE_SOURCE_DIR}/src/processors/layouts.qrc)
qt6_add_resources(FONTS_SRC ${CMAKE_SOURCE_DIR}/resources/fonts/fonts.qrc)

######################################################################
## Library setup
######################################################################

option(RIPES_BUILD_VERILATOR_PROCESSORS "Build verilator processors" OFF)
if(RIPES_BUILD_VERILATOR_PROCESSORS)
    if(NOT DEFINED ENV{VERILATOR_ROOT})
        message(FATAL_ERROR "'VERILATOR_ROOT' must be set when building Verilator-based processors!")
    endif()
    message(STATUS "Building with Verilator processors")
    include_directories(SYSTEM $ENV{VERILATOR_ROOT}/include)
    set(verilator_DIR $ENV{VERILATOR_ROOT})
    find_package(verilator)

    # Create library containing Verilator runtime files, which verilator processors can link against
    file(GLOB VERILATOR_SOURCES $ENV{VERILATOR_ROOT}/include/verilated.cpp)
    file(GLOB VERILATOR_HEADERS $ENV{VERILATOR_ROOT}/include/verilated.h)
    set(VERILATOR_LIB verilator_lib)
    add_library(${VERILATOR_LIB} ${VERILATOR_SOURCES} ${VERILATOR_HEADERS})
endif()

# Platform specific pre-target setup(s)
if(${APPLE})
    set(SYSTEM_FLAGS MACOSX_BUNDLE
            src/processors/RISC-V/rvss_trap/rv_special_csrs.h
            src/processors/RISC-V/rvss_trap/rv_rti_adder.h
            src/processors/RISC-V/rvss_trap/rv_rti_mux.h
            src/processors/RISC-V/rvss_trap/rv_trap_decoder.h
            src/processors/RISC-V/rvss_trap/rv_ss_trap_mems.h
            src/processors/RISC-V/rvss_trap/trap_checker.h
            src/processors/RISC-V/rvss_trap/rv_ss_trap_control.h
            src/processors/RISC-V/rvss_trap/rv_trap_decode.h
            src/io/ioplic.cpp
            src/io/ioplic.h
            src/io/iokeyboard.h
            src/io/iokeyboard.cpp
            src/processors/RISC-V/rvss_trap/rv_direction_mux.h
            src/io/iotextout.h
            src/io/iotextout.cpp
    )
elseif(${WIN32})
endif()

add_subdirectory(external)

# Fix the name of the ripes library.
set(RIPES_LIB ripes_lib)
set(ISA_LIB isa_lib)
add_subdirectory(src)

option(RIPES_BUILD_TESTS "Build Ripes tests" OFF)
if(RIPES_BUILD_TESTS)
    add_subdirectory(test)
endif()

set(APP_NAME Ripes)
qt_add_executable(${APP_NAME} ${SYSTEM_FLAGS} ${ICONS_SRC} ${EXAMPLES_SRC} ${LAYOUTS_SRC} ${FONTS_SRC} main.cpp)

# Link Qt libraries
target_link_libraries(${APP_NAME} PUBLIC Qt6::Core Qt6::Widgets)
# Link Ripes library
target_link_libraries(${APP_NAME} PUBLIC ${RIPES_LIB})

if(${CMAKE_SYSTEM_NAME} STREQUAL "Emscripten")
    # https://doc.qt.io/qt-6/wasm.html#asyncify
    target_link_options(${RIPES_LIB} PUBLIC -sASYNCIFY -Os)
    target_link_options(${APP_NAME} PUBLIC -sASYNCIFY -Os)
endif()


if(UNIX AND NOT APPLE) #Define the LINUX variable before testing it
    set(LINUX TRUE)
endif()

if(${LINUX})
    install(TARGETS ${APP_NAME}
        RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    )
elseif(${APPLE})
    install(TARGETS ${APP_NAME}
        RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
        BUNDLE  DESTINATION ${CMAKE_INSTALL_BINDIR}
    )
endif()

install(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/appdir/usr/
    DESTINATION ${CMAKE_INSTALL_PREFIX}
)
