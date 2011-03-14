/*
 * errlog.c
 * Description:
 * PCD error logger implementation file
 *
 * Copyright (C) 2010 Texas Instruments Incorporated - http://www.ti.com/
 *
 * This application is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public License
 * version 2.1, as published by the Free Software Foundation.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 *
 */

/* Author:
 * Hai Shalom, hai@rt-embedded.com 
 *
 * PCD Homepage: http://www.rt-embedded.com/pcd/
 * PCD Project at SourceForge: http://sourceforge.net/projects/pcd/
 *  
 */

/**************************************************************************/
/*      INCLUDES                                                          */
/**************************************************************************/
#include <unistd.h>
#include <malloc.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <pty.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <time.h>
#include "errlog.h"
#include "pcd.h"

#define PCD_ERRLOG_MIN_FREE_SIZE        ( ( PCD_ERRLOG_MAX_FILE_SIZE * 3 ) / 4 )

static int32_t fd = -1;
static char *logFilename = NULL;

PCD_status_e PCD_errlog_init( char *logFile )
{
    if ( !logFile )
    {
        return PCD_STATUS_BAD_PARAMS;
    }

    /* Allocate memory for filename */
    logFilename = malloc( strlen( logFile ) + 1 );

    if ( !logFilename )
    {
        return PCD_STATUS_NOK;
    }

    /* Save filename locally */
    strcpy( logFilename, logFile );

    PCD_DEBUG_PRINTF( "Error log filename: %s", logFilename );

    return PCD_STATUS_OK;
}

static PCD_status_e PCD_errlog_open_file( void )
{
    off_t off;

    /* Open it */
    if ( ( fd = open( logFilename, O_WRONLY | O_NONBLOCK | O_APPEND | O_CREAT | O_SYNC, S_IRWXU | S_IRWXG ) ) < 0 )
    {
        PCD_DEBUG_PRINTF( "Open log file failed: %s", logFilename );
        return PCD_STATUS_NOK;
    }

    /* Get current file size */
    off = lseek( fd, 0, SEEK_END );

    if ( off < 0 )
    {
        PCD_DEBUG_PRINTF( "lseek 1 log file failed: %s", logFilename );
        return PCD_STATUS_NOK;
    }

    /* Check if we are over the limit */
    if ( off >= PCD_ERRLOG_MAX_FILE_SIZE )
    {
        char tempBuffer[ PCD_ERRLOG_MAX_FILE_SIZE ];
        ssize_t bytes;

        close(fd);
        if ( ( fd = open( logFilename, O_RDWR | O_NONBLOCK | O_CREAT | O_SYNC, S_IRWXU | S_IRWXG ) ) < 0 )
        {
            PCD_DEBUG_PRINTF( "Open log file failed: %s", logFilename );
            return PCD_STATUS_NOK;
        }

        /* Move past oldest 25% of the file */
        if ( ( off = lseek( fd, PCD_ERRLOG_MAX_FILE_SIZE - PCD_ERRLOG_MIN_FREE_SIZE, SEEK_SET ) ) < 0 )
        {
            PCD_DEBUG_PRINTF( "lseek 2 log file failed: %s", logFilename );
            return PCD_STATUS_NOK;
        }

        /* Read newest 75% of the file */
        bytes = read( fd, tempBuffer, sizeof( tempBuffer ) );

        if ( bytes > 0 )
        {
            /* Truncate the file, erase it if fails */
            if( ftruncate( fd, PCD_ERRLOG_MIN_FREE_SIZE ) < 0 )
			{
				close(fd);		
				unlink( logFilename );				
			}
			else
			{
				if ( ( off = lseek( fd, 0, SEEK_SET ) ) < 0 )
				{
					PCD_DEBUG_PRINTF( "lseek 2 log file failed: %s", logFilename );
					return PCD_STATUS_NOK;
				}

				/* Write the data in the start of the file */
				bytes = write( fd, tempBuffer, bytes );

				close(fd);
			}
        }
        else
        {
			/* In case we failed to read the file due to flash corruption, remove it */            
			close(fd);		
            unlink( logFilename );
        }

        if ( ( fd = open( logFilename, O_WRONLY | O_NONBLOCK | O_APPEND | O_CREAT | O_SYNC, S_IRWXU | S_IRWXG ) ) < 0 )
        {
            PCD_DEBUG_PRINTF( "Open log file failed: %s", logFilename );
            return PCD_STATUS_NOK;
        }

    }
    return PCD_STATUS_OK;
}

static PCD_status_e PCD_errlog_close_file( void )
{
    if ( fd > 0 )
    {
        /* Close FIFO */
        close( fd );
        fd = -1;
    }

    return PCD_STATUS_OK;
}

PCD_status_e PCD_errlog_close( void )
{
    PCD_errlog_close_file();

    if ( logFilename )
    {
        free( logFilename );
        logFilename = NULL;
    }

    return PCD_STATUS_OK;
}

void PCD_errlog_log( char *buffer, bool_t timeStamp )
{
    char *timeBuf = NULL;
    struct timeval time;

    if ( !logFilename )
    {
        PCD_DEBUG_PRINTF( "Logfilename is NULL!" );
        return;
    }

    /* Open file for writing */
    if ( PCD_errlog_open_file() != PCD_STATUS_OK )
    {
        PCD_DEBUG_PRINTF( "Cannot open log file: %s", logFilename );
        return;
    }

    if ( timeStamp )
    {
        /* Add timestamp */
        gettimeofday(&time, NULL);

        if ( ( timeBuf = ctime( &time.tv_sec ) ) != NULL )
        {
            u_int32_t len = strlen( timeBuf );

            timeBuf[ len - 1 ] = ' ';
            if( write( fd, timeBuf, len ) <= 0 )
				goto close_file;
        }

        /* Ignore the "pcd: " prefix */
        buffer += strlen( PCD_PRINT_PREFIX );
    }

    /* Add the error log, check return value to avoid warnings */
    if( write( fd, buffer, strlen( buffer ) ) <= 0 )
		goto close_file;

close_file:
	/* Close file (we want to avoid FFS corruption) */
    PCD_errlog_close_file();
}

