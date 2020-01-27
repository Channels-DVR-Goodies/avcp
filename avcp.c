/**
	Created by Paul Chambers on 12/21/19.

	Copyright (c) 2019, Paul Chambers, All rights reserved.
*/

#define _XOPEN_SOURCE 500

#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>
#include <limits.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <string.h>
#include <strings.h>
#include <ctype.h>
#include <math.h>
#include <errno.h>
#include <unistd.h>
#include <sys/param.h>
#include <dlfcn.h>
#include <libgen.h>

#include <MediaInfoDLL/MediaInfoDLL.h>

#include "argtable3.h"  /* used to parse command line options */

#include "avcp.h"

const char * gExecutableName;

/* global arg_xxx structs */
struct
{
    const char * myName;
    struct arg_lit  * help;
    struct arg_lit  * version;
    struct arg_lit  * delete;
    struct arg_file * config;
    struct arg_file * target;
    struct arg_file * file;
    struct arg_end  * end;
} gOption;

typedef void * tMediaInfoHandle;

typedef struct fileInfo
{
    struct fileInfo * next;
    const char * name;
    struct stat stat;
} tFileInfo;

static tFileInfo * gFileInfoRoot = NULL;
static tFileInfo * gTarget = NULL;

struct
{
    unsigned int count;
} gStream[MediaInfo_Stream_Max];

typedef enum
{
    frameRateUnknown,
    frameRateConstant,
    frameRateVariable
} tFrameRateType;

const char * frameRateTypeNames[] =
{
    [frameRateUnknown]  = "Unknown",
    [frameRateConstant] = "Constant",
    [frameRateVariable] = "Variable"
};

typedef enum
{
    scanUnknown,
    scanInterlaced,
    scanProgressive
} tScanType;

const char * scanTypeNames[] =
{
    [scanUnknown]     = "Unknown",
    [scanInterlaced]  = "Interlaced",
    [scanProgressive] = "Progressive"
};

typedef enum {
    profileLevelUknown,
    profileLevelMain,
    profileLevelHigh
} tProfileLevel;

const char * profileLevelNames[] = {
    [profileLevelUknown] = "Unknown",
    [profileLevelMain]   = "Main",
    [profileLevelHigh]   = "High"
}

typedef float tProfileVersion;

typedef struct {
    const char *    string;
    tProfileLevel   level;
    tProfileVersion version;
} tCodecProfile;

const char * gStreamTypeNames[MediaInfo_Stream_Max] =
{
    [MediaInfo_Stream_General] = "General",
    [MediaInfo_Stream_Video]   = "Video",
    [MediaInfo_Stream_Audio]   = "Audio",
    [MediaInfo_Stream_Text]    = "Text",
    [MediaInfo_Stream_Other]   = "Other",
    [MediaInfo_Stream_Image]   = "Image",
    [MediaInfo_Stream_Menu]    = "Menu"
};

const char * GetInfoString( tMediaInfoHandle mediaHandle, MediaInfo_stream_C streamType, int stream, const char * parameter )
{
    const char * result = MediaInfo_Get( mediaHandle, streamType, stream, parameter, MediaInfo_Info_Text, MediaInfo_Info_Name );
    debugf( "%24s: %s", parameter, result );
    return result;
}

unsigned int GetInfoUInt( tMediaInfoHandle mediaHandle, MediaInfo_stream_C streamType, int stream, const char * parameter )
{
    unsigned int result = 0;
    const char * param = GetInfoString( mediaHandle, streamType, stream, parameter );
    if ( param != NULL )
    {
        if ( sscanf( param, "%u", &result ) != 1 )
        {
            result = 0;
        }
    }

    return result;
}

float GetInfoFloat( tMediaInfoHandle mediaHandle, MediaInfo_stream_C streamType, int stream, const char * parameter )
{
    float result = 0;
    const char * param = MediaInfo_Get( mediaHandle, streamType, stream, parameter, MediaInfo_Info_Text, MediaInfo_Info_Name );
    if ( param != NULL )
    {
        if ( sscanf( param, "%f", &result ) != 1 )
        {
            result = 0;
        }
        debugf( "%24s: %2.3f (%ld)", parameter, result, lrintf( result ) );
    }

    return result;
}

tFrameRateType GetInfoFrameRateType( tMediaInfoHandle mediaHandle, int stream )
{
    tFrameRateType result = frameRateUnknown;
    /* VFR or CFR */
    const char * frameRateType = MediaInfo_Get( mediaHandle, MediaInfo_Stream_Video, stream, "FrameRate_Mode", MediaInfo_Info_Text, MediaInfo_Info_Name );
    if ( strcasecmp( frameRateType, "VFR" ) == 0 )
        result = frameRateVariable;
    if ( strcasecmp( frameRateType, "CFR" ) == 0 )
        result = frameRateConstant;

    debugf("%24s: %s", "Frame Rate Type", frameRateTypeNames[result] );

    return result;
}

tScanType GetInfoScanType( tMediaInfoHandle mediaHandle, int stream )
{
    tScanType result = scanUnknown;
    const char * scanType = MediaInfo_Get( mediaHandle, MediaInfo_Stream_Video, stream, "ScanType", MediaInfo_Info_Text, MediaInfo_Info_Name );
    if ( strcasecmp( scanType, "Progressive" ) == 0 )
        result = scanProgressive;
    else if ( strcasecmp( scanType, "Interlaced" ) == 0 )
        result = scanInterlaced;

    debugf("%24s: %s", "Scan Type", scanTypeNames[result] );

    return result;
}



int extractGeneralInfo( tMediaInfoHandle mediaHandle, unsigned int count )
{
    int result = -1;

    if (count == 1)
    {
        const char * format;
        format = GetInfoString( mediaHandle, MediaInfo_Stream_General, 0, "Format" );

        result = 0;
    }
    return result;
}

int extractVideoInfo( tMediaInfoHandle mediaHandle, unsigned int count )
{
    int result = 0;
    unsigned int  width = 0;
    unsigned int height = 0;
    tFrameRateType frameRateType = 0;
    float framerate = 0.0;
    tScanType scanType = scanUnknown;

    for (unsigned int stream = 0; stream < count; stream++ )
    {
        const char * format;
        /* codec */
        format = GetInfoString( mediaHandle, MediaInfo_Stream_Video, stream, "Format" );
        format = GetInfoString( mediaHandle, MediaInfo_Stream_Video, stream, "Format_Profile" );

        /* 1920, 1280, 720, etc. */
        width = GetInfoUInt( mediaHandle, MediaInfo_Stream_Video, stream, "Width" );
        /* 480, 720, 1080, etc. */
        height = GetInfoUInt( mediaHandle, MediaInfo_Stream_Video, stream, "Height" );

        /* VFR or CFR */
        frameRateType = GetInfoFrameRateType( mediaHandle, stream );
        /* 23.976 */
        framerate = GetInfoFloat( mediaHandle, MediaInfo_Stream_Video, stream, "FrameRate" );

        /* Progressive, Interlaced */
        scanType = GetInfoScanType( mediaHandle, stream );
    }
    return result;
}

int extractAudioInfo( tMediaInfoHandle mediaHandle, unsigned int count )
{
    int result = 0;

    for (unsigned int stream = 0; stream < count; stream++ )
    {
        debugf( "    Stream %d", stream );

        const char * format;
        format = GetInfoString( mediaHandle, MediaInfo_Stream_Audio, stream, "Format" );
        format = GetInfoString( mediaHandle, MediaInfo_Stream_Audio, stream, "Format_Profile" );
        format = GetInfoString( mediaHandle, MediaInfo_Stream_Audio, stream, "Matrix_Audio" );
        format = GetInfoString( mediaHandle, MediaInfo_Stream_Audio, stream, "Channel(s)" );
        format = GetInfoString( mediaHandle, MediaInfo_Stream_Audio, stream, "Matrix_Channel(s)" );
    }
    return result;
}

int extractMediaInfo( tMediaInfoHandle mediaHandle )
{
    int result = 0;

    for ( MediaInfo_stream_C streamType = MediaInfo_Stream_General; streamType < MediaInfo_Stream_Max; streamType++ )
    {
        gStream[ streamType ].count = MediaInfo_Count_Get( mediaHandle, streamType, -1 );
        if ( gStream[ streamType ].count > 0 )
        {
            debugf( "### %8s: %u", gStreamTypeNames[ streamType ], gStream[ streamType ].count );

            switch ( streamType )
            {
            case MediaInfo_Stream_General:
                result = extractGeneralInfo( mediaHandle, gStream[ MediaInfo_Stream_General ].count );
                break;

            case MediaInfo_Stream_Video:
                result = extractVideoInfo( mediaHandle, gStream[ MediaInfo_Stream_Video ].count );
                break;

            case MediaInfo_Stream_Audio:
                result = extractAudioInfo( mediaHandle, gStream[ MediaInfo_Stream_Audio ].count );
                break;

            case MediaInfo_Stream_Text:
            case MediaInfo_Stream_Other:
            case MediaInfo_Stream_Image:
            case MediaInfo_Stream_Menu:
                break;

            default:
                errorf("unknown stream type");
                result = -1;
                break;
            }
        }
    }
    return result;
}

/**
 * @brief Check a path, and create it if missing
 *
 * FixMe: create a missing path without depending on gnu extensions to realpath
 */

int makePath( const char * directory )
{
    int  result = 0;
    char path[MAXPATHLEN];

    /* Perhaps part of the path leading to the file is missing - create it if so. */
    path[0] = '\0';

    while ( realpath( directory, path ) == NULL )
    {
        switch (errno)
        {
        case ENOENT:
            /* gnu'ism - path will be populated up to (and including) the first missing element.
               If the first character is still a null, then it's not the gnu version of realpath() */
            if ( path[0] != '\0' )
            {
                debugf( "mkdir: \'%s\'", path );
                result = mkdir( path, S_IRWXU | S_IRWXG );
                if ( result != 0 )
                {
                    errorf( "unable to create directory \'%s\'", path );
                }
            }
            else
            {
                /* non-GNU implementations leave path untouched
                   on error, so this approach won't work */
                return errno;
            }
            break;

        default:
            errorf( "error in realpath" );
            return errno;
        }
    }
    /* at this point, either the path already existed, or we
       just created it. but there's still no actual file */
    debugf( "realpath: \'%s\'", path );
    return result;
}

int checkTarget( const char * filename )
{
    int  result = 0;
    char dir[MAXPATHLEN];

    debugf( "target: %s", filename );
    gTarget = calloc( 1, sizeof(tFileInfo) );
    if ( gTarget != NULL )
    {
        result = 0;

        gTarget->next = NULL;
        gTarget->name = filename;

        if ( stat( filename, &gTarget->stat ) == 0 )
        {
            tMediaInfoHandle mediaHandle = MediaInfo_New();
            MediaInfo_Open( mediaHandle, filename );
            extractMediaInfo( mediaHandle );
            MediaInfo_Delete( mediaHandle );

            /* file exists, see if we can write to it */
            result = access( filename, W_OK );
            if ( result != 0 )
            {
                errorf( "unable to write to \'%s\'", filename );
            }
        }
        else
        {
            /* can't confirm the file exists - investigate further */
            switch (errno)
            {
            case ENOENT: /* the path or file doesn't exist. */
                /* dirname() may modify the string passed to it, so pass it a
                   copy. strdup would be easier, but that would leak memory. */
                strncpy( dir, filename, sizeof(dir) );
                char * directory = dirname( dir );
                result = makePath( directory );
                /* at this point, either the path already existed, or we
                   just created it, but there's still no actual file */
                break;

            default:
                errorf( "unable to get info about \'%s\'", filename );
                break;
            }
        }
    }
    return result;
}

int processFile( const char * filename )
{
    int result = -1;
    debugf( "filename: \'%s\'", filename );

    tFileInfo * file = calloc( 1, sizeof(tFileInfo) );
    if ( file != NULL )
    {
        file->next = NULL;
        file->name = filename;
        if ( stat( filename, &file->stat ) == 0 )
        {
            result = 0;
            tMediaInfoHandle mediaHandle = MediaInfo_New();
            MediaInfo_Open( mediaHandle, filename );
            extractMediaInfo( mediaHandle );
            MediaInfo_Delete( mediaHandle );
        }
        else
        {
            switch (errno)
            {
            case ENOENT:
                errorf( "file \'%s\' is missing\n", filename );
                result = 0;
                break;

            default:
                errorf( "unable to get info about \'%s\'", filename );
                break;
            }
        }
        tFileInfo * p = gFileInfoRoot;
        while (p != NULL && p->next != NULL)
        {
            p = p->next;
        }
        if (p != NULL)
        {
            p->next = file;
        }
    }
    return result;
}

int main( int argc, char *argv[] )
{
    int result = 0;

    gOption.myName = strrchr( argv[0], '/' );
    /* If we found a slash, increment past it. If there's no slash, point at the full argv[0] */
    if ( gOption.myName++ == NULL)
    {
        gOption.myName = argv[0];
    }

    /* the global arg_xxx structs above are statically initialised within argtable */
    void * argtable[] =
    {
        gOption.help    = arg_litn( NULL, "help", 0, 1, "display this help (and exit)" ),

        gOption.version = arg_litn( NULL, "version", 0, 1, "display version info (and exit)" ),

        gOption.delete  = arg_litn( "l", "link", 0, 1, "hard-link the files, instead of copying them." ),

        gOption.target  = arg_filen( "t", "target", "<file>", 0, 1, "specify a destination file." ),

        gOption.config  = arg_filen( "c", "config", "<config file>", 0, 1,
        "the configuration file controls what is considered 'better' quality." ),

        gOption.file    = arg_filen( NULL, NULL, "<file>", 1, 999, "input files" ),

        gOption.end     = arg_end( 20 )
    };

    MediaInfoDLL_Load();

    int nerrors = arg_parse( argc, argv, argtable );

    if ( gOption.help->count > 0 ) 	/* special case: '--help' takes precedence over everything else */
    {
        fprintf( stdout, "Usage: %s", gExecutableName );
        arg_print_syntax( stdout, argtable, "\n" );
        fprintf( stdout, "process hash file into a header file.\n\n" );
        arg_print_glossary( stdout, argtable, "  %-32s %s\n" );
        fprintf( stdout, "\n" );

        result = 0;
    }
    else if ( gOption.version->count > 0 )   /* ditto for '--version' */
    {
        fprintf( stdout, "%s, version %s\n", gExecutableName, "(work in progress)" );

        result = 0;
    }
    else if (nerrors > 0) 	/* If the parser returned any errors then display them and exit */
    {
        /* Display the error details contained in the arg_end struct.*/
        arg_print_errors( stdout, gOption.end, gOption.myName );
        fprintf( stdout, "Try '%s --help' for more information.\n", gOption.myName );
        result = 1;
    }
    else
    {
        int count = gOption.file->count;
        const char * target;

        if ( gOption.target->count > 0 )
        {
            /* the destination file was provided explicitly by the user */
            target = gOption.target->filename[0];
        }
        else
        {
            /* the destination file is the last one in the list */
            --count;
            target = gOption.file->filename[count];
        }

        result = checkTarget( target );

        for ( int i = 0; i < count && result == 0; i++ )
        {
            result = processFile( gOption.file->filename[i] );
        }
    }

    /* release each non-null entry in argtable[] */
    arg_freetable( argtable, sizeof(argtable) / sizeof(argtable[0]) );

    return result;
}
