//
// Created by paul on 2/16/20.
//

#ifndef AVCP_FILEMEDIAINFO_H
#define AVCP_FILEMEDIAINFO_H

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
    languageUnknown,
    languageEnglish,
    languageFrench,
    languageSpanish,
    languageGerman,
    languageMax
} tLanguage;

typedef struct fileInfo
{
    struct fileInfo * next;
    const char      * name;
    struct stat       stat;

    struct {
        const char *  shortName;    ///> abbreviated name
        const char *  longName;     ///> friendly name
        unsigned long duration;     ///> in seconds
        unsigned long bitrate;      ///> in bits per second
        unsigned int  streamCount;
        unsigned int  chapterCount;
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
            int           ID;
            const char *  shortName;    ///> abbreviated name
            const char *  longName;     ///> friendly name
            tProfileLevel profile;
            unsigned int  level;
        } codec;
    } video;

    struct {
        const char * shortName;     ///> abbreviated name
        const char * longName;      ///> friendly name
        int streamIndex;
        int streamCount;
        unsigned long bitrate;      ///> in bits per second
        unsigned long samplerate;   ///> in frames per second
        unsigned int  sampleLength;

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
