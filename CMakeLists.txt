cmake_minimum_required(VERSION 3.16)
set(CMAKE_POLICY_VERSION_MINIMUM 3.5)
project(SuperMarioKart)

#set(FETCHCONTENT_FULLY_DISCONNECTED ON)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

add_compile_definitions(_HAS_ITERATOR_DEBUGGING=0)
add_compile_definitions(_ITERATOR_DEBUG_LEVEL=0)

set(BUILD_UTILS OFF CACHE BOOL "Disable GLEW utilities" FORCE)
# 關閉所有標準測試程式編譯
set(BUILD_TESTING OFF CACHE BOOL "Disable tests" FORCE)
# 隱藏那些不是你寫的 CMake 目標，不讓它們出現在預設編譯清單中
set(CMAKE_FOLDER "ThirdParty")

# 關閉 SDL2_image 的範例 (消除 showimage, showanim)
set(SDL2IMAGE_SAMPLES OFF CACHE BOOL "Disable SDL2_image samples" FORCE)
set(SDL2IMAGE_TESTS   OFF CACHE BOOL "Disable SDL2_image tests" FORCE)
# 關閉 SDL2_ttf 的範例 (消除 glfont, showfont)
set(SDL2TTF_SAMPLES   OFF CACHE BOOL "Disable SDL2_ttf samples" FORCE)
# 關閉 SDL2_mixer 的範例 (消除 playwave, playmus)
set(SDL2MIXER_SAMPLES OFF CACHE BOOL "Disable SDL2_mixer samples" FORCE)
# 關閉 SDL2 底層的測試
set(SDL_TEST          OFF CACHE BOOL "Disable SDL2 tests" FORCE)
set(SDL_TESTS         OFF CACHE BOOL "Disable SDL2 tests" FORCE)
# 關閉 GLEW 的工具 (你之前加的)
set(BUILD_UTILS       OFF CACHE BOOL "Disable GLEW utilities" FORCE)
set(BUILD_TESTING     OFF CACHE BOOL "Disable tests" FORCE)


# 1. 抓取 PTSD 遊戲引擎
include(FetchContent)
set(FETCHCONTENT_QUIET FALSE)

#FetchContent_Declare(
#    ptsd
#    URL         https://github.com/ntut-open-source-club/practical-tools-for-simple-design/archive/refs/tags/v0.2.zip
#    SOURCE_DIR  ${CMAKE_CURRENT_SOURCE_DIR}/PTSD
#)
#FetchContent_MakeAvailable(ptsd)
add_subdirectory(PTSD)
include_directories(external)

# 2. 讓 PTSD 知道外部依賴
target_include_directories(PTSD SYSTEM PRIVATE ${DEPENDENCY_INCLUDE_DIRS})

# 3. 將編譯任務交棒給 Game 資料夾
add_subdirectory(Game)
