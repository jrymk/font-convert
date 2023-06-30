cmake_minimum_required(VERSION 3.22)
project(fontConvert)

include(FetchContent)
FetchContent_Declare(SFML
        GIT_REPOSITORY https://github.com/SFML/SFML.git
        GIT_TAG 2.4.2)
FetchContent_MakeAvailable(SFML)

set(CMAKE_CXX_STANDARD 17)

include_directories(cmake-build-debug/_deps/sfml-src/include
        ${CMAKE_CURRENT_LIST_DIR}/freetype-2.9/include)

add_executable(fontConvert main.cpp)

target_link_libraries(fontConvert PRIVATE sfml-system sfml-network sfml-graphics sfml-window)

add_custom_command(TARGET fontConvert POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy $<TARGET_RUNTIME_DLLS:fontConvert> $<TARGET_FILE_DIR:fontConvert> COMMAND_EXPAND_LISTS)

target_compile_definitions(fontConvert PUBLIC -DFT_CONFIG_OPTION_ERROR_STRINGS)

install(TARGETS fontConvert)