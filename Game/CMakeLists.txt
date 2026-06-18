# 自動遞迴搜尋當前目錄 (Game) 下所有的 .cpp 和 .hpp 檔案
file(GLOB_RECURSE GAME_FILES_
    "pch.hpp"
    "Application/*.cpp" "Application/*.h"
    "GameCore/*.cpp"    "GameCore/*.h"
    "Gameplay/*.cpp"    "Gameplay/*.h"
    "Renderers/*.cpp"   "Renderers/*.h"
    "Systems/*.cpp"     "Systems/*.h"
    "States/*.cpp"       "States/*.h"
)
 
# 建立執行檔目標
add_executable(MyGame ${GAME_FILES_})
target_compile_definitions(MyGame PRIVATE _SILENCE_STDEXT_ARR_ITERS_DEPRECATION_WARNING)

# 連結 PTSD 引擎與 SDL2main
target_link_libraries(MyGame
    SDL2::SDL2main
    PTSD
)

# 告訴編譯器：引擎的標頭檔 (在 PTSD/include 下) 和外部依賴在哪裡
target_include_directories(MyGame SYSTEM PRIVATE
    ${CMAKE_SOURCE_DIR}/PTSD/include 
    ${DEPENDENCY_INCLUDE_DIRS}       
)

# 告訴編譯器：你自己的遊戲標頭檔在哪裡
target_include_directories(MyGame PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR} 
)

# 定義資源資料夾巨集 (指向根目錄的 Resource)
target_compile_definitions(MyGame PRIVATE RESOURCE_DIR="${CMAKE_SOURCE_DIR}/Resources")
