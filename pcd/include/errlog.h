/*
 * errlog.h
 * Description:
 * PCD Error logger header file
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

#ifndef _ERRLOG_H_
#define _ERRLOG_H_

/***************************************************************************/
/*! \file errlog.h
 *  \brief Error log
****************************************************************************/

/**************************************************************************/
/*      INCLUDES                                                          */
/**************************************************************************/
#include "system_types.h"

/**************************************************************************/
/*      INTERFACE TYPES and STRUCT Definitions                            */
/**************************************************************************/

/*! \def PCD_ERRLOG_MAX_FILE_SIZE
 *  \brief Holds the maximum size for the error log file size
 */
#define PCD_ERRLOG_MAX_FILE_SIZE        ( 4096 )

/**************************************************************************/
/*      INTERFACE FUNCTIONS Prototypes:                                   */
/**************************************************************************/

/*! \fn				PCD_errlog_init
 *  \brief 			Initialize errlog module
 *  \param[in] 		Log filename
 *  \param[in,out] 	None
 *  \return			PCD_STATUS_OK - Success, Otherwise - Error
 */
PCD_status_e PCD_errlog_init( char *logFile );

/*! \fn				PCD_errlog_close
 *  \brief 			Close errlog module
 *  \param[in] 		None
 *  \param[in,out] 	None
 *  \return			Pointer to Rule - Success, NULL - Error
 */
PCD_status_e PCD_errlog_close( void );

/*! \fn				PCD_errlog_log
 *  \brief 			Add a new log entry in the error log file
 *  \param[in] 		Buffer, time stamp flag
 *  \param[in,out] 	None
 *  \return			Pointer to Rule - Success, NULL - Error
 */
void PCD_errlog_log( char *buffer, bool_t timeStamp );

#endif /* _ERRLOG_H_ */

