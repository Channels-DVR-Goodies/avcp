/**
	Created by Paul Chambers on 12/21/19.

	Copyright (c) 2019, Paul Chambers, All rights reserved.
*/

#define _XOPEN_SOURCE 500

#include <stdlib.h>
#include <stdio.h>
//#include <stdarg.h>
#include <limits.h>
//#include <sys/types.h>
#include <sys/stat.h>
#include <string.h>
#include <strings.h>
//#include <ctype.h>
//#include <math.h>
#include <errno.h>
#include <unistd.h>
//#include <sys/param.h>
//#include <dlfcn.h>
#include <libgen.h>

#include "argtable3.h"  /* used to parse command line options */

#include "avcp.h"
#include "filemediainfo.h"

const char * gExecutableName;

static tFileInfo * gFileInfoRoot = NULL;
static tFileInfo * gTarget       = NULL;


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

#if 0
static int openCodecContext( int * streamIndexResult,
                             AVCodecContext ** decodeContext,
                             AVFormatContext * formatContext,
                             enum AVMediaType mediaType )
{
	int result, streamIndex;
	AVStream     * stream;
	AVCodec      * decodec = NULL;
	AVDictionary * options = NULL;
	const char   * mediaTypeAsString;

	mediaTypeAsString = av_get_media_type_string( mediaType );

	result = av_find_best_stream( formatContext, mediaType, -1, -1, NULL, 0 );
	if ( result < 0 )
	{
		fprintf( stderr, "Could not find %s stream\n", mediaTypeAsString );
	}
	else
	{
		streamIndex = result;
		debugf( "%s stream is index %d", mediaTypeAsString, streamIndex );

		stream  = formatContext->streams[streamIndex];
		/* find decoder for the stream */
		decodec = avcodec_find_decoder( stream->codecpar->codec_id );
		if ( decodec == NULL)
		{
			fprintf( stderr, "Failed to find %s codec\n", mediaTypeAsString );
			result = AVERROR( EINVAL );
		}
		else
		{
			/* Allocate a codec context for the decoder */
			*decodeContext = avcodec_alloc_context3( decodec );
			if ( *decodeContext == NULL)
			{
				errorf( "Failed to allocate a context for the %s codec\n", mediaTypeAsString );
				result = AVERROR( ENOMEM );
			}
			else
			{
				/* Copy codec parameters from input stream to output codec context */
				result = avcodec_parameters_to_context( *decodeContext, stream->codecpar );
				if ( result < 0 )
				{
					errorf( "Failed to copy %s codec parameters to decoder context\n", mediaTypeAsString );
				}
				else
				{
					/* Init the decoders, without reference counting */
					av_dict_set( &options, "refcounted_frames", "0", 0 );
					result = avcodec_open2( *decodeContext, decodec, &options );
					if ( result < 0 )
					{
						errorf( "Failed to open %s codec\n", mediaTypeAsString );
					}
					*streamIndexResult = streamIndex;
				}
			}
		}
	}
	return result;
}
#endif

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

#if 0
void dump_stream_format( AVFormatContext * formatContext, int streamIdx )
{
    int ret;

    debugf( "" );
    debugf( "Stream #%d", streamIdx );

    AVStream * streamContext = formatContext->streams[streamIdx];

    AVDictionaryEntry * lang = av_dict_get( streamContext->metadata, "language", NULL, 0 );
    if ( lang )
    {
        debugf( "lang: %s", lang->value );
    }

    AVCodecContext * codecContext = avcodec_alloc_context3(NULL);
    if ( codecContext != NULL )
    {
        ret = avcodec_parameters_to_context( codecContext, streamContext->codecpar );
        if ( ret >= 0 )
        {
            const char * profile    = avcodec_profile_name( codecContext->codec_id, codecContext->profile );

            if (profile != NULL)
            {
                debugf( "   profile: %s", profile );
            }
            switch ( codecContext->codec_type )
            {
            case AVMEDIA_TYPE_VIDEO:
                debugf( "%dx%d", codecContext->width, codecContext->height );
                if ( codecContext->coded_width != 0 )
                {
                    debugf( "Coded: %dx%d", codecContext->coded_width, codecContext->coded_height );
                }
                int fps = streamContext->avg_frame_rate.den && streamContext->avg_frame_rate.num;
                if ( fps )
                {
                    print_fps( av_q2d( streamContext->avg_frame_rate ), "fps" );
                }
                break;

            case AVMEDIA_TYPE_AUDIO:
                debugf( "%d Hz", codecContext->sample_rate );
                break;

            default:
                break;
            }

//            avcodec_string( buf, sizeof( buf ), codecContext, 0 );
        }

        avcodec_free_context( &codecContext );
    }


//    debugf( ">>> %s", buf );

/*    if ( st->sample_aspect_ratio.num &&
         av_cmp_q( st->sample_aspect_ratio, st->codecpar->sample_aspect_ratio ))
    {
        AVRational display_aspect_ratio;
        av_reduce( &display_aspect_ratio.num, &display_aspect_ratio.den,
                   st->codecpar->width * (int64_t) st->sample_aspect_ratio.num,
                   st->codecpar->height * (int64_t) st->sample_aspect_ratio.den,
                   1024 * 1024 );
        debugf( "SAR %d:%d DAR %d:%d",
               st->sample_aspect_ratio.num, st->sample_aspect_ratio.den,
               display_aspect_ratio.num, display_aspect_ratio.den );
    }
    */
}
#endif

int processFile( const char * filename )
{
    int result = -1;
    //fprintf(stderr,"-----\n");
    //debugf( "filename: \'%s\'", filename );

    tFileInfo * file = calloc( 1, sizeof(tFileInfo) );
    if ( file != NULL )
    {
        file->next = NULL;
        file->name = filename;
        if ( stat( filename, &file->stat ) == 0 )
        {
            result = 0;
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

        if ( S_ISREG(file->stat.st_mode ) )
        {
            processMediaInfo( file );
            printMediaInfo( file );
            // dumpMediaInfo( file );

            tFileInfo * p = gFileInfoRoot;
            while ( p != NULL && p->next != NULL )
            {
                p = p->next;
            }
            if ( p != NULL )
            {
                p->next = file;
            }
        }
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
