##########################################################################################
cmake_minimum_required(VERSION 3.5)

project(vstgui-scintilla)

##########################################################################################
if(CMAKE_CONFIGURATION_TYPES)
	set(CMAKE_CONFIGURATION_TYPES Debug Release)
	set_property(GLOBAL PROPERTY USE_FOLDERS ON)
endif()

set(VSTGUI_DISABLE_STANDALONE_EXAMPLES 1)
set(VSTGUI_DISABLE_UNITTESTS 1)

add_subdirectory("vstgui/vstgui/")

set(target scintilla-example)
set(${target}_sources
	"source/app.cpp"
	"source/scintillaeditorview.cpp"
	"source/scintillaeditorview.h"
)

if(CMAKE_HOST_APPLE)
	get_filename_component(InfoPlistFile "resource/Info.plist" ABSOLUTE)
	set(APP_PROPERTIES
		MACOSX_BUNDLE TRUE
		MACOSX_BUNDLE_INFO_PLIST ${InfoPlistFile}
		OUTPUT_NAME "scintilla-example"
	)
    set(${target}_resources
		"resource/Editor.uidesc"
	)
    set_source_files_properties("resource/Editor.uidesc" PROPERTIES
      MACOSX_PACKAGE_LOCATION "Resources"
    )
	set(BUNDLE_PKG_INFO
		"resource/PkgInfo"
	)
	set_source_files_properties(${BUNDLE_PKG_INFO} PROPERTIES
		MACOSX_PACKAGE_LOCATION "."
	)
    set(${target}_resources
		${${target}_resources}
		${BUNDLE_PKG_INFO}
	)

    set(${target}_sources
		${${target}_sources}
		"source/scintillaeditorview_mac.mm"
		"scintilla/cocoa/ScintillaFramework/ScintillaFramework.xcodeproj"
	)

	set(${target}_PLATFORM_LIBS
		"-framework Cocoa"
		"-framework OpenGL"
		"-framework QuartzCore"
		"-framework Accelerate"
		"-framework Scintilla"
	)

	set(ScintillaFramework 
#		${CMAKE_CURRENT_SOURCE_DIR}/scintilla/cocoa/ScintillaFramework/build/Debug/Scintilla.framework
		${CMAKE_CURRENT_SOURCE_DIR}/scintilla/cocoa/ScintillaFramework/build/Release//Scintilla.framework
	)
	set_source_files_properties(${ScintillaFramework} PROPERTIES
		MACOSX_PACKAGE_LOCATION "Frameworks"
	)

    set(${target}_resources
		${${target}_resources}
		${ScintillaFramework}
	)

	set(APP_PROPERTIES ${APP_PROPERTIES}
		XCODE_ATTRIBUTE_FRAMEWORK_SEARCH_PATHS
		$<$<CONFIG:Debug>:${CMAKE_CURRENT_SOURCE_DIR}/scintilla/cocoa/ScintillaFramework/build/Debug/>$<$<CONFIG:Release>:${CMAKE_CURRENT_SOURCE_DIR}/scintilla/cocoa/ScintillaFramework/build/Release/>$<$<CONFIG:ReleaseLTO>:${CMAKE_CURRENT_SOURCE_DIR}/scintilla/cocoa/ScintillaFramework/build/Release/>
	)

endif(CMAKE_HOST_APPLE)

##########################################################################################
if(MSVC)
    set(${target}_sources
		${${target}_sources}
		"source/scintillaeditorview_win32.cpp"
	)

	set(${target}_resources
		${${target}_resources}
		"resource/windows.rc"
	)
	set(APP_PROPERTIES
		LINK_FLAGS "/SUBSYSTEM:windows /INCLUDE:wWinMain"
	)
    set(${target}_resources
		${${target}_resources}
#		"scintilla/bin/Scintilla.dll"
	)
	set(${target}_PLATFORM_LIBS
#		"scintilla/bin/Scintilla"
	)
endif()

add_executable(${target}
	${${target}_sources}
	${${target}_resources}
)

target_link_libraries(${target}
	vstgui
	vstgui_uidescription
	vstgui_standalone
	${${target}_PLATFORM_LIBS}
)
target_compile_definitions(${target} ${VSTGUI_COMPILE_DEFINITIONS})
target_include_directories(${target} PRIVATE
	"vstgui"
	"scintilla/include"
)
set_target_properties(${target} PROPERTIES
	${APP_PROPERTIES}
	${VSTGUI_STANDALONE_EXAMPLES}
)

vstgui_set_cxx_version(${target} 17)

