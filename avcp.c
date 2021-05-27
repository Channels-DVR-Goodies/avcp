/**
	Created by Paul Chambers on 12/21/19.

	Copyright (c) 2019, Paul Chambers, All rights reserved.
*/

#define _XOPEN_SOURCE 500

#include <stdlib.h>
#include <stdio.h>
#include <limits.h>
#include <sys/stat.h>
#include <string.h>
#include <strings.h>
#include <time.h>
#include <errno.h>
#include <unistd.h>
#include <libgen.h>

#include "argtable3.h"  /* used to parse command line options */

#include "avcp.h"
#include "filemediainfo.h"

const char * gExecutableName;

static tFileInfo * gFileInfoRoot = NULL;
static tFileInfo * gTarget       = NULL;

typedef enum { lsmode, lnmode, cpmode } tAppMode;

/* global arg_xxx structs */
struct {
    const char * myName;
    tAppMode     mode;
    struct arg_lit  * help;
    struct arg_lit  * version;
    struct arg_lit  * link;
    struct arg_lit  * delete;
    struct arg_file * config;
    struct arg_file * target;
    struct arg_file * file;
    struct arg_end  * end;
} gOption;

/**
 * @brief recurive function that creates any missing directories in the path provided
 * @param path
 */
static int _recurseConfig( const char * path )
{
	int    result = 0;
	char   temp[PATH_MAX];
	struct stat pathStat;

	if ( stat( path, &pathStat ) != 0 )
	{
		switch (errno)
		{
		case ENOENT: /* the directory is missing */
			/* dirname may modify its argument, so use a local copy */
			strncpy( temp, path, sizeof(temp) );
			_recurseConfig( dirname(temp) ); /* pop up one level, and try again */

			/* as we unwind, add one level of directory on each turn */
			debugf( "mkdir \'%s\'", path );
			if ( mkdir( path, S_IRWXU | S_IRWXG ) != 0 )
			{
				errorf( "Unable to create \'%s\'", path );
				result = errno;
			}
			break;

		default: /* any other error is unexpected, so return it */
			errorf( "Path \'%s\' can't be accessed", path );
			result = errno;
			break;
		}
	}
	/* if the path can now be stat'd without error, it must exist, so return */
	return result;
}

int checkTarget( const char * filename )
{
    int  result = 0;

    debugf( "target: %s", filename );
    gTarget = calloc( 1, sizeof(tFileInfo) );
    if ( gTarget != NULL )
    {
        result = 0;

        gTarget->next = NULL;
        gTarget->name = filename;

        if ( stat( filename, &gTarget->stat ) == 0 )
        {
            /* file exists, see if we can write to it */
            result = access( filename, W_OK );
            if ( result != 0 )
            {
                errorf( "unable to write to \'%s\'", filename );
            }
        }
        else
        {
	        char temp[PATH_MAX];

	        /* can't confirm the file exists - investigate further */
            switch (errno)
            {
            case ENOENT: /* the path or file doesn't exist. */
	            /* dirname may modify its argument, so make a copy first */
	            strncpy( temp, filename, sizeof( temp ));

	            result = _recurseConfig( dirname( temp ));
                /* at this point, either the path already existed, or we just
                 * created it, though there may not be an actual file yet */
                break;

            default:
                errorf( "unable to get info about \'%s\'", filename );
				result = errno;
                break;
            }
        }
    }
    return result;
}


int processFile( const char * filename )
{
    int result = -1;

    struct timespec start, stop;

    tFileInfo * file = calloc( 1, sizeof(tFileInfo) );
    if ( file != NULL )
    {
        clock_gettime( CLOCK_REALTIME, &start );

        file->next = NULL;
        file->name = filename;
        if ( stat( filename, &file->stat ) == 0 )
        {
            result = 0;
        }
        else
        {
            switch ( errno )
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

        if ( S_ISREG( file->stat.st_mode ))
        {
            processMediaInfo( file );
            // printMediaInfo( file );
            // dumpMediaInfo( file );

            tFileInfo * p = gFileInfoRoot;
            if ( p == NULL)
            {
                gFileInfoRoot = file;
            }
            else
            {
                while ( p != NULL && p->next != NULL)
                {
                    p = p->next;
                }
                if ( p != NULL)
                {
                    p->next = file;
                }
            }
        }

        clock_gettime( CLOCK_REALTIME, &stop );
        if ( stop.tv_nsec < start.tv_nsec )
        {
            /* add a second to the nano part */
            stop.tv_nsec += 1000000000;
            /* and subtract it from the seconds part */
            /* we only get here if stop.tv_nsec > start.tv_nsec */
            stop.tv_sec  -= 1;
        }

        file->duration.tv_nsec = stop.tv_nsec - start.tv_nsec;
        file->duration.tv_sec  = stop.tv_sec  - start.tv_sec;
    }
    return result;
}

int main( int argc, char *argv[] )
{
    int result = 0;

    initMediaInfo();

    gOption.myName = strrchr( argv[0], '/' );
    /* If we found a slash, increment past it. If there's no slash, point at the full argv[0] */
    if ( gOption.myName++ == NULL)
    {
        gOption.myName = argv[0];
    }

    if (strcmp(gOption.myName, "avln") == 0)
    {
        gOption.mode = lnmode;
    } else if ( strcmp( gOption.myName, "avcp" ) == 0 )
    {
        gOption.mode = cpmode;
    } else {
        gOption.mode = lsmode;
    }

    /* the global arg_xxx structs above are statically initialised within argtable */
    void * argtable[] =
    {
        gOption.help    = arg_litn(NULL, "help", 0, 1, "display this help (and exit)" ),

        gOption.version = arg_litn(NULL, "version", 0, 1, "display version info (and exit)" ),

        gOption.link    = arg_litn( "l", "link", 0, 1, "hard-link the files, instead of copying them." ),

        gOption.delete  = arg_litn( "d", "delete", 0, 1, "remove the files that didn't win" ),

        gOption.target = arg_filen( "t", "target", "<file>", 0, 1,
                                "specify a destination file." ),

        gOption.config = arg_filen( "c", "config", "<config file>", 0, 1,
                                    "the configuration file controls what is considered 'better' quality." ),

        gOption.file = arg_filen(NULL, NULL, "<file>", 1, 999, "input files" ),

        gOption.end = arg_end( 20 )
    };

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
        const char * target = NULL;
        int count = gOption.file->count;

        if ( gOption.link->count > 0 )
        {
            gOption.mode = lnmode;
        }

        /* ls mode doesn't use a target */
        if ( gOption.mode == lsmode )
        {
            if ( gOption.target->count > 0 )
            {
                fprintf(stderr, "Error: %s- the -t option is not compatible with ls mode\n", gOption.myName);
            }
        } else {
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
        }

        for ( int i = 0; i < count && result == 0; i++ )
        {
            result = processFile( gOption.file->filename[i] );
        }

        tFileInfo * file = gFileInfoRoot;
        if (gOption.mode == lsmode )
        {
            while ( file != NULL)
            {
                printMediaInfo( file );
                // dumpMediaInfo( file );

                file = file->next;
            }
        }
        else
        {
            /* cpmode and lnmode only differ in the linking vs. copying choice */
        }
    }

    /* release each non-null entry in argtable[] */
    arg_freetable( argtable, sizeof(argtable) / sizeof(argtable[0]) );

    return result;
}
