cmake_minimum_required(VERSION 3.7)

project(castcli LANGUAGES C)

add_executable(${PROJECT_NAME}
    client.c
    pipe.c
    list_speakers.c list_speakers.h)


set(SERVER_LIBRARIES
    ${CMAKE_THREAD_LIBS_INIT}
    common)

find_package(Threads REQUIRED)


target_link_libraries(${PROJECT_NAME} ${SERVER_LIBRARIES})

target_include_directories(${PROJECT_NAME} PUBLIC "${PROJECT_BINARY_DIR}")

include(GNUInstallDirs)
install(TARGETS ${PROJECT_NAME} RUNTIME DESTINATION "${CMAKE_INSTALL_BINDIR}")