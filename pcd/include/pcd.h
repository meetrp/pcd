/*
* pcd.h
* Description:
* PCD main header file
*
* Copyright (C) 2010 Texas Instruments Incorporated - http://www.ti.com/
*
* This application is free software; you can redistribute it and/or
* modify it under the terms of the GNU Lesser General Public License
* version 2.1, as published by the Free Software Foundation.
*
* This library is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
* Lesser General Public License for more details.
*
* You should have received a copy of the GNU Lesser General Public
* License along with this library; if not, write to the Free Software
* Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA
*
* Copyright (C) 2010 PCD Project - http://www.rt-embedded.com/pcd
* 
* Change log:
* - Support dynamic configurations from kconfig
*/

/* Author:
* Hai Shalom, hai@rt-embedded.com
*
* PCD Homepage: http://www.rt-embedded.com/pcd/
* PCD Project at SourceForge: http://sourceforge.net/projects/pcd/
*
*/

#ifndef _PCD_H_
#define _PCD_H_

/***************************************************************************/
/*! \file pcd.h
 *  \brief Process control daemon main header file
****************************************************************************/

/**************************************************************************/
/*      INCLUDES                                                          */
/**************************************************************************/
#include "system_types.h"
#include "errlog.h"
#include "pcd_autoconf.h"

/**************************************************************************/
/*      INTERFACE TYPES and STRUCT Definitions                            */
/**************************************************************************/
extern bool_t verboseOutput;
extern bool_t debugMode;

/*! \def PCD_VERSION
 *  \brief PCD Version
 */
#define PCD_VERSION									"1.1.6"

/*! \def PCD_PRINT_PREFIX
 *  \brief Holds the prefix for printouts
 */
#define PCD_PRINT_PREFIX                            "pcd: "

/*! \def PCD_PRINTF_STDOUT
 *  \brief Print a message to standard output
 */
#define PCD_PRINTF_STDOUT( _format, _args... )        \
do { if( verboseOutput ) fprintf( stdout, "%s"_format "%s", PCD_PRINT_PREFIX, ##_args, ".\n" ); } while( 0 )

/*! \def PCD_PRINTF_WARNING_STDOUT
 *  \brief Print a warning message to standard output
 */
#define PCD_PRINTF_WARNING_STDOUT( _format, _args... ) \
do { if( verboseOutput ) fprintf( stdout, "%s%s"_format "%s", PCD_PRINT_PREFIX, "Warning: ", ##_args, ".\n" ); } while( 0 )

/*! \def PCD_PRINTF_STDERR
 *  \brief Print a message to standard error
 */
#define PCD_PRINTF_STDERR( _format, _args... )        \
do { char tmpLogBuffer[ CONFIG_PCD_MAX_LOG_SIZE ]; \
    snprintf( tmpLogBuffer, CONFIG_PCD_MAX_LOG_SIZE - 1, "%s%s"_format "%s", PCD_PRINT_PREFIX, "Error: ", ##_args, ".\n" ); \
	if( verboseOutput ) fprintf( stderr, "%s", tmpLogBuffer ); \
    PCD_errlog_log( tmpLogBuffer, True ); \
} while( 0 )


/*! \def CONFIG_PCD_DEBUG
 *  \brief Enable debug prints if defined
 */
#ifdef CONFIG_PCD_DEBUG
    #define PCD_DEBUG_PRINTF( format, args... )         PCD_PRINTF_STDOUT( format, ## args )
    #define PCD_FUNC_ENTER_PRINT                        PCD_PRINTF_STDOUT( "--->Entering %s", __FUNCTION__ );
    #define PCD_FUNC_EXIT_PRINT                         PCD_PRINTF_STDOUT( "<---Exiting %s",  __FUNCTION__ );
#else
    #define PCD_DEBUG_PRINTF(...)
    #define PCD_FUNC_ENTER_PRINT
    #define PCD_FUNC_EXIT_PRINT
#endif

/*!\fn PCD_main_force_iteration
 * \brief Forces process iteration right after timer tick
 */
void PCD_main_force_iteration( void );

/*!\fn PCD_main_set_self_priority
 * \brief Set the priority of the PCD in the system
 */
void PCD_main_set_self_priority( int32_t priority, int32_t policy );

#endif /* _PCD_H_ */

