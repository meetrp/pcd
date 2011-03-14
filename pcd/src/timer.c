/*
 * timer.c
 * Description:
 * PCD timer implementation file
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
#include <sys/time.h>
#include <signal.h>
#include <unistd.h>
#include <stdio.h>
#include <time.h>
#include <errno.h>
#include <fcntl.h>
#include <sys/stat.h>
#include "system_types.h"
#include "timer.h"
#include "condchk.h"
#include "process.h"
#include "failact.h"
#include "pcd.h"

/**************************************************************************/
/*      LOCAL DEFINITIONS AND VARIABLES                                   */
/**************************************************************************/

extern int32_t errno;

static timerObj_t *timerObjHead = NULL;
static bool_t timerEnabled = False;

typedef struct timerQueueList
{
    timerObj_t *timerObj;
    rule_t *rule;
    struct timerQueueList *next;

} timerQueueList;

static timerQueueList *enqueueList = NULL;
static timerQueueList *dequeueList = NULL;

/* Functions to add to queues */
static void PCD_timer_add_to_dequeue_list( timerObj_t *timerObj );
static void PCD_timer_add_to_enqueue_list( rule_t *rule );

/* Enqueue/Dequeue functions */
static void PCD_timer_dequeue( timerObj_t *timerObj );
static void PCD_timer_enqueue( timerObj_t *timerObj );
static timerObj_t *PCD_timer_new( rule_t *rule );

/* Handle queue lists */
static void PCD_timer_dequeue_handle( void );
static void PCD_timer_enqueue_handle( void );

/* Handle conditions */
static bool_t PCD_timer_handle_start_condition( timerObj_t *timerObj );
static bool_t PCD_timer_handle_end_condition( timerObj_t *timerObj );

#define PCD_START_COND_KEYWORD( keyword ) \
    &PCD_START_COND_FUNCTION( keyword ),

/* Start condition functions array */
condCheckFunc startCondFuncs[] =
{
    PCD_START_COND_KEYWORDS
    NULL
};

#undef PCD_START_COND_KEYWORD

#define PCD_END_COND_KEYWORD( keyword ) \
    &PCD_END_COND_FUNCTION( keyword ),

/* End condition functions array */
condCheckFunc endCondFuncs[] =
{
    PCD_END_COND_KEYWORDS
    NULL
};

#undef PCD_END_COND_KEYWORD

extern int32_t errno;

/**************************************************************************/
/*      IMPLEMENTATION                                                    */
/**************************************************************************/

PCD_status_e PCD_timer_init( void )
{
    return PCD_STATUS_OK;
}

bool_t PCD_timer_iterate( void )
{
    timerObj_t *searchList;
    bool_t processFlag = False;

    /* Check if we have something to do */
    if ( ( timerEnabled == False ) || ( !timerObjHead ) )
        return False;

    searchList = timerObjHead;

    while ( searchList )
    {
        rule_t *rule = searchList->rule;

        switch ( rule->ruleState )
        {
            case PCD_RULE_START_CONDITION_WAITING:
                processFlag |= PCD_timer_handle_start_condition( searchList );
                break;

            case PCD_RULE_END_CONDITION_WAITING:
                processFlag |= PCD_timer_handle_end_condition( searchList );
                break;

            default:
                /* Remove from queue */
                PCD_timer_add_to_dequeue_list( searchList );
                break;
        }

        searchList = searchList->next;
    }

    /* Enqueue and Dequeue as required, note that we still have the semaphore! */
    PCD_timer_dequeue_handle();
    PCD_timer_enqueue_handle();

    return processFlag;
}

PCD_status_e PCD_timer_start( void )
{
    timerEnabled = True;
    return PCD_STATUS_OK;
}


PCD_status_e PCD_timer_stop( void )
{
    timerEnabled = False;
    return PCD_STATUS_OK;
}

static void PCD_timer_dequeue( timerObj_t *timerObj )
{
    /* Remove from timer queue */
    if ( timerObj->prev )
    {
        timerObj->prev->next = timerObj->next;
    }
    else
    {
        /* No head */
        timerObjHead = timerObj->next;
    }

    if ( timerObj->next )
    {
        timerObj->next->prev = timerObj->prev;
    }

    free( timerObj );
    timerObj = NULL;
}

static timerObj_t *PCD_timer_new( rule_t *rule )
{
    timerObj_t *newObj;

    if ( !rule )
        return NULL;

    /* Create a new timer object */
    newObj = malloc( sizeof( timerObj_t ) );

    if ( !newObj )
    {
        PCD_PRINTF_STDERR( "memory allocation failure" );
        return NULL;
    }

    newObj->startCondCheckFunc = PCD_start_cond_check_get_function( rule->startCondition.type );
    newObj->endCondCheckFunc = PCD_end_cond_check_get_function( rule->endCondition.type );
    newObj->failureActionFunc = PCD_failure_action_get_function( rule->failureAction.action );
    newObj->rule = rule;
    newObj->timeout = rule->timeout;
    newObj->prev = NULL;
    newObj->next = NULL;

    return newObj;
}

static void PCD_timer_enqueue( timerObj_t *newObj )
{
    timerObj_t *timerObj = timerObjHead;

    if ( !timerObjHead )
    {
        /* Create the new head */
        timerObjHead = newObj;
        return;
    }

    while ( timerObj->next )
    {
        timerObj = timerObj->next;
    }

    /* Put new object in the end */
    timerObj->next = newObj;
    newObj->prev = timerObj;
}

PCD_status_e PCD_timer_enqueue_rule( rule_t *rule )
{
    timerObj_t *newObj;

    if ( !rule )
        return PCD_STATUS_NOK;

    if ( ( PCD_RULE_ACTIVE( rule ) ) || ( rule->proc ) )
    {
        return PCD_STATUS_INVALID_RULE;
    }

    newObj = PCD_timer_new( rule );

    if ( !newObj )
    {
        return PCD_STATUS_NOK;
    }

    /* Enqueue the new rule in the timer queue */
    rule->ruleState = PCD_RULE_START_CONDITION_WAITING;
    PCD_timer_enqueue( newObj );

    return PCD_STATUS_OK;
}

PCD_status_e PCD_timer_dequeue_rule( rule_t *rule, bool_t failed )
{
    timerObj_t *searchList;

    if ( !rule )
        return PCD_STATUS_NOK;

    searchList = timerObjHead;

    while ( searchList )
    {
        /* Find the requsted rule to be removed */
        if ( searchList->rule == rule )
        {
            /* In this case, a process is already running. We need to stop it first */
            if ( rule->proc )
            {
                PCD_process_stop( rule, False, NULL );
            }

            /* Remove from queue */
            PCD_timer_dequeue( searchList );

            /* Failed flag comes only from process module, which detects process failures */
            if ( failed )
            {
                rule->ruleState = PCD_RULE_FAILED;
            }
            else
            {
                /* Do not change rule state if it completed successfully or idle */
                if ( ( rule->ruleState != PCD_RULE_COMPLETED ) && ( rule->ruleState != PCD_RULE_IDLE ) )
                {
                    /* We are here because the rule did not complete */
                    rule->ruleState = PCD_RULE_NOT_COMPLETED;
                }
            }

            return PCD_STATUS_OK;
        }

        searchList = searchList->next;
    }


    return PCD_STATUS_INVALID_RULE;
}

static void PCD_timer_dequeue_handle( void )
{
    timerQueueList *searchList = dequeueList;

    /* Go through the dequeue list and delete the objects */
    while ( searchList )
    {
        timerObj_t *timerObj = searchList->timerObj;
        timerQueueList *searchListTmp;

        /* Remove from queue */
        PCD_timer_dequeue( timerObj );

        searchListTmp = searchList;
        searchList = searchList->next;

        free( searchListTmp );
        searchListTmp = NULL;
    }

    dequeueList = NULL;
}

static void PCD_timer_enqueue_handle( void )
{
    timerQueueList *searchList = enqueueList;

    /* Go through the enqueue list and create the objects */
    while ( searchList )
    {
        timerQueueList *searchListTmp;
        timerObj_t *newObj;

        /* Create a new timer object */
        newObj = PCD_timer_new( searchList->rule );

        if ( !newObj )
        {
            /* No memory */
            return;
        }

        /* Set rule to start condition */
        searchList->rule->ruleState = PCD_RULE_START_CONDITION_WAITING;

        /* Add new object to timer list */
        PCD_timer_enqueue( newObj );

        searchListTmp = searchList;
        searchList = searchList->next;

        free( searchListTmp );
        searchListTmp = NULL;
    }

    enqueueList = NULL;
}

static void PCD_timer_add_to_dequeue_list( timerObj_t *timerObj )
{
    timerQueueList *searchList = dequeueList;
    timerQueueList *newObj;

    newObj = malloc( sizeof( timerQueueList) );

    if ( !newObj )
    {
        PCD_PRINTF_STDERR( "memory allocation failure" );
        return;
    }

    newObj->timerObj = timerObj;
    newObj->rule = NULL;
    newObj->next = searchList;

    dequeueList = newObj;
}

static void PCD_timer_add_to_enqueue_list( rule_t *rule )
{
    timerQueueList *searchList = enqueueList;
    timerQueueList *newObj;

    /* In case the failure action does not require to restart or execute another rule */
    if ( !rule )
        return;

    newObj = malloc( sizeof( timerQueueList) );

    if ( !newObj )
    {
        PCD_PRINTF_STDERR( "memory allocation failure" );
        return;
    }

    newObj->timerObj = NULL;
    newObj->rule = rule;
    newObj->next = searchList;

    enqueueList = newObj;
}

static bool_t PCD_timer_handle_start_condition( timerObj_t *timerObj )
{
    rule_t *rule = timerObj->rule;
    PCD_status_e retval;
    PCD_DEBUG_PRINTF( "Rule %s_%s: Waiting for start condition", rule->ruleId.groupName, rule->ruleId.ruleName );

    if ( timerObj->startCondCheckFunc )
    {
        /* Check start condition */
        retval = timerObj->startCondCheckFunc( rule );
    }
    else
    {
        /* In case there is no start condition function (should not happen) */
        retval = PCD_STATUS_OK;
    }

    /* Condition ok, spawn the process */
    if ( retval == PCD_STATUS_OK )
    {
        /* Check if this is a pseudo (sync) rule, which does not require any process */
        if ( strcmp( rule->command, "NONE" ) == 0 )
        {
            PCD_DEBUG_PRINTF( "Rule %s_%s: Pseudo rule start condition success", rule->ruleId.groupName, rule->ruleId.ruleName );

            /* Update rule state */
            rule->ruleState = PCD_RULE_END_CONDITION_WAITING;
            return False;
        }

        /* Check if process was enqueued */
        if ( ( retval = PCD_process_enqueue( rule ) ) == PCD_STATUS_OK )
        {
            /* Update rule state */
            rule->ruleState = PCD_RULE_END_CONDITION_WAITING;
            return True;
        }
        else
        {
            /* Ok, we have a problem... */
            if ( retval == PCD_STATUS_INVALID_RULE )
            {
                /* In this case, we already have a running process (how could this be??)
                 Lets kill it and try again in the next iteration */
                PCD_process_stop( rule, True, NULL );

                return True;
            }
            else
            {
                /* Are we out of memory? */
                PCD_PRINTF_STDERR( "Rule %s_%s: Failed to enqueue process %s", rule->ruleId.groupName, rule->ruleId.ruleName, rule->command );

                /* We failed */
                rule->ruleState = PCD_RULE_FAILED;

                /* Remove from queue */
                PCD_timer_add_to_dequeue_list( timerObj );

                /* Failure action */
                if ( timerObj->failureActionFunc )
                {
                    PCD_timer_add_to_enqueue_list( timerObj->failureActionFunc( rule ) );
                }
            }
        }
    }

    return False;
}

static bool_t PCD_timer_handle_end_condition( timerObj_t *timerObj )
{
    rule_t *rule = timerObj->rule;
    PCD_status_e retval;

    PCD_DEBUG_PRINTF( "Rule %s_%s: Waiting for end condition", rule->ruleId.groupName, rule->ruleId.ruleName );

    if ( timerObj->endCondCheckFunc )
    {
        /* Check end condition */
        retval = timerObj->endCondCheckFunc( rule );
    }
    else
    {
        /* In case there is no end condition function (should not happen) */
        retval = PCD_STATUS_OK;
    }

    /* Condition ok, remove from queue */
    if ( retval == PCD_STATUS_OK )
    {
        if ( rule->proc )
        {
            PCD_PRINTF_STDOUT( "Rule %s_%s: Success (Process %s (%d))", rule->ruleId.groupName, rule->ruleId.ruleName, rule->command, rule->proc->pid );
        }
        else
        {
            PCD_PRINTF_STDOUT( "Rule %s_%s: Success", rule->ruleId.groupName, rule->ruleId.ruleName );
        }


        rule->ruleState = PCD_RULE_COMPLETED;

        /* Remove from queue */
        PCD_timer_add_to_dequeue_list( timerObj );
        return False;
    }

    /* Check if Timeout is forever */
    if ( timerObj->timeout == ~0 )
        return False;

    /* Now check if timeout expiered */
    if ( timerObj->timeout == 0 )
    {
        PCD_PRINTF_STDERR( "Rule %s_%s: Timeout", rule->ruleId.groupName, rule->ruleId.ruleName );

        /* If a process is running, kill it */
        if ( rule->proc )
            PCD_process_stop( rule, True, NULL );

        rule->ruleState = PCD_RULE_NOT_COMPLETED;

        /* Remove from queue */
        PCD_timer_add_to_dequeue_list( timerObj );

        /* Timeout action */
        if ( timerObj->failureActionFunc )
        {
            PCD_timer_add_to_enqueue_list( timerObj->failureActionFunc( rule ) );
        }

        return True;
    }
    else
    {
        if ( timerObj->timeout <= PCD_TIMER_TICK )
        {
            /* Round timeout for the next iteration */
            timerObj->timeout = 0;
        }
        else
        {
            /* Reduce a tick */
            timerObj->timeout -= PCD_TIMER_TICK;
        }
    }

    return False;
}

