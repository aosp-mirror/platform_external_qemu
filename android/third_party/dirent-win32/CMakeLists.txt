cmake_minimum_required(VERSION 3.5)

# Dirent for windows-msvc.
add_library(dirent-win32 INTERFACE)
android_target_include_directories(dirent-win32 windows_msvc-x86_64 INTERFACE include/)
