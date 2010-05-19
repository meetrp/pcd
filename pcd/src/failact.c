/*
 * failact.c
 * Description:
 * PCD failure action implementation file
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
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include "system_types.h"
#include "failact.h"
#include "rulestate.h"
#include "ruleid.h"
#include "rules_db.h"
#include "process.h"
#include "except.h"
#include "pcd.h"

/**************************************************************************/
/*      LOCAL DEFINITIONS AND VARIABLES                                   */
/**************************************************************************/

#define PCD_FAILURE_ACTION_KEYWORD( keyword ) \
    PCD_FAILURE_ACTION_FUNCTION( keyword ),

/* Function prototypes */
static const failActionFunc failureActionFuncTable[] =
{
    PCD_FAILURE_ACTION_KEYWORDS
    NULL,
};

#undef PCD_END_COND_KEYWORD

/**************************************************************************/
/*      IMPLEMENTATION                                                    */
/**************************************************************************/

failActionFunc PCD_failure_action_get_function( failureAction_e cond )
{
    return failureActionFuncTable[ cond ];
}

rule_t *PCD_failure_action_NONE( rule_t *rule )
{
    PCD_PRINTF_STDERR( "Rule %s_%s failed", rule->ruleId.groupName, rule->ruleId.ruleName );
    return NULL;
}

rule_t *PCD_failure_action_REBOOT( rule_t *rule )
{
    /* Check for exceptions before rebooting */
    PCD_exception_listen();

    /* Reboot the system; Initiate termination sigal to PCD */
    kill( getpid(), SIGTERM );
    return NULL;
}

rule_t *PCD_failure_action_RESTART( rule_t *rule )
{
    if ( !rule )
    {
        return NULL;
    }

    /* If a process exists, kill it first */
    if ( rule->proc )
    {
        PCD_process_stop( rule, True, NULL );
    }

    /* Reenqueue rule */
    return( rule );
}

rule_t *PCD_failure_action_EXEC_RULE( rule_t *rule )
{
    rule_t *execRule;

    /* Find rule to execute */
    execRule = PCD_rulesdb_get_rule_by_id( &rule->failureAction.ruleId );

    if ( !execRule )
    {
        PCD_PRINTF_STDERR( "Failed to execute rule %s_%s, rule not found!", rule->failureAction.ruleId.groupName, rule->failureAction.ruleId.ruleName );
        return NULL;
    }

    /* Check if rule is already running */
    if ( PCD_RULE_ACTIVE( execRule ) )
    {
        PCD_PRINTF_STDERR( "Failed to execute rule %s_%s, rule already running!", rule->failureAction.ruleId.groupName, rule->failureAction.ruleId.ruleName );
        return NULL;
    }

    return PCD_failure_action_RESTART( execRule );
}
