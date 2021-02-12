# The MIT License (MIT)
#
# Copyright (c) 2017 Nathan Osman, Copyright (c) 2021 Chris Hennes
#
# Permission is hereby granted, free of charge, to any person obtaining a copy
# of this software and associated documentation files (the "Software"), to deal
# in the Software without restriction, including without limitation the rights
# to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
# copies of the Software, and to permit persons to whom the Software is
# furnished to do so, subject to the following conditions:
#
# The above copyright notice and this permission notice shall be included in
# all copies or substantial portions of the Software.
#
# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
# IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
# FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
# AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
# LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
# OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
# SOFTWARE.

find_package(Qt5Core REQUIRED)

# Retrieve the absolute path to qmake and then use that path to find
# the windeployqt binary
set(_qt_bin_dir "${_qt5_root_dir}/bin")
find_program(WINDEPLOYQT_EXECUTABLE windeployqt HINTS "${_qt_bin_dir}" REQUIRED)

# Add commands that copy the Qt runtime to the target's output directory after
# build and install the Qt runtime
function(windeployqt target)

    message(STATUS "Running windeployqt for target ${target}")

    # Run windeployqt immediately after build
    add_custom_command(TARGET ${target} POST_BUILD
        COMMAND "${CMAKE_COMMAND}" -E
            env "${WINDEPLOYQT_EXECUTABLE}"
                --verbose 0
                --no-compiler-runtime
                --no-angle
                --no-opengl-sw
                \"$<TARGET_FILE:${target}>\"
    )

    cmake_policy(SET CMP0087 NEW)

    # Before installation, run a series of commands that copy each of the Qt
    # runtime files to the appropriate directory for installation
    install(CODE
        "
        message(STATUS \"Running ${WINDEPLOYQT_EXECUTABLE} on $<TARGET_FILE:${target}>\")
        execute_process(
            COMMAND \"${CMAKE_COMMAND}\" -E
                env \"${WINDEPLOYQT_EXECUTABLE}\"
                    --dry-run
                    --no-compiler-runtime
                    --no-angle
                    --no-opengl-sw
                    --list mapping
                    \"$<TARGET_FILE:${target}>\"
            OUTPUT_VARIABLE _output
            ERROR_VARIABLE _error
            OUTPUT_STRIP_TRAILING_WHITESPACE
        )
        if(NOT _error STREQUAL \"\")
            message(ERROR \"Error from windeployqt: ${_error}\")
        endif()
        if(_output STREQUAL \"\")
            message(ERROR \"No output from windeployqt!\")
        endif()
        separate_arguments(_files WINDOWS_COMMAND \${_output})
        while(_files)
            list(GET _files 0 _src)
            list(GET _files 1 _dest)
            execute_process(
                COMMAND \"${CMAKE_COMMAND}\" -E
                    copy \${_src} \"\${CMAKE_INSTALL_PREFIX}/bin/\${_dest}\"
            )
            list(REMOVE_AT _files 0 1)
        endwhile()
        "
    )

    # windeployqt doesn't work correctly with the system runtime libraries,
    # so we fall back to one of CMake's own modules for copying them over
    set(CMAKE_INSTALL_UCRT_LIBRARIES TRUE)
    include(InstallRequiredSystemLibraries)
    foreach(lib ${CMAKE_INSTALL_SYSTEM_RUNTIME_LIBS})
        get_filename_component(filename "${lib}" NAME)
        add_custom_command(TARGET ${target} POST_BUILD
            COMMAND "${CMAKE_COMMAND}" -E
                copy_if_different "${lib}" \"$<TARGET_FILE_DIR:${target}>\"
        )
    endforeach()

endfunction()
