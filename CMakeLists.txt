#
# Первоначальные настройки
#

cmake_minimum_required(VERSION 3.5)

project(lader LANGUAGES C)

set(CMAKE_INCLUDE_CURRENT_DIR ON)

if (NOT CMAKE_BUILD_TYPE OR CMAKE_BUILD_TYPE STREQUAL "None")
    set(CMAKE_BUILD_TYPE "Release")
endif ()

#
# Проверка наличия в системе doxygen
#

find_program(DOXYGEN doxygen)
if (DOXYGEN)
    message(STATUS "Doxygen found: ${DOXYGEN}")
else()
    message(STATUS "Doxygen not found. HTML documentation is unavailable")
    message(STATUS "Command to install this package:\n\t sudo apt install doxygen")
endif()

#
# Настройка проекта
#

set(SOURCES_DIR   "${CMAKE_CURRENT_SOURCE_DIR}/src")
set(HEADERS_DIR   "${CMAKE_CURRENT_SOURCE_DIR}/include")

file(GLOB_RECURSE SOURCES   "${SOURCES_DIR}/*.c")
file(GLOB_RECURSE HEADERS   "${HEADERS_DIR}/*.h")

add_executable(${PROJECT_NAME} ${EXECUTABLE_OPTIONS} ${SOURCES} ${HEADERS} ${UIS} ${RESOURCES})

target_include_directories(${PROJECT_NAME} PRIVATE ${HEADERS_DIR})

#
# Настройка правила install
#

set(INSTALL_BIN_DIR "bin/")

install(TARGETS ${PROJECT_NAME}
        RUNTIME DESTINATION "${INSTALL_BIN_DIR}")

#
# Настройка правила uninstall
#

add_custom_target(uninstall
                  COMMENT "Removing installed files"
                  COMMAND ${CMAKE_COMMAND} -P ${CMAKE_SOURCE_DIR}/cmake/uninstall.cmake)
