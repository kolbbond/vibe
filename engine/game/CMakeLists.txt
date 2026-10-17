# vibe-game: local multiplayer layer (input, lobby, views, loop, app shell)
# installed and exported through the Vibe package (cmake/VibeConfig.cmake.in)

# GameCore: headless (no SDL, no GL; Magnum Math types only)
add_library(vibegamecore STATIC
    src/ActionMap.cxx
    src/InputSystem.cxx
    src/Lobby.cxx
    src/ViewLayout.cxx
    src/InputScript.cxx
    src/GameList.cxx
)
add_library(Vibe::GameCore ALIAS vibegamecore)

# expose headers as vibe/game/X.hxx in the build tree (matches the other engine libs)
file(MAKE_DIRECTORY ${CMAKE_BINARY_DIR}/include/vibe)
if(NOT EXISTS ${CMAKE_BINARY_DIR}/include/vibe/game)
    file(CREATE_LINK ${CMAKE_CURRENT_SOURCE_DIR}/include ${CMAKE_BINARY_DIR}/include/vibe/game RESULT _vibe_link SYMBOLIC)
    if(NOT _vibe_link EQUAL 0)
        # no symlink permission (e.g. Windows without developer mode): copy; refreshed only on a clean configure
        file(COPY ${CMAKE_CURRENT_SOURCE_DIR}/include/ DESTINATION ${CMAKE_BINARY_DIR}/include/vibe/game)
    endif()
endif()

target_include_directories(vibegamecore PUBLIC $<BUILD_INTERFACE:${CMAKE_BINARY_DIR}/include> $<INSTALL_INTERFACE:include>)
find_package(jsoncpp CONFIG REQUIRED)
target_link_libraries(vibegamecore PUBLIC Magnum::Magnum PRIVATE JsonCpp::JsonCpp)
target_compile_options(vibegamecore PRIVATE "$<$<NOT:$<CXX_COMPILER_ID:MSVC>>:-Wall;-Wextra;-Wpedantic;-Wshadow>")

# Game: SDL input, ImGui HUD and the GameApp shell (smg::GuiBase)
add_library(vibegame STATIC
    src/Hud.cxx
    src/SdlInput.cxx
    src/GameApp.cxx
)
add_library(Vibe::Game ALIAS vibegame)
target_link_libraries(vibegame PUBLIC vibegamecore Vibe::Graphics Vibe::Audio smg::smg)
# GameConfig::music resolves under here; defaults to the assets/ of the project building the engine (the game repo)
set(VIBE_ASSETS_DIR "${CMAKE_SOURCE_DIR}/assets" CACHE PATH "assets dir games load music from")
target_compile_definitions(vibegame PRIVATE VIBE_ASSETS_DIR="${VIBE_ASSETS_DIR}")
target_compile_options(vibegame PRIVATE "$<$<NOT:$<CXX_COMPILER_ID:MSVC>>:-Wall;-Wextra;-Wpedantic;-Wshadow>")

## INSTALLATION
include(GNUInstallDirs)
set_target_properties(vibegamecore PROPERTIES EXPORT_NAME GameCore)
set_target_properties(vibegame PROPERTIES EXPORT_NAME Game)
install(TARGETS vibegamecore vibegame
    EXPORT vibegame-targets
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}/vibe
)
install(DIRECTORY include/ DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/vibe/game)
install(EXPORT vibegame-targets
    FILE VibeGameTargets.cmake
    NAMESPACE Vibe::
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/Vibe
)

if(ENABLE_TESTING)
    add_subdirectory(test)
endif()
