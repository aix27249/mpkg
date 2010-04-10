#!/bin/sh
set -e
./create_dirs.sh
./select_language
./select_partitioning
./select_formatting
./select_swap
./select_root
./select_mountpoints
./select_pkgsource
./update_cache
./select_pkgset
./select_bootloader
./select_bootloader_target
./select_framebuffer
./run_setup
