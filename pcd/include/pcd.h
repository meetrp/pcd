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

/**************************************************************************/
/*      INTERFACE TYPES and STRUCT Definitions                            */
/**************************************************************************/
extern Bool verboseOutput;
extern Bool debugMode;

/*! \def PCD_VERSION
 *  \brief PCD Version
 */
#define PCD_VERSION									"1.0.4"

/*! \def PCD_PRINT_PREFIX
 *  \brief Holds the prefix for printouts
 */
#define PCD_PRINT_PREFIX                            "pcd: "

/*! \def PCD_PRINTF_STDOUT
 *  \brief Print a message to standard output
 */
#define PCD_PRINTF_STDOUT( format, args... )        if( verboseOutput ) fprintf( stdout, PCD_PRINT_PREFIX format ".\n", ## args )

/*! \def PCD_PRINTF_WARNING_STDOUT
 *  \brief Print a warning message to standard output
 */
#define PCD_PRINTF_WARNING_STDOUT( format, args... )        if( verboseOutput ) fprintf( stdout, PCD_PRINT_PREFIX "Warning: " format ".\n", ## args )

/*! \def PCD_PRINTF_STDERR
 *  \brief Print a message to standard error
 */
#define PCD_PRINTF_STDERR( format, args... )        \
{   Char tmpLogBuffer[ PCD_MAX_LOG_SIZE ]; \
    sprintf( tmpLogBuffer, PCD_PRINT_PREFIX "Error: " format ".\n", ## args ); \
    if( verboseOutput ) fprintf( stderr, tmpLogBuffer ); \
    PCD_errlog_log( tmpLogBuffer, True ); \
}

/*! \def CONFIG_TI_PCD_DEBUG
 *  \brief Enable debug prints if defined
 */
#ifdef CONFIG_TI_PCD_DEBUG
    #define PCD_DEBUG_PRINTF( format, args... )         PCD_PRINTF_STDOUT( format, ## args )
    #define PCD_FUNC_ENTER_PRINT                        PCD_PRINTF_STDOUT( "--->Entering %s", __FUNCTION__ );
    #define PCD_FUNC_EXIT_PRINT                         PCD_PRINTF_STDOUT( "<---Exiting %s",  __FUNCTION__ );
#else
    #define PCD_DEBUG_PRINTF(...)
    #define PCD_FUNC_ENTER_PRINT
    #define PCD_FUNC_EXIT_PRINT
#endif

/*! \def PCD_MAX_PARAM_SIZE
 *  \brief Maximum size of the process parameters
 */
#define PCD_MAX_PARAM_SIZE      256

/*! \def PCD_MAX_LOG_SIZE
 *  \brief Maximum size of a log message
 */
#define PCD_MAX_LOG_SIZE        256

/*! \def PCD_PRIORITY
 *  \brief Define PCD priority in the system
 */
#define PCD_PRIORITY        1

/*! \def PCD_OWNER_ID
 *  \brief Define PCD owner ID
 */
#ifndef PCD_OWNER_ID
    #define PCD_OWNER_ID        3085
#endif /* PCD_OWNER_ID */

/*! \def PCD_SERVER_NAME
 *  \brief Define PCD server socket name
 */
#define PCD_SERVER_NAME "pcd-server"

/*! \def PCD_CLIENTS_NAME_PREFIX
 *  \brief Define PCD clients socket name prefix
 */
#define PCD_CLIENTS_NAME_PREFIX "pcd-client-"

/*! \def PCD_TEMP_PATH
 *  \brief Path for temporary files (platform depended - can be overridden by the makefile)
 */
#ifndef PCD_TEMP_PATH
    #define PCD_TEMP_PATH     "/tmp"
#endif /* PCD_TEMP_PATH */

/*! \def PCD_PROCESS_SELF_EXCEPTION_DIRECTORY
 *  \brief Path for PCD exception dump file (platform depended - can be overridden by the makefile)
 */
#ifndef PCD_PROCESS_SELF_EXCEPTION_DIRECTORY
    #define PCD_PROCESS_SELF_EXCEPTION_DIRECTORY    "/nvram"
#endif /* PCD_PROCESS_SELF_EXCEPTION_DIRECTORY */

/*! \def PCD_PROCESS_SELF_EXCEPTION_FILE
 *  \brief File name for PCD exception dump file
 */
#ifndef PCD_PROCESS_SELF_EXCEPTION_FILE
    #define PCD_PROCESS_SELF_EXCEPTION_FILE         "pcd_self_exception.txt"
#endif /* PCD_PROCESS_SELF_EXCEPTION_FILE */

/*!\fn PCD_main_force_iteration
 * \brief Forces process iteration right after timer tick
 */
void PCD_main_force_iteration( void );

/*!\fn PCD_main_set_self_priority
 * \brief Set the priority of the PCD in the system
 */
void PCD_main_set_self_priority( Int32 priority, Int32 policy );

#endif /* _PCD_H_ */

