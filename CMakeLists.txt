##########################################################################################
cmake_minimum_required(VERSION 3.19)

project(vstgui-scintilla)

include(ExternalProject)

##########################################################################################
if(CMAKE_CONFIGURATION_TYPES)
  set(CMAKE_CONFIGURATION_TYPES Debug Release)
  set_property(GLOBAL PROPERTY USE_FOLDERS ON)
endif()

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY
	$<$<CONFIG:Debug>:${CMAKE_BINARY_DIR}/Debug/>$<$<CONFIG:Release>:${CMAKE_BINARY_DIR}/Release/>$<$<CONFIG:ReleaseLTO>:${CMAKE_BINARY_DIR}/ReleaseLTO/>
)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY
	$<$<CONFIG:Debug>:${CMAKE_BINARY_DIR}/Debug/libs/>$<$<CONFIG:Release>:${CMAKE_BINARY_DIR}/Release/libs/>$<$<CONFIG:ReleaseLTO>:${CMAKE_BINARY_DIR}/ReleaseLTO/libs/>
)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY
	$<$<CONFIG:Debug>:${CMAKE_BINARY_DIR}/Debug/libs>$<$<CONFIG:Release>:${CMAKE_BINARY_DIR}/Release/libs>$<$<CONFIG:ReleaseLTO>:${CMAKE_BINARY_DIR}/ReleaseLTO/libs>
)

if(NOT VSTGUI_PATH)
	set(VSTGUI_PATH "../vstgui")
endif()
if(NOT SCINTILLA_PATH)
	set(SCINTILLA_PATH "${CMAKE_CURRENT_LIST_DIR}/../external/scintilla" CACHE PATH "Path to scintilla")
endif()
if(NOT LEXILLA_PATH)
	set(LEXILLA_PATH "${CMAKE_CURRENT_LIST_DIR}/../external/lexilla" CACHE PATH "Path to lexilla")
endif()

set(VSTGUI_STANDALONE_EXAMPLES 0)
set(VSTGUI_TOOLS 0)
set(VSTGUI_DISABLE_UNITTESTS 1)

add_subdirectory(${VSTGUI_PATH} vstgui-build)

set(target scintilla-example)
set(${target}_sources
  "source/app.cpp"
  "source/scintillaeditorview.cpp"
  "source/scintillaeditorview.h"
)

set(${target}_resources
  "resource/Editor.uidesc"
)

##########################################################################################
if(CMAKE_HOST_APPLE)
  set(${target}_sources
    ${${target}_sources}
    "source/scintillaeditorview_mac.mm"
  )

  set_source_files_properties("source/scintillaeditorview_mac.mm" PROPERTIES
      COMPILE_FLAGS "-fobjc-arc"
  )

  set(${target}_resources
    ${${target}_resources}
  )

#    "${SCINTILLA_PATH}/cocoa/Scintilla/Scintilla.xcodeproj"
#    "${LEXILLA_PATH}/src/Lexilla/Lexilla.xcodeproj"

endif(CMAKE_HOST_APPLE)

##########################################################################################
if(MSVC)
    set(${target}_sources
    ${${target}_sources}
    "source/scintillaeditorview_win32.cpp"
  )
endif()

vstgui_add_executable(${target}
  "${${target}_sources}"
)
vstgui_add_resources(${target}
  "${${target}_resources}"
)

vstgui_add_resources(${target}
  "resource/fonts/Hack-Regular.ttf" "fonts"
)

vstgui_set_target_infoplist(${target} resource/Info.plist)
vstgui_set_target_rcfile(${target} resource/windows.rc)

target_include_directories(${target} PRIVATE
  "${VSTGUI_PATH}"
  "${SCINTILLA_PATH}/include"
  "${LEXILLA_PATH}/access"
  "${LEXILLA_PATH}/include"
)

vstgui_set_cxx_version(${target} 17)

if(CMAKE_HOST_APPLE)
  ExternalProject_Add(Scintilla
    SOURCE_DIR "${SCINTILLA_PATH}"
    CONFIGURE_COMMAND ""
    BUILD_COMMAND xcodebuild -project "${SCINTILLA_PATH}/cocoa/Scintilla/Scintilla.xcodeproj" -configuration Release
    BUILD_IN_SOURCE TRUE
    INSTALL_COMMAND ""
  )
  ExternalProject_Add(Lexilla
    SOURCE_DIR "${LEXILLA_PATH}"
    CONFIGURE_COMMAND ""
    BUILD_COMMAND xcodebuild -project "${LEXILLA_PATH}/src/Lexilla/Lexilla.xcodeproj" -configuration Release
    BUILD_IN_SOURCE TRUE
    INSTALL_COMMAND ""
  )
  set_source_files_properties(
    ${SCINTILLA_PATH}/cocoa/Scintilla/build/Release/Scintilla.framework
    ${LEXILLA_PATH}/src/Lexilla/build/Release/liblexilla.dylib
    PROPERTIES 
      GENERATED TRUE 
      MACOSX_PACKAGE_LOCATION "Frameworks"
  )
  target_sources(${target} PRIVATE
    ${LEXILLA_PATH}/src/Lexilla/build/Release/liblexilla.dylib
  )
  set_target_properties(${target} PROPERTIES
    XCODE_ATTRIBUTE_FRAMEWORK_SEARCH_PATHS
    ${SCINTILLA_PATH}/cocoa/Scintilla/build/Release/
    XCODE_ATTRIBUTE_LIBRARY_SEARCH_PATHS
    ${LEXILLA_PATH}/src/Lexilla/build/Release/
	XCODE_ATTRIBUTE_LD_RUNPATH_SEARCH_PATHS
	@executable_path/../Frameworks
    XCODE_EMBED_FRAMEWORKS
    ${SCINTILLA_PATH}/cocoa/Scintilla/build/Release/Scintilla.framework
    XCODE_EMBED_FRAMEWORKS_CODE_SIGN_ON_COPY
    YES
  )
  
  target_link_libraries(${target} "-framework Scintilla" "lexilla")

endif(CMAKE_HOST_APPLE)

if(MSVC)
  ExternalProject_Add(Scintilla
    SOURCE_DIR ${SCINTILLA_PATH}/win32/
    CONFIGURE_COMMAND ""
    BUILD_COMMAND nmake DEBUG=1 -b -f ${SCINTILLA_PATH}/win32/scintilla.mak
    BUILD_IN_SOURCE TRUE
    INSTALL_COMMAND ""
  )
  ExternalProject_Add(Lexilla
    SOURCE_DIR ${LEXILLA_PATH}/src/
    CONFIGURE_COMMAND ""
    BUILD_COMMAND nmake DEBUG=1 -b -f ${LEXILLA_PATH}/src/lexilla.mak
    BUILD_IN_SOURCE TRUE
    INSTALL_COMMAND ""
  )
  set(WIN32_DLLS
    ${SCINTILLA_PATH}/bin/Scintilla.dll
    ${LEXILLA_PATH}/bin/lexilla.dll
  )
  set_source_files_properties(
  	${WIN32_DLLS}
    PROPERTIES 
      GENERATED TRUE 
  )
  vstgui_add_resources(${target}
  	"${WIN32_DLLS}"
    "libs"
  )
endif(MSVC)

add_dependencies(${target} Scintilla Lexilla)