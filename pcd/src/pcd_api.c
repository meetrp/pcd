/*
 * pcd_api.c
 * Description:
 * PCD Core API implementation file
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
#include <signal.h>
#include "rules_db.h"
#include "system_types.h"
#include "rulestate.h"
#include "condchk.h"
#include "process.h"
#include "timer.h"
#include "ipc.h"
#include "pcd_api.h"
#include "pcd.h"
#include "misc.h"

/**************************************************************************/
/*      LOCAL DEFINITIONS AND VARIABLES                                   */
/**************************************************************************/
static IPC_context_t pcdContext;

/**************************************************************************/
/*      IMPLEMENTATION                                                    */
/**************************************************************************/

static PCD_status_e PCD_api_stop( rule_t *rule, bool_t brutal, void *cookie )
{
    PCD_status_e retval;

    /* Clear optional parameters */
    PCD_rulesdb_clear_optional_params( rule );

    /* Dequeue the rule from the timer queue */
    retval = PCD_timer_dequeue_rule( rule, False );

    if ( retval == PCD_STATUS_INVALID_RULE )
    {
        /* Fall here if the rule is completed and running (daemon) */
        retval = PCD_process_stop( rule, brutal, cookie );

        if ( retval == PCD_STATUS_OK )
        {
            /* Rule is now idle */
            rule->ruleState = PCD_RULE_IDLE;

            /* Wait for end of process */
            if ( cookie )
            {
                retval = PCD_STATUS_WAIT;
            }
        }
    }

    return retval;
}

static PCD_status_e PCD_api_start( rule_t *rule )
{
    /* Enqueue the rule */
    return PCD_timer_enqueue_rule( rule );
}

static PCD_status_e PCD_api_signal( rule_t *rule, int32_t sig )
{
    /* Allow only SIGUSR signals */
    if ( ( sig == SIGUSR1 ) || ( sig == SIGUSR2 ) )
    {
        return PCD_process_signal_by_rule( rule, sig );
    }

    return PCD_STATUS_BAD_PARAMS;
}

PCD_status_e PCD_api_init( void )
{
    /* Init IPC */
    if ( IPC_init( 0 ) != PCD_STATUS_OK )
    {
        PCD_PRINTF_STDERR( "Failed to init PCD API");

        return PCD_STATUS_NOK;
    }

    /* Start IPC */
    if ( IPC_start( CONFIG_PCD_SERVER_NAME, &pcdContext, 0 ) != PCD_STATUS_OK )
    {
        PCD_PRINTF_STDERR( "Failed to start IPC" );

        return PCD_STATUS_NOK;
    }

    /* Set owner */
    if ( IPC_set_owner( pcdContext, CONFIG_PCD_OWNER_ID ) != PCD_STATUS_OK )
    {
        IPC_stop( pcdContext );

        PCD_PRINTF_STDERR( "Failed to set ownership");

        return PCD_STATUS_NOK;
    }

    return PCD_STATUS_OK;
}

PCD_status_e PCD_api_deinit( void )
{
    /* Stopping and terminating IPC - Use before end of life only! */
    IPC_stop( pcdContext );    
    
    return PCD_STATUS_OK;
}

PCD_status_e PCD_api_check_messages( void )
{
    IPC_message_t *msg;
    u_int32_t budget = 5;

    /* Check for incoming messages */
    while ( budget-- && IPC_wait_msg( pcdContext, &msg, IPC_TIMEOUT_IMMEDIATE ) == 0 )
    {
        pcdApiMessage_t *data = IPC_get_msg( msg );
        rule_t *rule;
        PCD_status_e retval = PCD_STATUS_NOK;
        IPC_message_t *replyMsg = NULL;
        pcdApiReplyMessage_t *replyData = NULL;
        IPC_context_t msgContext;

        /* Check if we need to reply */
        if ( IPC_get_msg_context( msg, &msgContext ) == PCD_STATUS_OK )
        {
            /* Allocate memory for reply message */
            replyMsg = IPC_alloc_msg( pcdContext, sizeof( pcdApiReplyMessage_t ) );

            if ( !replyMsg )
            {
                PCD_PRINTF_STDERR( "Failed to allocate memory for reply message" );
                IPC_free_msg( msg );
                return PCD_STATUS_NOK;
            }
            else
            {
                /* Initialize the reply pointer */
                replyData = IPC_get_msg( replyMsg );

                /* Return the message ID */
                replyData->msgId = data->msgId;
            }
        }

        if ( data->type == PCD_API_PROCESS_READY )
        {
            /* Find the rule */
            rule = PCD_process_get_rule_by_pid( data->pid );

            if ( !rule )
            {
                PCD_PRINTF_WARNING_STDOUT( "Got READY event, but cannot find an associated rule to pid %d", data->pid );
                retval = PCD_STATUS_INVALID_RULE;
            }
            else
            {
                /* Setup ready only if this is the end condition */
                if ( rule->endCondition.type == PCD_END_COND_KEYWORD_PROCESS_READY )
                {
                    rule->endCondition.processReady = True;
                    retval = PCD_STATUS_OK;
                }
                else
                {
                    PCD_PRINTF_WARNING_STDOUT( "Got READY event, but rule end condition is different" );
                    retval = PCD_STATUS_BAD_PARAMS;
                }
            }
        }
        else if ( ( data->type == PCD_API_REDUCE_NETRX_PRIORITY ) || ( data->type == PCD_API_RESTORE_NETRX_PRIORITY ) )
        {
            /* Handle net-rx related commands */
            if ( data->type == PCD_API_REDUCE_NETRX_PRIORITY )
            {
                retval = PCD_misc_reduce_net_rx_priority( data->priority );
            }
            else
            {
                retval = PCD_misc_restore_net_rx_priority();
            }
        }
        else
        {
            /* Find the rule */
            rule = PCD_rulesdb_get_rule_by_id( &data->ruleId );

            if ( rule )
            {
                /* Activate the required command */
                switch ( data->type )
                {
                    case PCD_API_START_PROCESS:
                        if ( data->params[ 0 ] )
                            PCD_rulesdb_setup_optional_params( rule, data->params );
                        retval = PCD_api_start( rule );
                        break;

                    case PCD_API_TERMINATE_PROCESS_SYNC:
                        /* Send the incoming message as a cookie */
                        retval = PCD_api_stop( rule, False, ( void *)msg );

                        if ( retval == PCD_STATUS_WAIT )
                        {
                            /* We don't reply now. Calling context is blocked */
                            if ( replyMsg )
                            {
                                IPC_free_msg( replyMsg );
                                replyMsg = NULL;
                            }
                        }
                        break;

                    case PCD_API_TERMINATE_PROCESS:
                        retval = PCD_api_stop( rule, False, NULL );
                        break;

                    case PCD_API_KILL_PROCESS:
                        retval = PCD_api_stop( rule, True, NULL );
                        break;

                    case PCD_API_SIGNAL_PROCESS:
                        retval = PCD_api_signal( rule, data->sig );
                        break;

                    case PCD_API_GET_RULE_STATE:
                        retval = PCD_STATUS_OK;

                        switch ( rule->ruleState )
                        {
                            case PCD_RULE_IDLE:
                                replyData->ruleState = PCD_API_RULE_IDLE;
                                break;

                            case PCD_RULE_ACTIVE:
                            case PCD_RULE_START_CONDITION_WAITING:
                            case PCD_RULE_END_CONDITION_WAITING:
                                replyData->ruleState = PCD_API_RULE_RUNNING;
                                break;

                            case PCD_RULE_COMPLETED:
                                if ( rule->proc )
                                {
                                    replyData->ruleState = PCD_API_RULE_COMPLETED_PROCESS_RUNNING;
                                }
                                else
                                {
                                    replyData->ruleState = PCD_API_RULE_COMPLETED_PROCESS_EXITED;
                                }
                                break;

                            case PCD_RULE_FAILED:
                                replyData->ruleState = PCD_API_RULE_FAILED;
                                break;

                            case PCD_RULE_NOT_COMPLETED:
                                replyData->ruleState = PCD_API_RULE_NOT_COMPLETED;
                                break;

                            default:
                                retval = PCD_STATUS_NOK;
                                break;
                        }
                        break;

                    default:
                        retval = PCD_STATUS_BAD_PARAMS;
                        PCD_PRINTF_WARNING_STDOUT( "Invalid request %d (for rule %s_%s), aborting", data->type, data->ruleId.groupName, data->ruleId.ruleName );
                        break;
                }
            }
            else
            {
                PCD_PRINTF_WARNING_STDOUT( "Rule %s_%s not found, aborting request %d", data->ruleId.groupName, data->ruleId.ruleName, data->type );

                /* Rule not found */
                retval = PCD_STATUS_INVALID_RULE;
            }
        }

        if ( replyMsg )
        {
            /* Return value in response */
            replyData->retval = retval;

            /* Send response */
            if ( IPC_reply_msg( msg, replyMsg ) != PCD_STATUS_OK )
            {
                IPC_free_msg( replyMsg );
            }

        }

        /* Free only if completed. Don't free in sync termination */
        if ( retval != PCD_STATUS_WAIT )
        {
            /* Free incoming message */
            IPC_free_msg( msg );
        }
    }

    return PCD_STATUS_OK;
}

void PCD_api_reply_message( void *cookie, PCD_status_e retval )
{
    IPC_message_t *msg = cookie;
    pcdApiMessage_t *data;
    IPC_message_t *replyMsg = NULL;
    pcdApiReplyMessage_t *replyData = NULL;
    IPC_context_t msgContext;

    if ( !msg )
    {
        return;
    }

    data = IPC_get_msg( msg );

    /* Check if we need to reply */
    if ( IPC_get_msg_context( msg, &msgContext ) == PCD_STATUS_OK )
    {
        /* Allocate memory for reply message */
        replyMsg = IPC_alloc_msg( pcdContext, sizeof( pcdApiReplyMessage_t ) );

        if ( !replyMsg )
        {
            PCD_PRINTF_STDERR( "Failed to allocate memory for termination reply message" );
            IPC_free_msg( msg );
            return;
        }
        else
        {
            /* Initialize the reply pointer */
            replyData = IPC_get_msg( replyMsg );

            /* Return the message ID */
            replyData->msgId = data->msgId;
        }

        /* Return value in response */
        replyData->retval = retval;

        /* Send response */
        if ( IPC_reply_msg( msg, replyMsg ) != PCD_STATUS_OK )
        {
            PCD_PRINTF_STDERR( "Failed to send termination reply message" );
            IPC_free_msg( replyMsg );

            /* Free incoming message */
            IPC_free_msg( msg );
            return;
        }
    }

    /* Free incoming message */
    IPC_free_msg( msg );
}

