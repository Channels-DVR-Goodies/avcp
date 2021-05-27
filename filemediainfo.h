//
// Created by paul on 2/16/20.
//

#ifndef AVCP_FILEMEDIAINFO_H
#define AVCP_FILEMEDIAINFO_H


typedef enum
{
    videoCodecUnknown = 0,
    videoCodecMPEG2,
    videoCodecMPEG4,
    videoCodecH264,
    videoCodecH265
} tVideoCodec;

typedef enum
{
    frameRateUnknown,
    frameRateConstant,
    frameRateVariable
} tFrameRateType;

typedef enum
{
    scanUnknown,
    scanInterlaced,
    scanProgressive
} tScanType;

typedef enum
{
    profileLevelUknown,
    profileLevelMain,
    profileLevelHigh
} tProfileLevel;

typedef enum {
    audioCodecUnknown = 0,
    audioCodecMP3,      ///< AV_CODEC_ID_MP3 preferred ID for decoding MPEG audio layer 1, 2 or 3
    audioCodecAAC,      ///< AV_CODEC_ID_AAC
    audioCodecAC3,      ///< AV_CODEC_ID_AC3
    audioCodecEAC3,     ///< AV_CODEC_ID_EAC3
    audioCodecDTS,      ///< AV_CODEC_ID_DTS
    audioCodecTrueHD    ///< AV_CODEC_ID_TRUEHD
} tAudioCodec;

typedef enum {
    layoutUnknown,
    layoutMono,
    layoutStereo,
    layout2dot1,
    layout5dot0,
    layout5dot1,
    layout7dot1
} tChannelLayout;

typedef enum {
    orientationUnknown = 0,
    orientationLandscapeWide,
    orientationLandscape,
    orientationPortrait,
    orientationPortraitTall
} tFrameOrientation;

typedef enum {
    languageEnglish,
    languageFrench,
    languageSpanish,
    languageGerman,
    languageUnknown
} tLanguage;

typedef struct fileInfo
{
    struct fileInfo * next;
    const char      * name;

    unsigned long     score;  /* to determine which is the 'best' file */

    struct timespec   duration;
    struct stat       stat;

    struct {
        struct
        {
            const char * brief;    ///> abbreviated name
            const char * full;     ///> friendly name
        } name;
        unsigned long duration;     ///> in seconds
        unsigned long bitrate;      ///> in bits per second
        struct {
            unsigned int count;
        } stream;
        struct {
            unsigned int count;
        } chapter;
    } container;

    struct {
        int                 streamIndex;
        int                 streamCount;
        unsigned long       bitrate;
        unsigned int        width;
        unsigned int        height;
        tFrameOrientation   orientation;
        unsigned int        frameRate;
        tFrameRateType      frameRateType;
        tScanType           scanType;
        struct {
            tVideoCodec   id;           ///> our tVideoCodec enum, remapped from AV_CODEC_ID
            struct
            {
                const char * brief;    ///> abbreviated name
                const char * full;     ///> friendly name
            } name;
            tProfileLevel profile;
            unsigned int  level;
        } codec;
    } video;

    struct {
        int streamIndex;
        int streamCount;
        unsigned long bitrate;         ///> in bits per second
        tLanguage     language;        ///> natural language
        struct {
            tAudioCodec id;
            struct
            {
                const char * brief;    ///> abbreviated name
                const char * full;     ///> friendly name
            } name;
        } codec;
        struct
        {
            unsigned long rate;        ///> in frames per second
            unsigned int  length;
        } sample;
        struct {
            int            count;
            tChannelLayout layout;
        } channel;
    } audio;

} tFileInfo;

/* set up the ffmpeg libraries */
int initMediaInfo( void );

/* single line summary */
void printMediaInfo( tFileInfo * file );

/* dump out the media info collected */
void dumpMediaInfo( tFileInfo * file );

/* populate the media related fields, courtesy of the ffmpeg libraries */
int processMediaInfo( tFileInfo * file );

#endif //AVCP_FILEMEDIAINFO_H
