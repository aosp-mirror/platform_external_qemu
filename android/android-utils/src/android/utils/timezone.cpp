/* Copyright (C) 2007-2008 The Android Open Source Project
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
#include "android/utils/timezone.h"

#include "android/base/memory/ScopedPtr.h"
#include "android/utils/bufprint.h"
#include "android/utils/debug.h"
#include "android/utils/file_io.h"
#include "android/utils/eintr_wrapper.h"

#include <string.h>
#include <stdio.h>

#include <string>

#define  DEBUG  1

#if 1
#  define  D_ACTIVE   VERBOSE_CHECK(timezone)
#else
#  define  D_ACTIVE   DEBUG
#endif

#if DEBUG
#  define  D(...)  do { if (D_ACTIVE) fprintf(stderr, __VA_ARGS__); } while (0)
#else
#  define  D(...)  ((void)0)
#endif



static const char* get_zoneinfo_timezone( void );  /* forward */

static char         android_timezone0[256];
static const char*  android_timezone;

static int
check_timezone_is_zoneinfo(const char*  tz)
{
    const char*  slash1 = NULL, *slash2 = NULL;

    if (tz == NULL)
        return 0;

    /* the name must be of the form Area/Location or Area/Location/SubLocation */
    slash1 = strchr( tz, '/' );
    if (slash1 == NULL || slash1[1] == 0)
        return 0;

    slash2 = strchr( slash1+1, '/');
    if (slash2 != NULL) {
        if (slash2[1] == 0 || strchr(slash2+1, '/') != NULL)
            return 0;
    }

    return 1;
}

char*
bufprint_zoneinfo_timezone( char*  p, char*  end )
{
    const char*  tz = get_zoneinfo_timezone();

    if (tz == NULL || !check_timezone_is_zoneinfo(tz))
        return bufprint(p, end, "Unknown/Unknown");
    else
        return bufprint(p, end, "%s", tz);
}

/* on OS X, the timezone directory is always "zoneinfo"
 * 1, look up the real value of the symlink /etc/localtime
 * 2, check if the path contains "/zoneinfo/"
 * 3, timezone starts after "/zoneinfo/"
 */
#if defined(__APPLE__)

#ifndef _MSC_VER
#include <unistd.h>
#endif
#include <limits.h>
#define  LOCALTIME_FILE  "/etc/localtime"
#define  ZONEINFO_DIR    "/zoneinfo/"
static const char*
get_zoneinfo_timezone( void )
{
    if (!android_timezone) {
        const char*  tz = getenv("TZ");
        char         buff[PATH_MAX+1];

        if (tz == NULL) {
            int   len = readlink(LOCALTIME_FILE, buff, sizeof(buff));
            if (len < 0) {
                dwarning("Could not read %s, something is very wrong on your system",
                        LOCALTIME_FILE);
                return NULL;
            }

            buff[len] = 0;
            D("%s: %s points to %s\n", __FUNCTION__, LOCALTIME_FILE, buff);
            char* zoneinfo_dir = NULL;
            if ( zoneinfo_dir = strstr(buff, ZONEINFO_DIR) ) {
                tz = zoneinfo_dir + sizeof(ZONEINFO_DIR) - 1;
                if ( !check_timezone_is_zoneinfo(tz) ) {
                    dwarning("%s does not point to zoneinfo-compatible timezone name", LOCALTIME_FILE );
                    return NULL;
                }
            }
        }
        snprintf(android_timezone0, sizeof(android_timezone0), "%s", tz );
        android_timezone = android_timezone0;
    }
    D( "found timezone %s", android_timezone );
    return android_timezone;
}

#endif  /* __APPLE__ */

/* on Linux, with glibc2, the zoneinfo directory can be changed with TZDIR environment variable
 * but should be /usr/share/zoneinfo by default. /etc/localtime is not guaranteed to exist on
 * all platforms, so if it doesn't, try $TZDIR/localtime, then /usr/share/zoneinfo/locatime
 * ugly, isn't it ?
 *
 * besides, modern Linux distribution don't make /etc/localtime a symlink but a straight copy of
 * the original timezone file. the only way to know which zoneinfo name to retrieve is to compare
 * it with all files in $TZDIR (at least those that match Area/Location or Area/Location/SubLocation
 */
#if defined(__linux__) || defined (__FreeBSD__)

#ifndef _MSC_VER
#include <unistd.h>
#endif
#include <limits.h>
#include <sys/stat.h>
#include <dirent.h>
#include <fcntl.h>
#include <errno.h>
#include <string.h>

#define  ZONEINFO_DIR  "/usr/share/zoneinfo/"
#define  LOCALTIME_FILE1  "/etc/localtime"

typedef struct {
    const char*   localtime;
    struct stat   localtime_st;
    char*         path_end;
    char*         path_root;
    char          path[ PATH_MAX ];
} ScanDataRec;

static int
compare_timezone_to_localtime( ScanDataRec*  scan,
                               const char*   path )
{
    struct  stat  st;
    int           fd1, fd2, result = 0;

    D( "%s: comparing %s:", __FUNCTION__, path );

    if ( stat( path, &st ) < 0 ) {
        D( " can't stat: %s\n", strerror(errno) );
        return 0;
    }

    if ( st.st_size != scan->localtime_st.st_size ) {
        D( " size mistmatch (%zd != %zd)\n", (size_t)st.st_size, (size_t)scan->localtime_st.st_size );
        return 0;
    }

    fd1 = open( scan->localtime, O_RDONLY );
    if (fd1 < 0) {
        D(" can't open %s: %s\n", scan->localtime, strerror(errno) );
        return 0;
    }
    fd2 = open( path, O_RDONLY );
    if (fd2 < 0) {
        D(" can't open %s: %s\n", path, strerror(errno) );
        close(fd1);
        return 0;
    }
    do {
        off_t  nn;

        for (nn = 0; nn < st.st_size; nn++) {
            char  temp[2];
            int   ret;

            ret = HANDLE_EINTR(read(fd1, &temp[0], 1));
            if (ret < 0) break;

            ret = HANDLE_EINTR(read(fd2, &temp[1], 1));
            if (ret < 0) break;

            if (temp[0] != temp[1])
                break;
        }

        result = (nn == st.st_size);

    } while (0);

    D( result ? " MATCH\n" : "no match\n" );

    close(fd2);
    close(fd1);

    return result;
}

static const char*
scan_timezone_dir( ScanDataRec*  scan,
                   char*         top,
                   int           depth )
{
    DIR*         d = opendir( scan->path );
    const char*  result = NULL;

    D( "%s: entering '%s\n", __FUNCTION__, scan->path );
    if (d != NULL) {
        struct  dirent*  ent;
        while ((ent = readdir(d)) != NULL) {
            struct stat   ent_st;
            char*         p = top;

            if  (ent->d_name[0] == '.')  /* avoid hidden and special files */
                continue;

            p = bufprint( p, scan->path_end, "/%s", ent->d_name );
            if (p >= scan->path_end)
                continue;

            //D( "%s: scanning '%s'\n", __FUNCTION__, scan->path );

            // Important: use lstat() instead of stat() because recent
            // Ubuntu distributions creates directories full of links, e.g.
            // /usr/share/info/posix/Australia/Sydney -> ../../Australia/Sydney
            // and we want to ignore them.
            if ( lstat( scan->path, &ent_st ) < 0 )
                continue;

            if ( S_ISDIR(ent_st.st_mode) && depth < 2 )
            {
                //D( "%s: directory '%s'\n", __FUNCTION__, scan->path );
                result = scan_timezone_dir( scan, p, depth + 1 );
                if (result != NULL)
                    break;
            }
            else if ( S_ISREG(ent_st.st_mode) && (depth >= 1 && depth <= 2) )
            {
                char*   name = scan->path_root + 1;

                if ( check_timezone_is_zoneinfo( name ) )
                {
                    if (compare_timezone_to_localtime( scan, scan->path ))
                    {
                        result = strdup( name );
                        D( "%s: found '%s'\n", __FUNCTION__, result );
                        break;
                    }
                }
                else
                {
                    //D( "%s: ignoring '%s'\n", __FUNCTION__, scan->path );
                }
            }
        }
        closedir(d);
    }
    return  result;
}

static const char*
get_zoneinfo_timezone( void )
{
    if (!android_timezone)
    {
        const char*  tz = getenv( "TZ" );
        char         temp[ PATH_MAX ];

        // if we ever allocate into tz, this object will take care of it.
        android::base::ScopedCPtr<const char> tzDeleter;

        if ( tz != NULL && !check_timezone_is_zoneinfo(tz) ) {
            D( "%s: ignoring non zoneinfo formatted TZ environment variable: '%s'\n",
               __FUNCTION__, tz );
            tz = NULL;
        }

        if (tz == NULL) {
            int          len;
            std::string  tzdir;
            std::string  localtime;

            /* determine the correct timezone directory */
            {
                const char*  env = getenv("TZDIR");
                const char*  zoneinfo_dir = ZONEINFO_DIR;

                if (env == NULL)
                    env = zoneinfo_dir;

                if ( android_access( env, R_OK ) != 0 ) {
                    if ( env == zoneinfo_dir ) {
                        dwarning("could not find %s directory. unable to determine host timezone", env);
                    } else {
                        D( "%s: TZDIR does not point to valid directory, using %s instead\n",
                           __FUNCTION__, zoneinfo_dir );
                        env = zoneinfo_dir;
                    }
                    return NULL;
                }
                tzdir = env;
            }

            /* remove trailing slash, if any */
            if (!tzdir.empty() && tzdir.back() == '/') {
                tzdir.pop_back();
            }
            D( "%s: found timezone dir as %s\n", __FUNCTION__, tzdir.c_str() );

            /* try to find the localtime file */
            const char* localtimePtr = LOCALTIME_FILE1;
            if ( android_access( localtimePtr, R_OK ) != 0 ) {
                char  *p = temp, *end = p + sizeof(temp);

                p = bufprint( p, end, "%s/%s", tzdir.c_str(), "localtime" );
                if (p >= end || android_access( temp, R_OK ) != 0 ) {
                    dwarning("could not find %s or %s. unable to determine host timezone",
                                     LOCALTIME_FILE1, temp );
                    goto Exit;
                }
                localtimePtr = temp;
            }
            localtime = localtimePtr;
            D( "%s: found localtime file as %s\n", __FUNCTION__, localtime.c_str() );

#if 1
            /* if the localtime file is a link, make a quick check */
            len = readlink( localtime.c_str(), temp, sizeof(temp)-1 );
            if (len >= 0 && len > static_cast<int>(tzdir.size()) + 2) {
                temp[len] = 0;

                /* verify that the link points to tzdir/<something> where <something> is a valid zoneinfo name */
                if ( !memcmp( temp, tzdir.c_str(), tzdir.size() ) && temp[tzdir.size()] == '/' ) {
                    if ( check_timezone_is_zoneinfo( temp + tzdir.size() + 1 ) ) {
                        /* we have it ! */
                        tz = temp + tzdir.size() + 1;
                        D( "%s: found zoneinfo timezone %s from %s symlink\n", __FUNCTION__, tz, localtime.c_str() );
                        goto Exit;
                    }
                    D( "%s: %s link points to non-zoneinfo filename %s, comparing contents\n",
                       __FUNCTION__, localtime.c_str(), temp );
                }
            }
#endif

            /* otherwise, parse all files under tzdir and see if we have something that looks like it */
            {
                ScanDataRec  scan[1];

                if ( stat( localtime.c_str(), &scan->localtime_st ) < 0 ) {
                    dwarning("can't access '%s', unable to determine host timezone",
                             localtime.c_str() );
                    goto Exit;
                }

                scan->localtime = localtime.c_str();
                scan->path_end  = scan->path + sizeof(scan->path);
                scan->path_root = bufprint( scan->path, scan->path_end, "%s", tzdir.c_str() );

                tz = scan_timezone_dir( scan, scan->path_root, 0 );
                tzDeleter.reset(tz);
            }

        Exit:
            if (tz == NULL)
                return NULL;
        }

        snprintf(android_timezone0, sizeof(android_timezone0), "%s", tz);
        android_timezone = android_timezone0;

        D( "found timezone %s\n", android_timezone );
    }
    return android_timezone;
}

#endif /* __linux__ */


/* on Windows, we need to translate the Windows timezone into a ZoneInfo one */
#ifdef _WIN32

#include <windows.h>
typedef struct {
    const char*  win_name;
    const char*  zoneinfo_name;
} Win32Timezone;

/* table generated from http://www.unicode.org/cldr/charts/latest/supplemental/zone_tzid.html */
static const Win32Timezone  _win32_timezones[] = {
    { "AUS Central Standard Time",    "Australia/Darwin" },
    { "AUS Eastern Standard Time",    "Australia/Sydney" },
    { "AUS Eastern Standard Time",    "Australia/Melbourne" },
    { "Afghanistan Standard Time",    "Asia/Kabul" },
    { "Alaskan Standard Time",    "America/Anchorage" },
    { "Alaskan Standard Time",    "America/Juneau" },
    { "Alaskan Standard Time",    "America/Metlakatla" },
    { "Alaskan Standard Time",    "America/Nome" },
    { "Alaskan Standard Time",    "America/Sitka" },
    { "Alaskan Standard Time",    "America/Yakutat" },
    { "Aleutian Standard Time",    "America/Adak" },
    { "Altai Standard Time",    "Asia/Barnaul" },
    { "Arab Standard Time",    "Asia/Riyadh" },
    { "Arab Standard Time",    "Asia/Bahrain" },
    { "Arab Standard Time",    "Asia/Kuwait" },
    { "Arab Standard Time",    "Asia/Qatar" },
    { "Arab Standard Time",    "Asia/Aden" },
    { "Arabian Standard Time",    "Asia/Dubai" },
    { "Arabian Standard Time",    "Asia/Muscat" },
    { "Arabian Standard Time",    "Etc/GMT-4" },
    { "Arabic Standard Time",    "Asia/Baghdad" },
    { "Argentina Standard Time",    "America/Buenos_Aires" },
    { "Argentina Standard Time",    "America/Argentina/La_Rioja" },
    { "Argentina Standard Time",    "America/Argentina/Rio_Gallegos" },
    { "Argentina Standard Time",    "America/Argentina/Salta" },
    { "Argentina Standard Time",    "America/Argentina/San_Juan" },
    { "Argentina Standard Time",    "America/Argentina/San_Luis" },
    { "Argentina Standard Time",    "America/Argentina/Tucuman" },
    { "Argentina Standard Time",    "America/Argentina/Ushuaia" },
    { "Argentina Standard Time",    "America/Catamarca" },
    { "Argentina Standard Time",    "America/Cordoba" },
    { "Argentina Standard Time",    "America/Jujuy" },
    { "Argentina Standard Time",    "America/Mendoza" },
    { "Astrakhan Standard Time",    "Europe/Astrakhan" },
    { "Astrakhan Standard Time",    "Europe/Ulyanovsk" },
    { "Atlantic Standard Time",    "America/Halifax" },
    { "Atlantic Standard Time",    "Atlantic/Bermuda" },
    { "Atlantic Standard Time",    "America/Glace_Bay" },
    { "Atlantic Standard Time",    "America/Goose_Bay" },
    { "Atlantic Standard Time",    "America/Moncton" },
    { "Atlantic Standard Time",    "America/Thule" },
    { "Aus Central W. Standard Time",    "Australia/Eucla" },
    { "Azerbaijan Standard Time",    "Asia/Baku" },
    { "Azores Standard Time",    "Atlantic/Azores" },
    { "Azores Standard Time",    "America/Scoresbysund" },
    { "Bahia Standard Time",    "America/Bahia" },
    { "Bangladesh Standard Time",    "Asia/Dhaka" },
    { "Bangladesh Standard Time",    "Asia/Thimphu" },
    { "Belarus Standard Time",    "Europe/Minsk" },
    { "Bougainville Standard Time",    "Pacific/Bougainville" },
    { "Canada Central Standard Time",    "America/Regina" },
    { "Canada Central Standard Time",    "America/Swift_Current" },
    { "Cape Verde Standard Time",    "Atlantic/Cape_Verde" },
    { "Cape Verde Standard Time",    "Etc/GMT+1" },
    { "Caucasus Standard Time",    "Asia/Yerevan" },
    { "Cen. Australia Standard Time",    "Australia/Adelaide" },
    { "Cen. Australia Standard Time",    "Australia/Broken_Hill" },
    { "Central America Standard Time",    "America/Guatemala" },
    { "Central America Standard Time",    "America/Belize" },
    { "Central America Standard Time",    "America/Costa_Rica" },
    { "Central America Standard Time",    "Pacific/Galapagos" },
    { "Central America Standard Time",    "America/Tegucigalpa" },
    { "Central America Standard Time",    "America/Managua" },
    { "Central America Standard Time",    "America/El_Salvador" },
    { "Central America Standard Time",    "Etc/GMT+6" },
    { "Central Asia Standard Time",    "Asia/Almaty" },
    { "Central Asia Standard Time",    "Antarctica/Vostok" },
    { "Central Asia Standard Time",    "Asia/Urumqi" },
    { "Central Asia Standard Time",    "Indian/Chagos" },
    { "Central Asia Standard Time",    "Asia/Bishkek" },
    { "Central Asia Standard Time",    "Asia/Qyzylorda" },
    { "Central Asia Standard Time",    "Etc/GMT-6" },
    { "Central Brazilian Standard Time",    "America/Cuiaba" },
    { "Central Brazilian Standard Time",    "America/Campo_Grande" },
    { "Central Europe Standard Time",    "Europe/Budapest" },
    { "Central Europe Standard Time",    "Europe/Tirane" },
    { "Central Europe Standard Time",    "Europe/Prague" },
    { "Central Europe Standard Time",    "Europe/Podgorica" },
    { "Central Europe Standard Time",    "Europe/Belgrade" },
    { "Central Europe Standard Time",    "Europe/Ljubljana" },
    { "Central Europe Standard Time",    "Europe/Bratislava" },
    { "Central European Standard Time",    "Europe/Warsaw" },
    { "Central European Standard Time",    "Europe/Sarajevo" },
    { "Central European Standard Time",    "Europe/Zagreb" },
    { "Central European Standard Time",    "Europe/Skopje" },
    { "Central Pacific Standard Time",    "Pacific/Guadalcanal" },
    { "Central Pacific Standard Time",    "Antarctica/Macquarie" },
    { "Central Pacific Standard Time",    "Pacific/Ponape" },
    { "Central Pacific Standard Time",    "Pacific/Kosrae" },
    { "Central Pacific Standard Time",    "Pacific/Noumea" },
    { "Central Pacific Standard Time",    "Pacific/Efate" },
    { "Central Pacific Standard Time",    "Etc/GMT-11" },
    { "Central Standard Time",    "America/Chicago" },
    { "Central Standard Time",    "America/Winnipeg" },
    { "Central Standard Time",    "America/Rainy_River" },
    { "Central Standard Time",    "America/Rankin_Inlet" },
    { "Central Standard Time",    "America/Resolute" },
    { "Central Standard Time",    "America/Matamoros" },
    { "Central Standard Time",    "America/Indiana/Knox" },
    { "Central Standard Time",    "America/Indiana/Tell_City" },
    { "Central Standard Time",    "America/Menominee" },
    { "Central Standard Time",    "America/North_Dakota/Beulah" },
    { "Central Standard Time",    "America/North_Dakota/Center" },
    { "Central Standard Time",    "America/North_Dakota/New_Salem" },
    { "Central Standard Time",    "CST6CDT" },
    { "Central Standard Time (Mexico)",    "America/Mexico_City" },
    { "Central Standard Time (Mexico)",    "America/Bahia_Banderas" },
    { "Central Standard Time (Mexico)",    "America/Merida" },
    { "Central Standard Time (Mexico)",    "America/Monterrey" },
    { "Chatham Islands Standard Time",    "Pacific/Chatham" },
    { "China Standard Time",    "Asia/Shanghai" },
    { "China Standard Time",    "Asia/Hong_Kong" },
    { "China Standard Time",    "Asia/Macau" },
    { "Cuba Standard Time",    "America/Havana" },
    { "Dateline Standard Time",    "Etc/GMT+12" },
    { "E. Africa Standard Time",    "Africa/Nairobi" },
    { "E. Africa Standard Time",    "Antarctica/Syowa" },
    { "E. Africa Standard Time",    "Africa/Djibouti" },
    { "E. Africa Standard Time",    "Africa/Asmera" },
    { "E. Africa Standard Time",    "Africa/Addis_Ababa" },
    { "E. Africa Standard Time",    "Indian/Comoro" },
    { "E. Africa Standard Time",    "Indian/Antananarivo" },
    { "E. Africa Standard Time",    "Africa/Khartoum" },
    { "E. Africa Standard Time",    "Africa/Mogadishu" },
    { "E. Africa Standard Time",    "Africa/Juba" },
    { "E. Africa Standard Time",    "Africa/Dar_es_Salaam" },
    { "E. Africa Standard Time",    "Africa/Kampala" },
    { "E. Africa Standard Time",    "Indian/Mayotte" },
    { "E. Africa Standard Time",    "Etc/GMT-3" },
    { "E. Australia Standard Time",    "Australia/Brisbane" },
    { "E. Australia Standard Time",    "Australia/Lindeman" },
    { "E. Europe Standard Time",    "Europe/Chisinau" },
    { "E. South America Standard Time",    "America/Sao_Paulo" },
    { "Easter Island Standard Time",    "Pacific/Easter" },
    { "Eastern Standard Time",    "America/New_York" },
    { "Eastern Standard Time",    "America/Nassau" },
    { "Eastern Standard Time",    "America/Toronto" },
    { "Eastern Standard Time",    "America/Iqaluit" },
    { "Eastern Standard Time",    "America/Montreal" },
    { "Eastern Standard Time",    "America/Nipigon" },
    { "Eastern Standard Time",    "America/Pangnirtung" },
    { "Eastern Standard Time",    "America/Thunder_Bay" },
    { "Eastern Standard Time",    "America/Detroit" },
    { "Eastern Standard Time",    "America/Indiana/Petersburg" },
    { "Eastern Standard Time",    "America/Indiana/Vincennes" },
    { "Eastern Standard Time",    "America/Indiana/Winamac" },
    { "Eastern Standard Time",    "America/Kentucky/Monticello" },
    { "Eastern Standard Time",    "America/Louisville" },
    { "Eastern Standard Time",    "EST5EDT" },
    { "Eastern Standard Time (Mexico)",    "America/Cancun" },
    { "Egypt Standard Time",    "Africa/Cairo" },
    { "Ekaterinburg Standard Time",    "Asia/Yekaterinburg" },
    { "FLE Standard Time",    "Europe/Kiev" },
    { "FLE Standard Time",    "Europe/Mariehamn" },
    { "FLE Standard Time",    "Europe/Sofia" },
    { "FLE Standard Time",    "Europe/Tallinn" },
    { "FLE Standard Time",    "Europe/Helsinki" },
    { "FLE Standard Time",    "Europe/Vilnius" },
    { "FLE Standard Time",    "Europe/Riga" },
    { "FLE Standard Time",    "Europe/Uzhgorod" },
    { "FLE Standard Time",    "Europe/Zaporozhye" },
    { "Fiji Standard Time",    "Pacific/Fiji" },
    { "GMT Standard Time",    "Europe/London" },
    { "GMT Standard Time",    "Atlantic/Canary" },
    { "GMT Standard Time",    "Atlantic/Faeroe" },
    { "GMT Standard Time",    "Europe/Guernsey" },
    { "GMT Standard Time",    "Europe/Dublin" },
    { "GMT Standard Time",    "Europe/Isle_of_Man" },
    { "GMT Standard Time",    "Europe/Jersey" },
    { "GMT Standard Time",    "Europe/Lisbon" },
    { "GMT Standard Time",    "Atlantic/Madeira" },
    { "GTB Standard Time",    "Europe/Bucharest" },
    { "GTB Standard Time",    "Asia/Nicosia" },
    { "GTB Standard Time",    "Europe/Athens" },
    { "Georgian Standard Time",    "Asia/Tbilisi" },
    { "Greenland Standard Time",    "America/Godthab" },
    { "Greenwich Standard Time",    "Atlantic/Reykjavik" },
    { "Greenwich Standard Time",    "Africa/Ouagadougou" },
    { "Greenwich Standard Time",    "Africa/Abidjan" },
    { "Greenwich Standard Time",    "Africa/Accra" },
    { "Greenwich Standard Time",    "Africa/Banjul" },
    { "Greenwich Standard Time",    "Africa/Conakry" },
    { "Greenwich Standard Time",    "Africa/Bissau" },
    { "Greenwich Standard Time",    "Africa/Monrovia" },
    { "Greenwich Standard Time",    "Africa/Bamako" },
    { "Greenwich Standard Time",    "Africa/Nouakchott" },
    { "Greenwich Standard Time",    "Atlantic/St_Helena" },
    { "Greenwich Standard Time",    "Africa/Freetown" },
    { "Greenwich Standard Time",    "Africa/Dakar" },
    { "Greenwich Standard Time",    "Africa/Sao_Tome" },
    { "Greenwich Standard Time",    "Africa/Lome" },
    { "Haiti Standard Time",    "America/Port-au-Prince" },
    { "Hawaiian Standard Time",    "Pacific/Honolulu" },
    { "Hawaiian Standard Time",    "Pacific/Rarotonga" },
    { "Hawaiian Standard Time",    "Pacific/Tahiti" },
    { "Hawaiian Standard Time",    "Pacific/Johnston" },
    { "Hawaiian Standard Time",    "Etc/GMT+10" },
    { "India Standard Time",    "Asia/Calcutta" },
    { "Iran Standard Time",    "Asia/Tehran" },
    { "Israel Standard Time",    "Asia/Jerusalem" },
    { "Jordan Standard Time",    "Asia/Amman" },
    { "Kaliningrad Standard Time",    "Europe/Kaliningrad" },
    { "Korea Standard Time",    "Asia/Seoul" },
    { "Libya Standard Time",    "Africa/Tripoli" },
    { "Line Islands Standard Time",    "Pacific/Kiritimati" },
    { "Line Islands Standard Time",    "Etc/GMT-14" },
    { "Lord Howe Standard Time",    "Australia/Lord_Howe" },
    { "Magadan Standard Time",    "Asia/Magadan" },
    { "Marquesas Standard Time",    "Pacific/Marquesas" },
    { "Mauritius Standard Time",    "Indian/Mauritius" },
    { "Mauritius Standard Time",    "Indian/Reunion" },
    { "Mauritius Standard Time",    "Indian/Mahe" },
    { "Middle East Standard Time",    "Asia/Beirut" },
    { "Montevideo Standard Time",    "America/Montevideo" },
    { "Morocco Standard Time",    "Africa/Casablanca" },
    { "Morocco Standard Time",    "Africa/El_Aaiun" },
    { "Mountain Standard Time",    "America/Denver" },
    { "Mountain Standard Time",    "America/Edmonton" },
    { "Mountain Standard Time",    "America/Cambridge_Bay" },
    { "Mountain Standard Time",    "America/Inuvik" },
    { "Mountain Standard Time",    "America/Yellowknife" },
    { "Mountain Standard Time",    "America/Ojinaga" },
    { "Mountain Standard Time",    "America/Boise" },
    { "Mountain Standard Time",    "MST7MDT" },
    { "Mountain Standard Time (Mexico)",    "America/Chihuahua" },
    { "Mountain Standard Time (Mexico)",    "America/Mazatlan" },
    { "Myanmar Standard Time",    "Asia/Rangoon" },
    { "Myanmar Standard Time",    "Indian/Cocos" },
    { "N. Central Asia Standard Time",    "Asia/Novosibirsk" },
    { "N. Central Asia Standard Time",    "Asia/Omsk" },
    { "Namibia Standard Time",    "Africa/Windhoek" },
    { "Nepal Standard Time",    "Asia/Katmandu" },
    { "New Zealand Standard Time",    "Pacific/Auckland" },
    { "New Zealand Standard Time",    "Antarctica/McMurdo" },
    { "Newfoundland Standard Time",    "America/St_Johns" },
    { "Norfolk Standard Time",    "Pacific/Norfolk" },
    { "North Asia East Standard Time",    "Asia/Irkutsk" },
    { "North Asia Standard Time",    "Asia/Krasnoyarsk" },
    { "North Asia Standard Time",    "Asia/Novokuznetsk" },
    { "North Korea Standard Time",    "Asia/Pyongyang" },
    { "Pacific SA Standard Time",    "America/Santiago" },
    { "Pacific SA Standard Time",    "Antarctica/Palmer" },
    { "Pacific Standard Time",    "America/Los_Angeles" },
    { "Pacific Standard Time",    "America/Vancouver" },
    { "Pacific Standard Time",    "America/Dawson" },
    { "Pacific Standard Time",    "America/Whitehorse" },
    { "Pacific Standard Time",    "PST8PDT" },
    { "Pacific Standard Time (Mexico)",    "America/Tijuana" },
    { "Pacific Standard Time (Mexico)",    "America/Santa_Isabel" },
    { "Pakistan Standard Time",    "Asia/Karachi" },
    { "Paraguay Standard Time",    "America/Asuncion" },
    { "Romance Standard Time",    "Europe/Paris" },
    { "Romance Standard Time",    "Europe/Brussels" },
    { "Romance Standard Time",    "Europe/Copenhagen" },
    { "Romance Standard Time",    "Europe/Madrid" },
    { "Romance Standard Time",    "Africa/Ceuta" },
    { "Russia Time Zone 10",    "Asia/Srednekolymsk" },
    { "Russia Time Zone 11",    "Asia/Kamchatka" },
    { "Russia Time Zone 11",    "Asia/Anadyr" },
    { "Russia Time Zone 3",    "Europe/Samara" },
    { "Russian Standard Time",    "Europe/Moscow" },
    { "Russian Standard Time",    "Europe/Kirov" },
    { "Russian Standard Time",    "Europe/Simferopol" },
    { "Russian Standard Time",    "Europe/Volgograd" },
    { "SA Eastern Standard Time",    "America/Cayenne" },
    { "SA Eastern Standard Time",    "Antarctica/Rothera" },
    { "SA Eastern Standard Time",    "America/Fortaleza" },
    { "SA Eastern Standard Time",    "America/Belem" },
    { "SA Eastern Standard Time",    "America/Maceio" },
    { "SA Eastern Standard Time",    "America/Recife" },
    { "SA Eastern Standard Time",    "America/Santarem" },
    { "SA Eastern Standard Time",    "Atlantic/Stanley" },
    { "SA Eastern Standard Time",    "America/Paramaribo" },
    { "SA Eastern Standard Time",    "Etc/GMT+3" },
    { "SA Pacific Standard Time",    "America/Bogota" },
    { "SA Pacific Standard Time",    "America/Rio_Branco" },
    { "SA Pacific Standard Time",    "America/Eirunepe" },
    { "SA Pacific Standard Time",    "America/Coral_Harbour" },
    { "SA Pacific Standard Time",    "America/Guayaquil" },
    { "SA Pacific Standard Time",    "America/Jamaica" },
    { "SA Pacific Standard Time",    "America/Cayman" },
    { "SA Pacific Standard Time",    "America/Panama" },
    { "SA Pacific Standard Time",    "America/Lima" },
    { "SA Pacific Standard Time",    "Etc/GMT+5" },
    { "SA Western Standard Time",    "America/La_Paz" },
    { "SA Western Standard Time",    "America/Antigua" },
    { "SA Western Standard Time",    "America/Anguilla" },
    { "SA Western Standard Time",    "America/Aruba" },
    { "SA Western Standard Time",    "America/Barbados" },
    { "SA Western Standard Time",    "America/St_Barthelemy" },
    { "SA Western Standard Time",    "America/Kralendijk" },
    { "SA Western Standard Time",    "America/Manaus" },
    { "SA Western Standard Time",    "America/Boa_Vista" },
    { "SA Western Standard Time",    "America/Porto_Velho" },
    { "SA Western Standard Time",    "America/Blanc-Sablon" },
    { "SA Western Standard Time",    "America/Curacao" },
    { "SA Western Standard Time",    "America/Dominica" },
    { "SA Western Standard Time",    "America/Santo_Domingo" },
    { "SA Western Standard Time",    "America/Grenada" },
    { "SA Western Standard Time",    "America/Guadeloupe" },
    { "SA Western Standard Time",    "America/Guyana" },
    { "SA Western Standard Time",    "America/St_Kitts" },
    { "SA Western Standard Time",    "America/St_Lucia" },
    { "SA Western Standard Time",    "America/Marigot" },
    { "SA Western Standard Time",    "America/Martinique" },
    { "SA Western Standard Time",    "America/Montserrat" },
    { "SA Western Standard Time",    "America/Puerto_Rico" },
    { "SA Western Standard Time",    "America/Lower_Princes" },
    { "SA Western Standard Time",    "America/Port_of_Spain" },
    { "SA Western Standard Time",    "America/St_Vincent" },
    { "SA Western Standard Time",    "America/Tortola" },
    { "SA Western Standard Time",    "America/St_Thomas" },
    { "SA Western Standard Time",    "Etc/GMT+4" },
    { "SE Asia Standard Time",    "Asia/Bangkok" },
    { "SE Asia Standard Time",    "Antarctica/Davis" },
    { "SE Asia Standard Time",    "Indian/Christmas" },
    { "SE Asia Standard Time",    "Asia/Jakarta" },
    { "SE Asia Standard Time",    "Asia/Pontianak" },
    { "SE Asia Standard Time",    "Asia/Phnom_Penh" },
    { "SE Asia Standard Time",    "Asia/Vientiane" },
    { "SE Asia Standard Time",    "Asia/Saigon" },
    { "SE Asia Standard Time",    "Etc/GMT-7" },
    { "Saint Pierre Standard Time",    "America/Miquelon" },
    { "Sakhalin Standard Time",    "Asia/Sakhalin" },
    { "Samoa Standard Time",    "Pacific/Apia" },
    { "Singapore Standard Time",    "Asia/Singapore" },
    { "Singapore Standard Time",    "Asia/Brunei" },
    { "Singapore Standard Time",    "Asia/Makassar" },
    { "Singapore Standard Time",    "Asia/Kuala_Lumpur" },
    { "Singapore Standard Time",    "Asia/Kuching" },
    { "Singapore Standard Time",    "Asia/Manila" },
    { "Singapore Standard Time",    "Etc/GMT-8" },
    { "South Africa Standard Time",    "Africa/Johannesburg" },
    { "South Africa Standard Time",    "Africa/Bujumbura" },
    { "South Africa Standard Time",    "Africa/Gaborone" },
    { "South Africa Standard Time",    "Africa/Lubumbashi" },
    { "South Africa Standard Time",    "Africa/Maseru" },
    { "South Africa Standard Time",    "Africa/Blantyre" },
    { "South Africa Standard Time",    "Africa/Maputo" },
    { "South Africa Standard Time",    "Africa/Kigali" },
    { "South Africa Standard Time",    "Africa/Mbabane" },
    { "South Africa Standard Time",    "Africa/Lusaka" },
    { "South Africa Standard Time",    "Africa/Harare" },
    { "South Africa Standard Time",    "Etc/GMT-2" },
    { "Sri Lanka Standard Time",    "Asia/Colombo" },
    { "Syria Standard Time",    "Asia/Damascus" },
    { "Taipei Standard Time",    "Asia/Taipei" },
    { "Tasmania Standard Time",    "Australia/Hobart" },
    { "Tasmania Standard Time",    "Australia/Currie" },
    { "Tocantins Standard Time",    "America/Araguaina" },
    { "Tokyo Standard Time",    "Asia/Tokyo" },
    { "Tokyo Standard Time",    "Asia/Jayapura" },
    { "Tokyo Standard Time",    "Pacific/Palau" },
    { "Tokyo Standard Time",    "Asia/Dili" },
    { "Tokyo Standard Time",    "Etc/GMT-9" },
    { "Tomsk Standard Time",    "Asia/Tomsk" },
    { "Tonga Standard Time",    "Pacific/Tongatapu" },
    { "Tonga Standard Time",    "Pacific/Enderbury" },
    { "Tonga Standard Time",    "Pacific/Fakaofo" },
    { "Tonga Standard Time",    "Etc/GMT-13" },
    { "Transbaikal Standard Time",    "Asia/Chita" },
    { "Turkey Standard Time",    "Europe/Istanbul" },
    { "Turks And Caicos Standard Time",    "America/Grand_Turk" },
    { "US Eastern Standard Time",    "America/Indianapolis" },
    { "US Eastern Standard Time",    "America/Indiana/Marengo" },
    { "US Eastern Standard Time",    "America/Indiana/Vevay" },
    { "US Mountain Standard Time",    "America/Phoenix" },
    { "US Mountain Standard Time",    "America/Dawson_Creek" },
    { "US Mountain Standard Time",    "America/Creston" },
    { "US Mountain Standard Time",    "America/Fort_Nelson" },
    { "US Mountain Standard Time",    "America/Hermosillo" },
    { "US Mountain Standard Time",    "Etc/GMT+7" },
    { "UTC",    "Etc/GMT" },
    { "UTC",    "America/Danmarkshavn" },
    { "UTC+12",    "Etc/GMT-12" },
    { "UTC+12",    "Pacific/Tarawa" },
    { "UTC+12",    "Pacific/Majuro" },
    { "UTC+12",    "Pacific/Kwajalein" },
    { "UTC+12",    "Pacific/Nauru" },
    { "UTC+12",    "Pacific/Funafuti" },
    { "UTC+12",    "Pacific/Wake" },
    { "UTC+12",    "Pacific/Wallis" },
    { "UTC-02",    "Etc/GMT+2" },
    { "UTC-02",    "America/Noronha" },
    { "UTC-02",    "Atlantic/South_Georgia" },
    { "UTC-08",    "Etc/GMT+8" },
    { "UTC-08",    "Pacific/Pitcairn" },
    { "UTC-09",    "Etc/GMT+9" },
    { "UTC-09",    "Pacific/Gambier" },
    { "UTC-11",    "Etc/GMT+11" },
    { "UTC-11",    "Pacific/Pago_Pago" },
    { "UTC-11",    "Pacific/Niue" },
    { "UTC-11",    "Pacific/Midway" },
    { "Ulaanbaatar Standard Time",    "Asia/Ulaanbaatar" },
    { "Ulaanbaatar Standard Time",    "Asia/Choibalsan" },
    { "Venezuela Standard Time",    "America/Caracas" },
    { "Vladivostok Standard Time",    "Asia/Vladivostok" },
    { "Vladivostok Standard Time",    "Asia/Ust-Nera" },
    { "W. Australia Standard Time",    "Australia/Perth" },
    { "W. Australia Standard Time",    "Antarctica/Casey" },
    { "W. Central Africa Standard Time",    "Africa/Lagos" },
    { "W. Central Africa Standard Time",    "Africa/Luanda" },
    { "W. Central Africa Standard Time",    "Africa/Porto-Novo" },
    { "W. Central Africa Standard Time",    "Africa/Kinshasa" },
    { "W. Central Africa Standard Time",    "Africa/Bangui" },
    { "W. Central Africa Standard Time",    "Africa/Brazzaville" },
    { "W. Central Africa Standard Time",    "Africa/Douala" },
    { "W. Central Africa Standard Time",    "Africa/Algiers" },
    { "W. Central Africa Standard Time",    "Africa/Libreville" },
    { "W. Central Africa Standard Time",    "Africa/Malabo" },
    { "W. Central Africa Standard Time",    "Africa/Niamey" },
    { "W. Central Africa Standard Time",    "Africa/Ndjamena" },
    { "W. Central Africa Standard Time",    "Africa/Tunis" },
    { "W. Central Africa Standard Time",    "Etc/GMT-1" },
    { "W. Europe Standard Time",    "Europe/Berlin" },
    { "W. Europe Standard Time",    "Europe/Andorra" },
    { "W. Europe Standard Time",    "Europe/Vienna" },
    { "W. Europe Standard Time",    "Europe/Zurich" },
    { "W. Europe Standard Time",    "Europe/Busingen" },
    { "W. Europe Standard Time",    "Europe/Gibraltar" },
    { "W. Europe Standard Time",    "Europe/Rome" },
    { "W. Europe Standard Time",    "Europe/Vaduz" },
    { "W. Europe Standard Time",    "Europe/Luxembourg" },
    { "W. Europe Standard Time",    "Europe/Monaco" },
    { "W. Europe Standard Time",    "Europe/Malta" },
    { "W. Europe Standard Time",    "Europe/Amsterdam" },
    { "W. Europe Standard Time",    "Europe/Oslo" },
    { "W. Europe Standard Time",    "Europe/Stockholm" },
    { "W. Europe Standard Time",    "Arctic/Longyearbyen" },
    { "W. Europe Standard Time",    "Europe/San_Marino" },
    { "W. Europe Standard Time",    "Europe/Vatican" },
    { "W. Mongolia Standard Time",    "Asia/Hovd" },
    { "West Asia Standard Time",    "Asia/Tashkent" },
    { "West Asia Standard Time",    "Antarctica/Mawson" },
    { "West Asia Standard Time",    "Asia/Oral" },
    { "West Asia Standard Time",    "Asia/Aqtau" },
    { "West Asia Standard Time",    "Asia/Aqtobe" },
    { "West Asia Standard Time",    "Indian/Maldives" },
    { "West Asia Standard Time",    "Indian/Kerguelen" },
    { "West Asia Standard Time",    "Asia/Dushanbe" },
    { "West Asia Standard Time",    "Asia/Ashgabat" },
    { "West Asia Standard Time",    "Asia/Samarkand" },
    { "West Asia Standard Time",    "Etc/GMT-5" },
    { "West Bank Standard Time",    "Asia/Hebron" },
    { "West Bank Standard Time",    "Asia/Gaza" },
    { "West Pacific Standard Time",    "Pacific/Port_Moresby" },
    { "West Pacific Standard Time",    "Antarctica/DumontDUrville" },
    { "West Pacific Standard Time",    "Pacific/Truk" },
    { "West Pacific Standard Time",    "Pacific/Guam" },
    { "West Pacific Standard Time",    "Pacific/Saipan" },
    { "West Pacific Standard Time",    "Etc/GMT-10" },
    { "Yakutsk Standard Time",    "Asia/Yakutsk" },
    { "Yakutsk Standard Time",    "Asia/Khandyga" },
    { NULL, NULL }
};

static const char*
get_zoneinfo_timezone( void )
{
    if (!android_timezone)
    {
        char		          tzname[128];
        time_t		          t = time(NULL);
        struct tm*            tm = localtime(&t);
        if (!tm) {
            D("%s: could not determine current date/time\n", __FUNCTION__);
            return NULL;
        }
        // The lookup table above doesn't include "Daylight" in its timezone
        // name. Thus, we always set dst to 0 (b/117424123)
        struct tm local_tm = *tm;
        local_tm.tm_isdst = 0;
        const Win32Timezone*  win32tz = _win32_timezones;


        memset(tzname, 0, sizeof(tzname));
        strftime(tzname, sizeof(tzname) - 1, "%Z", &local_tm);

        for (win32tz = _win32_timezones; win32tz->win_name != NULL; win32tz++)
            if ( !strcmp(win32tz->win_name, tzname) ) {
                android_timezone = win32tz->zoneinfo_name;
                goto Exit;
            }

#if 0  /* TODO */
    /* we didn't find it, this may come from localized versions of Windows. we're going to explore the registry,
    * as the code in Postgresql does...
    */
#endif
        D( "%s: could not determine current timezone\n", __FUNCTION__ );
        return NULL;
    }
Exit:
    D( "emulator: found timezone %s\n", android_timezone );
    return android_timezone;
}

#endif /* _WIN32 */

int
timezone_set(const char *tzname)
{
    int   len;
    if (!check_timezone_is_zoneinfo(tzname))
        return -1;
    len = strlen(tzname);
    if (len > (int)sizeof(android_timezone0)-1)
        return -1;
    strcpy(android_timezone0, tzname);
    android_timezone = android_timezone0;
    return 0;
}

long
android_tzoffset_in_seconds(time_t* utc_time)
{
    struct tm local = *localtime(utc_time);
    time_t local_time = mktime(&local);
    // mktime() converts struct tm according to local timezone.
    time_t gmt_time = mktime(gmtime(utc_time));
    return (long)difftime(local_time, gmt_time);
}

