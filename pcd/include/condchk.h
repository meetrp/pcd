/*
 * condchk.h
 * Description:
 * PCD condition check header file
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

#ifndef _CONDCHK_H_
#define _CONDCHK_H_

/***************************************************************************/
/*! \file condchk.h
 *  \brief Condition check module header file
****************************************************************************/

/**************************************************************************/
/*      INCLUDES                                                          */
/**************************************************************************/
#include <net/if.h>
#include "system_types.h"
#include "ruleid.h"



struct rule_t;

/**************************************************************************/
/*      INTERFACE TYPES and STRUCT Definitions                            */
/**************************************************************************/
typedef PCD_status_e (*condCheckFunc)( struct rule_t *rule );

#define PCD_COND_MAX_SIZE   32

/* List of all start condition keywords */
#define PCD_START_COND_KEYWORDS \
    PCD_START_COND_KEYWORD( NONE )\
    PCD_START_COND_KEYWORD( FILE )\
    PCD_START_COND_KEYWORD( RULE_COMPLETED )\
    PCD_START_COND_KEYWORD( NETDEVICE )\
    PCD_START_COND_KEYWORD( IPC_OWNER )\
    PCD_START_COND_KEYWORD( ENV_VAR )\

#define PCD_START_COND_PREFIX( x )   PCD_START_COND_KEYWORD_##x

#define PCD_START_COND_KEYWORD( keyword ) \
    PCD_START_COND_PREFIX( keyword ),

/*! \enum  startCond_e
 *  \brief Start conditions enumeration
 */
typedef enum startCond_e
{
    PCD_START_COND_KEYWORDS
    PCD_START_COND_LAST,

} startCond_e;

#undef PCD_START_COND_KEYWORD

#define PCD_START_COND_FUNCTION(x)   PCD_start_cond_check_##x

#define PCD_START_COND_KEYWORD( keyword ) \
    PCD_status_e PCD_START_COND_FUNCTION( keyword )( struct rule_t *);

/* Function prototypes */
PCD_START_COND_KEYWORDS

#undef PCD_START_COND_KEYWORD

/* List of all end condition keywords */
#define PCD_END_COND_KEYWORDS \
    PCD_END_COND_KEYWORD( NONE )\
    PCD_END_COND_KEYWORD( FILE )\
    PCD_END_COND_KEYWORD( EXIT )\
    PCD_END_COND_KEYWORD( NETDEVICE )\
    PCD_END_COND_KEYWORD( IPC_OWNER )\
    PCD_END_COND_KEYWORD( PROCESS_READY )\
    PCD_END_COND_KEYWORD( WAIT )\

#define PCD_END_COND_PREFIX( x )   PCD_END_COND_KEYWORD_##x

#define PCD_END_COND_KEYWORD( keyword ) \
    PCD_END_COND_PREFIX( keyword ),

/*! \enum  endCond_e
 *  \brief End conditions enumeration
 */
typedef enum endCond_e
{
    PCD_END_COND_KEYWORDS
    PCD_END_COND_LAST,

} endCond_e;

#undef PCD_END_COND_KEYWORD

#define PCD_END_COND_FUNCTION(x)   PCD_end_cond_check_##x

#define PCD_END_COND_KEYWORD( keyword ) \
    PCD_status_e PCD_END_COND_FUNCTION( keyword )( struct rule_t *);

/* Function prototypes */
PCD_END_COND_KEYWORDS

#undef PCD_END_COND_KEYWORD

#define PCD_START_COND_MAX_IDS      8

/*! \struct rule_t
 *  \brief Environment variable structure
 */
typedef struct envVar_t
{
    char    envVarName[ PCD_COND_MAX_SIZE ];
    char    envVarValue[ PCD_COND_MAX_SIZE ];

} envVar_t;

struct rule_t;

/*! \ruleCache_t
 *  \brief Rule cache structure
 */
typedef struct ruleCache_t
{
    struct rule_t   *rule;      /* For Cache only - Speed up rule search */
    ruleId_t        ruleId;

} ruleCache_t;

/*! \struct startCond_t
 *  \brief Start conditions structure
 */
typedef struct startCond_t
{
    startCond_e type;
    union
    {
        char        filename[ PCD_COND_MAX_SIZE ];
        ruleCache_t ruleCompleted[ PCD_START_COND_MAX_IDS ];
        char        netDevice[ IF_NAMESIZE ];
        u_int32_t   ipcOwner;
        envVar_t    envVar;
    };

} startCond_t;

/*! \struct endCond_t
 *  \brief End conditions structure
 */
typedef struct endCond_t
{
    endCond_e type;
    union
    {
    char    filename[ PCD_COND_MAX_SIZE ];
    u_int32_t  delay[2];
    char    netDevice[ IF_NAMESIZE ];
    u_int32_t  ipcOwner;
    u_int32_t  exitStatus;
    u_int32_t  signal;
    bool_t    processReady;
    };

} endCond_t;

/*! \fn             PCD_start_cond_check_get_function
 *  \brief          Get pointer to the start condition check function
 *  \param[in]      Start
 *  \param[in,out]  None
 *  \return         Pointer to function - Success, NULL - Error
 */
condCheckFunc PCD_start_cond_check_get_function( startCond_e cond );

/*! \fn             PCD_end_cond_check_get_function
 *  \brief          Get pointer to the end condition check function
 *  \param[in]      End condition
 *  \param[in,out]  None
 *  \return         Pointer to function - Success, NULL - Error
 */
condCheckFunc PCD_end_cond_check_get_function( endCond_e cond );

#endif /* _CONDCHK_H_ */
