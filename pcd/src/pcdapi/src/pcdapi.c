/*
 * pcdapi.c
 * Description:
 *
 *
 * Copyright (C) 2010 Texas Instruments Incorporated - http://www.ti.com/
 *
 * This library is free software; you can redistribute it and/or
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
 * Copyright (C) 2010-12 PCD Project - http://www.rt-embedded.com/pcd
 * 
 * Change log:
 * - Support dynamic configurations from kconfig
 * - Support MIPS platform
 * - Support x86 platform
 * - Support x64 platform
 * - Fix the process find function
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
#include <errno.h>
#include <unistd.h>
#include <sys/ucontext.h>
#include <sys/time.h>
#include <time.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include "rules_db.h"
#include "system_types.h"
#include "ipc.h"
#include "pcd_api.h"
#include "pcdapi.h"
#include "except.h"


/*! \def PCD_API_REPLY_TIMEOUT
 *  \brief Timeout for PCD response
 */
#define PCD_API_REPLY_TIMEOUT     5000

static bool_t pcdApiInitDone = False;
bool_t verboseOutput = True;
static char procName[ PCD_EXCEPTION_MAX_PROCESS_NAME ];
static Cleanup_func cleanupFunc = NULL;
static char mapsFile[ 18 ];
static char mapsTmpFile[ 22 ];

static void PCD_exception_default_handler(int32_t signo, siginfo_t *info, void *context);

#define SETSIG(sa, sig, func) \
    {    memset( &sa, 0, sizeof( struct sigaction ) ); \
         sa.sa_sigaction = func; \
         sa.sa_flags = SA_RESTART | SA_SIGINFO; \
		sigaction(sig, &sa, 0L); \
    }

/**************************************************************************/
/*      IMPLEMENTATION                                                    */
/**************************************************************************/

/**************************************************************************/
/*! \fn PCD_api_malloc_and_send()									*/
/**************************************************************************/
/*  \brief 		Allocate and send an IPC message                       *
 *  \param[in] 		ruleId, type, optional parameters, pid 			*
 *  \param[in,out] 	None										*
 *  \return			PCD_STATUS_OK - Success, <0 - Error	               *
 **************************************************************************/
static PCD_status_e PCD_api_malloc_and_send( const struct ruleId_t *ruleId, pcdApi_e type, void *ptr, int32_t value )
{
    IPC_message_t *msg;
    IPC_context_t pcdCtx, pcdTmpCtx;
    IPC_timeout_e timeout = PCD_API_REPLY_TIMEOUT;
    pcdApiMessage_t *data;
    PCD_status_e retval = PCD_STATUS_OK;
    u_int32_t msgId;
    char pcdClient[ 32 ];

    if ( !pcdApiInitDone )
    {
        IPC_init( 0 );
        pcdApiInitDone = True;
    }

    if ( IPC_get_context_by_owner( &pcdCtx, CONFIG_PCD_OWNER_ID ) != PCD_STATUS_OK )
    {
        printf( "pcd: Error: Failed to find PCD context\n");       
        return PCD_STATUS_INVALID_RULE;
    }

    sprintf( pcdClient, CONFIG_PCD_CLIENTS_NAME_PREFIX "%d", getpid() );

    /* Create temporary destination point */
    if ( IPC_start( pcdClient, &pcdTmpCtx, 0) != PCD_STATUS_OK )
    {
        printf( "pcd: Error: Failed to start IPC\n");
        return PCD_STATUS_NOK;
    }

    /* Allocate a message */
    msg = IPC_alloc_msg( pcdTmpCtx, sizeof( pcdApiMessage_t ) );

    if ( !msg )
    {
        printf( "pcd: Error: Failed to allocate memory\n" );
        retval = PCD_STATUS_NOK;

        goto end_malloc_and_send;
    }

    data = IPC_get_msg( msg );

    /* Generate a message ID using an uninitialized data[0] */
    msgId = *(u_int32_t *)( data ) - (u_int32_t)(pcdTmpCtx);

    /* Clear data */
    memset( data, 0, sizeof( pcdApiMessage_t ) );

    /* Setup message */
    data->type = type;
    data->msgId = msgId;

    if ( ruleId )
    {
        /* Check that the given ruleId is not NULL */
        if ( ( !ruleId->groupName[ 0 ] ) || ( !ruleId->ruleName[ 0 ] ) )
        {
            printf( "pcd: Error: Invalid rule ID\n" );
            IPC_free_msg( msg );
            retval = PCD_STATUS_NOK;

            goto end_malloc_and_send;
        }

        /* Initialize rule */
        memcpy( &data->ruleId, ruleId, sizeof( ruleId_t ) );
    }

    switch ( type )
    {
        case PCD_API_PROCESS_READY:
            data->pid = value;
            break;

        case PCD_API_START_PROCESS:
            if ( ptr )
            {
                /* Copy optional parameters to activate the rule differently */
                strncpy( data->params, ( char *)ptr, CONFIG_PCD_MAX_PARAM_SIZE - 1 );
            }
            break;

        case PCD_API_SIGNAL_PROCESS:
            data->sig = value;
            break;

        case PCD_API_TERMINATE_PROCESS_SYNC:
            /* Process termination may take longer */
            timeout = PCD_API_REPLY_TIMEOUT * 4;
            break;

        case PCD_API_REDUCE_NETRX_PRIORITY:
            data->priority = value;
            break;

        case PCD_API_RESTORE_NETRX_PRIORITY:
        case PCD_API_KILL_PROCESS:
        case PCD_API_TERMINATE_PROCESS:
        case PCD_API_GET_RULE_STATE:
            break;

        default:
            IPC_free_msg( msg );
            retval = PCD_STATUS_BAD_PARAMS;
            goto end_malloc_and_send;
    }

    /* Send the request to the PCD */
    if ( IPC_send_msg( pcdCtx, msg ) == 0 )
    {
        IPC_message_t *replyMsg;

        do
        {
            /* Wait for incoming reply */
            if ( IPC_wait_msg( pcdTmpCtx, &replyMsg, timeout ) == 0 )
            {
                pcdApiReplyMessage_t *replyData = IPC_get_msg( replyMsg );

                if ( replyData->msgId != msgId )
                {
                    /* This is not our message!? - Delete it to avoid resource leak */
                    IPC_free_msg( replyMsg );
                    continue;
                }

                retval = replyData->retval;

                if ( type == PCD_API_GET_RULE_STATE )
                {
                    pcdApiRuleState_e *ruleState = (pcdApiRuleState_e *)ptr;

                    if ( ruleState )
                    {
                        /* Return rule state */
                        *ruleState = replyData->ruleState;
                    }
                }

                /* Free the message, we are done */
                IPC_free_msg( replyMsg );
                break;
            }
            else
            {
                /* Return with status timeout, don't dispose message */
                retval = PCD_STATUS_TIMEOUT;
                break;
            }

        } while ( 1 );
    }
    else
    {
        retval = PCD_STATUS_NOK;
        IPC_free_msg( msg );
    }

    end_malloc_and_send:

    /* Delete temporary dest point */
    IPC_stop( pcdTmpCtx );

    return retval;
}

/**************************************************************************/
/*! \fn PCD_api_start_process()									*/
/**************************************************************************/
/*  \brief 		Start a process associated with a rule				*
 *  \param[in] 		ruleId, optional parameters				     *
 *  \param[in,out] 	None										*
 *  \return			PCD_STATUS_OK - Success, <0 - Error	               *
 **************************************************************************/
PCD_status_e PCD_api_start_process( const struct ruleId_t *ruleId, const char *optionalParams )
{
    return PCD_api_malloc_and_send( ruleId, PCD_API_START_PROCESS, ( void *)optionalParams, -1 );
}

/**************************************************************************/
/*! \fn PCD_api_signal_process()									*/
/**************************************************************************/
/*  \brief 		Signal a process associated with a rule				*
 *  \param[in] 		ruleId, signal id				     *
 *  \param[in,out] 	None										*
 *  \return			PCD_STATUS_OK - Success, <0 - Error	               *
 **************************************************************************/
PCD_status_e PCD_api_signal_process( const struct ruleId_t *ruleId, int32_t sig )
{
    return PCD_api_malloc_and_send( ruleId, PCD_API_SIGNAL_PROCESS, NULL, sig );
}

/**************************************************************************/
/*! \fn PCD_api_terminate_process()								*/
/**************************************************************************/
/*  \brief 		Terminate a process associated with a rule  			*
 *  \param[in] 		ruleId								     *
 *  \param[in,out] 	None										*
 *  \return			PCD_STATUS_OK - Success, <0 - Error	               *
 **************************************************************************/
PCD_status_e PCD_api_terminate_process( const struct ruleId_t *ruleId )
{
    return PCD_api_malloc_and_send( ruleId, PCD_API_TERMINATE_PROCESS_SYNC, NULL, -1 );
}

/**************************************************************************/
/*! \fn PCD_api_terminate_process_non_blocking()								*/
/**************************************************************************/
/*  \brief 		Terminate a process associated with a rule  			*
 *  \param[in] 		ruleId								     *
 *  \param[in,out] 	None										*
 *  \return			PCD_STATUS_OK - Success, <0 - Error	               *
 **************************************************************************/
PCD_status_e PCD_api_terminate_process_non_blocking( const struct ruleId_t *ruleId )
{
    return PCD_api_malloc_and_send( ruleId, PCD_API_TERMINATE_PROCESS, NULL, -1 );
}

/**************************************************************************/
/*! \fn PCD_api_kill_process()									*/
/**************************************************************************/
/*  \brief 		Kill a process associated with a rule				*
 *  \param[in] 		ruleId								     *
 *  \param[in,out] 	None										*
 *  \return			PCD_STATUS_OK - Success, <0 - Error	               *
 **************************************************************************/
PCD_status_e PCD_api_kill_process( const struct ruleId_t *ruleId )
{
    return PCD_api_malloc_and_send( ruleId, PCD_API_KILL_PROCESS, NULL, -1 );
}

/**************************************************************************/
/*! \fn PCD_api_send_process_ready()								*/
/**************************************************************************/
/*  \brief 		Send PROCESS_READY event to PCD                        *
 *  \param[in] 		None
 *  \param[in,out] 	None										*
 *  \return			PCD_STATUS_OK - Success, <0 - Error	               *
 **************************************************************************/
PCD_status_e PCD_api_send_process_ready( void )
{
    return PCD_api_malloc_and_send( NULL, PCD_API_PROCESS_READY, NULL, getpid() );
}

/**************************************************************************/
/*! \fn PCD_api_get_rule_state()									*/
/**************************************************************************/
/*  \brief 		Get rule state				*
 *  \param[in] 		ruleId                      				     *
 *  \param[in,out] 	ruleState, see pcdApiRuleState_e                    *
 *  \return			PCD_STATUS_OK - Success, <0 - Error	               *
 **************************************************************************/
PCD_status_e PCD_api_get_rule_state( const struct ruleId_t *ruleId, pcdApiRuleState_e *ruleState )
{
    if ( !ruleState )
    {
        return PCD_STATUS_BAD_PARAMS;
    }

    return PCD_api_malloc_and_send( ruleId, PCD_API_GET_RULE_STATE, ( void *)ruleState, -1 );
}

/**************************************************************************/
/*! \fn PCD_api_register_exception_handlers()                  			*/
/**************************************************************************/
/*  \brief 		Register default PCD exception handler                 *
 *  \param[in] 		argv[0]       								*
 *  \param[in,out] 	None										*
 *  \return			PCD_STATUS_OK - Success, <0 - Error	               *
 **************************************************************************/
PCD_status_e PCD_api_register_exception_handlers( char *name, Cleanup_func cleanup )
{
    struct sigaction sa;
    pid_t pid = getpid();

    if ( !name )
        return PCD_STATUS_NOK;

    if ( cleanup )
        cleanupFunc = cleanup;

    memset( procName, 0, PCD_EXCEPTION_MAX_PROCESS_NAME );
    strncpy( procName, name, PCD_EXCEPTION_MAX_PROCESS_NAME - 1 );

    strcpy( mapsFile, "/proc/self/maps" );
    sprintf( mapsTmpFile, "%s/%d.maps", CONFIG_PCD_TEMP_PATH, pid );

    SETSIG(sa, SIGINT,  PCD_exception_default_handler);
    SETSIG(sa, SIGSEGV, PCD_exception_default_handler);
    SETSIG(sa, SIGILL,  PCD_exception_default_handler);
    SETSIG(sa, SIGBUS,  PCD_exception_default_handler);
    SETSIG(sa, SIGQUIT, PCD_exception_default_handler);
    SETSIG(sa, SIGFPE,  PCD_exception_default_handler);

    return PCD_STATUS_OK;
}

/**************************************************************************/
/*! \fn PCD_api_find_process_id( char *name )                             */
/**************************************************************************/
/*  \brief Find process ID, detects if another instance alrady running.   *
 *  \param[in] 		Process name       							*
 *  \return pid on success, or 0 if not found                             *
 **************************************************************************/
pid_t PCD_api_find_process_id( char *name )
{
    int32_t fd = -1;
    char filename[ 25 ];
    char lbuf[ 255 ];
    int32_t currPid, endPid;

    endPid = getpid();

    for ( currPid = 1; currPid < endPid; currPid++ )
    {
        /* Read command line from /proc/PID/stat */
        sprintf( filename, "/proc/%d/stat", currPid );
        fd = open( filename, O_RDONLY );

        if ( fd >= 0 )
        {
            if ( read( fd, lbuf, 255 ) > 0 )
            {
                char *p1, *p2;

                /* command is sorrounded by () */
                p1 = strchr( lbuf, '(' );
                p2 = strchr( lbuf, ')' );

                if ( ( p1 ) && ( p2 ) )
                {
                    p1++;
                    *p2 = '\0';

                    if ( strncmp( p1, name, strlen( p1 ) ) == 0 )
                    {
                        close(fd);
                        return currPid;
                    }
                }
            }

            close(fd);
        }
    }

    /* Not found */
    return 0;
}

/**************************************************************************/
/*! \fn PCD_api_reboot( char *reason )                                    */
/**************************************************************************/
/*  \brief Display a reboot reason (optional) and reboot the system.      *
 *  \param[in] 		Reboot reason (optinal)       						  *
 *  \param[in] 		Force: Force reboot even if PCD is in debug mode.     *
 *  \return Never returns                                                 *
 **************************************************************************/
void PCD_api_reboot( const char *reason, bool_t force )
{
    char fname[30]; /* Provides space for 15 digits for the pid */
    int32_t fd;
    pid_t pid = getpid();
    char cmdline[ 255 ] = { '\0'};

    /* Get filename by PID */
    sprintf(fname, "/proc/%d/cmdline", pid);
    fd = open( fname, O_RDONLY );

    if ( fd >= 0 )
    {
        /* Get the process command line */
        if ( read( fd, cmdline, sizeof(cmdline)-1 ) < 0 )
        {
            sprintf(cmdline, "<Unknown>" );
        }
        close(fd);
    }
    else
    {
        sprintf(cmdline, "<Unknown>" );
    }

    printf( "pcd: Process %s (%d) requested system reboot.\n", cmdline, pid );
    if ( reason )
    {
        printf( "Reboot reason:\n%s\n", reason );
    }

    /* Flush all messages */
    fflush( stdout );

    if ( ( force == False ) && ( pid = PCD_api_find_process_id( "/usr/sbin/pcd" ) ) > 0 )
    {
        /* Prefer that PCD will take the system down */
        kill( pid, SIGTERM );
    }
    else
    {
        /* In case PCD is not there (not likely) or forcing reboot, kill init */
        kill( 1, SIGTERM );
    }

    /* Never exit, wait for system termination */
    while ( 1 );
}

static void PCD_exception_default_handler(int32_t signo, siginfo_t *info, void *context)
{
    exception_t exception;
    int32_t fd1, fd2;
    int32_t total = sizeof( exception_t );
    int32_t i;

    /* Get platform specific registers */
#if defined(CONFIG_PCD_PLATFORM_ARM) || defined(CONFIG_PCD_PLATFORM_X86) \
    || defined(CONFIG_PCD_PLATFORM_MIPS) || defined(CONFIG_PCD_PLATFORM_X64)
    ucontext_t *ctx = (ucontext_t *)context;
#endif

    exception.magic = PCD_EXCEPTION_MAGIC;

    /* Copy process name. Avoid using strcpy which is not safe in our condition */
    for ( i=0; i < PCD_EXCEPTION_MAX_PROCESS_NAME; i++ )
        exception.process_name[ i ] = procName[ i ];

    exception.process_id = getpid();
    exception.signal_code = info->si_code;
    exception.signal_number = signo;
    exception.signal_errno = info->si_errno;
    exception.handler_errno = errno;
    exception.fault_address = info->si_addr;
    clock_gettime(CLOCK_REALTIME, &exception.time);
#ifdef CONFIG_PCD_PLATFORM_ARM /* ARM registers */
    exception.regs = ctx->uc_mcontext;
#endif

/* X86 processor context */ /* MIPS registers */
#if defined(CONFIG_PCD_PLATFORM_X86) || defined(CONFIG_PCD_PLATFORM_MIPS)
    exception.uc_mctx = ctx->uc_mcontext;
#endif

#if defined(CONFIG_PCD_PLATFORM_X64) /* x64 Registers */
    exception.uc_mcontext = ctx->uc_mcontext;
#endif

    fd1 = open( mapsFile, O_RDONLY );

    if ( fd1 > 0 )
    {
        u_int8_t buf[ 512 ];
        int32_t readBytes = 0;

        /* Create a temporary file which will be available after the process is dead */
        fd2 = open( mapsTmpFile, O_CREAT | O_WRONLY | O_SYNC, S_IRWXU | S_IRWXG );

        if ( fd2 > 0 )
        {
            /* Read the maps file */
            while ( ( readBytes = read( fd1, buf, sizeof(buf) ) ) > 0 )
            {
                /* Best effort write */
                if( write( fd2, buf, readBytes ) <= 0 )
					break;
            }

            close(fd2);
        }

        close(fd1);
    }

    /* Send exception information to PCD */
    fd1 = open( PCD_EXCEPTION_FILE, O_WRONLY | O_SYNC );

    if ( fd1 < 0 )
        exit(1);

    while ( total > 0 )
    {
        int32_t written;

        written = write( fd1, &exception, sizeof( exception_t ) );

        if ( written > 0 )
        {
            total -= written;
        }
    }

    close( fd1 );

    /* If the process registered a cleanup function, call it */
    if ( cleanupFunc )
        cleanupFunc( signo );

    exit( 1 );
}


/*! \fn PCD_api_reduce_net_rx_priority
 *  \brief Reduce net-rx task priority to a given priority value (non preemtive mode)
 *  \param[in] 		New priority: 19 (lowest) to -19 (highest)
 *  \return 		PCD_STATUS_OK - Success, <0 - Error
 */
PCD_status_e PCD_api_reduce_net_rx_priority( int32_t priority )
{
    return PCD_api_malloc_and_send( NULL, PCD_API_REDUCE_NETRX_PRIORITY, NULL, priority );
}

/*! \fn PCD_api_restore_net_rx_priority
 *  \brief Restore net-rx task original priority
 *  \param[in,out] 	None
 *  \return			PCD_STATUS_OK - Success, <0 - Error
 */
PCD_status_e PCD_api_restore_net_rx_priority( void )
{
    return PCD_api_malloc_and_send( NULL, PCD_API_RESTORE_NETRX_PRIORITY, NULL, 0 );
}
