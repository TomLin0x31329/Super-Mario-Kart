# 1. 自動遞迴搜尋當前目錄 (Game) 下所有的 .cpp 和 .hpp 檔案
file(GLOB_RECURSE GAME_FILES_
    "pch.hpp"
    "Application/*.cpp" "Application/*.h"
    "GameCore/*.cpp"    "GameCore/*.h"
    "Gameplay/*.cpp"    "Gameplay/*.h"
    "Renderers/*.cpp"   "Renderers/*.h"
    "Systems/*.cpp"     "Systems/*.h"
)
 
# 2. 建立執行檔目標
add_executable(MyGame_ ${GAME_FILES_})

# 3. 連結 PTSD 引擎與 SDL2main
# 注意：這裡可以直接用到 PTSD，因為根目錄已經先 build 好 PTSD 模組了
target_link_libraries(MyGame_
    SDL2::SDL2main
    PTSD
)

# 1. 告訴編譯器：引擎的標頭檔 (在 PTSD/include 下) 和外部依賴在哪裡
target_include_directories(MyGame_ SYSTEM PRIVATE
    ${CMAKE_SOURCE_DIR}/PTSD/include   # 🌟 重點：加上 PTSD/include，這樣才找得到 Core/ 和 Util/
    ${DEPENDENCY_INCLUDE_DIRS}       
)

# 2. 告訴編譯器：你自己的遊戲標頭檔在哪裡
# CMAKE_CURRENT_SOURCE_DIR 目前已經代表 "Game" 資料夾了，所以只要接 "/include" 就好
target_include_directories(MyGame_ PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR} 
)

# 5. 定義資源資料夾巨集 (指向根目錄的 Resource)
# CMAKE_SOURCE_DIR 代表的是「最上層根目錄」，而 CMAKE_CURRENT_SOURCE_DIR 是 Game 目錄
target_compile_definitions(MyGame_ PRIVATE RESOURCE_DIR="${CMAKE_SOURCE_DIR}/Resources")
