cmake_minimum_required(VERSION 3.4)

project(corelib)


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


# -------------------------- definitions -------------------------
add_definitions(-DCURL_STATICLIB)
add_definitions(-DCORELIB_EXPORTS)

if(MSVC)
    set(SYSTEM_LIBRARIES Ws2_32 Wldap32)
    set(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} /NODEFAULTLIB:LIBCMTD.lib")
elseif(APPLE)
    find_library(MAC_VIDEO_TOOLBOX VideoToolbox)
    find_library(MAC_CORE_VIDEO CoreVideo)
    find_library(MAC_APP_KIT AppKit)
    find_library(MAC_AUDIO_TOOLBOX AudioToolbox)
    find_library(MAC_AUDIO_UNIT AudioUnit)
    find_library(MAC_IO_KIT IOKit)
    find_library(MAC_SECURITY Security)
    find_library(MAC_SYSTEM_CONFIGURATION SystemConfiguration)
    find_library(MAC_CORE_MEDIA CoreMedia)
    find_library(MAC_OPEN_GL OpenGL)
    find_library(MAC_CORE_GRAPHICS CoreGraphics)
    find_library(MAC_CORE_AUDIO CoreAudio)
    find_library(MAC_AV_FOUNDATION AVFoundation)
    find_library(MAC_FOUNDATION Foundation)

    mark_as_advanced(
        MAC_VIDEO_TOOLBOX
        MAC_CORE_VIDEO
        MAC_APP_KIT
        MAC_AUDIO_TOOLBOX
        MAC_AUDIO_UNIT
        MAC_IO_KIT
        MAC_SECURITY
        MAC_SYSTEM_CONFIGURATION
        MAC_CORE_MEDIA
        MAC_OPEN_GL
        MAC_CORE_GRAPHICS
        MAC_CORE_AUDIO
        MAC_AV_FOUNDATION
        MAC_FOUNDATION)

    set(SYSTEM_LIBRARIES
        ${MAC_VIDEO_TOOLBOX}
        ${MAC_CORE_VIDEO}
        ${MAC_APP_KIT}
        ${MAC_AUDIO_TOOLBOX}
        ${MAC_AUDIO_UNIT}
        ${MAC_IO_KIT}
        ${MAC_SECURITY}
        ${MAC_SYSTEM_CONFIGURATION}
        ${MAC_CORE_MEDIA}
        ${MAC_OPEN_GL}
        ${MAC_CORE_GRAPHICS}
        ${MAC_CORE_AUDIO}
        ${MAC_AV_FOUNDATION}
        ${MAC_FOUNDATION})
elseif(LINUX)
    set(SYSTEM_LIBRARIES ${ICQ_EXTERNAL_PLATFORM}/libminizip.a)
endif()


# --------------------------   corelib   -------------------------
set(SUBPROJECT_ROOT "${ICQ_ROOT}/corelib")

find_sources(SUBPROJECT_SOURCES "${SUBPROJECT_ROOT}" "cpp")
find_sources(SUBPROJECT_HEADERS "${SUBPROJECT_ROOT}" "h")

set_source_group("sources" "${SUBPROJECT_ROOT}" ${SUBPROJECT_SOURCES} ${SUBPROJECT_HEADERS})


# ----------------------------------------------------------------
include_directories(${SUBPROJECT_ROOT})

if(MSVC)
    source_group("sources" FILES corelib.def)
    source_group("sources" FILES corelib.rc)
    add_library(${PROJECT_NAME} SHARED ${SUBPROJECT_SOURCES} ${SUBPROJECT_HEADERS} corelib.def corelib.rc)
    use_precompiled_header_msvc("stdafx.h" "${SUBPROJECT_ROOT}/stdafx.cpp" ${SUBPROJECT_SOURCES})
elseif(APPLE)
    use_precompiled_header_mac(PCH_BIN "${SUBPROJECT_ROOT}/stdafx.h" ${SUBPROJECT_SOURCES})
    add_library(${PROJECT_NAME} ${SUBPROJECT_SOURCES} ${SUBPROJECT_HEADERS} ${PCH_BIN})
elseif(LINUX)
    use_precompiled_header_linux(PCH_BIN "${SUBPROJECT_ROOT}/stdafx.h" ${SUBPROJECT_SOURCES})
    add_library(${PROJECT_NAME} ${SUBPROJECT_SOURCES} ${SUBPROJECT_HEADERS} ${PCH_BIN})
endif()

if(ICQ_CORELIB_STATIC_LINKING)
    target_link_libraries(${PROJECT_NAME}
        ${SYSTEM_LIBRARIES}
        core
        ${Boost_LIBRARIES}
        ${OPENSSL_LIBRARIES}
        ${LIBCRYPTO_LIBRARIES}
        ${CURL_LIBRARIES}
        ${LIBEVENT_LIBRARIES})
else()
    target_link_libraries(${PROJECT_NAME}
        ${SYSTEM_LIBRARIES}
        ${Boost_LIBRARIES}
        core
        ${OPENSSL_LIBRARIES}
        ${LIBCRYPTO_LIBRARIES}
        ${CURL_LIBRARIES}
        ${VOIP_LIBRARIES}
        ${ZLIB_LIBRARIES}
        ${LIBEVENT_LIBRARIES})
endif()
