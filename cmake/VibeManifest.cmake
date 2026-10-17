# game and tool manifests -> build/games.json (the launcher's list of what can be played)
#
#   vibe_add_manifest(<target> <game.json>)  after add_subdirectory() of a game or tool
#   vibe_write_manifests()                    once, at the top level, after games/ and tools/
#
# The manifest's "name" must equal the target, which is also its directory,
# executable and Wayland window class. "binary" is filled in from the target.

set(_VIBE_MANIFEST_DIR "${CMAKE_CURRENT_LIST_DIR}")

function(vibe_add_manifest target manifest_file)
    if(NOT EXISTS ${manifest_file} OR NOT TARGET ${target})
        return()
    endif()
    set_property(DIRECTORY APPEND PROPERTY CMAKE_CONFIGURE_DEPENDS ${manifest_file})
    file(READ ${manifest_file} _manifest)
    string(JSON _name GET "${_manifest}" name)
    if(NOT _name STREQUAL target)
        message(FATAL_ERROR "${manifest_file}: name must be \"${target}\" (directory, executable and window class)")
    endif()
    string(JSON _manifest SET "${_manifest}" binary "\"$<TARGET_FILE:${target}>\"")
    get_property(_all GLOBAL PROPERTY VIBE_MANIFESTS_JSON)
    if(_all)
        string(APPEND _all ",\n")
    endif()
    string(APPEND _all "${_manifest}")
    set_property(GLOBAL PROPERTY VIBE_MANIFESTS_JSON "${_all}")
    set_property(GLOBAL APPEND PROPERTY VIBE_MANIFEST_NAMES ${target})
endfunction()

function(vibe_write_manifests)
    get_property(_all GLOBAL PROPERTY VIBE_MANIFESTS_JSON)
    get_property(_names GLOBAL PROPERTY VIBE_MANIFEST_NAMES)
    file(GENERATE OUTPUT ${CMAKE_BINARY_DIR}/games.json CONTENT "[\n${_all}\n]\n")
    if(ENABLE_TESTING AND _names)
        string(REPLACE ";" "|" _expected "${_names}")
        add_test(NAME GamesManifestCheck
            COMMAND ${CMAKE_COMMAND} -DGAMES_JSON=${CMAKE_BINARY_DIR}/games.json -DEXPECTED=${_expected}
                    -P ${_VIBE_MANIFEST_DIR}/CheckGamesJson.cmake)
    endif()
endfunction()
