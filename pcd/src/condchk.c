/*
 * condchk.c
 * Description:
 * PCD condition check implementation file
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
#include <sys/types.h>
#include <sys/stat.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <unistd.h>
#include "rules_db.h"
#include "system_types.h"
#include "rulestate.h"
#include "condchk.h"
#include "process.h"
#include "timer.h"
#include "ipc.h"
#include "pcd.h"

/**************************************************************************/
/*      LOCAL DEFINITIONS AND VARIABLES                                   */
/**************************************************************************/
#define PCD_START_COND_FUNCTION(x)   PCD_start_cond_check_##x

#define PCD_START_COND_KEYWORD( keyword ) \
    PCD_START_COND_FUNCTION( keyword ),

/* Function prototypes */
static const condCheckFunc startFuncTable[] =
{
    PCD_START_COND_KEYWORDS
    NULL,
};

#undef PCD_START_COND_KEYWORD

#define PCD_END_COND_FUNCTION(x)   PCD_end_cond_check_##x

#define PCD_END_COND_KEYWORD( keyword ) \
    PCD_END_COND_FUNCTION( keyword ),

/* Function prototypes */
static const condCheckFunc endFuncTable[] =
{
    PCD_END_COND_KEYWORDS
    NULL,
};

#undef PCD_END_COND_KEYWORD

/**************************************************************************/
/*      IMPLEMENTATION                                                    */
/**************************************************************************/

condCheckFunc PCD_start_cond_check_get_function( startCond_e cond )
{
    return startFuncTable[ cond ];
}

condCheckFunc PCD_end_cond_check_get_function( endCond_e cond )
{
    return endFuncTable[ cond ];
}


PCD_status_e PCD_start_cond_check_NONE( rule_t *rule )
{
    return PCD_STATUS_OK;
}

PCD_status_e PCD_start_cond_check_FILE( rule_t *rule )
{
    struct stat fbuf;

    /* Try to open the file */
    if ( stat(rule->startCondition.filename, &fbuf) )
        return PCD_STATUS_NOK;

    return PCD_STATUS_OK;
}


PCD_status_e PCD_start_cond_check_PNAME( rule_t *rule )
{

    return PCD_STATUS_OK;
}


PCD_status_e PCD_start_cond_check_RULE_COMPLETED( rule_t *rule )
{
    rule_t *checkRule;
    u_int32_t i = 0;

    while ( i < PCD_START_COND_MAX_IDS )
    {
        if ( !rule->startCondition.ruleCompleted[ i ].ruleId.groupName[ 0 ] )
        {
            break;
        }

        /* Do we have the rule pointer in cache? */
        checkRule = rule->startCondition.ruleCompleted[ i ].rule;

        if ( !checkRule )
        {
            /* Fetch rule from db, and store in cache for next time */
            checkRule = PCD_rulesdb_get_rule_by_id( &rule->startCondition.ruleCompleted[ i ].ruleId );
            rule->startCondition.ruleCompleted[ i ].rule = checkRule;
        }

        if ( !checkRule )
        {
            PCD_PRINTF_STDERR( "Warning, rule %s_%s not found", rule->startCondition.ruleCompleted[ i ].ruleId.groupName, rule->startCondition.ruleCompleted[ i ].ruleId.ruleName );

            /* There is no way to complete this rule */
            rule->ruleState = PCD_RULE_NOT_COMPLETED;
            return PCD_STATUS_NOK;
        }

        /* Check status. If one of the rules is not completed, there is no point of checking the rest */
        if ( checkRule->ruleState != PCD_RULE_COMPLETED )
        {
            return PCD_STATUS_NOK;
        }

        /* Advance to next rule */
        i++;
    }

    /* All specified rules have completed */
    if ( i > 0 )
    {
        return PCD_STATUS_OK;
    }

    return PCD_STATUS_NOK;
}


PCD_status_e PCD_start_cond_check_NETDEVICE( rule_t *rule )
{
    int32_t sock;
    struct ifreq ifr;
    PCD_status_e ret = PCD_STATUS_NOK;

    sock = socket(AF_INET, SOCK_DGRAM, 0);
    ifr.ifr_addr.sa_family = AF_INET;

    if ( sock < 0 )
    {
        return PCD_STATUS_NOK;
    }

    strncpy(ifr.ifr_name, rule->startCondition.netDevice, IF_NAMESIZE);
    ifr.ifr_hwaddr.sa_family = 1;

    /* Get interface mac address */
    if ( ioctl(sock, SIOCGIFHWADDR, &ifr) < 0 )
    {
        ret = PCD_STATUS_NOK;
    }
    else
    {
        ret = PCD_STATUS_OK;
    }

    close(sock);

    return ret;
}


PCD_status_e PCD_start_cond_check_IPC_OWNER( rule_t *rule )
{
    IPC_context_t destContext;

    return IPC_get_context_by_owner( &destContext, rule->startCondition.ipcOwner );
}


PCD_status_e PCD_start_cond_check_ENV_VAR( rule_t *rule )
{
    char *sp;

    if ( ( sp = getenv(rule->startCondition.envVar.envVarName ) ) )
    {
        if ( strcmp(rule->startCondition.envVar.envVarValue, sp ) == 0 )
            return PCD_STATUS_OK;
    }

    return PCD_STATUS_NOK;
}


PCD_status_e PCD_end_cond_check_NONE( rule_t *rule )
{
    return PCD_STATUS_OK;
}


PCD_status_e PCD_end_cond_check_FILE( rule_t *rule )
{
    struct stat fbuf;

    /* Try to open the file */
    if ( stat(rule->endCondition.filename, &fbuf) )
        return PCD_STATUS_NOK;

    return PCD_STATUS_OK;
}


PCD_status_e PCD_end_cond_check_EXIT( rule_t *rule )
{
    if ( rule->proc )
    {
        if ( rule->proc->retstat == PCD_PROCESS_RETEXITED )
        {
            /* Process has exited */
            if ( rule->proc->retcode == rule->endCondition.exitStatus )
                return PCD_STATUS_OK;
        }
    }

    return PCD_STATUS_NOK;
}

PCD_status_e PCD_end_cond_check_NETDEVICE( rule_t *rule )
{
    int32_t sock;
    struct ifreq ifr;
    PCD_status_e ret = PCD_STATUS_NOK;

    sock = socket(AF_INET, SOCK_DGRAM, 0);
    ifr.ifr_addr.sa_family = AF_INET;

    if ( sock < 0 )
    {
        return PCD_STATUS_NOK;
    }

    strncpy(ifr.ifr_name, rule->endCondition.netDevice, IF_NAMESIZE);
    ifr.ifr_hwaddr.sa_family = 1;

    /* Get interface mac address */
    if ( ioctl(sock, SIOCGIFHWADDR, &ifr) < 0 )
    {
        ret = PCD_STATUS_NOK;
    }
    else
    {
        ret = PCD_STATUS_OK;
    }

    close(sock);

    return ret;
}


PCD_status_e PCD_end_cond_check_IPC_OWNER( rule_t *rule )
{
    IPC_context_t destContext;

    /* Check if the owner exists */
    return IPC_get_context_by_owner( &destContext, rule->endCondition.ipcOwner );
}


PCD_status_e PCD_end_cond_check_PROCESS_READY( rule_t *rule )
{
    /* Check processReady flag, which is set by PCD API */
    if ( rule->endCondition.processReady == True )
    {
        /* Clear the flag for next time */
        rule->endCondition.processReady = False;
        return PCD_STATUS_OK;
    }

    return PCD_STATUS_NOK;
}


PCD_status_e PCD_end_cond_check_WAIT( rule_t *rule )
{
    u_int32_t *delay = &rule->endCondition.delay[0];

    /* Reduce time tick and check if time has expired */
    if ( *delay < PCD_TIMER_TICK )
    {
        /* Restore the delay value and return OK, time has expired */
        *delay = rule->endCondition.delay[1];
        return PCD_STATUS_OK;
    }
    else
    {
        /* Time not expired yet */
        *delay -= PCD_TIMER_TICK;
    }

    return PCD_STATUS_NOK;
}
