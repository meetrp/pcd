/*
 * ruleid.h
 * Description:
 * PCD Rule ID type header file
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

#ifndef _RULEID_H_
#define _RULEID_H_

/***************************************************************************/
/*! \file ruleid.h
 *  \brief Rule ID header file
****************************************************************************/

/**************************************************************************/
/*      INCLUDES                                                          */
/**************************************************************************/
#include "system_types.h"

/**************************************************************************/
/*      INTERFACE TYPES and STRUCT Definitions                            */
/**************************************************************************/

/*! \def PCD_RULEID_MAX_GROUP_NAME_SIZE
 *  \brief Holds the maximum size for group name
 */
#define PCD_RULEID_MAX_GROUP_NAME_SIZE  16

/*! \def PCD_RULEID_MAX_RULE_NAME_SIZE
 *  \brief Holds the maximum size for rule name
 */
#define PCD_RULEID_MAX_RULE_NAME_SIZE   16

/*! \struct ruleId_t
 *  \brief Rule ID structure
 */
typedef struct ruleId_t
{
    char        groupName[ PCD_RULEID_MAX_GROUP_NAME_SIZE ];
    char        ruleName[ PCD_RULEID_MAX_RULE_NAME_SIZE ];

} ruleId_t;

#endif /* _RULEID_H_ */
