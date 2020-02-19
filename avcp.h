/**
	Created by Paul Chambers on 12/21/19.

	Copyright (c) 2019, Paul Chambers, All rights reserved.
*/

#ifndef AVCP_AVCP_H
#define AVCP_AVCP_H

#define _debugf( filename, lineNumber, format, ... )  \
		 fprintf( stderr, "%s:%-4d " format "\n", filename, lineNumber, ## __VA_ARGS__ )
#define debugf( format, ... ) _debugf( __FILE__, __LINE__, format, ## __VA_ARGS__ )

#define _errorf( errorNum, errorMsg, format, ... )  \
		 fprintf( stderr, "### Error: " format " (%d: %s)\n", ## __VA_ARGS__, errorNum, errorMsg )
#define errorf( format, ... ) _errorf( errno, strerror(errno), format, ## __VA_ARGS__ )

#endif //AVCP_AVCP_H
