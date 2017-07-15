cmake_minimum_required(VERSION 3.4)

project(core)


# ---------------------------  paths  ----------------------------
set(CMAKE_EXECUTABLE_OUTPUT_DIRECTORY_DEBUG ${ICQ_BIN_DIR})
set(CMAKE_EXECUTABLE_OUTPUT_DIRECTORY_RELEASE ${ICQ_BIN_DIR})
set(CMAKE_EXECUTABLE_OUTPUT_PATH ${ICQ_BIN_DIR})
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY_DEBUG ${ICQ_LIB_DIR})
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY_RELEASE ${ICQ_LIB_DIR})
set(CMAKE_LIBRARY_OUTPUT_PATH ${ICQ_LIB_DIR})
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY_DEBUG ${ICQ_LIB_DIR})
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY_RELEASE ${ICQ_LIB_DIR})
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${ICQ_LIB_DIR})
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_DEBUG ${ICQ_BIN_DIR})
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_RELEASE ${ICQ_BIN_DIR})
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${ICQ_BIN_DIR})


# --------------------------    core     -------------------------
set(SUBPROJECT_ROOT "${ICQ_ROOT}/core")

find_sources(SUBPROJECT_SOURCES "${SUBPROJECT_ROOT}" "cpp")
find_sources(SUBPROJECT_HEADERS "${SUBPROJECT_ROOT}" "h")
if(APPLE)
    find_sources(SUBPROJECT_MM_SOURCES "${SUBPROJECT_ROOT}" "mm")
endif()

set_source_group("sources" "${SUBPROJECT_ROOT}" ${SUBPROJECT_SOURCES} ${SUBPROJECT_MM_SOURCES} ${SUBPROJECT_HEADERS})


# --------------------------   common    -------------------------
set(COMMON_ROOT "${ICQ_ROOT}/common.shared")

find_sources(COMMON_SOURCES "${COMMON_ROOT}" "cpp")
find_sources(COMMON_HEADERS "${COMMON_ROOT}" "h")

set_source_group("sources" "${COMMON_ROOT}" ${COMMON_SOURCES} ${COMMON_HEADERS})


# --------------------------  minizip    -------------------------
if(APPLE)
    set(MINIZIP_ROOT "${ICQ_EXTERNAL}/minizip")

    find_sources(MINIZIP_SOURCES "${MINIZIP_ROOT}" "c")
    find_sources(MINIZIP_HEADERS "${MINIZIP_ROOT}" "h")

    list(REMOVE_ITEM MINIZIP_SOURCES "${MINIZIP_ROOT}/iowin32.c")
    list(REMOVE_ITEM MINIZIP_HEADERS "${MINIZIP_ROOT}/iowin32.h")
    list(REMOVE_ITEM MINIZIP_SOURCES "${MINIZIP_ROOT}/miniunz.c")
    list(REMOVE_ITEM MINIZIP_SOURCES "${MINIZIP_ROOT}/minizip.c")

    set_source_group("minizip" "${MINIZIP_ROOT}" ${MINIZIP_SOURCES} ${MINIZIP_HEADERS})
endif()


# ----------------------------------------------------------------
include_directories(${SUBPROJECT_ROOT})

if(MSVC)
    use_precompiled_header_msvc("stdafx.h" "${SUBPROJECT_ROOT}/stdafx.cpp" ${SUBPROJECT_SOURCES} ${COMMON_SOURCES})
elseif(APPLE)
    use_precompiled_header_mac(PCH_BIN "${SUBPROJECT_ROOT}/stdafx.h" ${SUBPROJECT_SOURCES} ${COMMON_SOURCES})
elseif(LINUX)
    use_precompiled_header_linux(PCH_BIN "${SUBPROJECT_ROOT}/stdafx.h" ${SUBPROJECT_SOURCES} ${COMMON_SOURCES})
endif()

add_library(${PROJECT_NAME} STATIC
    ${SUBPROJECT_SOURCES} ${SUBPROJECT_MM_SOURCES} ${SUBPROJECT_HEADERS}
    ${COMMON_SOURCES} ${COMMON_HEADERS}
    ${MINIZIP_SOURCES} ${MINIZIP_HEADERS}
    ${PCH_BIN})


# --------------------------  copy voip  -------------------------
if(MSVC)
    add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy ${VOIP_LIBRARY_1} ${VOIP_LIBRARY_2} ${VOIP_DX_COMPILER_LIBRARY} ${ICQ_BIN_DIR})
endif()
