/*
 * rules_db.h
 * Description:
 * PCD Rules database type header file
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

#ifndef _RULES_DB_H_
#define _RULES_DB_H_

/***************************************************************************/
/*! \file rules_db.h
 *  \brief Rules DB module header file
****************************************************************************/

/**************************************************************************/
/*      INCLUDES                                                          */
/**************************************************************************/
#include <sys/types.h>
#include "system_types.h"
#include <time.h>
#include "pcd.h"
#include "ruleid.h"
#include "condchk.h"
#include "schedtype.h"
#include "rulestate.h"
#include "failact.h"

/**************************************************************************/
/*      INTERFACE TYPES and STRUCT Definitions                            */
/**************************************************************************/

struct procObj_t;

/*! \struct rule_t
 *  \brief Rule structure
 */
typedef struct rule_t
{
    ruleId_t            ruleId;
    startCond_t         startCondition;
    endCond_t           endCondition;
    u_int32_t              timeout;
    char                *command;
    char                *params;
    char                *optionalParams;
    failureAction_t     failureAction;
    schedType_t         sched;
    bool_t                daemon;
    uid_t               uid;
    pcdRuleState_e      ruleState;
    bool_t                indexed;

    struct procObj_t    *proc;

    struct rule_t       *next;

} rule_t;

/*! \struct ruleGroup_t
 *  \brief Rule group structure
 */
typedef struct ruleGroup_t
{
    char                *groupName;
    rule_t              *firstRule;
    struct ruleGroup_t  *next;

} ruleGroup_t;

/**************************************************************************/
/*      INTERFACE FUNCTIONS Prototypes:                                   */
/**************************************************************************/

/*! \fn             PCD_rulesdb_add_rule
 *  \brief          Add a rule to the database
 *  \param[in]      Rule
 *  \param[in,out]  None
 *  \return         PCD_STATUS_OK - Success, Otherwise - Error
 */
PCD_status_e PCD_rulesdb_add_rule( rule_t *rule );

/*! \fn             PCD_rulesdb_get_rule_by_id
 *  \brief          Get rule by rule ID
 *  \param[in]      Rule ID
 *  \param[in,out]  None
 *  \return         Pointer to Rule - Success, NULL - Error
 */
rule_t *PCD_rulesdb_get_rule_by_id( ruleId_t *ruleId );

/*! \fn             PCD_rulesdb_get_rule_by_pid
 *  \brief          Get rule by process ID
 *  \param[in]      Process ID
 *  \param[in,out]  None
 *  \return         Pointer to Rule - Success, NULL - Error
 */
rule_t *PCD_rulesdb_get_rule_by_pid( int32_t pid );

/*! \fn             PCD_rulesdb_get_first
 *  \brief          Get the first rule in DB. Sequent calls to next.
 *  \param[in]      None
 *  \param[in,out]  None
 *  \return         Pointer to Rule - Success, NULL - Error
 */
rule_t *PCD_rulesdb_get_first( void );

/*! \fn             PCD_rulesdb_get_next
 *  \brief          Get next rule (subsequent calls)
 *  \param[in]      None
 *  \param[in,out]  None
 *  \return         Pointer to Rule - Success, NULL - Error
 */
rule_t *PCD_rulesdb_get_next( void );

/*! \fn             PCD_rulesdb_activate
 *  \brief          Enqueue all active rules to timer
 *  \param[in]      None
 *  \param[in,out]  None
 *  \return         PCD_STATUS_OK - Success, Otherwise - Error
 */
PCD_status_e PCD_rulesdb_activate( void );

/*! \fn             PCD_rulesdb_setup_optional_params
 *  \brief          Setup optional parameters in rule
 *  \param[in]      Rule, Optional parameters
 *  \param[in,out]  None
 *  \return         PCD_STATUS_OK - Success, Otherwise - Error
 */
PCD_status_e PCD_rulesdb_setup_optional_params( rule_t *rule, const char *optionalParams );

/*! \fn             PCD_rulesdb_clear_optional_params
 *  \brief          Clear optional parameters from rule
 *  \param[in]      Rule
 *  \param[in,out]  None
 *  \return         PCD_STATUS_OK - Success, Otherwise - Error
 */
PCD_status_e PCD_rulesdb_clear_optional_params( rule_t *rule );

#endif /* _RULES_DB_H_ */
