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

# Проверка наличия в системе meld
find_program(MELD meld)
if (MELD)
    message(STATUS "Meld found: ${MELD}")
else()
    message(STATUS "Meld not found. Failed Comapre tests will not be visualized")
    message(STATUS "Command to install this package:\n\t sudo apt install meld")
endif()

#
# Настройка проекта
#

set(SOURCES_DIR "${CMAKE_CURRENT_SOURCE_DIR}/src")
set(HEADERS_DIR "${CMAKE_CURRENT_SOURCE_DIR}/include")

file(GLOB_RECURSE SOURCES "${SOURCES_DIR}/*.c")
file(GLOB_RECURSE HEADERS "${HEADERS_DIR}/*.h")

add_executable(${PROJECT_NAME} ${SOURCES} ${HEADERS})

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

#
# Настройка правила создания документации
#
if (DOXYGEN)
    add_custom_target(doc
        COMMAND ${DOXYGEN} Doxyfile > ${CMAKE_BINARY_DIR}/doxygen.log 2>&1
        WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}/doc
        COMMENT "Generating HTML documentation. Log file: ${CMAKE_BINARY_DIR}/doxygen.log"
        VERBATIM
    )
endif()

#
# Настройка правил тестирования
#

add_custom_target(testEnv
    COMMAND bash -c "${CMAKE_SOURCE_DIR}/scripts/makeTestEnv"
)

add_custom_target(tests
    COMMAND bash -c "${CMAKE_SOURCE_DIR}/scripts/makeTests ${CMAKE_BINARY_DIR}/${PROJECT_NAME}"
)

add_dependencies(tests testEnv)
