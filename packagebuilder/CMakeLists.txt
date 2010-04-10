#
# MOPSLinux packaging system
#
#
cmake_minimum_required(VERSION 2.6)
    if(COMMAND cmake_policy)
      cmake_policy(SET CMP0003 NEW)
      cmake_policy(SET CMP0005 NEW)
    endif(COMMAND cmake_policy)
# Project name
PROJECT(MPKGPACKAGEBUILDER)

# Enabling building of shared libs
SET(BUILD_SHARED_LIBS ON)
# If you want to build a standalone server app, set SERVERAPP to ON
SET(SERVERAPP OFF)
SET(MAXSTATIC OFF)
# Check 64 bit
if (CMAKE_SIZEOF_VOID_P EQUAL 4)
	SET(X86_64 OFF)
else (CMAKE_SIZEOF_VOID_P EQUAL 4)
	SET(X86_64 ON)
	ADD_DEFINITIONS(-DX86_64)
endif(CMAKE_SIZEOF_VOID_P EQUAL 4)

ADD_DEFINITIONS(-DINSTALL_PREFIX="${CMAKE_INSTALL_PREFIX}")
ADD_DEFINITIONS(-D_FILE_OFFSET_BITS=64 -D_LARGEFILE_SOURCE -DLARGEFILE64_SOURCE -DDL_CLEANUP -I. -I/usr/include/libxml2 -Wall -W -DNEW_INSTALL_SYSTEM -DUSE_SYSTEM_SED -DDEP_NORMALIZE -DLINUX_DEFINED)

ADD_DEFINITIONS(${CXXFLAGS})

# Main part
# c++ sources
SET(packagebuilder_SRC
	main.cpp
	mainwindow.cpp
)
# headers for MOC
SET(packagebuilder_MOC_HDR
	mainwindow.h
)

# UI
SET(packagebuilder_UI
	package_builder.ui
)

FIND_PACKAGE(Qt4 REQUIRED)
INCLUDE( ${QT_USE_FILE} )
QT4_WRAP_UI(packagebuilder_UI_HDR ${packagebuilder_UI})
QT4_WRAP_CPP(packagebuilder_MOC_SRC ${packagebuilder_MOC_HDR})
INCLUDE_DIRECTORIES(${CMAKE_BINARY_DIR})
ADD_EXECUTABLE(packagebuilder ${packagebuilder_SRC} ${packagebuilder_MOC_SRC} ${packagebuilder_UI_HDR})
TARGET_LINK_LIBRARIES(packagebuilder ${QT_LIBRARIES} mpkg)

# Installation
 
SET(SYSCONFDIR "/etc")

FILE(GLOB appicon128 "${CMAKE_CURRENT_SOURCE_DIR}/appicons/128/mpkg-packagebuilder.png")
FILE(GLOB appicon16 "${CMAKE_CURRENT_SOURCE_DIR}/appicons/16/mpkg-packagebuilder.png")
FILE(GLOB appicon22 "${CMAKE_CURRENT_SOURCE_DIR}/appicons/22/mpkg-packagebuilder.png")
FILE(GLOB appicon24 "${CMAKE_CURRENT_SOURCE_DIR}/appicons/24/mpkg-packagebuilder.png")
FILE(GLOB appicon32 "${CMAKE_CURRENT_SOURCE_DIR}/appicons/32/mpkg-packagebuilder.png")
FILE(GLOB appicon48 "${CMAKE_CURRENT_SOURCE_DIR}/appicons/48/mpkg-packagebuilder.png")
FILE(GLOB appicon72 "${CMAKE_CURRENT_SOURCE_DIR}/appicons/72/mpkg-packagebuilder.png")


FILE(GLOB files "${CMAKE_CURRENT_SOURCE_DIR}/*.qm")
FILE(GLOB profiles "${CMAKE_CURRENT_SOURCE_DIR}/profiles/*.profile")
FILE(GLOB icons "${CMAKE_CURRENT_SOURCE_DIR}/icons/*.png")
INSTALL(TARGETS packagebuilder
		RUNTIME DESTINATION bin
)
INSTALL(FILES ${files} DESTINATION share/mpkg)
INSTALL(FILES ${icons} DESTINATION share/mpkg/icons)
INSTALL(FILES ${profiles} DESTINATION ${SYSCONFDIR}/skel/.packagebuilder/configure_templates)
INSTALL(FILES "${CMAKE_CURRENT_SOURCE_DIR}/PackageBuilder.desktop" DESTINATION share/applications)

INSTALL(FILES ${appicon128} DESTINATION share/icons/hicolor/128x128/apps/)
INSTALL(FILES ${appicon16} DESTINATION share/icons/hicolor/16x16/apps/)
INSTALL(FILES ${appicon22} DESTINATION share/icons/hicolor/22x22/apps/)
INSTALL(FILES ${appicon24} DESTINATION share/icons/hicolor/24x24/apps/)
INSTALL(FILES ${appicon32} DESTINATION share/icons/hicolor/32x32/apps/)
INSTALL(FILES ${appicon48} DESTINATION share/icons/hicolor/48x48/apps/)
INSTALL(FILES ${appicon72} DESTINATION share/icons/hicolor/72x72/apps/)

