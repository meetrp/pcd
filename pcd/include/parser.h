/*
 * parser.h
 * Description:
 * PCD Parser header file
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
 * Change log:
 * - Nick Stay, nlstay@gmail.com, Added optional USER field so that processes 
 *   can be executed as an arbitrary user.
 */

/* Author:
 * Hai Shalom, hai@rt-embedded.com 
 *
 * PCD Homepage: http://www.rt-embedded.com/pcd/
 * PCD Project at SourceForge: http://sourceforge.net/projects/pcd/
 *  
 */

#ifndef _PARSER_H_
#define _PARSER_H_

/***************************************************************************/
/*! \file parser.h
 *  \brief Parser module header file
****************************************************************************/

/**************************************************************************/
/*      INCLUDES                                                          */
/**************************************************************************/
#include "system_types.h"

/**************************************************************************/
/*      INTERFACE TYPES and STRUCT Definitions                            */
/**************************************************************************/

/*! \def PCD_PARSER_KEYWORDS
 *  \brief Holds the syntax keywords for the Rule confiuration file
 */
/* Keyword,		Mandatory */
#define PCD_PARSER_KEYWORDS \
    PCD_PARSER_KEYWORD( RULE,               1 )\
    PCD_PARSER_KEYWORD( START_COND,         1 )\
    PCD_PARSER_KEYWORD( COMMAND,            1 )\
    PCD_PARSER_KEYWORD( END_COND,           1 )\
    PCD_PARSER_KEYWORD( END_COND_TIMEOUT,   1 )\
    PCD_PARSER_KEYWORD( FAILURE_ACTION,     1 )\
    PCD_PARSER_KEYWORD( ACTIVE,             1 )\
    PCD_PARSER_KEYWORD( SCHED,              0 )\
    PCD_PARSER_KEYWORD( DAEMON,             0 )\
    PCD_PARSER_KEYWORD( USER,               0 )\
    PCD_PARSER_KEYWORD( VERSION,            0 )\
    PCD_PARSER_KEYWORD( INCLUDE,            0 )\

/**************************************************************************/
/*      INTERFACE FUNCTIONS Prototypes:                                   */
/**************************************************************************/

/*! \fn				PCD_parser_parse
 *  \brief 			Parse a configuration file
 *  \param[in] 		Filename
 *  \param[in,out] 	None
 *  \return			PCD_STATUS_OK - Success, Otherwise - Error
 */
PCD_status_e PCD_parser_parse( const char *filename );

/*! \fn				PCD_parser_enable_verbose
 *  \brief 			Enable verbose prints in the end of the parse
 *  \param[in] 		Enable flag
 *  \param[in,out] 	None
 *  \return			PCD_STATUS_OK - Success, Otherwise - Error
 */
PCD_status_e PCD_parser_enable_verbose( bool_t enable );

#endif /* _PARSER_H_ */
