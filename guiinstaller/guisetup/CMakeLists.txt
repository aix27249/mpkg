#
# MPKG packaging system
#
#
cmake_minimum_required(VERSION 2.6)
    if(COMMAND cmake_policy)
      cmake_policy(SET CMP0003 NEW)
      cmake_policy(SET CMP0005 NEW)
    endif(COMMAND cmake_policy)
# Project name
PROJECT(GUISETUP)

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

ADD_DEFINITIONS(-D_FILE_OFFSET_BITS=64 -D_LARGEFILE_SOURCE -DLARGEFILE64_SOURCE -DDL_CLEANUP -I. -I/usr/include/libxml2 -Wall -W -DNEW_INSTALL_SYSTEM -DUSE_SYSTEM_SED -DDEP_NORMALIZE -DLINUX_DEFINED)
ADD_DEFINITIONS(-DINSTALL_PREFIX="${CMAKE_INSTALL_PREFIX}")

ADD_DEFINITIONS(${CXXFLAGS})
# Main part
# c++ sources
SET(guisetup_SRC
	main.cpp
	mainwindow.cpp
	help.cpp
	mount.cpp
	thread.cpp
)
# headers for MOC
SET(guisetup_MOC_HDR
	mainwindow.h
	thread.h
	help.h
	mount.h
)

# UI
SET(guisetup_UI
	mainwindow.ui
	help.ui
	mount.ui
)
SET(guisetup_QRC
	mainwindow.qrc
)
FIND_PACKAGE(Qt4 REQUIRED)
INCLUDE( ${QT_USE_FILE} )
QT4_ADD_RESOURCES(guisetup_RCC_SRCS ${guisetup_QRC})
QT4_WRAP_UI(guisetup_UI_HDR ${guisetup_UI})
QT4_WRAP_CPP(guisetup_MOC_SRC ${guisetup_MOC_HDR})
INCLUDE_DIRECTORIES(${CMAKE_BINARY_DIR})
ADD_EXECUTABLE(guisetup ${guisetup_SRC} ${guisetup_MOC_SRC} ${guisetup_UI_HDR} ${guisetup_RCC_SRCS})
TARGET_LINK_LIBRARIES(guisetup ${QT_LIBRARIES} mpkg mpkg-parted)

FILE(GLOB icons "${CMAKE_CURRENT_SOURCE_DIR}/icons/*.png")
FILE(GLOB l10n "${CMAKE_CURRENT_SOURCE_DIR}/*.qm")
FILE(GLOB desktop "${CMAKE_CURRENT_SOURCE_DIR}/guisetup.desktop")

# Installation
 
INSTALL(TARGETS guisetup
		RUNTIME DESTINATION bin
		)
INSTALL(FILES ${icons} DESTINATION share/setup/icons)
INSTALL(FILES "${CMAKE_CURRENT_SOURCE_DIR}/default_image.png" DESTINATION share/setup)
INSTALL(FILES ${l10n} DESTINATION share/setup/l10n)
INSTALL(FILES ${desktop} DESTINATION share/applications)
INSTALL(FILES "${CMAKE_CURRENT_SOURCE_DIR}/icon/16/guisetup.png" DESTINATION share/icons/hicolor/apps/16)
INSTALL(FILES "${CMAKE_CURRENT_SOURCE_DIR}/icon/24/guisetup.png" DESTINATION share/icons/hicolor/apps/24)
INSTALL(FILES "${CMAKE_CURRENT_SOURCE_DIR}/icon/32/guisetup.png" DESTINATION share/icons/hicolor/apps/32)
INSTALL(FILES "${CMAKE_CURRENT_SOURCE_DIR}/icon/48/guisetup.png" DESTINATION share/icons/hicolor/apps/48)
INSTALL(FILES "${CMAKE_CURRENT_SOURCE_DIR}/icon/64/guisetup.png" DESTINATION share/icons/hicolor/apps/64)
INSTALL(FILES "${CMAKE_CURRENT_SOURCE_DIR}/icon/72/guisetup.png" DESTINATION share/icons/hicolor/apps/72)
INSTALL(FILES "${CMAKE_CURRENT_SOURCE_DIR}/icon/128/guisetup.png" DESTINATION share/icons/hicolor/apps/128)

# Help system now goes from another SVN.
#FILE(GLOB helpfiles_ru "${CMAKE_CURRENT_SOURCE_DIR}/help/ru/*.html")
#FILE(GLOB helpfiles_en "${CMAKE_CURRENT_SOURCE_DIR}/help/en/*.html")
#INSTALL(FILES ${helpfiles_ru} DESTINATION share/setup/help/ru_RU)
#INSTALL(FILES ${helpfiles_en} DESTINATION share/setup/help/en_US)

