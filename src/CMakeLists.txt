# MIT License
#
# Copyright(c) 2021 Chris Hennes <chennes@pioneerlibrarysystem.org>
#
# Permission is hereby granted, free of charge, to any person obtaining a copy
# of this software and associated documentation files (the "Software"), to deal
# in the Software without restriction, including without limitation the rights
# to use, copy, modify, merge, publish, distribute, sublicense, and /or sell
# copies of the Software, and to permit persons to whom the Software is
# furnished to do so, subject to the following conditions:
#
# The above copyright notice and this permission notice shall be included in all
# copies or substantial portions of the Software.
#
# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
# IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
# FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
# AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
# LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
# OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
# SOFTWARE.

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

find_package(Qt5 COMPONENTS Widgets REQUIRED)

set(UI_SRCS
    "CleanSARIF.cpp" 
    "CleanSARIF.h" 
    "CleanSARIF.qrc"
    "MainWindow.ui" 
    "MainWindow.h" 
    "MainWindow.cpp"
    "NewFileFilter.ui"
    "NewFileFilter.h"
    "NewFileFilter.cpp"
    "NewRuleSuppression.ui"
    "NewRuleSuppression.h"
    "NewRuleSuppression.cpp"
    "LoadingSARIF.ui"
    "LoadingSARIF.h"
    "LoadingSARIF.cpp"
    "QProgressIndicator.h"
    "QProgressIndicator.cpp"
)

set(CPP_SRCS
    "Cleaner.h"
    "Cleaner.cpp"
    "SARIF.h"
    "SARIF.cpp"
)

set(APP_ICON_RESOURCE_WINDOWS "${CMAKE_CURRENT_SOURCE_DIR}/CleanSARIF.rc")
add_executable (CleanSARIF WIN32 MACOSX_BUNDLE
    ${UI_SRCS}
    ${CPP_SRCS}
    "ui_icon.svg"
    ${APP_ICON_RESOURCE_WINDOWS}
)

target_link_libraries(CleanSARIF Qt5::Widgets)
target_include_directories(CleanSARIF PUBLIC
                           "${PROJECT_BINARY_DIR}"
                           )

install(TARGETS CleanSARIF DESTINATION bin)

if (MSVC)
    # Visual Studio will not populate the __cplusplus macro unless told to do so...
    target_compile_options(CleanSARIF PUBLIC "/Zc:__cplusplus")
endif()

if(WIN32)
    include(Windeployqt)
    windeployqt(CleanSARIF)
endif()

if(BUILD_TESTING)
  add_subdirectory(tests)
endif()

option(BUILD_DOCUMENTATION "Build the Doxygen documentation for the source code." OFF)
if(BUILD_DOCUMENTATION)
    find_package(Doxygen
                 REQUIRED dot
                 OPTIONAL_COMPONENTS mscgen dia)
    doxygen_add_docs(Documentation)
endif()