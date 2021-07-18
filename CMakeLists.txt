##########################################################################################
cmake_minimum_required(VERSION 3.5)

project(vstgui-scintilla)

##########################################################################################
if(CMAKE_CONFIGURATION_TYPES)
  set(CMAKE_CONFIGURATION_TYPES Debug Release)
  set_property(GLOBAL PROPERTY USE_FOLDERS ON)
endif()

if(NOT VSTGUI_PATH)
	set(VSTGUI_PATH "../vstgui")
endif()
if(NOT SCINTILLA_PATH)
	set(SCINTILLA_PATH "../external/scintilla")
endif()
if(NOT LEXILLA_PATH)
	set(LEXILLA_PATH "../external/lexilla")
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

if(CMAKE_HOST_APPLE)
  # get_filename_component(InfoPlistFile "resource/Info.plist" ABSOLUTE)
  # set(APP_PROPERTIES
  #   MACOSX_BUNDLE TRUE
  #   MACOSX_BUNDLE_INFO_PLIST ${InfoPlistFile}
  #   OUTPUT_NAME "scintilla-example"
  # )
  # set_source_files_properties("resource/Editor.uidesc" PROPERTIES
  #     MACOSX_PACKAGE_LOCATION "Resources"
  #   )
  # set(BUNDLE_PKG_INFO
  #   "resource/PkgInfo"
  # )
  # set_source_files_properties(${BUNDLE_PKG_INFO} PROPERTIES
  #   MACOSX_PACKAGE_LOCATION "."
  # )
  #   set(${target}_resources
  #   ${${target}_resources}
  #   ${BUNDLE_PKG_INFO}
  # )

  set(${target}_sources
    ${${target}_sources}
    "source/scintillaeditorview_mac.mm"
    "${SCINTILLA_PATH}/cocoa/Scintilla/Scintilla.xcodeproj"
    "${LEXILLA_PATH}/src/Lexilla/Lexilla.xcodeproj"
  )

  set_source_files_properties("source/scintillaeditorview_mac.mm" PROPERTIES
      COMPILE_FLAGS "-fobjc-arc"
  )

  set(ScintillaFramework 
   ${SCINTILLA_PATH}/cocoa/Scintilla/build/Debug/Scintilla.framework
#    ${SCINTILLA_PATH}/cocoa/Scintilla/build/Release/Scintilla.framework
  )
  set(LexillaDyLib
	"${LEXILLA_PATH}/src/Lexilla/build/Debug/liblexilla.dylib"
  )
  set(${target}_resources
    ${${target}_resources}
    ${ScintillaFramework}
    ${LexillaDyLib}
  )

endif(CMAKE_HOST_APPLE)

##########################################################################################
if(MSVC)
    set(${target}_sources
    ${${target}_sources}
    "source/scintillaeditorview_win32.cpp"
  )
#   set(${target}_resources
#     ${${target}_resources}
#     "resource/windows.rc"
#   )
#   set(APP_PROPERTIES
#     LINK_FLAGS "/SUBSYSTEM:windows /INCLUDE:wWinMain"
#   )
#     set(${target}_resources
#     ${${target}_resources}
# #    "scintilla/bin/Scintilla.dll"
#   )
#   set(${target}_PLATFORM_LIBS
# #    "scintilla/bin/Scintilla"
#   )
endif()

vstgui_add_executable(${target}
  "${${target}_sources}"
)
vstgui_add_resources(${target}
  "${${target}_resources}"
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
  set_source_files_properties(${ScintillaFramework} PROPERTIES
    MACOSX_PACKAGE_LOCATION "Frameworks"
  )
  set_source_files_properties(${LexillaDyLib} PROPERTIES
    MACOSX_PACKAGE_LOCATION "Frameworks"
  )
  set_target_properties(${target} PROPERTIES
    XCODE_ATTRIBUTE_FRAMEWORK_SEARCH_PATHS
    $<$<CONFIG:Debug>:${SCINTILLA_PATH}/cocoa/Scintilla/build/Debug/>$<$<CONFIG:Release>:${SCINTILLA_PATH}/cocoa/Scintilla/build/Release/>$<$<CONFIG:ReleaseLTO>:${SCINTILLA_PATH}/cocoa/Scintilla/build/Release/>
    XCODE_ATTRIBUTE_LIBRARY_SEARCH_PATHS
    $<$<CONFIG:Debug>:${LEXILLA_PATH}/src/Lexilla/build/Debug/>$<$<CONFIG:Release>:${LEXILLA_PATH}/src/Lexilla/build/Release/>$<$<CONFIG:ReleaseLTO>:${LEXILLA_PATH}/src/Lexilla/build/Release/>
	XCODE_ATTRIBUTE_LD_RUNPATH_SEARCH_PATHS
	@executable_path/../Frameworks
  )
  target_link_libraries(${target} "-framework Scintilla" "lexilla")

endif(CMAKE_HOST_APPLE)
