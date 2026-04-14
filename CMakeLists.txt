cmake_minimum_required(VERSION 3.16)
project(TPC3 CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(SFML 2.5 COMPONENTS graphics window system REQUIRED)

set(GAME_SOURCES
    main.cpp
    src/Controller.cpp
    src/Level.cpp
    src/Player.cpp
    src/Fish.cpp
    src/SuddenDeath.cpp
    src/ImageUtils.cpp
    src/Game.cpp
)

add_executable(TPC3 ${GAME_SOURCES})

target_include_directories(TPC3 PRIVATE
    ${CMAKE_SOURCE_DIR}
    ${CMAKE_SOURCE_DIR}/third_party
)

target_link_libraries(TPC3
    sfml-graphics
    sfml-window
    sfml-system
)

# On Windows the SFML DLLs must sit next to the executable or the Windows
# loader will refuse to start the program before main() is ever reached.
if(WIN32)
    add_custom_command(TARGET TPC3 POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            $<TARGET_FILE:sfml-graphics>
            $<TARGET_FILE:sfml-window>
            $<TARGET_FILE:sfml-system>
            $<TARGET_FILE_DIR:TPC3>
    )
endif()
