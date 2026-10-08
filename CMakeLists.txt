cmake_minimum_required(VERSION 3.21)

set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")

project(EveOverlay LANGUAGES CXX RC)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_SOURCE_DIR}/Binaries/$<CONFIG>")

find_package(nlohmann_json CONFIG REQUIRED)
find_package(imgui CONFIG REQUIRED)
find_path(WTL_INCLUDE_DIRS "atlapp.h" REQUIRED)

file(GLOB_RECURSE EVE_CORE_SOURCES CONFIGURE_DEPENDS "${CMAKE_SOURCE_DIR}/src/*.cpp")
list(REMOVE_ITEM EVE_CORE_SOURCES "${CMAKE_SOURCE_DIR}/src/Main.cpp")

add_library(EveCore STATIC ${EVE_CORE_SOURCES})
target_include_directories(EveCore PUBLIC src ${WTL_INCLUDE_DIRS})
target_compile_definitions(EveCore PUBLIC UNICODE _UNICODE WIN32_LEAN_AND_MEAN NOMINMAX _WIN32_WINNT=0x0A00)
target_compile_options(EveCore PUBLIC /W4 /permissive- /utf-8)
target_link_libraries(EveCore PUBLIC nlohmann_json::nlohmann_json imgui::imgui dwmapi dxgi d3d11 shell32 user32 ole32 comdlg32 gdiplus winmm winhttp)

add_executable(EveOverlay WIN32
    src/Main.cpp
    assets/EveOverlay.rc
)

add_custom_command(TARGET EveOverlay POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different "${CMAKE_SOURCE_DIR}/universeData/systems.csv" "$<TARGET_FILE_DIR:EveOverlay>/universeData/systems.csv"
    COMMAND ${CMAKE_COMMAND} -E copy_if_different "${CMAKE_SOURCE_DIR}/universeData/types.tsv" "$<TARGET_FILE_DIR:EveOverlay>/universeData/types.tsv"
    COMMAND ${CMAKE_COMMAND} -E copy_directory "${CMAKE_SOURCE_DIR}/assets/sounds" "$<TARGET_FILE_DIR:EveOverlay>/sounds")

target_link_options(EveOverlay PRIVATE /MANIFEST:NO)
target_link_libraries(EveOverlay PRIVATE EveCore)

foreach(ProbeName ServicesProbe ConfigProbe CoreProbe UiProbe SyncerProbe UniverseProbe)
    add_executable(${ProbeName} tests/${ProbeName}.cpp)
    target_link_libraries(${ProbeName} PRIVATE EveCore)
endforeach()

# Fake EVE client for the QA scripts; the output must be named ExeFile.exe to be picked up by the process monitor
add_executable(FakeEveClient WIN32 tests/qa/FakeClient.cpp)
set_target_properties(FakeEveClient PROPERTIES OUTPUT_NAME ExeFile RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/qa")
target_compile_definitions(FakeEveClient PRIVATE UNICODE _UNICODE WIN32_LEAN_AND_MEAN NOMINMAX)
target_compile_options(FakeEveClient PRIVATE /W4 /permissive- /utf-8)

