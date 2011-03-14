/*
 * outputhdr.h
 * Description:
 * PCD output header generation header file
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

/***************************************************************************/

/*! \file outputhdr.h
    \brief Header file for output header generation

    The generated header file defines a definition for each group name,
    as well as definitions for each rule in the group.

    A macro to easily define a ruleId_t structure will also be generated.

****************************************************************************/

#ifndef _OUTPUTHDR_H_
#define _OUTPUTHDR_H_

#include "system_types.h"

/**************************************************************************/
/*      INTERFACE FUNCTIONS Prototypes:                                   */
/**************************************************************************/

/*! \fn PCD_status_e PCD_output_header_create_file
 *  \brief open header file
 *  \param[in] headerFilename, headerHandle.
 *  \param[out] graphHandle.
 *  \return OK or error status.
 */
PCD_status_e PCD_output_header_create_file( const char *headerFilename, void **headerHandle );

/*! \fn PCD_status_e PCD_output_header_close_file
 *  \brief close header file
 *  \param[in] headerFilename, headerHandle.
 *  \param[out] no output.
 *  \return OK or error status.
 */
PCD_status_e PCD_output_header_close_file( const char *headerFilename, const void *headerHandle );

/*! \fn PCD_status_e PCD_output_header_update_file
 *  \brief Add a new item in graph
 *  \param[in] rule, headerHandle.
 *  \param[out] no output.
 *  \return OK or error status.
 */
PCD_status_e PCD_output_header_update_file( rule_t *newrule, const void *headerHandle );

#endif /* _OUTPUTHDR_H_ */

