cmake_minimum_required(VERSION 3.4)

if(APPLE)
    if(IS_ICQ)
        project(ICQ)
    elseif(IS_AGENT)
        project(Mail.Ru_Agent)
    endif()
else()
    project(icq)
endif()


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
if(MSVC)
    add_definitions(/FI"stdafx.h")
    set(SYSTEM_LIBRARIES d3d9 dxguid opengl32 Ws2_32 Imm32 Winmm runtimeobject strmiids secur32 glu32)
elseif(APPLE)
    set(ICQ_MAC_ROOT "${ICQ_ROOT}/mac/ICQ")
    add_definitions(-include "stdafx.h")
    include_directories(${ICQ_MAC_ROOT})

    find_library(MAC_OPENGL OpenGL)
    find_library(MAC_APP_KIT AppKit)
    find_library(MAC_FOUNDATION Foundation)
    find_library(MAC_IOKIT IOKit)
    find_library(MAC_SYSTEM_CONFIGURATION SystemConfiguration)
    find_library(MAC_SECURITY Security)
    find_library(MAC_CORE_MEDIA CoreMedia)
    find_library(MAC_CORE_AUDIO CoreAudio)
    find_library(MAC_AUDIO_TOOLBOX AudioToolbox)
    find_library(MAC_AUDIO_UNIT AudioUnit)
    find_library(MAC_CARBON Carbon)
    find_library(MAC_QUARTZ Quartz)
    find_library(MAC_HOCKEY_SDK HockeySDK PATHS ${ICQ_EXTERNAL_PLATFORM})
    find_library(MAC_SCRIPTING_BRIDGE ScriptingBridge)
    find_library(MAC_VIDEO_TOOLBOX VideoToolbox)
    find_library(MAC_VIDEO_DECODE_ACCELERATION VideoDecodeAcceleration)
    find_library(MAC_SPARKLE Sparkle PATHS ${ICQ_EXTERNAL_PLATFORM})

    mark_as_advanced(
        MAC_OPENGL
        MAC_APP_KIT
        MAC_FOUNDATION
        MAC_IOKIT
        MAC_SYSTEM_CONFIGURATION
        MAC_SECURITY
        MAC_CORE_MEDIA
        MAC_CORE_AUDIO
        MAC_AUDIO_TOOLBOX
        MAC_AUDIO_UNIT
        MAC_CARBON
        MAC_QUARTZ
        MAC_HOCKEY_SDK
        MAC_SCRIPTING_BRIDGE
        MAC_VIDEO_TOOLBOX
        MAC_VIDEO_DECODE_ACCELERATION
        MAC_SPARKLE)

    set(SYSTEM_LIBRARIES
        ${MAC_OPENGL}
        ${MAC_APP_KIT}
        ${MAC_FOUNDATION}
        ${MAC_IOKIT}
        ${MAC_SYSTEM_CONFIGURATION}
        ${MAC_SECURITY}
        ${MAC_CORE_MEDIA}
        ${MAC_CORE_AUDIO}
        ${MAC_AUDIO_TOOLBOX}
        ${MAC_AUDIO_UNIT}
        ${MAC_CARBON}
        ${MAC_QUARTZ}
        ${MAC_HOCKEY_SDK}
        ${MAC_SCRIPTING_BRIDGE}
        ${MAC_VIDEO_TOOLBOX}
        ${MAC_VIDEO_DECODE_ACCELERATION}
        ${ICQ_EXTERNAL_PLATFORM}/libqcocoa.a
        ${VOIP_LIBRARIES}
        libcups.2.dylib
        ${ICQ_EXTERNAL_PLATFORM}/libqcocoa.a
        iconv
        ldap
        z)

    if(ICQ_UPDATE)
        message(STATUS "updates: on")
        set(SYSTEM_LIBRARIES ${SYSTEM_LIBRARIES} ${MAC_SPARKLE})
    else()
        message(STATUS "updates: off")
    endif()

if(IS_ICQ)
    set(MACOSX_BUNDLE_ICON_FILE ${ICQ_MAC_ROOT}/icons-bundle-icq.icns)
elseif(IS_AGENT)
    set(MACOSX_BUNDLE_ICON_FILE ${ICQ_MAC_ROOT}/icons-bundle-agent.icns)
endif()
    set_source_files_properties(${MACOSX_BUNDLE_ICON_FILE} PROPERTIES MACOSX_PACKAGE_LOCATION Resources)
elseif(LINUX)
    add_definitions(-include "stdafx.h")
    add_definitions(-DSTRIP_VOIP)
    set(SYSTEM_LIBRARIES corelib core
        ${ICQ_EXTERNAL_PLATFORM}/libcurl_static.a
        ${ICQ_EXTERNAL_PLATFORM}/libssl_static.a
        ${ICQ_EXTERNAL_PLATFORM}/libcrypto_static.a
        ${ICQ_EXTERNAL_PLATFORM}/system/libidn.a
        ${ICQ_EXTERNAL_PLATFORM}/system/librtmp.a
        ${ICQ_EXTERNAL_PLATFORM}/system/libgcrypt.a
        ${ICQ_EXTERNAL_PLATFORM}/system/libgnutls.a
        ${ICQ_EXTERNAL_PLATFORM}/system/libgpg-error.a
        ${ICQ_EXTERNAL_PLATFORM}/system/libtasn1.a
        ${ICQ_EXTERNAL_PLATFORM}/system/libstdc++.a
        -Wl,-Bdynamic -ldl -lpthread -lX11 -lX11-xcb -lxcb)
    if(LINUX32)
        set(SYSTEM_LIBRARIES ${SYSTEM_LIBRARIES} ${ICQ_EXTERNAL_PLATFORM}/system/libp11-kit.a)
    endif()
endif()


# --------------------------    gui      -------------------------
set(SUBPROJECT_ROOT "${ICQ_ROOT}/gui")

find_sources(SUBPROJECT_SOURCES "${SUBPROJECT_ROOT}" "cpp")
if(APPLE)
    find_sources(SUBPROJECT_MM_SOURCES "${SUBPROJECT_ROOT}" "mm")
endif()
find_sources(SUBPROJECT_HEADERS "${SUBPROJECT_ROOT}" "h")

list(REMOVE_ITEM SUBPROJECT_SOURCES "${SUBPROJECT_ROOT}/cache/emoji/EmojiIndexData.cpp")

set_source_group("sources" "${SUBPROJECT_ROOT}" ${SUBPROJECT_SOURCES} ${SUBPROJECT_MM_SOURCES} ${SUBPROJECT_HEADERS})


# --------------------------   common    -------------------------
set(COMMON_ROOT "${ICQ_ROOT}/common.shared")

find_sources(COMMON_SOURCES "${COMMON_ROOT}" "cpp")
find_sources(COMMON_HEADERS "${COMMON_ROOT}" "h")

set_source_group("sources" "${COMMON_ROOT}" ${COMMON_SOURCES} ${COMMON_HEADERS})


# -------------------------   gui.shared   -----------------------
set(GUI_SHARED_ROOT "${ICQ_ROOT}/gui.shared")

find_sources(GUI_SHARED_SOURCES "${GUI_SHARED_ROOT}" "cpp")
find_sources(GUI_SHARED_HEADERS "${GUI_SHARED_ROOT}" "h")

set_source_group("sources" "${GUI_SHARED_ROOT}" ${GUI_SHARED_SOURCES} ${GUI_SHARED_HEADERS})


# ----------------------------   mac   ---------------------------
if(APPLE)
    set(ICQ_PROTOLIB_ROOT "${ICQ_EXTERNAL_PLATFORM}/protolib")

    include_directories(${ICQ_PROTOLIB_ROOT})

    find_sources(ICQ_PROTOLIB_SOURCES "${ICQ_PROTOLIB_ROOT}" "cpp")
    find_sources(ICQ_PROTOLIB_MM_SOURCES "${ICQ_PROTOLIB_ROOT}" "mm")
    find_sources(ICQ_PROTOLIB_HEADERS "${ICQ_PROTOLIB_ROOT}" "h")

    set_source_group("protolib" "${ICQ_PROTOLIB_ROOT}"
        ${ICQ_PROTOLIB_SOURCES} ${ICQ_PROTOLIB_MM_SOURCES} ${ICQ_PROTOLIB_HEADERS})


    set(ICQ_SSKEYCHAIN_ROOT "${ICQ_EXTERNAL_PLATFORM}/SSKeychain")

    include_directories(${ICQ_SSKEYCHAIN_ROOT})

    find_sources(ICQ_SSKEYCHAIN_MM_SOURCES "${ICQ_SSKEYCHAIN_ROOT}" "mm")
    find_sources(ICQ_SSKEYCHAIN_HEADERS "${ICQ_SSKEYCHAIN_ROOT}" "h")

    set_source_group("SSKeychain" "${ICQ_SSKEYCHAIN_ROOT}"
        ${ICQ_SSKEYCHAIN_MM_SOURCES} ${ICQ_SSKEYCHAIN_HEADERS})
endif()


# --------------------------    mocs    --------------------------
generate_mocs(ICQ_MOC_FILES ${SUBPROJECT_HEADERS} ${COMMON_HEADERS} ${GUI_SHARED_HEADERS})


# -------------------------  translations  -----------------------
set(ICQ_LANGUAGES ar cs de en fr pt ru uk)
set(ICQ_TRANSLATIONS_DIR "${SUBPROJECT_ROOT}/translations")
set(ICQ_LANGUAGE_FILES)
set(ICQ_QM_FILES)
get_filename_component(ICQ_LRELEASE_EXE "${ICQ_QT_BIN_DIR}/lrelease" ABSOLUTE)
foreach(lang IN ITEMS ${ICQ_LANGUAGES})
    set(lang_file "${ICQ_TRANSLATIONS_DIR}/${lang}.ts")
    set(qm_file "${ICQ_TRANSLATIONS_DIR}/${lang}.qm")
    add_custom_command(OUTPUT "${qm_file}"
        COMMAND ${ICQ_LRELEASE_EXE} ${lang_file} -qm ${qm_file}
        DEPENDS ${lang_file})
    list(APPEND ICQ_LANGUAGE_FILES "${lang_file}")
    list(APPEND ICQ_QM_FILES "${qm_file}")
endforeach()
source_group("translations" FILES ${ICQ_LANGUAGE_FILES})
source_group("translations" FILES ${ICQ_QM_FILES})


# --------------------------  resources  -------------------------
set(ICQ_RESOURCES_DIR "${SUBPROJECT_ROOT}/resources")

file(GLOB_RECURSE ICQ_GIF_FILES "${SUBPROJECT_ROOT}/*.gif")
file(GLOB_RECURSE ICQ_ICO_FILES "${SUBPROJECT_ROOT}/*.ico")
file(GLOB_RECURSE ICQ_JPG_FILES "${SUBPROJECT_ROOT}/*.jpg")
file(GLOB_RECURSE ICQ_PNG_FILES "${SUBPROJECT_ROOT}/*.png")
file(GLOB_RECURSE ICQ_TTF_FILES "${SUBPROJECT_ROOT}/*.ttf")
file(GLOB_RECURSE ICQ_WAV_FILES "${SUBPROJECT_ROOT}/*.wav")
set_source_group("resources" ${ICQ_RESOURCES_DIR} ${ICQ_GIF_FILES} ${ICQ_ICO_FILES}
    ${ICQ_JPG_FILES} ${ICQ_PNG_FILES} ${ICQ_TTF_FILES} ${ICQ_WAV_FILES})

file(GLOB_RECURSE ICQ_QSS_FILES "${SUBPROJECT_ROOT}/*.qss")
set_source_group("qss" ${SUBPROJECT_ROOT} ${ICQ_QSS_FILES})

set(ICQ_RESOURCE_FILES ${ICQ_QSS_FILES} ${ICQ_GIF_FILES} ${ICQ_ICO_FILES}
    ${ICQ_JPG_FILES} ${ICQ_PNG_FILES} ${ICQ_TTF_FILES} ${ICQ_WAV_FILES})

set(ICQ_RESOURCES "${SUBPROJECT_ROOT}/resource.qrc")
get_filename_component(ICQ_RCC_EXE "${ICQ_QT_BIN_DIR}/rcc" ABSOLUTE)
if(MSVC)
    set(ICQ_RESOURCES_OUTPUT "${ICQ_BIN_DIR}/qresource")
    add_custom_command(OUTPUT ${ICQ_RESOURCES_OUTPUT}
        COMMAND ${ICQ_RCC_EXE} ${ICQ_RESOURCES} -o ${ICQ_RESOURCES_OUTPUT} --binary
        DEPENDS ${ICQ_QM_FILES} ${ICQ_RESOURCES} ${ICQ_RESOURCE_FILES})

    set(RC_FILE "${SUBPROJECT_ROOT}/Resource.rc")
    if(IS_ICQ)
        set(RC_SOURCE "${SUBPROJECT_ROOT}/Resource-icq.rc")
    else()
        set(RC_SOURCE "${SUBPROJECT_ROOT}/Resource-agent.rc")
    endif()
    add_custom_command(OUTPUT ${RC_FILE}
        DEPENDS ${RC_SOURCE}
        COMMAND ${CMAKE_COMMAND} -E copy ${RC_SOURCE} ${RC_FILE})
    add_custom_target(generate_rc DEPENDS ${RC_FILE})
    source_group("sources" FILES ${RC_FILE})
    set(ADDITIONAL_FILES ${ADDITIONAL_FILES} ${RC_FILE})
else()
    set(ICQ_RESOURCES_OUTPUT "${CMAKE_CURRENT_BINARY_DIR}/qresource.cpp")
    add_custom_command(OUTPUT ${ICQ_RESOURCES_OUTPUT}
        COMMAND ${ICQ_RCC_EXE} ${ICQ_RESOURCES} -o ${ICQ_RESOURCES_OUTPUT}
        DEPENDS ${ICQ_QM_FILES} ${ICQ_RESOURCES} ${ICQ_RESOURCE_FILES})
endif()
source_group("resources" FILES ${ICQ_RESOURCES})
source_group("resources" FILES ${ICQ_RESOURCES_OUTPUT})


# ----------------------------------------------------------------
include_directories(${SUBPROJECT_ROOT})

if(MSVC)
    use_precompiled_header_msvc("stdafx.h" "${SUBPROJECT_ROOT}/stdafx.cpp"
        ${SUBPROJECT_SOURCES} ${COMMON_SOURCES} ${GUI_SHARED_SOURCES} ${ICQ_MOC_FILES})
elseif(APPLE)
    use_precompiled_header_mac(PCH_BIN "${SUBPROJECT_ROOT}/stdafx.h"
        ${SUBPROJECT_SOURCES} ${COMMON_SOURCES} ${GUI_SHARED_SOURCES} ${ICQ_MOC_FILES})
elseif(LINUX)
    use_precompiled_header_linux(PCH_BIN "${SUBPROJECT_ROOT}/stdafx.h"
        ${SUBPROJECT_SOURCES} ${COMMON_SOURCES} ${GUI_SHARED_SOURCES} ${ICQ_MOC_FILES})
endif()

add_executable(${PROJECT_NAME} MACOSX_BUNDLE WIN32
    ${SUBPROJECT_SOURCES} ${SUBPROJECT_MM_SOURCES} ${SUBPROJECT_HEADERS}
    ${COMMON_SOURCES} ${COMMON_HEADERS}
    ${GUI_SHARED_SOURCES} ${GUI_SHARED_HEADERS}
    ${ICQ_PROTOLIB_SOURCES} ${ICQ_PROTOLIB_MM_SOURCES} ${ICQ_PROTOLIB_HEADERS}
    ${ICQ_SSKEYCHAIN_MM_SOURCES} ${ICQ_SSKEYCHAIN_HEADERS}
    ${ICQ_MOC_FILES}
    ${ICQ_LANGUAGE_FILES}
    ${ICQ_QM_FILES}
    ${ICQ_RESOURCES_OUTPUT}
    ${ADDITIONAL_FILES}
    ${PCH_BIN}
    ${ICQ_RESOURCE_FILES}
    ${MACOSX_BUNDLE_ICON_FILE})
set_property(DIRECTORY ${SUBPROJECT_ROOT} PROPERTY VS_STARTUP_PROJECT ${PROJECT_NAME})

if(MSVC)
    #add_dependencies(${PROJECT_NAME} GENERATE_RC)
endif()

if(ICQ_CORELIB_STATIC_LINKING)
    target_link_libraries(${PROJECT_NAME}
        ${OPEN_AL_LIBRARIES}
        ${FFMPEG_LIBRARIES}
        ${PREPROCESSOR_LIBRARIES}
        ${QT_LIBRARIES}
        ${SYSTEM_LIBRARIES}
        corelib)
else()
    target_link_libraries(${PROJECT_NAME}
        ${OPEN_AL_LIBRARIES}
        ${FFMPEG_LIBRARIES}
        ${PREPROCESSOR_LIBRARIES}
        ${QT_LIBRARIES}
        ${ZLIB_LIBRARIES}
        ${SYSTEM_LIBRARIES})
    add_dependencies(${PROJECT_NAME} corelib)
endif()

if(MSVC)
    if(ICQ_DEBUG)
        set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} /NODEFAULTLIB:\"libcmt.lib\"")
    endif()
    set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} /SUBSYSTEM:WINDOWS")
endif()


# ---------------------------   bundle   -------------------------
if(APPLE)
    set(ICQ_BUNDLE "${ICQ_BIN_DIR}/${PROJECT_NAME}.app")

    set(ICQ_BUNDLE_DIR "${ICQ_BUNDLE}/Contents")

    set(ICQ_PLIST "${ICQ_BUNDLE_DIR}/Info.plist")
    set(PLIST_BUDDY "/usr/libexec/PlistBuddy")

    if(IS_ICQ)
        add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
            COMMAND cp -f "${ICQ_MAC_ROOT}/imosx-icq-Info.plist" ${ICQ_PLIST})
        add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
            COMMAND ${PLIST_BUDDY} -c "Set :CFBundleExecutable ${PROJECT_NAME}" ${ICQ_PLIST} VERBATIM)
        add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
            COMMAND ${PLIST_BUDDY} -c "Set :CFBundleName ${PROJECT_NAME}" ${ICQ_PLIST} VERBATIM)
    elseif(IS_AGENT)
        add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
            COMMAND cp -f "${ICQ_MAC_ROOT}/imosx-agent-Info.plist" ${ICQ_PLIST})
    endif()

    add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
        COMMAND ${PLIST_BUDDY} -c "Set :LSMinimumSystemVersion 10.7" ${ICQ_PLIST} VERBATIM)

    add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy "${ICQ_MAC_ROOT}/PkgInfo" ${ICQ_BUNDLE_DIR})

    set(ICQ_FRAMEWORKS_BUNDLE_DIR "${ICQ_BUNDLE_DIR}/Frameworks")
    add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E make_directory ${ICQ_FRAMEWORKS_BUNDLE_DIR})
    add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
        COMMAND rsync -K -a --exclude=Headers --exclude=Versions/A/Headers "${ICQ_EXTERNAL_PLATFORM}/HockeySDK.framework" "${ICQ_FRAMEWORKS_BUNDLE_DIR}")

    if(ICQ_UPDATE)
        add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
            COMMAND rsync -K -a --exclude=Headers --exclude=Versions/A/Headers "${ICQ_EXTERNAL_PLATFORM}/Sparkle.framework" "${ICQ_FRAMEWORKS_BUNDLE_DIR}")
    endif()

    set(ICQ_RESOURCES_BUNDLE_DIR "${ICQ_BUNDLE_DIR}/Resources")
    add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E make_directory ${ICQ_RESOURCES_BUNDLE_DIR})
    add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy "${ICQ_MAC_ROOT}/dsa_pub.pem" ${ICQ_RESOURCES_BUNDLE_DIR})

    if(ICQ_QT_DYNAMIC)
        set(ICQ_PLUGINS_BUNDLE_DIR "${ICQ_BUNDLE_DIR}/PlugIns")
        add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E make_directory ${ICQ_PLUGINS_BUNDLE_DIR})

        set(ICQ_PLUGINS_PLATFORMS_BUNDLE_DIR "${ICQ_PLUGINS_BUNDLE_DIR}/platforms")
        add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E make_directory ${ICQ_PLUGINS_PLATFORMS_BUNDLE_DIR})

        add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
            COMMAND cp -f ${QT_PLUGINS_TO_COPY} ${ICQ_PLUGINS_PLATFORMS_BUNDLE_DIR})

        add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
            COMMAND cp -f ${QT_LIBRARIES_TO_COPY} ${ICQ_FRAMEWORKS_BUNDLE_DIR})

        add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
            COMMAND install_name_tool -add_rpath @loader_path/../Frameworks ${ICQ_BUNDLE_DIR}/MacOS/${PROJECT_NAME})
    endif()
endif()
