//
// Created by paul on 2/16/20.
//

#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>

/* note: libavformat-dev is a dependency */
#include <libavformat/avformat.h>
#include <libavutil/dict.h>
#include <libavutil/error.h>

#include "avcp.h"
#include "filemediainfo.h"

const char * frameRateTypeNames[] =
                   {
                           [frameRateUnknown]  = "Unknown",
                           [frameRateConstant] = "Constant",
                           [frameRateVariable] = "Variable"
                   };

const char * scanTypeNames[] =
                   {
                           [scanUnknown]     = "Unknown",
                           [scanInterlaced]  = "Interlaced",
                           [scanProgressive] = "Progressive"
                   };

const char * profileNames[] =
                   {
                           [profileLevelUknown] = "Unknown",
                           [profileLevelMain]   = "Main",
                           [profileLevelHigh]   = "High"
                   };

const char * layoutNames[] =
                   {
                           [layoutUnknown] = "(unknown)",
                           [layoutMono]    = "1.0",
                           [layoutStereo]  = "2.0",
                           [layout2dot1]   = "2.1",
                           [layout5dot0]   = "5.0",
                           [layout5dot1]   = "5.1",
                           [layout7dot1]   = "7.1"
                   };

const char * orientationNames[] =
                   {
                           [orientationUnknown]       = "Unknown",
                           [orientationLandscapeWide] = "Landscape - wide",
                           [orientationLandscape]     = "Landscape",
                           [orientationPortrait]      = "Portrait",
                           [orientationPortraitTall]  = "Portrait - tall"
                   };

const char * languageNames[] =
                   {
                           [languageUnknown] = "(unknown)",
                           [languageEnglish] = "English",
                           [languageFrench]  = "Francais",
                           [languageSpanish] = "Espanol",
                           [languageGerman]  = "Deutsch"
                   };

const char * languageKeys[] =
                   {
                           [languageEnglish] = "eng",
                           [languageFrench]  = "fra",
                           [languageSpanish] = "spa",
                           [languageGerman]  = "ger",
                           [languageMax]     = NULL
                   };


int initMediaInfo( void )
{
    // initialise libavformat
    av_register_all();

    // Left at the default log level, there's a lot of 'noise' from probing files that otherwise seem
    // OK. AV_LOG_FATAL seems to be the minimum level required to suppress it being sent to stderr.
    av_log_set_level( AV_LOG_FATAL );

    return 0;
}

void printMediaInfo( tFileInfo * file )
{
    if ( file->container.streamCount == 0 )
    {
        printf( "%40c %s\n", ' ', file->name );
    }
    else
    {
        char scanType;
        switch ( file->video.scanType )
        {
        case scanProgressive:
            scanType = 'p';
            break;

        case scanInterlaced:
            scanType = 'i';
            break;

        default:
            scanType = ' ';
            break;
        }

        char fpsStr[24];

        unsigned int integer  = file->video.frameRate / 1000;
        unsigned int fraction = file->video.frameRate % 1000;
        if ( fraction == 0 )
        {
            snprintf( fpsStr, sizeof( fpsStr ), "%u%c", integer, scanType );
        }
        else
        {
            while ( (fraction % 10) == 0 )
            {
                fraction /= 10;
            }

            snprintf( fpsStr, sizeof( fpsStr ), "%u.%u%c", integer, fraction, scanType );
        }

        printf( "%-5.5s %4u x %-4u @ %-7s %-5.5s %-6s %s\n",
                file->video.codec.shortName,
                file->video.width,
                file->video.height,
                fpsStr,
                file->audio.shortName,
                layoutNames[ file->audio.channel.layout ],
                file->name );
    }
}

void dumpMediaInfo( tFileInfo * file )
{
    if ( file->container.streamCount == 0 )
    {
        debugf( "### not a media file" );
    }
    else
    {
        debugf( "_______________________" );
        debugf( "Container" );

        debugf( "%12s: %s",  "format",    file->container.shortName );
        debugf( "%12s: %s",  "long name", file->container.longName );
        debugf( "%12s: %lu", "bitrate",   file->container.bitrate );
        debugf( "%12s: %lu", "duration",  file->container.duration );
        debugf( "%12s: %u",  "chapters",  file->container.chapterCount );
        debugf( "%12s: %u",  "streams",   file->container.streamCount );

        debugf( "_______________________" );
        debugf( "Video" );

        debugf( "%12s: %d", "stream",    file->video.streamIndex );
        debugf( "%12s: %u", "count",     file->video.streamCount );
        debugf( "%12s: %s", "codec",     file->video.codec.shortName );
        debugf( "%12s: %s", "long name", file->video.codec.longName );

        debugf( "%12s: %s @ %d.%d", "profile", profileNames[ file->video.codec.profile ],
                file->video.codec.level / 10, file->video.codec.level % 10 );

        if ( file->video.bitrate > 0 )
        {
            debugf( "%12s: %lu", "bitrate", file->video.bitrate );
        }
        debugf( "%12s: %u x %u", "resolution", file->video.width, file->video.height );

        if ( (file->video.frameRate % 1000) == 0 )
        {
            debugf( "%12s: %u", "fps", file->video.frameRate / 1000 );
        } else {
            debugf( "%12s: %u.%03u", "fps", file->video.frameRate / 1000, file->video.frameRate % 1000 );
        }
        debugf( "%12s: %s", "scan type",   scanTypeNames[ file->video.scanType ] );
        debugf( "%12s: %s", "orientation", orientationNames[ file->video.orientation ] );


        debugf( "_______________________" );
        debugf( "Audio" );

        debugf( "%12s: %d", "stream", file->audio.streamIndex );
        debugf( "%12s: %d", "count", file->audio.streamCount );
        debugf( "%12s: %s", "codec", file->audio.shortName );
        debugf( "%12s: %s", "long name", file->audio.longName );
        debugf( "%12s: %lu", "bitrate", file->audio.bitrate );
        debugf( "%12s: %lu Hz", "sample rate", file->audio.samplerate );
        debugf( "%12s: %u", "sample bits", file->audio.sampleLength );
        debugf( "%12s: %d", "channels", file->audio.channel.count );
        debugf( "%12s: %s", "layout", layoutNames[ file->audio.channel.layout ] );
    }
}

int processMediaInfo( tFileInfo * file )
{
    int result = 0;
    AVFormatContext * formatContext = NULL;
    enum AVMediaType mediaTypesPresent[AVMEDIA_TYPE_NB];
    char             temp[256];

    char * url;
    unsigned int len = 5 + strlen( file->name ) + 1; /* add in the 'file:' and a trailing null */;
    url = malloc( len );

    result = AVERROR_BUG;
    if ( url != NULL)
    {
        snprintf( url, len, "file:%s", file->name );
        result = avformat_open_input( &formatContext, url, NULL, NULL);
        free( url );

        switch ( result )
        {
        default:
            av_strerror( result, temp, sizeof( temp ));
            debugf( "error = %x: %s", result, temp );
            break;

        case AVERROR_INVALIDDATA:
            /* ffmpeg didn't recognize the file contents, so leave everything at zero. Elsewhere we
             * use a container stream count of zero as the indication that it's not a media file */
            break;

        case 0:
            /* retrieve stream information */
            if ( avformat_find_stream_info( formatContext, NULL) < 0 )
            {
                fprintf( stderr, "Could not find stream information\n" );
                result = 1;
            }
            else
            {
                file->container.streamCount  = formatContext->nb_streams;
                file->container.chapterCount = formatContext->nb_chapters;
                file->container.bitrate      = formatContext->bit_rate;
                file->container.duration     = formatContext->duration / AV_TIME_BASE;

                if ( formatContext->iformat != NULL)
                {
                    file->container.shortName = formatContext->iformat->name;
                    file->container.longName  = formatContext->iformat->long_name;
                }

                for ( unsigned int i = 0; i < AVMEDIA_TYPE_NB; ++i )
                {
                    mediaTypesPresent[i] = 0;
                }
                for ( unsigned int streamIdx = 0;
                      streamIdx < formatContext->nb_streams;
                      streamIdx++ )
                {
                    enum AVMediaType mediaType = formatContext->streams[streamIdx]->codecpar->codec_type;
                    if ( mediaType < 0 || mediaType > AVMEDIA_TYPE_NB )
                    {
                        mediaType = AVMEDIA_TYPE_NB;
                    }
                    ++mediaTypesPresent[mediaType];
                }

                file->video.streamCount = mediaTypesPresent[AVMEDIA_TYPE_VIDEO];
                file->audio.streamCount = mediaTypesPresent[AVMEDIA_TYPE_AUDIO];

                /* * * video codec * * */

                AVCodec * videoDecoder;
                file->video.streamIndex = av_find_best_stream( formatContext, AVMEDIA_TYPE_VIDEO,
                                                               -1, -1,
                                                               &videoDecoder, 0 );

                AVStream * videoStreamContext = formatContext->streams[file->video.streamIndex];

                if ( videoDecoder != NULL && videoStreamContext != NULL)
                {
                    file->video.codec.shortName = videoDecoder->name;
                    file->video.codec.longName  = videoDecoder->long_name;

                    AVCodecContext * videoCodecContext = avcodec_alloc_context3( videoDecoder );
                    if ( videoCodecContext != NULL)
                    {
                        int ret = avcodec_parameters_to_context( videoCodecContext, videoStreamContext->codecpar );
                        if ( ret >= 0 )
                        {
                            file->video.bitrate = videoCodecContext->bit_rate;

                            file->video.codec.ID = videoCodecContext->codec_id;

                            file->video.codec.profile = profileLevelUknown;

                            if ( file->video.codec.ID != AV_CODEC_ID_NONE )
                            {
                                const char * profileName = avcodec_profile_name( file->video.codec.ID,
                                                                                 videoCodecContext->profile );
                                if ( profileName != NULL)
                                {
                                    if ( strcasecmp( profileName, "main" ) == 0 )
                                    {
                                        file->video.codec.profile = profileLevelMain;
                                    }
                                    else if ( strcasecmp( profileName, "high" ) == 0 )
                                    {
                                        file->video.codec.profile = profileLevelHigh;
                                    }

                                    file->video.codec.level = videoCodecContext->level;
                                }
                            }

                            file->video.width  = videoCodecContext->width;
                            file->video.height = videoCodecContext->height;

                            if ( file->video.width > file->video.height )
                            {
                                if (((file->video.width * 1000) / file->video.height) > 1500 )
                                {
                                    file->video.orientation = orientationLandscapeWide;
                                }
                                else
                                {
                                    file->video.orientation = orientationLandscape;
                                }
                            }
                            else if ( file->video.width < file->video.height )
                            {
                                if ( ((file->video.height * 1000) / file->video.width) > 1500 )
                                {
                                    file->video.orientation = orientationPortraitTall;
                                }
                                else
                                {
                                    file->video.orientation = orientationPortrait;
                                }
                            }
                            else
                            {
                                file->video.orientation = orientationUnknown;
                            }

                            switch ( videoCodecContext->field_order )
                            {
                            case AV_FIELD_UNKNOWN:
                                file->video.scanType = scanUnknown;
                                break;

                            case AV_FIELD_PROGRESSIVE:
                                file->video.scanType = scanProgressive;
                                break;

                            default:
                                file->video.scanType = scanInterlaced;
                                break;
                            }

                            file->video.frameRate = videoStreamContext->avg_frame_rate.num * 1000 /
                                                    videoStreamContext->avg_frame_rate.den;
                        }
                    }
                }

                /* * * audio codec * * */

                AVCodec * audioDecoder;
                file->audio.streamIndex = av_find_best_stream( formatContext, AVMEDIA_TYPE_AUDIO,
                                                               -1, -1,
                                                               &audioDecoder, 0 );
                AVStream * audioStreamContext = formatContext->streams[file->audio.streamIndex];

                if ( audioDecoder != NULL && audioStreamContext != NULL)
                {
                    file->audio.shortName = audioDecoder->name;
                    file->audio.longName  = audioDecoder->long_name;

                    AVDictionaryEntry * lang = av_dict_get( audioStreamContext->metadata,
                                                            "language", NULL, 0 );
                    if ( lang != NULL)
                    {
                        //debugf( "%12s: %s", "language", lang->value );
                    }

                    AVCodecContext * audioCodecContext = avcodec_alloc_context3( audioDecoder );
                    int ret = avcodec_parameters_to_context( audioCodecContext, audioStreamContext->codecpar );
                    if ( ret >= 0 )
                    {
                        file->audio.bitrate       = audioCodecContext->bit_rate;
                        file->audio.samplerate    = audioCodecContext->sample_rate;
                        file->audio.sampleLength  = 8 * av_get_bytes_per_sample( audioCodecContext->sample_fmt);
                        file->audio.channel.count = audioCodecContext->channels;

                        switch ( audioCodecContext->channel_layout )
                        {
                        case AV_CH_LAYOUT_MONO:
                            file->audio.channel.layout = layoutMono;
                            break;

                        case AV_CH_LAYOUT_STEREO:
                            file->audio.channel.layout = layoutStereo;
                            break;

                        case AV_CH_LAYOUT_2_1:
                            file->audio.channel.layout = layout2dot1;
                            break;

                        case AV_CH_LAYOUT_5POINT0:
                        case AV_CH_LAYOUT_5POINT0_BACK:
                            file->audio.channel.layout = layout5dot0;
                            break;

                        case AV_CH_LAYOUT_5POINT1:
                        case AV_CH_LAYOUT_5POINT1_BACK:
                            file->audio.channel.layout = layout5dot1;
                            break;

                        case AV_CH_LAYOUT_7POINT1:
                        case AV_CH_LAYOUT_7POINT1_WIDE:
                        case AV_CH_LAYOUT_7POINT1_WIDE_BACK:
                            file->audio.channel.layout = layout7dot1;
                            break;

                        default:
                            file->audio.channel.layout = layoutUnknown;
                            break;
                        }
                    }
                }
            }

            avformat_close_input( &formatContext );
            break;
        }
    }
    return result;
}