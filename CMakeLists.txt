cmake_minimum_required (VERSION 3.15)

# Options
option (NRI_STATIC_LIBRARY "Build static library" OFF)

# Options: backends
option (NRI_ENABLE_NONE_SUPPORT "Enable NONE backend" ON)
option (NRI_ENABLE_D3D11_SUPPORT "Enable D3D11 backend" ON)
option (NRI_ENABLE_D3D12_SUPPORT "Enable D3D12 backend" ON)
option (NRI_ENABLE_VK_SUPPORT "Enable VULKAN backend" ON)

# Options: VK specific
option (NRI_ENABLE_XLIB_SUPPORT "Enable 'xlib' support" ON)
option (NRI_ENABLE_WAYLAND_SUPPORT "Enable 'wayland' support" ON)

# Options: D3D12 specific
option (NRI_ENABLE_AGILITY_SDK_SUPPORT "Enable Agility SDK suupport to unlock access to recent D3D12 features" OFF)
set (NRI_AGILITY_SDK_PATH "C:/AgilitySDK" CACHE STRING "Path to a directory containing Agility SDK (contents of '.nupkg/build/native/')")
set (NRI_AGILITY_SDK_VERSION "614" CACHE STRING "Agility SDK version")
set (NRI_AGILITY_SDK_DIR "AgilitySDK" CACHE STRING "Directory where Agility SDK binaries will be copied to relative to 'CMAKE_RUNTIME_OUTPUT_DIRECTORY'")

# Options: D3D11/D3D12 specific
option (NRI_ENABLE_EXTERNAL_LIBRARIES "Enable vendor specific extension libraries (NVAPI and AMD AGS)" ON)

# Is submodule?
if (${CMAKE_SOURCE_DIR} STREQUAL ${CMAKE_CURRENT_SOURCE_DIR})
    set (IS_SUBMODULE OFF)
else ()
    set (IS_SUBMODULE ON)
endif ()

# Cached
if (NOT IS_SUBMODULE)
    set (CMAKE_CONFIGURATION_TYPES "Debug;Release" CACHE STRING "")
    set (CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_SOURCE_DIR}/_Bin" CACHE STRING "")
endif ()

# Generate PDB for Release builds
if (MSVC)
    set (CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} /Zi")
    set (CMAKE_SHARED_LINKER_FLAGS_RELEASE "${CMAKE_SHARED_LINKER_FLAGS_RELEASE} /DEBUG /OPT:REF /OPT:ICF")
endif ()

# Create project
file (READ "${CMAKE_CURRENT_SOURCE_DIR}/Include/NRI.h" ver_h)
string (REGEX MATCH "NRI_VERSION_MAJOR ([0-9]*)" _ ${ver_h})
set (VERSION_MAJOR ${CMAKE_MATCH_1})
string (REGEX MATCH "NRI_VERSION_MINOR ([0-9]*)" _ ${ver_h})
set (VERSION_MINOR ${CMAKE_MATCH_1})

message ("NRI v${VERSION_MAJOR}.${VERSION_MINOR}")
project (NRI LANGUAGES C CXX)

if (IS_SUBMODULE)
    file (RELATIVE_PATH PROJECT_FOLDER ${CMAKE_SOURCE_DIR} ${CMAKE_CURRENT_SOURCE_DIR})
else ()
    set (PROJECT_FOLDER ${PROJECT_NAME})
endif ()

# Find Windows SDK
if (WIN32)
    if (CMAKE_SIZEOF_VOID_P EQUAL 8)
        set (BIN_ARCHITECTURE "x64")
        set (NVAPI_BIN_ARCHITECTURE "amd64")
    else ()
        set (BIN_ARCHITECTURE "x86")
        set (NVAPI_BIN_ARCHITECTURE "x86")
    endif ()

    if ("$ENV{WIN10_SDK_PATH}$ENV{WIN10_SDK_VERSION}" STREQUAL "")
        get_filename_component (WINDOWS_SDK_PATH "[HKEY_LOCAL_MACHINE\\SOFTWARE\\Microsoft\\Windows Kits\\Installed Roots;KitsRoot10]" ABSOLUTE CACHE)
        set (WINDOWS_SDK_VERSION ${CMAKE_VS_WINDOWS_TARGET_PLATFORM_VERSION})

        if ("${WINDOWS_SDK_VERSION}" STREQUAL "")
            set (WINDOWS_SDK_VERSION ${CMAKE_SYSTEM_VERSION})
            message ("Using CMAKE_SYSTEM_VERSION as WindowsSDK version.(${CMAKE_SYSTEM_VERSION})")
        endif ()
    else ()
        set (WINDOWS_SDK_PATH $ENV{WIN10_SDK_PATH})
        set (WINDOWS_SDK_VERSION $ENV{WIN10_SDK_VERSION})
    endif ()

    if ("${WINDOWS_SDK_PATH}" STREQUAL "")
        message (FATAL_ERROR "Failed to detect WindowsSDK path. Please add enviroment variables WIN10_SDK_PATH and WIN10_SDK_VERSION or use MSVC")
    endif ()

    if ("${WINDOWS_SDK_VERSION}" STREQUAL "")
        message (FATAL_ERROR "Failed to detect WindowsSDK version. Please add enviroment variables WIN10_SDK_PATH and WIN10_SDK_VERSION or use MSVC")
    endif ()

    if (IS_DIRECTORY "${WINDOWS_SDK_PATH}/Include/${WINDOWS_SDK_VERSION}.0")
        set (WINDOWS_SDK_VERSION "${WINDOWS_SDK_VERSION}.0")
    endif ()

    set (WINDOWS_SDK_LIB_PATH "${WINDOWS_SDK_PATH}/Lib/${WINDOWS_SDK_VERSION}/um/${BIN_ARCHITECTURE}")

    find_library (INPUT_LIB_D3D11 NAMES d3d11 HINTS ${WINDOWS_SDK_LIB_PATH} REQUIRED)
    find_library (INPUT_LIB_D3D12 NAMES d3d12 HINTS ${WINDOWS_SDK_LIB_PATH} REQUIRED)
    find_library (INPUT_LIB_DXGI NAMES dxgi HINTS ${WINDOWS_SDK_LIB_PATH} REQUIRED)
    find_library (INPUT_LIB_DXGUID NAMES dxguid HINTS ${WINDOWS_SDK_LIB_PATH} REQUIRED)
endif ()

# Globals?
set_property (GLOBAL PROPERTY USE_FOLDERS ON)

set (CMAKE_CXX_STANDARD 17)
set (CMAKE_CXX_STANDARD_REQUIRED ON)
set (CMAKE_CXX_VISIBILITY_PRESET hidden)
set (CMAKE_CXX_EXTENSIONS OFF)
set (CMAKE_POSITION_INDEPENDENT_CODE ON)
set (CMAKE_POSITION_INDEPENDENT_BINARIES ON)
set (CMAKE_VISIBILITY_INLINES_HIDDEN ON)

if (MSVC AND NOT DEFINED CMAKE_MSVC_RUNTIME_LIBRARY)
    set (CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
endif ()

# Compile options
if (CMAKE_CXX_COMPILER_ID MATCHES "Clang")
    set (COMPILE_OPTIONS -msse4.1 -Wextra -Wno-missing-field-initializers)
elseif (CMAKE_CXX_COMPILER_ID MATCHES "GNU")
    set (COMPILE_OPTIONS -msse4.1 -Wextra -Wno-missing-field-initializers)
elseif (MSVC)
    set (COMPILE_OPTIONS /W4 /WX /wd4324)
else ()
    message (WARNING "Unknown compiler!")
endif ()

set (COMPILE_DEFINITIONS WIN32_LEAN_AND_MEAN NOMINMAX _CRT_SECURE_NO_WARNINGS)
if (NRI_ENABLE_EXTERNAL_LIBRARIES)
    set (COMPILE_DEFINITIONS ${COMPILE_DEFINITIONS} NRI_USE_EXT_LIBS=1)
endif ()

# External libs
if (WIN32 AND NRI_ENABLE_EXTERNAL_LIBRARIES AND (NRI_ENABLE_D3D11_SUPPORT OR NRI_ENABLE_D3D12_SUPPORT))
    find_library (NVAPI_LIB NAMES nvapi64 nvapi PATHS "External/nvapi/${NVAPI_BIN_ARCHITECTURE}" REQUIRED) # statically linked
    find_library (AGS_LIB NAMES amd_ags_${BIN_ARCHITECTURE} PATHS "External/amdags/ags_lib/lib" REQUIRED) # dynamically loaded

    file (GLOB NVAPI_HEADERS "External/nvapi/*.h")
    source_group ("External/nvapi" FILES ${NVAPI_HEADERS})
    file (GLOB AMDAGS_HEADERS "External/amdags/ags_lib/inc/*.h")
    source_group ("External/amdags" FILES ${AMDAGS_HEADERS})
endif ()

# NONE
if (NRI_ENABLE_NONE_SUPPORT)
    message ("NRI adding backend: NONE")
    set (COMPILE_DEFINITIONS ${COMPILE_DEFINITIONS} NRI_USE_NONE=1)

    file (GLOB NONE_SOURCE "Source/NONE/*")
    source_group ("" FILES ${NONE_SOURCE})
    add_library (NRI_NONE STATIC ${NONE_SOURCE})
    target_include_directories (NRI_NONE PRIVATE "Include" "Source/Shared")
    target_compile_definitions (NRI_NONE PRIVATE ${COMPILE_DEFINITIONS})
    target_compile_options (NRI_NONE PRIVATE ${COMPILE_OPTIONS})
    set_property (TARGET NRI_NONE PROPERTY FOLDER ${PROJECT_FOLDER})
endif ()

# D3D11
if (WIN32 AND NRI_ENABLE_D3D11_SUPPORT)
    message ("NRI adding backend: D3D11")

    set (COMPILE_DEFINITIONS ${COMPILE_DEFINITIONS} NRI_USE_D3D11=1)
    set (INPUT_LIBS_D3D11 ${INPUT_LIB_D3D11} ${INPUT_LIB_DXGI})

    file (GLOB D3D11_SOURCE "Source/D3D11/*")
    source_group ("" FILES ${D3D11_SOURCE})

    if (NRI_ENABLE_EXTERNAL_LIBRARIES)
        add_library (NRI_D3D11 STATIC ${D3D11_SOURCE} ${NVAPI_HEADERS} ${AMDAGS_HEADERS})
        target_link_libraries (NRI_D3D11 PRIVATE ${NVAPI_LIB})
    else ()
        add_library (NRI_D3D11 STATIC ${D3D11_SOURCE})
    endif ()

    target_include_directories (NRI_D3D11 PRIVATE "Include" "Source/Shared" "External")
    target_compile_definitions (NRI_D3D11 PRIVATE ${COMPILE_DEFINITIONS})
    target_compile_options (NRI_D3D11 PRIVATE ${COMPILE_OPTIONS})
    target_link_libraries (NRI_D3D11 PRIVATE NRI_Shared ${INPUT_LIBS_D3D11} ${INPUT_LIB_DXGUID})
    set_property (TARGET NRI_D3D11 PROPERTY FOLDER ${PROJECT_FOLDER})
endif ()

# D3D12
if (WIN32 AND NRI_ENABLE_D3D12_SUPPORT)
    message ("NRI adding backend: D3D12")

    set (COMPILE_DEFINITIONS ${COMPILE_DEFINITIONS} NRI_USE_D3D12=1)
    set (INPUT_LIBS_D3D12 ${INPUT_LIB_D3D12} ${INPUT_LIB_DXGI})

    file (GLOB D3D12_SOURCE "Source/D3D12/*" "External/memalloc/D3D12*.*")
    source_group ("" FILES ${D3D12_SOURCE})

    if (NRI_ENABLE_EXTERNAL_LIBRARIES)
        add_library (NRI_D3D12 STATIC ${D3D12_SOURCE} ${NVAPI_HEADERS} ${AMDAGS_HEADERS})
        target_link_libraries (NRI_D3D12 PRIVATE ${NVAPI_LIB})
    else ()
        add_library (NRI_D3D12 STATIC ${D3D12_SOURCE})
    endif ()

    target_include_directories (NRI_D3D12 PRIVATE "Include" "Source/Shared" "External")
    target_compile_definitions (NRI_D3D12 PRIVATE ${COMPILE_DEFINITIONS})
    target_compile_options (NRI_D3D12 PRIVATE ${COMPILE_OPTIONS})
    target_link_libraries (NRI_D3D12 PRIVATE NRI_Shared ${INPUT_LIBS_D3D12})
    set_property (TARGET NRI_D3D12 PROPERTY FOLDER ${PROJECT_FOLDER})

    if (NRI_ENABLE_AGILITY_SDK_SUPPORT)
        target_include_directories (NRI_D3D12 PRIVATE "${NRI_AGILITY_SDK_PATH}/include")
        target_compile_definitions (NRI_D3D12 PRIVATE NRI_USE_AGILITY_SDK)

        # Copy Agility SDK binaries
        message (STATUS "Copying Agility SDK binaries to '${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/${NRI_AGILITY_SDK_DIR}'")
        file(COPY "${NRI_AGILITY_SDK_PATH}/bin/x64" DESTINATION "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/${NRI_AGILITY_SDK_DIR}")

        # Generate "NRIAgilitySDK.h"
        file (WRITE Include/NRIAgilitySDK.h
            "// This file is auto-generated during project deployment. Do not modify!\n"
            "#ifdef __cplusplus\n"
            "extern \"C\" {\n"
            "#endif\n\n"
            "__declspec(dllexport) extern const uint32_t D3D12SDKVersion = ${NRI_AGILITY_SDK_VERSION};\n"
            "__declspec(dllexport) extern const char* D3D12SDKPath = \"../${NRI_AGILITY_SDK_DIR}/x64/\";\n\n"
            "#ifdef __cplusplus\n"
            "}\n"
            "#endif\n"
        )
    endif ()
endif ()

# VK
if (NRI_ENABLE_VK_SUPPORT)
    message ("NRI adding backend: VK")
    set (COMPILE_DEFINITIONS ${COMPILE_DEFINITIONS} NRI_USE_VK=1)

    file (GLOB VK_SOURCE "Source/VK/*" "External/memalloc/vk_mem_alloc.h")
    source_group ("" FILES ${VK_SOURCE})
    add_library (NRI_VK STATIC ${VK_SOURCE})
    target_include_directories (NRI_VK PRIVATE "Include" "Source/Shared" "External" "External/vulkan/include")
    target_compile_definitions (NRI_VK PRIVATE ${COMPILE_DEFINITIONS})
    target_compile_options (NRI_VK PRIVATE ${COMPILE_OPTIONS})
    target_link_libraries (NRI_VK PRIVATE NRI_Shared)
    set_property (TARGET NRI_VK PROPERTY FOLDER ${PROJECT_FOLDER})

    if (WIN32)
        target_compile_definitions (NRI_VK PRIVATE VK_USE_PLATFORM_WIN32_KHR)
    elseif (APPLE)
        target_compile_definitions (NRI_VK PRIVATE VK_USE_PLATFORM_METAL_EXT VK_ENABLE_BETA_EXTENSIONS)
    else ()
        if (NRI_ENABLE_XLIB_SUPPORT)
            find_path (XLIB_HEADERS NAMES "X11/Xlib.h")

            if (NOT XLIB_HEADERS)
                message (FATAL_ERROR "Can't find 'X11/xlib.h'.(libx11-dev is not installed?)")
            endif ()

            target_compile_definitions (NRI_VK PRIVATE VK_USE_PLATFORM_XLIB_KHR)
        endif ()

        if (NRI_ENABLE_WAYLAND_SUPPORT)
            find_path (WAYLAND_HEADERS NAMES "wayland-client.h")

            if (NOT WAYLAND_HEADERS)
                message (FATAL_ERROR "Can't find 'wayland-client.h'.(libwayland-dev is not installed?)")
            endif ()

            target_compile_definitions (NRI_VK PRIVATE VK_USE_PLATFORM_WAYLAND_KHR)
        endif ()
    endif ()
endif ()

# Shared
file (GLOB SHARED_SOURCE "Source/Shared/*.cpp" "Source/Shared/*.h" "Source/Shared/*.hpp")
source_group ("" FILES ${SHARED_SOURCE})
add_library (NRI_Shared STATIC ${SHARED_SOURCE})
target_include_directories (NRI_Shared PRIVATE "Include" "Source/Shared")
target_compile_definitions (NRI_Shared PRIVATE ${COMPILE_DEFINITIONS})
target_compile_options (NRI_Shared PRIVATE ${COMPILE_OPTIONS})
set_property (TARGET NRI_Shared PROPERTY FOLDER ${PROJECT_FOLDER})

# Validation
file (GLOB NRI_VALIDATION_SOURCE "Source/Validation/*.cpp" "Source/Validation/*.h" "Source/Validation/*.hpp")
source_group ("" FILES ${NRI_VALIDATION_SOURCE})
add_library (NRI_Validation STATIC ${NRI_VALIDATION_SOURCE})
target_include_directories (NRI_Validation PRIVATE "Include" "Source/Shared")
target_compile_definitions (NRI_Validation PRIVATE ${COMPILE_DEFINITIONS})
target_compile_options (NRI_Validation PRIVATE ${COMPILE_OPTIONS})
target_link_libraries (NRI_Validation PRIVATE NRI_Shared)
set_property (TARGET NRI_Validation PROPERTY FOLDER ${PROJECT_FOLDER})

# NRI
file (GLOB NRI_HEADERS "Include/*.h" "Include/*.hpp")
source_group ("Include" FILES ${NRI_HEADERS})

file (GLOB NRI_EXTENSIONS "Include/Extensions/*.h" "Include/Extensions/*.hpp")
source_group ("Include/Extensions" FILES ${NRI_EXTENSIONS})

file (GLOB NRI_SOURCE "Source/Creation/*.cpp" "Source/Creation/*.h")
source_group ("Sources" FILES ${NRI_SOURCE})

file (GLOB NRI_RESOURCES "Resources/*")
source_group ("Resources" FILES ${NRI_RESOURCES})

if (NRI_STATIC_LIBRARY)
    set (COMPILE_DEFINITIONS ${COMPILE_DEFINITIONS} NRI_STATIC_LIBRARY)
    add_library (${PROJECT_NAME} STATIC ${NRI_SOURCE} ${NRI_HEADERS} ${NRI_RESOURCES} ${NRI_EXTENSIONS})
else ()
    add_library (${PROJECT_NAME} SHARED ${NRI_SOURCE} ${NRI_HEADERS} ${NRI_RESOURCES} ${NRI_EXTENSIONS})

    if (WIN32)
        target_compile_definitions (${PROJECT_NAME} PRIVATE "NRI_API=extern \"C\" __declspec(dllexport)")
    else ()
        target_compile_definitions (${PROJECT_NAME} PRIVATE "NRI_API=extern \"C\" __attribute__((visibility(\"default\")))")
    endif ()
endif ()

target_include_directories (${PROJECT_NAME} PRIVATE "Include" "Source/Shared" "External/vulkan/include")
target_compile_definitions (${PROJECT_NAME} PRIVATE ${COMPILE_DEFINITIONS})
target_compile_options (${PROJECT_NAME} PRIVATE ${COMPILE_OPTIONS})

target_link_libraries (${PROJECT_NAME} PRIVATE NRI_Shared NRI_Validation)
if (WIN32)
    target_link_libraries (${PROJECT_NAME} PRIVATE ${INPUT_LIB_DXGI} ${INPUT_LIB_DXGUID}) # for nriReportLiveObjects
else ()
    target_link_libraries (${PROJECT_NAME} PRIVATE ${CMAKE_DL_LIBS})
endif ()

if (NRI_ENABLE_NONE_SUPPORT)
    target_link_libraries (${PROJECT_NAME} PRIVATE NRI_NONE)
endif ()
if (WIN32 AND NRI_ENABLE_D3D11_SUPPORT)
    target_link_libraries (${PROJECT_NAME} PRIVATE NRI_D3D11)
endif ()
if (WIN32 AND NRI_ENABLE_D3D12_SUPPORT)
    target_link_libraries (${PROJECT_NAME} PRIVATE NRI_D3D12)
endif ()
if (NRI_ENABLE_VK_SUPPORT)
    target_link_libraries (${PROJECT_NAME} PRIVATE NRI_VK)
endif ()

set_property (TARGET ${PROJECT_NAME} PROPERTY FOLDER ${PROJECT_FOLDER})

set_target_properties (${PROJECT_NAME} PROPERTIES ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}")
message ("NRI output path: '${CMAKE_RUNTIME_OUTPUT_DIRECTORY}'")

if (WIN32)
    if (NRI_ENABLE_D3D11_SUPPORT OR NRI_ENABLE_D3D12_SUPPORT)
        # Function - copy a library to the output folder of a project
        function (copy_library PROJECT LIBRARY_NAME)
            add_custom_command (TARGET ${PROJECT} POST_BUILD
                COMMAND ${CMAKE_COMMAND} -E copy_if_different ${LIBRARY_NAME} $<TARGET_FILE_DIR:${PROJECT}>
                COMMAND_EXPAND_LISTS)
        endfunction ()

        # Copy AMD AGS into the output folder
        find_file (AMD_AGS_DLL NAMES amd_ags_x64.dll PATHS "External/amdags/ags_lib/lib")
        copy_library (${PROJECT_NAME} ${AMD_AGS_DLL})
    endif ()
endif ()
