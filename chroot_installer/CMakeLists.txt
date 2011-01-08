cmake_minimum_required(VERSION 2.6)
    if(COMMAND cmake_policy)
      cmake_policy(SET CMP0003 NEW)
      cmake_policy(SET CMP0005 NEW)
    endif(COMMAND cmake_policy)
# Project name
PROJECT(MKLIVECD)

# Main scripts will be in /usr/share/mklivecd/scripts/
# Plugins will be in /usr/share/mklivecd/plugins/
# Examples will be in /usr/doc/mklivecd/examples/
# Global config: /etc/mklivecd.conf
# Main wrapper: /usr/sbin/mklivecd

FILE(GLOB scripts "${CMAKE_CURRENT_SOURCE_DIR}/*.sh")
FILE(GLOB conftemplate "${CMAKE_CURRENT_SOURCE_DIR}/mpkg.xml*")
FILE(GLOB dbtemplate "${CMAKE_CURRENT_SOURCE_DIR}/packages.db")

INSTALL(FILES "${CMAKE_CURRENT_SOURCE_DIR}/mklivecd" DESTINATION sbin)
INSTALL(FILES ${scripts} DESTINATION share/mklivecd/scripts)
INSTALL(FILES ${conftemplate} DESTINATION share/mklivecd)
INSTALL(FILES ${dbtemplate} DESTINATION share/mklivecd)

