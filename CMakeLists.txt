cmake_minimum_required(VERSION 3.4)

project(icq)

if(NOT APP_TYPE)
    set(IS_ICQ ON)
    message(STATUS "building: ICQ (NOT APP_TYPE)")
elseif(${APP_TYPE} STREQUAL "ICQ")
    set(IS_ICQ ON)
    message(STATUS "building: ICQ")
elseif(${APP_TYPE} STREQUAL "Agent")
    set(IS_AGENT ON)
    message(STATUS "building: Agent")
else()
    message(FATAL_ERROR "You must specify the application type (-DAPP_TYPE=ICQ|Agent)")
endif()

if(NOT ICQ_FINAL_BUILD)
    add_definitions(-DICQ_DEVELOPMENT_BUILD)
    message(STATUS "build type: dev")
else()
    add_definitions(-DICQ_FINAL_BUILD)
    message(STATUS "build type: final")
endif()

if(NOT ICQ_QT_DYNAMIC)
    add_definitions(-DICQ_QT_STATIC)
    message(STATUS "Qt linking: static")
else()
    add_definitions(-DICQ_QT_DYNAMIC)
    message(STATUS "Qt linking: dynamic")
endif()

get_filename_component(ICQ_ROOT "${CMAKE_SOURCE_DIR}" ABSOLUTE)

if(${CMAKE_BUILD_TYPE} STREQUAL "Debug")
    set(ICQ_DEBUG ON)
    set(ICQ_BIN_DIR "${ICQ_ROOT}/bin/Debug")
    set(ICQ_LIB_DIR "${ICQ_ROOT}/bin/Debug")
    set(CMAKE_CONFIGURATION_TYPES "Debug" CACHE STRING "" FORCE)
elseif(${CMAKE_BUILD_TYPE} STREQUAL "Release")
    set(ICQ_RELEASE ON)
    set(ICQ_BIN_DIR "${ICQ_ROOT}/bin/Release")
    set(ICQ_LIB_DIR "${ICQ_ROOT}/bin/Release")
    set(CMAKE_CONFIGURATION_TYPES "Release" CACHE STRING "" FORCE)
else()
    message(FATAL_ERROR "Need to specify CMAKE_BUILD_TYPE (-DCMAKE_BUILDTYPE=Debug|Release)")
endif()

file(MAKE_DIRECTORY ${ICQ_BIN_DIR})

message(STATUS "binaries output: " ${ICQ_BIN_DIR})
message(STATUS "libraries output: " ${ICQ_LIB_DIR})

set(ICQ_CORELIB_STATIC_LINKING ON)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if(MSVC)
    set(ICQ_PLATFORM windows)
    set(ICQ_CORELIB_STATIC_LINKING OFF)
    if(ICQ_DEBUG)
        add_definitions(/MTd)
    else()
        add_definitions(/MT)
        add_definitions(/Oi /GL)
        set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} /LTCG")
    endif()
    add_definitions(-D_UNICODE)
    add_definitions(-D_VARIADIC_MAX=6)
    add_definitions(/Zm500)
    add_definitions(/Zi)
    add_definitions(/W4)
    add_definitions(/wd4100)
    set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} /DEBUG /OPT:REF /OPT:ICF")
    set(CMAKE_SHARED_LINKER_FLAGS_RELEASE "${CMAKE_SHARED_LINKER_FLAGS_RELEASE} /DEBUG /OPT:REF /OPT:ICF")
    set(WIN_SDK8_DIR "$ENV{ProgramFiles\(x86\)}/Windows Kits/8.0")
    include_directories("${WIN_SDK8_DIR}/Include/shared")
    include_directories("${WIN_SDK8_DIR}/Include/winrt")
    include_directories("${WIN_SDK8_DIR}/Include/um")
    link_directories("${WIN_SDK8_DIR}/Lib/win8/um/x86")
elseif(APPLE)
    set(ICQ_PLATFORM mac)
    set(CMAKE_XCODE_ATTRIBUTE_DEBUG_INFORMATION_FORMAT "dwarf-with-dsym")
    set(CMAKE_XCODE_ATTRIBUTE_GCC_GENERATE_DEBUGGING_SYMBOLS "YES")
    set(CMAKE_OSX_DEPLOYMENT_TARGET 10.8)
    list(APPEND _build_flags "-fvisibility-inlines-hidden")
    list(APPEND _build_flags "-fvisibility=hidden")
    list(APPEND _build_flags "-fasm-blocks")
    list(APPEND _build_flags "-stdlib=libc++")
    add_definitions(-fvisibility-inlines-hidden)
    add_definitions(-fvisibility=hidden)
    add_definitions(-fasm-blocks)
    add_definitions(-stdlib=libc++)
    add_definitions(-mmacosx-version-min=10.8)
    set(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} -stdlib=libc++")
    set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -stdlib=libc++")
elseif(UNIX)
    set(LINUX ON)
    if(LINUX_ARCH EQUAL 64)
        set(LINUX64 ON)
        set(ARCH_FLAGS "-m64")
        set(ICQ_PLATFORM linux/x64)
        set(ICQ_BIN_DIR "${ICQ_ROOT}/bin/${CMAKE_CONFIGURATION_TYPES}64")
        set(ICQ_LIB_DIR "${ICQ_ROOT}/bin/${CMAKE_CONFIGURATION_TYPES}64")
        message(STATUS "Linux x64")
    else()
        set(LINUX32 ON)
        set(ARCH_FLAGS "-m32")
        set(ICQ_PLATFORM linux)
        set(ICQ_BIN_DIR "${ICQ_ROOT}/bin/${CMAKE_CONFIGURATION_TYPES}32")
        set(ICQ_LIB_DIR "${ICQ_ROOT}/bin/${CMAKE_CONFIGURATION_TYPES}32")
        message(STATUS "Linux x32")
    endif()
    add_definitions(-fPIC)
    add_definitions(-DSTRIP_VOIP)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${ARCH_FLAGS}")
    set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} ${ARCH_FLAGS} -static-libgcc -static-libstdc++")
    set(CMAKE_SHARED_LINKER_FLAGS_RELEASE "${CMAKE_SHARED_LINKER_FLAGS_RELEASE} ${ARCH_FLAGS} -static-libgcc -static-libstdc++")
else()
    message(FATAL_ERROR "Unknown platform")
endif()

message(STATUS "building on: ${ICQ_PLATFORM}")
if(ICQ_CORELIB_STATIC_LINKING)
    message(STATUS "corelib linking: static")
    add_definitions(-DICQ_CORELIB_STATIC_LINKING)
else()
    message(STATUS "corelib linking: dynamic")
endif()


# ---------------------------  paths  ----------------------------
set(CMAKE_EXECUTABLE_OUTPUT_DIRECTORY_DEBUG ${ICQ_BIN_DIR})
set(CMAKE_EXECUTABLE_OUTPUT_DIRECTORY_RELEASE ${ICQ_BIN_DIR})
set(CMAKE_EXECUTABLE_OUTPUT_PATH ${ICQ_BIN_DIR})
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY_DEBUG ${ICQ_LIB_DIR})
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY_RELEASE ${ICQ_LIB_DIR})
set(CMAKE_LIBRARY_OUTPUT_PATH ${ICQ_LIB_DIR})
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_DEBUG ${ICQ_BIN_DIR})
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_RELEASE ${ICQ_BIN_DIR})
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${ICQ_BIN_DIR})


# --------------------------  external  --------------------------
set(ICQ_EXTERNAL "${ICQ_ROOT}/external")
set(ICQ_EXTERNAL_PLATFORM "${ICQ_EXTERNAL}/${ICQ_PLATFORM}")


# --------------------------    boost   --------------------------
set(BOOST_INCLUDEDIR "${ICQ_EXTERNAL}/boost/include")
set(BOOST_LIBRARYDIR "${ICQ_EXTERNAL_PLATFORM}/boost/lib")
set(Boost_USE_STATIC_LIBS ON)
set(Boost_USE_STATIC_RUNTIME ON)
set(Boost_NO_SYSTEM_PATHS ON)
if(MSVC)
    add_definitions(-D_SCL_SECURE_NO_WARNINGS)
    find_package(Boost REQUIRED chrono date_time filesystem locale system thread unit_test_framework)
elseif(APPLE)
    find_package(Boost REQUIRED)
    set(Boost_LIBRARIES ${Boost_LIBRARIES} "${ICQ_EXTERNAL_PLATFORM}/boost/lib/boost.a")
    set(Boost_LIBRARIES ${Boost_LIBRARIES} "${ICQ_EXTERNAL_PLATFORM}/boost/lib/boost_locale.a")
    set(Boost_LIBRARIES ${Boost_LIBRARIES} "${ICQ_EXTERNAL_PLATFORM}/boost/lib/libboost_unit_test_framework.a")
elseif(LINUX)
    set(Boost_LIBRARIES ${Boost_LIBRARIES} "${ICQ_EXTERNAL_PLATFORM}/libboost_filesystem.a")
    set(Boost_LIBRARIES ${Boost_LIBRARIES} "${ICQ_EXTERNAL_PLATFORM}/libboost_locale.a")
    set(Boost_LIBRARIES ${Boost_LIBRARIES} "${ICQ_EXTERNAL_PLATFORM}/libboost_system.a")
    set(Boost_LIBRARIES ${Boost_LIBRARIES} "${ICQ_EXTERNAL_PLATFORM}/libboost_thread.a")
endif()
include_directories(${BOOST_INCLUDEDIR})


# -------------------------- rapid-json --------------------------
add_definitions(-DRAPIDJSON_HAS_STDSTRING=1)
set(ICQ_RAPID_JSON "${ICQ_EXTERNAL}/rapidjson-master")
include_directories(${ICQ_RAPID_JSON}/include)


# --------------------------    curl    --------------------------
add_definitions(-DCURL_STATICLIB)
set(ICQ_CURL "${ICQ_EXTERNAL}/curl")
set(CURL_INCLUDE_DIR "${ICQ_CURL}/include")
include_directories(${CURL_INCLUDE_DIR})
if(MSVC)
    set(CURL_LIBRARY_DIR "${ICQ_CURL}/lib")
    set(CURL_LIBRARIES
        optimized "${CURL_LIBRARY_DIR}/libcurl.lib"
        debug "${CURL_LIBRARY_DIR}/libcurld.lib")
elseif(APPLE)
    set(CURL_LIBRARIES "${ICQ_EXTERNAL_PLATFORM}/libcurl.a")
endif()


# -------------------------   libevent   -------------------------
set(ICQ_LIBEVENT "${ICQ_EXTERNAL_PLATFORM}/libevent-2.1.8")
include_directories(${ICQ_LIBEVENT}/include)
if(MSVC)
    set(LIBEVENT_LIBRARIES
        optimized "${ICQ_LIBEVENT}/lib/event.lib" debug "${ICQ_LIBEVENT}/lib/eventd.lib"
        optimized "${ICQ_LIBEVENT}/lib/event_core.lib" debug "${ICQ_LIBEVENT}/lib/event_cored.lib"
        optimized "${ICQ_LIBEVENT}/lib/event_extra.lib" debug "${ICQ_LIBEVENT}/lib/event_extrad.lib")
elseif(LINUX)
    set(LIBEVENT_LIBRARIES
        "${ICQ_LIBEVENT}/lib/libevent.a"
        "${ICQ_LIBEVENT}/lib/libevent_pthreads.a"
        )
elseif(APPLE)
    set(LIBEVENT_LIBRARIES
        "${ICQ_LIBEVENT}/lib/libevent.a"
        "${ICQ_LIBEVENT}/lib/libevent_pthreads.a"
        )
endif()


# --------------------------   openssl   -------------------------
set(ICQ_OPENSSL "${ICQ_EXTERNAL}/openssl")
include_directories(${ICQ_OPENSSL})
if(MSVC)
    set(OPENSSL_LIBRARIES
        optimized "${ICQ_OPENSSL}/libeay32.lib" optimized "${ICQ_OPENSSL}/ssleay32.lib"
        debug "${ICQ_OPENSSL}/debug/libeay32.lib" debug "${ICQ_OPENSSL}/debug/ssleay32.lib")
elseif(LINUX)
elseif(APPLE)
    set(OPENSSL_LIBRARIES "${ICQ_EXTERNAL_PLATFORM}/libssl.a")
endif()


# --------------------------    zlib    --------------------------
if(MSVC)
    set(ICQ_ZLIB "${ICQ_EXTERNAL}/zlib")
    set(ZLIB_ROOT "${ICQ_ZLIB}")
    find_package(ZLIB)
    include_directories(${ZLIB_INCLUDE_DIRS})
endif()


# ------------------------    libcrypto    -----------------------
if(APPLE)
    set(LIBCRYPTO_LIBRARIES "${ICQ_EXTERNAL_PLATFORM}/libcrypto.a")
endif()


# --------------------------   OpenAl   --------------------------
set(ICQ_OPEN_AL "${ICQ_EXTERNAL}/OpenAl")
include_directories(${ICQ_OPEN_AL}/include)
add_definitions(-DAL_LIBTYPE_STATIC)
if(MSVC)
    set(OPEN_AL_LIBRARIES
        optimized "${ICQ_OPEN_AL}/lib/OpenAL32.lib"
        debug "${ICQ_OPEN_AL}/lib/OpenAL32d.lib")
elseif(APPLE)
    set(OPEN_AL_LIBRARIES "${ICQ_OPEN_AL}/lib/libopenal.a")
elseif(LINUX)
    set(OPEN_AL_LIBRARIES "${ICQ_OPEN_AL}/lib/${ICQ_PLATFORM}/libopenal.a")
endif()


# -----------------------   preprocessor   -----------------------
if(MSVC)
    set(PREPROCESSOR_LIBRARIES
        optimized "${ICQ_EXTERNAL_PLATFORM}/preprocessor.lib"
        debug "${ICQ_EXTERNAL_PLATFORM}/preprocessord.lib")
endif()


# --------------------------   ffmpeg   --------------------------
set(ICQ_FFMPEG "${ICQ_EXTERNAL}/ffmpeg")
include_directories(${ICQ_FFMPEG}/include)
if(MSVC)
    include_directories(${ICQ_FFMPEG}/include/windows)
    set(FFMPEG_LIBRARIES "${ICQ_FFMPEG}/lib/libavcodec.lib"
        "${ICQ_FFMPEG}/lib/libavfilter.lib" "${ICQ_FFMPEG}/lib/libavformat.lib"
        "${ICQ_FFMPEG}/lib/libavutil.lib" "${ICQ_FFMPEG}/lib/libswresample.lib"
        "${ICQ_FFMPEG}/lib/libswscale.lib")
elseif(APPLE)
    set(FFMPEG_LIBRARIES "${ICQ_FFMPEG}/lib/libavcodec.a"
        "${ICQ_FFMPEG}/lib/libavfilter.a" "${ICQ_FFMPEG}/lib/libavformat.a"
        "${ICQ_FFMPEG}/lib/libavutil.a" "${ICQ_FFMPEG}/lib/libswresample.a"
        "${ICQ_FFMPEG}/lib/libswscale.a")
elseif(LINUX)
    set(FFMPEG_LIBRARIES "${ICQ_FFMPEG}/lib/${ICQ_PLATFORM}/libavcodec.a"
        "${ICQ_FFMPEG}/lib/${ICQ_PLATFORM}/libavfilter.a" "${ICQ_FFMPEG}/lib/${ICQ_PLATFORM}/libavformat.a"
        "${ICQ_FFMPEG}/lib/${ICQ_PLATFORM}/libavutil.a" "${ICQ_FFMPEG}/lib/${ICQ_PLATFORM}/libswresample.a"
        "${ICQ_FFMPEG}/lib/${ICQ_PLATFORM}/libswscale.a")
endif()


# --------------------------   libvoip   -------------------------
if(MSVC)
    set(ICQ_VOIP "${ICQ_ROOT}/core/Voip/libvoip/lib")
    set(VOIP_LIBRARIES
        optimized "${ICQ_VOIP}/release/libvoip_x86.lib"
        debug "${ICQ_VOIP}/debug/libvoip_x86.lib")
    set(VOIP_LIBRARY_1 "${ICQ_VOIP}/${CMAKE_BUILD_TYPE}/libvoip_x86.dll")
    set(VOIP_LIBRARY_2 "${ICQ_VOIP}/${CMAKE_BUILD_TYPE}/libvoip_x86.lib")
    set(VOIP_DX_COMPILER_LIBRARY "${ICQ_VOIP}/${CMAKE_BUILD_TYPE}/D3DCompiler_43.dll")
elseif(APPLE)
    set(ICQ_VOIP "${ICQ_ROOT}/core/Voip/libvoip/lib.mac")
    set(VOIP_LIBRARIES "${ICQ_VOIP}/libvoip_x64.a" "${ICQ_VOIP}/libippcore_l.a" "${ICQ_VOIP}/libippvc_l.a")
endif()


# --------------------------     qt     --------------------------
if(LINUX)
    set(ICQ_QT_ROOT "${ICQ_EXTERNAL}/linux/Qt")
    set(ICQ_QT_INCLUDE_DIR "${ICQ_QT_ROOT}/include")
    if(LINUX32)
        set(ICQ_QT_LIB_DIR "${ICQ_QT_ROOT}/lib")
    else()
        set(ICQ_QT_LIB_DIR "${ICQ_QT_ROOT}/x64/lib")
    endif()
    set(ICQ_QT_BIN_DIR "${ICQ_QT_ROOT}/bin")
else()
    set(ICQ_QT_ROOT "${ICQ_EXTERNAL_PLATFORM}/Qt")
    set(ICQ_QT_INCLUDE_DIR "${ICQ_QT_ROOT}/include")
    set(ICQ_QT_LIB_DIR "${ICQ_QT_ROOT}/lib")
    set(ICQ_QT_BIN_DIR "${ICQ_QT_ROOT}/bin")
endif()

include_directories("${ICQ_QT_INCLUDE_DIR}")
include_directories("${ICQ_QT_INCLUDE_DIR}/QtCore")
include_directories("${ICQ_QT_INCLUDE_DIR}/QtGui")
include_directories("${ICQ_QT_INCLUDE_DIR}/QtWidgets")
include_directories("${ICQ_QT_INCLUDE_DIR}/QtOpenGL")

if(ICQ_QT_DYNAMIC)
    if(MSVC)
        message(FATAL_ERROR "Not implemented yet")
    elseif(APPLE)
        add_definitions(-DQT_GUI_LIB=1)
        set(QT_LIBRARIES
            optimized ${ICQ_QT_LIB_DIR}/libQt5Core.5.dylib debug ${ICQ_QT_LIB_DIR}/libQt5Core_debug.5.dylib
            optimized ${ICQ_QT_LIB_DIR}/libQt5Gui.5.dylib debug ${ICQ_QT_LIB_DIR}/libQt5Gui_debug.5.dylib
            optimized ${ICQ_QT_LIB_DIR}/libQt5Widgets.5.dylib debug ${ICQ_QT_LIB_DIR}/libQt5Widgets_debug.5.dylib
            optimized ${ICQ_QT_LIB_DIR}/libQt5PrintSupport.5.dylib debug ${ICQ_QT_LIB_DIR}/libQt5PrintSupport_debug.5.dylib
            optimized ${ICQ_QT_LIB_DIR}/libQt5Multimedia.5.dylib debug ${ICQ_QT_LIB_DIR}/libQt5Multimedia_debug.5.dylib
            optimized ${ICQ_QT_LIB_DIR}/libQt5Network.5.dylib debug ${ICQ_QT_LIB_DIR}/libQt5Network_debug.5.dylib
            )

        if(ICQ_DEBUG)
            set(QT_LIBRARIES_TO_COPY
                ${ICQ_QT_LIB_DIR}/libQt5Core_debug.5.dylib
                ${ICQ_QT_LIB_DIR}/libQt5Gui_debug.5.dylib
                ${ICQ_QT_LIB_DIR}/libQt5Widgets_debug.5.dylib
                ${ICQ_QT_LIB_DIR}/libQt5PrintSupport_debug.5.dylib
                ${ICQ_QT_LIB_DIR}/libQt5Multimedia_debug.5.dylib
                ${ICQ_QT_LIB_DIR}/libQt5Network_debug.5.dylib
            )
        else()
            set(QT_LIBRARIES_TO_COPY
                ${ICQ_QT_LIB_DIR}/libQt5Core.5.dylib
                ${ICQ_QT_LIB_DIR}/libQt5Gui.5.dylib
                ${ICQ_QT_LIB_DIR}/libQt5Widgets.5.dylib
                ${ICQ_QT_LIB_DIR}/libQt5PrintSupport.5.dylib
                ${ICQ_QT_LIB_DIR}/libQt5Multimedia.5.dylib
                ${ICQ_QT_LIB_DIR}/libQt5Network.5.dylib
            )
        endif()

        if(ICQ_DEBUG)
            set(QT_PLUGINS_TO_COPY ${ICQ_QT_LIB_DIR}/libqcocoa_debug.dylib)
        else()
            set(QT_PLUGINS_TO_COPY ${ICQ_QT_LIB_DIR}/libqcocoa.dylib)
        endif()
    elseif(LINUX)
        message(FATAL_ERROR "Not implemented yet")
    endif()
else()
    add_definitions(-DQT_STATIC)
    if(MSVC)
        set(QT_LIBRARIES
            optimized ${ICQ_QT_LIB_DIR}/Qt5Gui.lib debug ${ICQ_QT_LIB_DIR}/Qt5Guid.lib
            optimized ${ICQ_QT_LIB_DIR}/Qt5Core.lib debug ${ICQ_QT_LIB_DIR}/Qt5Cored.lib
            optimized ${ICQ_QT_LIB_DIR}/Qt5Widgets.lib debug ${ICQ_QT_LIB_DIR}/Qt5Widgetsd.lib
            optimized ${ICQ_QT_LIB_DIR}/Qt5PlatformSupport.lib debug ${ICQ_QT_LIB_DIR}/Qt5PlatformSupportd.lib
            optimized ${ICQ_QT_LIB_DIR}/Qt5Multimedia.lib debug ${ICQ_QT_LIB_DIR}/Qt5Multimediad.lib
            optimized ${ICQ_QT_LIB_DIR}/Qt5MultimediaWidgets.lib debug ${ICQ_QT_LIB_DIR}/Qt5MultimediaWidgetsd.lib
            optimized ${ICQ_QT_LIB_DIR}/Qt5Network.lib debug ${ICQ_QT_LIB_DIR}/Qt5Networkd.lib
            optimized ${ICQ_QT_LIB_DIR}/qtharfbuzzng.lib debug ${ICQ_QT_LIB_DIR}/qtharfbuzzngd.lib
            optimized ${ICQ_QT_LIB_DIR}/qtmain.lib debug ${ICQ_QT_LIB_DIR}/qtmaind.lib
            optimized ${ICQ_QT_LIB_DIR}/qwindows.lib debug ${ICQ_QT_LIB_DIR}/qwindowsd.lib
            optimized ${ICQ_QT_LIB_DIR}/qico.lib debug ${ICQ_QT_LIB_DIR}/qicod.lib
            optimized ${ICQ_QT_LIB_DIR}/qtaudio_windows.lib debug ${ICQ_QT_LIB_DIR}/qtaudio_windowsd.lib
            optimized ${ICQ_QT_LIB_DIR}/qtiff.lib debug ${ICQ_QT_LIB_DIR}/qtiffd.lib
            optimized ${ICQ_QT_LIB_DIR}/qjp2.lib debug ${ICQ_QT_LIB_DIR}/qjp2d.lib
            optimized ${ICQ_QT_LIB_DIR}/Qt5OpenGL.lib debug ${ICQ_QT_LIB_DIR}/Qt5OpenGLd.lib
            optimized ${ICQ_QT_LIB_DIR}/qtpcre.lib debug ${ICQ_QT_LIB_DIR}/qtpcred.lib
            optimized ${ICQ_QT_LIB_DIR}/qtfreetype.lib debug ${ICQ_QT_LIB_DIR}/qtfreetyped.lib
    )
    elseif(APPLE)
        add_definitions(-DQT_GUI_LIB=1)
        set(QT_LIBRARIES
            ${ICQ_QT_LIB_DIR}/libQt5Concurrent.a
            ${ICQ_QT_LIB_DIR}/libQt5Core.a
            ${ICQ_QT_LIB_DIR}/libQt5DBus.a
            ${ICQ_QT_LIB_DIR}/libQt5Gui.a
            ${ICQ_QT_LIB_DIR}/libQt5Multimedia.a
            ${ICQ_QT_LIB_DIR}/libQt5Network.a
            ${ICQ_QT_LIB_DIR}/libQt5PlatformSupport.a
            ${ICQ_QT_LIB_DIR}/libQt5PrintSupport.a
            ${ICQ_QT_LIB_DIR}/libQt5Widgets.a
	        ${ICQ_QT_LIB_DIR}/libQt5MacExtras.a
            ${ICQ_QT_LIB_DIR}/libqtfreetype.a
            ${ICQ_QT_LIB_DIR}/libqtharfbuzzng.a
            ${ICQ_QT_LIB_DIR}/libqtiff.a
            ${ICQ_QT_LIB_DIR}/libqico.a
            ${ICQ_QT_LIB_DIR}/libqtpcre.a
            ${ICQ_QT_LIB_DIR}/libqcocoa.a
            )
    elseif(LINUX)
        set(QT_LIBRARIES
            ${ICQ_QT_LIB_DIR}/libQt5Widgets.a
            ${ICQ_QT_LIB_DIR}/libqtmedia_audioengine.a
            ${ICQ_QT_LIB_DIR}/libqtmultimedia_m3u.a
            ${ICQ_QT_LIB_DIR}/libQt5Multimedia.a
            ${ICQ_QT_LIB_DIR}/libqxcb.a
            ${ICQ_QT_LIB_DIR}/libQt5XcbQpa.a
            ${ICQ_EXTERNAL_PLATFORM}/system/libXi.a
            ${ICQ_QT_LIB_DIR}/libxcb-static.a
            ${ICQ_EXTERNAL_PLATFORM}/system/libxkbcommon-x11.a
            ${ICQ_EXTERNAL_PLATFORM}/system/libxkbcommon.a
            ${ICQ_QT_LIB_DIR}/libQt5PlatformSupport.a
            ${ICQ_EXTERNAL_PLATFORM}/system/libfontconfig.a
            ${ICQ_EXTERNAL_PLATFORM}/system/libfreetype.a
            ${ICQ_QT_LIB_DIR}/libqico.a
            ${ICQ_QT_LIB_DIR}/libQt5Gui.a
            ${ICQ_QT_LIB_DIR}/libqconnmanbearer.a
            ${ICQ_QT_LIB_DIR}/libqgenericbearer.a
            ${ICQ_QT_LIB_DIR}/libqnmbearer.a
            ${ICQ_QT_LIB_DIR}/libQt5Network.a
            ${ICQ_QT_LIB_DIR}/libQt5DBus.a
            ${ICQ_EXTERNAL_PLATFORM}/system/libdbus-1.a
            ${ICQ_QT_LIB_DIR}/libQt5Core.a
            ${ICQ_QT_LIB_DIR}/libqtpcre.a
            ${ICQ_EXTERNAL_PLATFORM}/system/librt.a
            ${ICQ_EXTERNAL_PLATFORM}/system/libxcb-util.a
            ${ICQ_EXTERNAL_PLATFORM}/system/libffi.a
            ${ICQ_EXTERNAL_PLATFORM}/system/libpcre.a
            ${ICQ_EXTERNAL_PLATFORM}/system/libexpat.a
            ${ICQ_EXTERNAL_PLATFORM}/system/libXext.a
            ${ICQ_EXTERNAL_PLATFORM}/system/libXau.a
            ${ICQ_EXTERNAL_PLATFORM}/system/libXdmcp.a
            ${ICQ_EXTERNAL_PLATFORM}/system/libz.a)
    endif()
endif()


# ------------------------ source groups  ------------------------
macro(set_source_group group_name root_dir ${ARGN})
    get_filename_component(_path ${root_dir} ABSOLUTE)
    foreach(_source IN ITEMS ${ARGN})
        get_filename_component(_source_path "${_source}" PATH)
        string(REPLACE "${_path}" "" _source_trimmed "${_source_path}")
        set(_source_group "${group_name}${_source_trimmed}")
        string(REPLACE "/" "\\" _source_group "${_source_group}")
        source_group("${_source_group}" FILES "${_source}")
    endforeach()
endmacro()


# -------------------------- precompiled -------------------------
macro(use_precompiled_header_msvc pch_h pch_cpp ${ARGN})
    set(_pch_bin "${PROJECT_BINARY_DIR}/${CMAKE_BUILD_TYPE}.${PROJECT_NAME}.pch")
    foreach(_source IN ITEMS ${ARGN})
        set_source_files_properties(${_source} PROPERTIES
            COMPILE_FLAGS "/Yu\"${pch_h}\" /Fp\"${_pch_bin}\""
            OBJECT_DEPENDS "${_pch_bin}")
    endforeach()
    set_source_files_properties("${pch_cpp}" PROPERTIES
        COMPILE_FLAGS "/Yc\"${pch_h}\" /Fp\"${_pch_bin}\""
        OBJECT_DEPENDS "${_pch_bin}")
endmacro()


function(use_precompiled_header_mac output pch_h ${ARGN})
    set(_pch_bin "${PROJECT_BINARY_DIR}/${CMAKE_BUILD_TYPE}-${PROJECT_NAME}.pch")
    if(CMAKE_OSX_SYSROOT)
        list(APPEND _build_flags -isysroot "${CMAKE_OSX_SYSROOT}")
    endif()
    get_property(_raw_includes DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR} PROPERTY INCLUDE_DIRECTORIES)
    foreach(_it IN ITEMS ${_raw_includes})
        list(APPEND _paths "-I${_it}")
    endforeach()
    get_property(_raw_definitions DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR} PROPERTY COMPILE_DEFINITIONS)
    foreach(_it IN ITEMS ${_raw_definitions})
        list(APPEND _definitions "-D${_it}")
    endforeach()
    if(ICQ_RELEASE)
        string(REPLACE " " ";" _cmake_flags ${CMAKE_CXX_FLAGS_RELEASE})
        foreach(it IN ITEMS ${_cmake_flags})
            list(APPEND _build_flags "${it}")
        endforeach()
    endif()
    add_custom_command(OUTPUT ${_pch_bin}
        COMMAND ${CMAKE_CXX_COMPILER} ${_build_flags} ${_paths} ${_definitions} -mmacosx-version-min=10.8 -std=gnu++11 -x c++-header ${pch_h} -o ${_pch_bin}
        DEPENDS ${pch_h} VERBATIM)
    foreach(_source IN ITEMS ${ARGN})
        set_source_files_properties(${_source} PROPERTIES
            COMPILE_FLAGS "-include-pch ${_pch_bin}"
            OBJECT_DEPENDS "${_pch_bin}")
    endforeach()
    set(${output} ${_pch_bin} PARENT_SCOPE)
endfunction()


function(use_precompiled_header_linux output pch_h ${ARGN})
    set(_pch_bin "${pch_h}.gch")
    get_property(_raw_includes DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR} PROPERTY INCLUDE_DIRECTORIES)
    foreach(_it IN ITEMS ${_raw_includes})
        list(APPEND _paths "-I${_it}")
    endforeach()
    get_property(_raw_definitions DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR} PROPERTY COMPILE_DEFINITIONS)
    foreach(_it IN ITEMS ${_raw_definitions})
        list(APPEND _definitions "-D${_it}")
    endforeach()
    if(ICQ_RELEASE)
        string(REPLACE " " ";" _cmake_flags ${CMAKE_CXX_FLAGS_RELEASE})
        foreach(it IN ITEMS ${_cmake_flags})
            list(APPEND _build_flags "${it}")
        endforeach()
    endif()

    add_custom_command(OUTPUT ${_pch_bin}
        COMMAND ${CMAKE_CXX_COMPILER} ${_build_flags} ${_paths} ${_definitions} -std=c++0x ${ARCH_FLAGS} -fPIC  -x c++-header -o ${_pch_bin} ${pch_h}
        DEPENDS ${pch_h} VERBATIM)
    foreach(_source IN ITEMS ${ARGN})
        set_source_files_properties(${_source} PROPERTIES
            COMPILE_FLAGS "-std=c++0x ${ARCH_FLAGS} -Winvalid-pch"
            OBJECT_DEPENDS "${_pch_bin}")
    endforeach()
    set(${output} ${_pch_bin} PARENT_SCOPE)
endfunction()


# --------------------------    scan    --------------------------
function(find_sources output source_dir extension)
    file(GLOB_RECURSE _path_list "${source_dir}/*.${extension}")
    foreach(_path IN ITEMS ${_path_list})
        get_filename_component(_dir "${_path}" DIRECTORY)
        string(REGEX MATCH "win32$" _win32 ${_dir})
        string(REGEX MATCH "linux$" _linux ${_dir})
        string(REGEX MATCH "macos$" _macos ${_dir})
        if(WIN32)
            if(_linux OR _macos)
                message(STATUS "skipped: " ${_path})
                list(REMOVE_ITEM _path_list ${_path})
            endif()
        elseif(APPLE)
            if(_linux OR _win32)
                message(STATUS "skipped: " ${_path})
                list(REMOVE_ITEM _path_list ${_path})
            endif()
        else()
            if(_win32 OR _macos)
                message(STATUS "skipped: " ${_path})
                list(REMOVE_ITEM _path_list ${_path})
            endif()
        endif()
    endforeach()
    set(${output} ${_path_list} PARENT_SCOPE)
endfunction()


# --------------------------    mocs    --------------------------
function(generate_mocs output ${ARGN})
    set(ICQ_MOC_DIR "${CMAKE_CURRENT_BINARY_DIR}/mocs")
    get_filename_component(ICQ_MOC_EXE "${ICQ_QT_BIN_DIR}/moc" ABSOLUTE)
    foreach(header IN ITEMS ${ARGN})
        get_filename_component(name "${header}" NAME_WE)
        file(RELATIVE_PATH relative_path ${ICQ_ROOT} ${header})
        get_filename_component(relative_dir "${relative_path}" DIRECTORY)
        set(moc_dir "${ICQ_MOC_DIR}/${relative_dir}")
        file(MAKE_DIRECTORY ${moc_dir})
        set(moc_file "${moc_dir}/moc_${name}.cpp")
        list(APPEND _moc_files ${moc_file})
        if(APPLE)
            add_custom_command(OUTPUT ${moc_file}
                COMMAND ${ICQ_MOC_EXE} -f ${header} -o ${moc_file} ${header} 2> /dev/null
                DEPENDS ${header} VERBATIM)
	elseif(LINUX)
            add_custom_command(OUTPUT ${moc_file}
                COMMAND ${ICQ_MOC_EXE} -D__linux__ -f ${header} -o ${moc_file} ${header} 2> /dev/null
                DEPENDS ${header} VERBATIM)
        else()
            add_custom_command(OUTPUT ${moc_file}
                COMMAND ${ICQ_MOC_EXE} -f ${header} -o ${moc_file} ${header}
                DEPENDS ${header} VERBATIM)
        endif()
    endforeach()
    set_source_group("mocs" "${ICQ_MOC_DIR}" ${_moc_files})
    set(${output} ${_moc_files} PARENT_SCOPE)
endfunction()


# -------------------------  translations  -----------------------
function(generate_translations language_files qm_files translations_dir ${ARGN})
    set(_language_files)
    set(_qm_files)
    get_filename_component(ICQ_LRELEASE_EXE "${ICQ_QT_BIN_DIR}/lrelease" ABSOLUTE)
    foreach(lang IN ITEMS ${ARGN})
        set(lang_file "${translations_dir}/${lang}.ts")
        set(qm_file "${translations_dir}/${lang}.qm")
        add_custom_command(OUTPUT "${qm_file}"
            COMMAND ${ICQ_LRELEASE_EXE} ${lang_file} -qm ${qm_file}
            DEPENDS ${lang_file})
        list(APPEND _language_files "${lang_file}")
        list(APPEND _qm_files "${qm_file}")
    endforeach()
    source_group("translations" FILES ${_language_files})
    source_group("translations" FILES ${_qm_files})
    set(${language_files} ${_language_files} PARENT_SCOPE)
    set(${qm_files} ${_qm_files} PARENT_SCOPE)
endfunction()


# --------------------------  subdirs   --------------------------
add_subdirectory(core)
add_subdirectory(corelib)
add_subdirectory(gui)
if(MSVC)
    add_subdirectory(coretest)
    add_subdirectory(tests/unit_tests)
endif()
