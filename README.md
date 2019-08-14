# avcp

Smart copy of A/V files, so a 'better quality' version won't be overwritten by a poorer quality one.

Particularly useful to use with ChanDVR2Plex.

## Usage

    avcp [-d] [-i <source file>] <destination file(s)>

    -i   Examine the source file to determine its 'quality' metadata, then compare it with the quality of
         each of the destination files. If one is higher quality than the source file, then hardlink it
         into the destination directory. If there is a file of the same name in the destination, remove it
         first. If the source and destination are on different filesystems, copy the file instead.
         
    -d   delete the destination file(s):
           a) if they are lower quality than the source file (if -i is present), or
           b) except the one with the highest quality (no -i given)
         in other words, delete any except the 'best quality' one.
         
    -c   specify a configuration file. This specifies the classification and priority ordering of
         different combinations of media attributes.
    
         
## Use with ChanDVR2Plex

This tool was originally written to compliment ChanDVR2Plex, a tool for figuring out where an external
media file (e.g. a media file from Channels DVR, KMTTG, TVMosaic, Tablo, etc.) should be put in a Plex
filesystem hierarchy. These tools generally manage their own file heirarchy, which is different from
the Plex one.

Since series name formatting varies between tools (and guide data providers), ChanDVR2Plex provides
some 'smarts' that helps it figure out that 'Marvels.Agents.of.S.H.I.E.L.D S04E13' should go in the
'Season 04' folder inside the 'Marvel's Agents of S.H.I.E.L.D. (2013)' folder in the Plex heirarchy.

ChanDVR2Plex mostly focuses on source filename anaylsis and fuzzy matching to populate a template for a
shell command to execute.

avcp is one possible shell command, one that is smart enough not to 'downgrade' an existing file by
overwriting it with a new recording that is lower quality (e.g. replacing a 720p/5.1 file with a
480p/2.0 one).
