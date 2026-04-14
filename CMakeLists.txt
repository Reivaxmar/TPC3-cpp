cmake_minimum_required(VERSION 3.16)
project(TPC3 CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Try to find a system-installed SFML first; if it is absent, download and
# build it automatically via FetchContent.
find_package(SFML 2.5 COMPONENTS graphics window system QUIET)

if(NOT SFML_FOUND)
    message(STATUS "SFML not found locally – fetching SFML 2.6.2 via FetchContent")
    include(FetchContent)
    FetchContent_Declare(
        SFML
        GIT_REPOSITORY https://github.com/SFML/SFML.git
        GIT_TAG        2.6.2
        GIT_SHALLOW    TRUE
    )
    # Disable extras we don't need so the build stays fast
    set(SFML_BUILD_AUDIO    OFF CACHE BOOL "" FORCE)
    set(SFML_BUILD_NETWORK  OFF CACHE BOOL "" FORCE)
    set(SFML_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
    set(SFML_BUILD_DOC      OFF CACHE BOOL "" FORCE)
    set(SFML_BUILD_TEST_SUITE OFF CACHE BOOL "" FORCE)
    FetchContent_MakeAvailable(SFML)
endif()

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
