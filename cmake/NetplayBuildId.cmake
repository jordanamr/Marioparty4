cmake_minimum_required(VERSION 3.25)
# The release commit pins the game, submodules, and their patches. For a
# developer build, also distinguish edits which have not been committed.
set(inputs src include libc patches tools/online CMakeLists.txt files.cmake cmake)
set(source_key "PartyBoard desktop netplay 1\nmusyx=${MUSYX}\n")
if(GIT_EXECUTABLE AND EXISTS "${SOURCE_DIR}/.git")
    function(read_git result)
        execute_process(COMMAND "${GIT_EXECUTABLE}" -c core.quotepath=false ${ARGN}
            WORKING_DIRECTORY "${SOURCE_DIR}" OUTPUT_VARIABLE value
            OUTPUT_STRIP_TRAILING_WHITESPACE COMMAND_ERROR_IS_FATAL ANY)
        set("${result}" "${value}" PARENT_SCOPE)
    endfunction()
    read_git(revision rev-parse --verify HEAD)
    string(APPEND source_key "${revision}\n")
    read_git(changed diff --no-renames --name-only HEAD -- ${inputs})
    read_git(added ls-files --others --exclude-standard -- ${inputs})
    string(REPLACE "\n" ";" paths "${changed}\n${added}")
    list(FILTER paths EXCLUDE REGEX "^$")
else()
    # Source archives have no revision. Hash the available game/lobby inputs.
    set(paths CMakeLists.txt files.cmake)
    file(GLOB_RECURSE game_sources RELATIVE "${SOURCE_DIR}"
        "${SOURCE_DIR}/src/*" "${SOURCE_DIR}/include/*" "${SOURCE_DIR}/libc/*"
        "${SOURCE_DIR}/patches/*.patch" "${SOURCE_DIR}/cmake/*.cmake")
    file(GLOB lobby_sources RELATIVE "${SOURCE_DIR}"
        "${SOURCE_DIR}/tools/online/*.cs" "${SOURCE_DIR}/tools/online/linux/*.cs"
        "${SOURCE_DIR}/tools/online/linux/*.csproj")
    list(APPEND paths ${game_sources} ${lobby_sources})
endif()
list(REMOVE_DUPLICATES paths)
list(SORT paths)
foreach(path IN LISTS paths)
    set(hash "deleted")
    if(EXISTS "${SOURCE_DIR}/${path}")
        file(READ "${SOURCE_DIR}/${path}" contents)
        string(REPLACE "\r\n" "\n" contents "${contents}")
        string(SHA256 hash "${contents}")
    endif()
    string(APPEND source_key "${path} ${hash}\n")
endforeach()
string(SHA256 source_id "${source_key}")
# Only entry.cpp includes this header; a no-op build preserves its timestamp.
file(CONFIGURE OUTPUT "${OUTPUT_DIR}/netplay_build_id.h"
    CONTENT "#define PARTY_BOARD_NETPLAY_SOURCE_ID \"@source_id@\"\n" @ONLY)
