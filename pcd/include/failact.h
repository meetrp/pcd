/*
 * failact.h
 * Description:
 * PCD Failure action header file
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

#ifndef _FAILACT_H_
#define _FAILACT_H_

/***************************************************************************/
/*! \file failact.h
 *  \brief Failure action module header file
****************************************************************************/

/**************************************************************************/
/*      INCLUDES                                                          */
/**************************************************************************/
#include "system_types.h"
#include "ruleid.h"

/**************************************************************************/
/*      INTERFACE TYPES and STRUCT Definitions                            */
/**************************************************************************/

struct rule_t;

typedef struct rule_t *(*failActionFunc)( struct rule_t *rule );

/* Failure actions keywords */
#define PCD_FAILURE_ACTION_KEYWORDS \
	PCD_FAILURE_ACTION_KEYWORD( NONE )\
	PCD_FAILURE_ACTION_KEYWORD( REBOOT )\
	PCD_FAILURE_ACTION_KEYWORD( RESTART )\
	PCD_FAILURE_ACTION_KEYWORD( EXEC_RULE )

#define PCD_FAILURE_ACTION_PREFIX( x )   PCD_FAILURE_ACTION_KEYWORD_##x

#define PCD_FAILURE_ACTION_KEYWORD( keyword ) \
	PCD_FAILURE_ACTION_PREFIX( keyword ),

/*! \enum failureAction_e
 *  \brief Failure actions enumeration
 */
typedef enum failureAction_e
{
    PCD_FAILURE_ACTION_KEYWORDS

} failureAction_e;

#undef PCD_FAILURE_ACTION_KEYWORD

#define PCD_FAILURE_ACTION_FUNCTION(x)   PCD_failure_action_##x

#define PCD_FAILURE_ACTION_KEYWORD( keyword ) \
	struct rule_t *PCD_FAILURE_ACTION_FUNCTION( keyword )( struct rule_t *);

/* Function prototypes */
PCD_FAILURE_ACTION_KEYWORDS

#undef PCD_FAILURE_ACTION_KEYWORD

/*! \struct failureAction_t
 *  \brief Failure action structure
 */
typedef struct failureAction_t
{
    failureAction_e action;
    union
    {
        ruleId_t ruleId;
    };

} failureAction_t;

/**************************************************************************/
/*      INTERFACE FUNCTIONS Prototypes:                                   */
/**************************************************************************/

/*! \fn				PCD_failure_action_get_function
 *  \brief 			Get pointer to failure action function
 *  \param[in] 		Failure action
 *  \param[in,out] 	None
 *  \return			Pointer to function - Success, NULL - Error
 */
failActionFunc PCD_failure_action_get_function( failureAction_e action );

#endif /* _FAILACT_H_ */
