cmake_minimum_required(VERSION 3.20)
project(TetrahedronTetractys LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(SFML_ROOT "" CACHE PATH "Path to SFML installation")
if(SFML_ROOT)
    set(CMAKE_PREFIX_PATH ${CMAKE_PREFIX_PATH} ${SFML_ROOT})
endif()

find_package(SFML 3 CONFIG COMPONENTS Graphics Window System REQUIRED)

if(NOT SFML_FOUND)
    message(FATAL_ERROR
        "SFML not found!\n"
        "Set SFML_ROOT or run: cmake .. -DSFML_ROOT=C:/SFML\n"
        "https://www.sfml-dev.org/download.php"
    )
endif()

find_package(OpenGL QUIET)
if(NOT OPENGL_FOUND)
    find_path(OPENGL_INCLUDE_DIR GL/gl.h
        PATHS "C:/Program Files/Microsoft SDKs/Windows/*/Include"
    )
    if(WIN32)
        find_library(OPENGL_LIBRARY opengl32)
        if(OPENGL_LIBRARY)
            set(OPENGL_LIBRARIES ${OPENGL_LIBRARY})
        endif()
    endif()
    if(OPENGL_INCLUDE_DIR)
        set(OPENGL_INCLUDE_DIRS ${OPENGL_INCLUDE_DIR})
    endif()
endif()

if(NOT OPENGL_LIBRARIES)
    message(WARNING "OpenGL not found; linking may fail.")
    set(OPENGL_LIBRARIES "")
endif()

# Executable (output binary: run)
add_executable(run
    main.cpp
    window_shape.cpp
    tetrahedron_mesh.cpp
    turn_geometry.cpp
    pyraminx_state.cpp
    renderer.cpp
)

if(SFML_INCLUDE_DIRS)
    target_include_directories(run PRIVATE ${SFML_INCLUDE_DIRS})
endif()
if(OPENGL_INCLUDE_DIRS)
    target_include_directories(run PRIVATE ${OPENGL_INCLUDE_DIRS})
endif()

if(TARGET SFML::System)
    target_link_libraries(run
        SFML::System
        SFML::Window
        SFML::Graphics
    )
elseif(SFML_LIBRARIES)
    target_link_libraries(run ${SFML_LIBRARIES})
endif()

if(OPENGL_LIBRARIES)
    target_link_libraries(run ${OPENGL_LIBRARIES})
endif()

if(WIN32)
    target_link_libraries(run glu32 gdi32)
endif()

# MSVC Release: discard unreferenced COMDATs and fold identical functions (smaller .exe).
if(MSVC)
    target_link_options(run PRIVATE $<$<CONFIG:Release>:/OPT:REF /OPT:ICF /INCREMENTAL:NO>)
endif()

if(WIN32)
    set_target_properties(run PROPERTIES WIN32_EXECUTABLE FALSE)
endif()

# Windows: copy SFML runtime DLLs next to run.exe so the app does not fail with STATUS_DLL_NOT_FOUND (0xC0000135).
if(WIN32)
    set(_sfml_bin "")
    if(SFML_ROOT)
        set(_sfml_bin "${SFML_ROOT}/bin")
    elseif(DEFINED SFML_DIR)
        get_filename_component(_sfml_root "${SFML_DIR}/../.." ABSOLUTE)
        set(_sfml_bin "${_sfml_root}/bin")
    endif()
    if(_sfml_bin AND EXISTS "${_sfml_bin}")
        # Only Graphics, Window, System — omit Audio/Network (app does not link them).
        # Copy Release vs Debug DLLs per CONFIG (GLOB would copy both and bloat the output folder).
        set(_sfml_rel sfml-graphics-3.dll sfml-window-3.dll sfml-system-3.dll)
        set(_sfml_dbg sfml-graphics-d-3.dll sfml-window-d-3.dll sfml-system-d-3.dll)
        foreach(_name ${_sfml_rel})
            add_custom_command(TARGET run POST_BUILD
                COMMAND ${CMAKE_COMMAND} -E copy_if_different
                    "${_sfml_bin}/${_name}"
                    "$<TARGET_FILE_DIR:run>/${_name}"
                COMMENT "Copy ${_name} next to run.exe"
                CONFIG Release RelWithDebInfo MinSizeRel)
        endforeach()
        foreach(_name ${_sfml_dbg})
            add_custom_command(TARGET run POST_BUILD
                COMMAND ${CMAKE_COMMAND} -E copy_if_different
                    "${_sfml_bin}/${_name}"
                    "$<TARGET_FILE_DIR:run>/${_name}"
                COMMENT "Copy Debug ${_name} next to run.exe"
                CONFIG Debug)
        endforeach()
        # Drop stale Debug SFML DLLs when building Release (older GLOB or mixed configs left them behind).
        add_custom_command(TARGET run POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E rm -f
                "$<TARGET_FILE_DIR:run>/sfml-graphics-d-3.dll"
                "$<TARGET_FILE_DIR:run>/sfml-window-d-3.dll"
                "$<TARGET_FILE_DIR:run>/sfml-system-d-3.dll"
            COMMENT "Remove Debug SFML DLLs from Release output"
            CONFIG Release RelWithDebInfo MinSizeRel)
    endif()
endif()
