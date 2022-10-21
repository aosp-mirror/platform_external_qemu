/* Copyright (C) 2007-2009 The Android Open Source Project
**
** This software is licensed under the terms of the GNU General Public
** License version 2, as published by the Free Software Foundation, and
** may be copied, distributed, and modified under those terms.
**
** This program is distributed in the hope that it will be useful,
** but WITHOUT ANY WARRANTY; without even the implied warranty of
** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
** GNU General Public License for more details.
*/

#pragma once

#include <android/utils/compiler.h>
#include <android/utils/system.h>
#include <stdbool.h>
#include <stdint.h>  /* for uint64_t */

#ifndef _WIN32
#include <limits.h>
#endif

ANDROID_BEGIN_HEADER

/** MISC FILE AND DIRECTORY HANDLING
 **/

/* O_BINARY is required in the MS C library to avoid opening file
 * in text mode (the default, ahhhhh)
 */
#if !defined(_WIN32) && !defined(O_BINARY)
#  define  O_BINARY  0
#endif

/* define  PATH_SEP as a string containing the directory separator */
#ifdef _WIN32
#  define  PATH_SEP   "\\"
#  define  PATH_SEP_C '\\'
#else
#  define  PATH_SEP   "/"
#  define  PATH_SEP_C '/'
#endif

/* Here we used to have a macro redefining MAX_PATH to be 1024 on Windows.
 * That's incorrect as if one wants to go beyond the Windows SDK MAX_PATH, he
 * has to format paths in a very special way - which isn't followed anywhere
 * in our codebase.
 * So let's just use what we got from windows.h here (260 if it's not in yet)
 */
#ifdef _WIN32
#  ifndef MAX_PATH
     // we haven't included <windows.h> yet and we can't here - it might break
     // some hardcore C code
#    define MAX_PATH 260
#  endif

#  ifdef PATH_MAX
#    undef   PATH_MAX
#  endif

#  define  PATH_MAX    MAX_PATH

#else
#  define  MAX_PATH    PATH_MAX
#endif

/* checks that a given file exists */
extern ABool  path_exists( const char*  path );

/* checks that a path points to a regular file */
extern ABool  path_is_regular( const char*  path );

/* checks that a path points to a directory */
extern ABool  path_is_dir( const char*  path );

/* checks that a path is absolute or not */
extern ABool  path_is_absolute( const char*  path );

/* checks that one can read/write a given (regular) file */
extern ABool  path_can_read( const char*  path );
extern ABool  path_can_write( const char*  path );

/* checks that one can execute a given file */
extern ABool  path_can_exec( const char* path );

/* a wrapper for open(filename, oflag, pmode) to support unicode paths on Windows */
extern int path_open(const char *filename, int oflag, int pmode);

/* try to make a directory */
extern APosixStatus   path_android_mkdir( const char*  path, int  mode );

/* ensure that a given directory exists, create it if not,
   0 on success, -1 on error */
extern APosixStatus   path_mkdir_if_needed( const char*  path, int  mode );

/* creates a directory if needed, and also disables copy-on-write */
/* bug: 117641628 */
extern APosixStatus   path_mkdir_if_needed_no_cow( const char*  path, int  mode );

/* return the size of a given file in '*psize'. returns 0 on
 * success, -1 on failure (error code in errno) */
extern APosixStatus   path_get_size( const char*  path, uint64_t  *psize );

/*  path_parent() can be used to return the n-level parent of a given directory
 *  this understands . and .. when encountered in the input path.
 *
 *  the returned string must be freed by the caller.
 */
extern char*  path_parent( const char*  path, int  levels );

/* split a path into a (dirname,basename) pair. the result strings must be freed
 * by the caller. Return 0 on success, or -1 on error. Error conditions include
 * the following:
 *   - 'path' is empty
 *   - 'path' is "/" or degenerate cases like "////"
 *   - basename is "." or ".."
 *
 * if there is no directory separator in path, *dirname will be set to "."
 * if the path is of type "/foo", then *dirname will be set to "/"
 *
 * pdirname can be NULL if you don't want the directory name
 * pbasename can be NULL if you don't want the base name
 */
extern int    path_split( const char*  path, char* *pdirname, char* *pbasename );

/* a convenience function to retrieve the directory name as returned by
 * path_split(). Returns NULL if path_split() returns an error.
 * the result string must be freed by the caller
 */
extern char*  path_dirname( const char*  path );

/* a convenience function to retrieve the base name as returned by
 * path_split(). Returns NULL if path_split() returns an error.
 * the result must be freed by the caller.
 */
extern char*  path_basename( const char*  path );

/* look for a given executable in the system path and return its full path.
 * Returns NULL if not found. Note that on Windows this doesn't append
 * an .exe prefix, or other magical thing like Cygwin usually does.
 */
extern char*  path_search_exec( const char* filename );

/* Return the absolute version of a path. If 'path' is already absolute,
 * this will be a simple copy. Otherwise, this function will prepend the
 * current working directory to the result.
 */
extern char*  path_get_absolute( const char* path );

/** OTHER FILE UTILITIES
 **
 **  path_is_same() checks the filesystem to see if two paths point to the
 **  same file or directory. This will follow symbolic links, handle case
 **  insensitive filesystem, and relative paths and still identify if the
 **  files or directories are the same.
 **
 **  path_empty_file() creates an empty file at a given path location.
 **  if the file already exists, it is truncated without warning
 **
 **  path_copy_file() copies one file into another.
 **
 **  path_delete_file() is equivalent toandroid_unlink() on Unix, on Windows,
 **  it will handle the case where _unlink() fails because the file is
 **  read-only by trying to change its access rights then calling _unlink()
 **  again.
 **
 **  these functions return 0 on success, and -1 on error
 **
 **  load_text_file() reads a file into a heap-allocated memory block,
 **  and appends a 0 to it. the caller must free it
 **/

/* Checks the filesystem to see if two paths point to the same file or
 * directory. This will follow symbolic links, handle case insensitive
 * filesystem and relative paths and still identify if the files or directories
 * are the same.
 */
APosixStatus path_is_same(const char* left, const char* right, bool* isSame);

/* creates an empty file at a given location. If the file already
 * exists, it is truncated without warning. returns 0 on success,
 * or -1 on failure.
 */
extern APosixStatus   path_empty_file( const char*  path );

/* copies on file into another one. 0 on success, -1 on failure
 * (error code in errno). Does not work on directories */
extern APosixStatus   path_copy_file( const char*  dest, const char*  source );

/* this version of path_copy_file() function is safe to call in an unstable
 * environment, e.g. when handling a crash. It uses much smaller buffer for
 * reading/writing files, so there's much lower chance of getting a second
 * crash from stack overflow or page fault while trying to map a new stack page
*/
extern APosixStatus   path_copy_file_safe(const char* dest,
                                          const char* source);

/* unlink/delete a given file. Note that on Win32, this will
 * fail if the program has an opened handle to the file
 */
extern APosixStatus   path_delete_file( const char*  path );

/* delete a given directory recursively. Same thing with Win32 - doesn't work
 * if any file inside is open.
 */
extern APosixStatus   path_delete_dir( const char*  path );

/* delete a given directory's contents recursively.
 * Same thing with Win32 - doesn't work if any file inside is open.
 */
extern APosixStatus   path_delete_dir_contents( const char*  path );

/* Returns whether or not the directory has files in it.
 * For regular files, returns false. */
extern bool           path_dir_has_files( const char*  path );

/* Mark a file to be deleted on reboot. */
extern APosixStatus   path_delete_file_on_reboot( const char*  path );

/* Mark files in a directory to be deleted on reboot. */
extern APosixStatus   path_delete_dir_contents_on_reboot( const char*  path );

/* try to load a given file into a heap-allocated block.
 * if 'pSize' is not NULL, this will set the file's size in '*pSize'
 * note that this actually zero-terminates the file for convenience.
 * In case of failure, NULL is returned and the error code is in errno
 */
extern void*          path_load_file( const char*  path, size_t  *pSize );

/* Modify the path to eliminate ',' and '=', which confuse parsers
 * that look at strings with multiple parameters.
 * This function makes these conversions: ',' --> '%' 'C'
 *                                        '=' --> '%' 'E'
 *                                        '%' --> '%' 'P'
 *
 * The returned string is on the heap. The caller is reponsible
 * for freeing this memory.
 */
extern char*          path_escape_path( const char* src );

/* Modify the path to restore ',' and '='.
 * This function makes these conversions: '%' 'C' --> ','
 *                                        '%' 'E' --> '='
 *                                        '%' 'P' --> '%'
 *
 * Because this can only reduce the length of the string,
 * this conversion is done in place.
 */
extern void           path_unescape_path( char* str );

/* Joins two components of a path using a path separator (optionally).
 * Returns a pointer to a newly allocated string that must be
 * freed by the caller
 */
extern char*          path_join( const char* part1,
                                 const char* part2);

/* Copy folder from src to dst. If an error happens in the middle (permission
 * errors, etc), the |dst| folder will be in half-copied state.
 */
extern APosixStatus   path_copy_dir(const char* dst, const char* src);

/* copy folder, but skip file if it is in the skipset (type
   std::set<std::string>)
   keeps copy
   */
extern APosixStatus path_copy_dir_ex(const char* dst,
                                     const char* src,
                                     void* pSkipSet);

ANDROID_END_HEADER
