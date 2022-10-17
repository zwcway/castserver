cmake_minimum_required(VERSION 3.7)

project(castserver LANGUAGES C)

option(BUILD_CLIENT "Client" ON)

IF (CMAKE_BUILD_TYPE STREQUAL Debug)
  ADD_DEFINITIONS(-DDEBUG)
else ()
  ADD_DEFINITIONS(-DNDEBUG)
ENDIF ()

set(SERVER_LIBRARIES
    ${CMAKE_THREAD_LIBS_INIT}
    common
    Threads::Threads
    microhttpd
    m
    )

find_package(Threads REQUIRED)
find_library(MHTD microhttpd REQUIRED)

file(GLOB SERVER_SOURCES CONFIGURE_DEPENDS
    "*.c"
    "command/*.c")
file(GLOB SERVER_HEADERS CONFIGURE_DEPENDS
    "*.h"
    "command/*.h")

add_executable(${PROJECT_NAME} ${SERVER_SOURCES} ${SERVER_HEADERS})

if (BUILD_CLIENT)
  add_subdirectory(client)
endif ()

add_subdirectory(common)

target_link_libraries(${PROJECT_NAME} ${SERVER_LIBRARIES})

target_include_directories(${PROJECT_NAME} PUBLIC "${PROJECT_BINARY_DIR}")

include(GNUInstallDirs)
install(TARGETS ${PROJECT_NAME} RUNTIME DESTINATION "${CMAKE_INSTALL_BINDIR}")
install(FILES conf/castserver.conf COMPONENT server DESTINATION ${CMAKE_INSTALL_SYSCONFDIR})
