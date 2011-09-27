/*
 * process.c
 * Description:
 * PCD process management implementation file
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
 * - Hai Shalom: Experimental: support uClinux vfork instead of fork for 
 *   MMU-less platforms.
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
#include <unistd.h>
#include <time.h>
#include <malloc.h>
#include <signal.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <pty.h>
#include <errno.h>
#include <syslog.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/ucontext.h>
#include <sys/time.h>
#include <fcntl.h>
#include <sched.h>
#include "rules_db.h"
#include "process.h"
#include "timer.h"
#include "pcd.h"
#include "except.h"
#include "pcd_api.h"
#include "ipc.h"

/**************************************************************************/
/*      LOCAL DEFINITIONS AND VARIABLES                                   */
/**************************************************************************/
extern int32_t errno;

procObj_t *procList = NULL;

#define PCD_PROCESS_MAX_PARAMS 32
#define PCD_PROCESS_NAME       "/var/pcd_proc"

#define NODE_ADD(head, node)    \
    {   node->next=head;        \
        node->prev=NULL;        \
        if(head) (head)->prev=node; \
        head=node;          \
    }

#define NODE_DEL(head, node)    \
    {   if(node->next) node->next->prev=node->prev; \
        if(node->prev) node->prev->next=node->next; \
        else head=node->next;               \
    }

#define SETSIG(sa, sig, func) \
    {    memset( &sa, 0, sizeof( struct sigaction ) ); \
         sa.sa_handler = func; \
         sa.sa_flags = SA_RESTART; \
        sigaction(sig, &sa, 0L); \
    }

#define SETSIGINFO(sa, sig, func) \
    {    memset( &sa, 0, sizeof( struct sigaction ) ); \
         sa.sa_sigaction = func; \
         sa.sa_flags = SA_RESTART | SA_SIGINFO; \
        sigaction(sig, &sa, 0L); \
    }

#define PCD_PROCESS_WHITE_SPACES    " \t\n\r"

#ifdef PCD_USE_VFORK
#define __fork vfork
#define __exit exit
#else
#define __fork fork
#define __exit _exit
#endif

/* Signal handlers */
static void PCD_process_terminate(int signo, siginfo_t *info, void *context);
static void PCD_process_chld(pid_t pid, int st);
static void PCD_process_chld_handler(int p);

/* Internal functions */
static procObj_t *PCD_process_find_by_state(procObj_t *p, procState_e state);
static void PCD_process_free( procObj_t *ptr );
static procObj_t *PCD_process_spawn(procObj_t *proc);
static procObj_t *PCD_process_new( rule_t *rule );
static void PCD_process_free( procObj_t *ptr );
static PCD_status_e PCD_process_trigger_action( rule_t *rule );

char *strsignal( int );

/**************************************************************************/
/*      IMPLEMENTATION                                                    */
/**************************************************************************/


static void PCD_process_terminate(int signo, siginfo_t *info, void *context)
{
    const char msg2[]= "pcd: Terminating PCD, rebooting system...\n";
    int32_t i;

    /* In case the PCD is terminated, reboot the system. */
    if ( signo != SIGTERM )
    {
        const char msg1[]= "pcd: Caught fault signal.\n";
        exception_t exception;
        int32_t fd;

    /* Get platform specific registers */
#if defined(CONFIG_PCD_PLATFORM_ARM) || defined(CONFIG_PCD_PLATFORM_X86) \
    || defined(CONFIG_PCD_PLATFORM_MIPS) || defined(CONFIG_PCD_PLATFORM_X64)
        ucontext_t *ctx = (ucontext_t *)context;
#endif
        char *procName = "pcd";
        const char msg3[]= "pcd: Wrote exception information.\n";

        /* Display an error message */
        i = write( STDERR_FILENO, msg1, sizeof(msg1) );

        /* Prepare a self exception file in case the PCD has crashed (Could never happen :-)) */
        exception.magic = PCD_EXCEPTION_MAGIC;

        /* Copy process name. Avoid using strcpy which is not safe in our condition */
        for ( i=0; i < 4; i++ )
            exception.process_name[ i ] = procName[ i ];

        for ( i=4; i < PCD_EXCEPTION_MAX_PROCESS_NAME; i++ )
            exception.process_name[ i ] = 0;

        /* Setup fault information */
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
        /* X64 registers */
#if defined(CONFIG_PCD_PLATFORM_X64)
        exception.uc_mcontext = ctx->uc_mcontext; 
#endif
        /* Open the self exception file */
        fd = open( CONFIG_PCD_PROCESS_SELF_EXCEPTION_DIRECTORY "/" CONFIG_PCD_PROCESS_SELF_EXCEPTION_FILE, O_CREAT | O_WRONLY | O_SYNC, S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP );

        if ( fd > 0 )
        {
            /* Write information for debug */
            i = write( fd, &exception, sizeof( exception ) );
            close(fd);
            i = write( STDERR_FILENO, msg3, sizeof( msg3) );
        }
    }

    /* Display reboot message */
    i = write( STDERR_FILENO, msg2, sizeof(msg2) );

    /* Stop IPC */
    PCD_api_deinit();

    /* Stop PCD timer */
    PCD_timer_stop();

    /* Avoid unsafe prints */
    verboseOutput = False;

    /* Close exception file */
    PCD_exception_close();

    /* Kill all the child processes and reboot */
    PCD_process_reboot();

    exit(1);
}

static void PCD_process_chld(pid_t pid, int st)
{
    procObj_t *ptr = procList;

    while ( ptr )
    {
        /* Find out what happend to the process, and what is the return code */
        if ( ptr->pid == pid )
        {
            if ( WIFEXITED(st) )
            {
                ptr->retstat = PCD_PROCESS_RETEXITED;
                ptr->retcode = WEXITSTATUS(st);
            }
            else if ( WIFSIGNALED(st) )
            {
                ptr->retstat = PCD_PROCESS_RETSIGNALED;
                ptr->retcode = WTERMSIG(st);
            }
            else if ( WIFSTOPPED(st) )
            {
                ptr->retstat = PCD_PROCESS_RETSTOPPED;
                ptr->retcode = WSTOPSIG(st);
            }

            /* STOPPING is a special case, due to the asynch signal */
            ptr->state = PCD_PROCESS_STOPPING;
            break;
        }
        ptr = ptr->next;
    }
}

static void PCD_process_chld_handler(int p)
{
    int st;
    pid_t pid;

    while ( (pid = waitpid(-1, &st, WNOHANG)) > 0 )
    {
        PCD_process_chld(pid, st);
    }
}

static procObj_t *PCD_process_spawn(procObj_t *proc)
{
    pid_t pid;
    sigset_t nmask, omask;
    int i;
    char *args[PCD_PROCESS_MAX_PARAMS+3];
    procObj_t *next;
    char params[ CONFIG_PCD_MAX_PARAM_SIZE ] = { 0};
    rule_t *rule;

    /* Check validity of parameters */
    if ( !proc )
        return NULL;

    rule = proc->rule;

    if ( !rule )
        return NULL;

    sigemptyset(&nmask);
    sigaddset(&nmask, SIGCHLD);
    sigprocmask(SIG_BLOCK, &nmask, &omask);

    /* Fork, create a child process */
    pid = __fork();

    if ( !pid )
    {
        char *token;
        char vars[ CONFIG_PCD_MAX_PARAM_SIZE ];
        u_int32_t varsIdx = 0;

        /* Setup executable name */
        args[0] = args[1] = rule->command;
        i = 2;

        if ( rule->optionalParams )
        {
            /* Spawn the process with the optional parameters received by the PCD API */
            strcpy( params, rule->optionalParams );
        }
        else if ( rule->params )
        {
            /* Use default parameters */
            strncpy( params, rule->params, CONFIG_PCD_MAX_PARAM_SIZE - 1 );
        }
        else
        {
            /* Skip the params handling code */
            goto no_params;
        }

        /* Clear parameters array */
        memset( vars, 0, CONFIG_PCD_MAX_PARAM_SIZE );

        /* Get first token */
        token = strtok( params, PCD_PROCESS_WHITE_SPACES );

        /* Setup parameters */
        while ( ( token != NULL ) && ( i < PCD_PROCESS_MAX_PARAMS ) )
        {
            char *ptr;
            char *p_var = NULL;

            /* Check if we have an environment variable in the parameters list */
            ptr = strchr( token, '$' );

            if ( ptr )
            {
                char *env = NULL;
                char *var;
                char buff[ CONFIG_PCD_MAX_PARAM_SIZE ];
                char var_name[ 64 ];

                if( ( *(ptr + 1) ) == '{' )
                {
                	p_var = strchr( ptr + 2, '}' );

                	if( p_var )
                	{
                    	u_int32_t val = p_var - ( ptr + 2 );

                		memset( var_name, 0, sizeof( var_name ) );
                    	if( val >= sizeof( var_name ) )
                			goto parse_param_next;

                		memcpy( var_name, ptr + 2, val );
                	}
                }

                /* Get the variable value from CONFIG_PCD_TEMP_PATH */
                {
                    int32_t fd;
                    char filename[ 255 ];

                    sprintf( filename, CONFIG_PCD_TEMP_PATH "/%s", var_name );

                    fd =  open( filename, O_RDONLY );
                    if ( fd > 0 )
                    {
                        memset( buff, 0, CONFIG_PCD_MAX_PARAM_SIZE );

                        /* Read the variable value from the file */
                        if ( read( fd, buff, CONFIG_PCD_MAX_PARAM_SIZE-1 ) > 0 )
                        {
                            env = buff;
                        }
                        close(fd);
                    }
                }

                if ( !env )
                {
                    /* Perhaps it is in the environment ? */
                    env = getenv( var_name );

                    if ( !env )
                    {
                        /* No such variable, ignore */
                        goto parse_param_next;
                    }
                    else
                    {
                        memset( buff, 0, CONFIG_PCD_MAX_PARAM_SIZE );

                        /* Copy the value to the temporary buffer */
                        strncpy( buff, env, CONFIG_PCD_MAX_PARAM_SIZE-1 );
                        env = buff;
                    }
                }

                /* Copy to a local buffer */
                strncpy( vars + varsIdx, env, CONFIG_PCD_MAX_PARAM_SIZE-varsIdx-1 );
                env = &vars[ varsIdx ];

                /* Advance the index for next buffer */
                varsIdx += strlen( vars ) + 1;
                if ( varsIdx >= CONFIG_PCD_MAX_PARAM_SIZE - 1 )
                {
                    /* No place in buffer */
                    varsIdx = CONFIG_PCD_MAX_PARAM_SIZE - 1;
                }

                /* Replace white spaces with NULLs */
                if ( (var = strpbrk( env, PCD_PROCESS_WHITE_SPACES ) ) != NULL )
                {
                    *var = 0;
                }

                while ( i < PCD_PROCESS_MAX_PARAMS )
                {
                    /* Init argument */
                    args[ i ] = env;

                    if ( !var )
                    {
                    	/* No more arguments in the buffer, look for other strings in the buffer */
                    	if( p_var )
                        {
                        	strncat( env, p_var + 1, CONFIG_PCD_MAX_PARAM_SIZE-varsIdx-1 );
                        	varsIdx += strlen( p_var + 1 );
                        }
                        break;
                    }

                    /* Advance to next argument in the buffer */
                    env = var + 1;

                    /* Replace white spaces with NULLs */
                    if ( (var = strpbrk( env, PCD_PROCESS_WHITE_SPACES ) ) != NULL )
                    {
                        *var = 0;
                    }
                    else
                    {
                        if ( !(*env) )
                        {
                            /* No more arguments in the buffer, look for other strings in the buffer */
                            if( p_var )
                            {
                            	strncat( args[ i ], p_var + 1, CONFIG_PCD_MAX_PARAM_SIZE-varsIdx-1 );
                            	varsIdx += strlen( p_var + 1 );
                            }

                        	break;
                        }
                    }

                    /* Advance to next index */
                    i++;
                }

                goto parse_param_advance;
            }

            /* Check redirection */
            ptr = strchr( token, '>' );

            if ( ptr )
            {
                /* Get next token */
                token = strtok( NULL, PCD_PROCESS_WHITE_SPACES );

                if ( token )
                {
                    int32_t fd;

                    fd = open( token, O_WRONLY | O_CREAT, S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP );
                    dup2(fd, 1); // redirect output to the file
                    close(fd);
                }
                goto parse_param_next;
            }

            args[ i ] = token;

            parse_param_advance:
            /* Advance args index */
            i++;

            parse_param_next:
            /* Get next token */
            token = strtok( NULL, PCD_PROCESS_WHITE_SPACES );
        }

        no_params:
        args[ i ] = 0;

        sigprocmask(SIG_SETMASK, &omask, 0L);

        /* Setup default signals for the new process */
        for ( i = 1; i <= NSIG; i++ )
            signal(i, SIG_DFL);

        /* Setup the priority of the process */
        {
            struct sched_param setParam;

            if ( sched_getparam( pid, &setParam ) == 0 )
            {
                if ( rule->sched.type == PCD_SCHED_TYPE_NICE )
                {
                    setParam.sched_priority = 0;
                    sched_setscheduler( pid, SCHED_OTHER, &setParam );
                    setpriority( PRIO_PROCESS, 0, rule->sched.niceSched );
                }
                else
                {
                    setParam.sched_priority = rule->sched.fifoSched;
                    sched_setscheduler( pid, SCHED_FIFO, &setParam );
                }
            }
        }
        
        /* Change the UID of the process if necessary 
         * Note: If the UID = 0, we do not change the UID because if we're 
         * already root, no need to change to UID 0, and if we're not root, 
         * then we don't allow running as root when PCD is running as a 
         * non-root user */
        if ( rule->uid )
        {
            setreuid( rule->uid, rule->uid );
        }

        /* Execute the file */
        if ( execvp(args[1], args+1)<0 )
        {
            exit( SIGABRT );
        }

        /* Not reaching here... */
        __exit(0);
    }

    proc->pid = pid;
    proc->state = PCD_PROCESS_STARTING;

    /* Wait for 3 iterations until marking the process as running-state */
    proc->tm = 3;

    next = proc->next;

    sigprocmask(SIG_SETMASK, &omask, 0L);

    return next;
}

static procObj_t *PCD_process_new( rule_t *rule )
{
    procObj_t *ptr;

    if ( !rule )
        return NULL;

    ptr = malloc( sizeof(procObj_t) );

    if ( ptr )
    {
        memset( ptr, 0, sizeof(procObj_t) );
        ptr->state = PCD_PROCESS_RUNME;
        ptr->retstat = PCD_PROCESS_RETNOTHING;
        ptr->rule = rule;
        ptr->signaled = False;
    }

    return ptr;
}

static void PCD_process_free( procObj_t *ptr )
{
    if ( !ptr )
        return;

    ptr->rule = NULL;

    free(ptr);
}

static procObj_t *PCD_process_find_by_state(procObj_t *p, procState_e state)
{
    while ( p )
    {
        if ( p->state == state )
            return p;
        p = p->next;
    }
    return 0;
}

PCD_status_e PCD_process_enqueue( rule_t *rule )
{
    procObj_t *newProc;

    if ( !rule )
        return PCD_STATUS_NOK;

    /* Check if we already have a running process associated with this rule */
    if ( rule->proc )
    {
        PCD_PRINTF_WARNING_STDOUT( "Cannot start process, process %s (%d) already running (Rule %s_%s)", rule->command, rule->proc->pid, rule->ruleId.groupName, rule->ruleId.ruleName );
        return PCD_STATUS_INVALID_RULE;
    }

    /* Allocate and init new process object */
    newProc = PCD_process_new( rule );

    if ( !newProc )
    {
        PCD_PRINTF_STDERR( "Failed to allocate memory for new process (Rule %s_%s)", rule->ruleId.groupName, rule->ruleId.ruleName );
        return PCD_STATUS_NOK;
    }

    NODE_ADD( procList, newProc );
    rule->proc = newProc;
    return PCD_STATUS_OK;
}

PCD_status_e PCD_process_stop( rule_t *rule, bool_t brutal, void *cookie )
{
    procObj_t *proc;

    if ( !rule )
        return PCD_STATUS_NOK;

    if ( !rule->proc )
    {
        PCD_PRINTF_WARNING_STDOUT( "Cannot stop process, no process is associated with rule %s_%s", rule->ruleId.groupName, rule->ruleId.ruleName );
        return PCD_STATUS_INVALID_RULE;
    }
    else
    {
        proc = rule->proc;
    }

    if ( ( proc->state == PCD_PROCESS_STOPPED ) || ( proc->state == PCD_PROCESS_STOPPING ) )
    {
        /* Special case, where process exited and then we got a stop command, simulate signal */
        proc->signaled = True;
    }
    else
    {
        /* Allow to stop only processes which are actually running */
        if ( ( proc->state != PCD_PROCESS_STARTING ) && ( proc->state != PCD_PROCESS_RUNNING ) && ( proc->state != PCD_PROCESS_RUNME ) )
        {
            PCD_PRINTF_WARNING_STDOUT( "Cannot stop process, process %s is not running (Rule %s_%s)", rule->command, rule->ruleId.groupName, rule->ruleId.ruleName );
            return PCD_STATUS_INVALID_RULE;
        }
    }

    if ( brutal == True )
    {
        proc->state = PCD_PROCESS_KILLME;
    }
    else
    {
        proc->state = PCD_PROCESS_TERMME;
    }

    /* Terminate the process immediately */
    proc->tm = 0;

    /* Setup cookie */
    proc->cookie = cookie;

    /* Disconnect the rule from this process */
    rule->proc = NULL;

    return PCD_STATUS_OK;
}

PCD_status_e PCD_process_init( void )
{
    int32_t i;
    struct sigaction sa;

    /* Ignore all signals! */
    for ( i = 1; i <= NSIG; i++ )
    {
        signal(i, SIG_IGN);
    }

    /* Install signal handlers */
    SETSIG(sa, SIGCHLD, PCD_process_chld_handler);
    SETSIGINFO(sa, SIGTERM, PCD_process_terminate);
    SETSIGINFO(sa, SIGINT,  PCD_process_terminate);
    SETSIGINFO(sa, SIGSEGV, PCD_process_terminate);
    SETSIGINFO(sa, SIGILL,  PCD_process_terminate);
    SETSIGINFO(sa, SIGBUS,  PCD_process_terminate);
    SETSIGINFO(sa, SIGQUIT, PCD_process_terminate);

    return PCD_STATUS_OK;
}


PCD_status_e PCD_process_iterate_start( void )
{
    procObj_t *p;

    p = procList;

    while ( p )
    {
        rule_t *rule = p->rule;

        switch ( p->state )
        {
            case PCD_PROCESS_RUNME:
                PCD_PRINTF_STDOUT( "Starting process %s (Rule %s_%s)", rule->command, rule->ruleId.groupName, rule->ruleId.ruleName );
                PCD_process_spawn(p);
                break;

            case PCD_PROCESS_STARTING:
                if ( p->tm == 0 )
                {
                    p->state = PCD_PROCESS_RUNNING;
                }
                else
                {
                    p->tm--;
                }
                break;
            default:
                break;
        }
        p = p->next;
    }


    return PCD_STATUS_OK;
}

PCD_status_e PCD_process_iterate_stop( void )
{
    procObj_t *p;
    procObj_t *next;

    p = procList;

    while ( p )
    {
        rule_t *rule = p->rule;

        switch ( p->state )
        {
            case PCD_PROCESS_STOPPING:
                /* Deal with this in the next iteration */
                p->state = PCD_PROCESS_STOPPED;
                break;

            case PCD_PROCESS_STOPPED:
                {
                    /* Disconnect from rule */
                    if ( rule->proc == p )
                        rule->proc = NULL;

                    /* IPC resource cleanup */
                    IPC_cleanup_proc( p->pid );

                    switch ( p->retstat )
                    {
                        case PCD_PROCESS_RETEXITED:

                            /* Check first if the process exited due to PCD termination signal */
                            if ( p->signaled == False )
                            {
                                if ( rule->daemon == True )
                                {
                                    PCD_PRINTF_STDERR( "Process %s (%d) exited unexpectedly (Rule %s_%s)", rule->command, p->pid, rule->ruleId.groupName, rule->ruleId.ruleName );
                                    if ( PCD_process_trigger_action( rule ) != PCD_STATUS_OK )
                                    {
                                        /* Try again next iteration */
                                        continue;
                                    }
                                }
                                else
                                {
                                    bool_t trigger = False;

                                    if ( rule->endCondition.type == PCD_END_COND_KEYWORD_EXIT )
                                    {
                                        /* The process has existed with a differnt exit code */
                                        if ( p->retcode != rule->endCondition.exitStatus )
                                        {
                                            trigger = True;
                                        }
                                    }
                                    else
                                    {
                                        if ( p->retcode != 0 )
                                        {
                                            trigger = True;
                                        }
                                    }

                                    if ( trigger )
                                    {
                                        PCD_PRINTF_STDERR( "Process %s (%d) exited with result code %d (Rule %s_%s)", rule->command, p->pid, p->retcode, rule->ruleId.groupName, rule->ruleId.ruleName );

                                        /* Trigger failure action */
                                        if ( PCD_process_trigger_action( rule ) != PCD_STATUS_OK )
                                        {
                                            /* Try again next iteration */
                                            continue;
                                        }
                                    }
                                }
                            }
                            else
                            {
                                if ( p->cookie )
                                {
                                    PCD_api_reply_message( p->cookie, PCD_STATUS_OK );
                                }
                            }
                            break;

                        case PCD_PROCESS_RETSIGNALED:
                            /* Nothing to do here, it might be signaled by PCD */
                            if ( p->signaled == False )
                            {
                                if ( rule->daemon == True )
                                {
                                    PCD_PRINTF_STDERR( "Unhandled exception %d (%s) in process %s (%d) (Rule %s_%s)", p->retcode, strsignal( p->retcode ), rule->command, p->pid, rule->ruleId.groupName, rule->ruleId.ruleName );
                                    if ( PCD_process_trigger_action( rule ) != PCD_STATUS_OK )
                                    {
                                        /* Try again next iteration */
                                        continue;
                                    }
                                }
                            }
                            else
                            {
                                if ( p->cookie )
                                {
                                    PCD_api_reply_message( p->cookie, PCD_STATUS_OK );
                                }
                            }
                            break;

                        case PCD_PROCESS_RETSTOPPED:
                            PCD_PRINTF_STDERR( "Exception %d (%s) caused process %s (%d) to stop (Rule %s_%s)", p->retcode, strsignal( p->retcode ), rule->command, p->pid, rule->ruleId.groupName, rule->ruleId.ruleName );
                            if ( PCD_process_trigger_action( rule ) != PCD_STATUS_OK )
                            {
                                /* Try again next iteration */
                                continue;
                            }
                            break;

                        default:
                            break;
                    }

                    next = p->next;
                    PCD_DEBUG_PRINTF("Deleting process %s (%d) (Rule %s_%s)", rule->command, p->pid, rule->ruleId.groupName, rule->ruleId.ruleName );
                    NODE_DEL(procList, p);

                    p->rule = NULL;
                    PCD_process_free(p);
                    p = next;
                    continue;
                }
                break;

            case PCD_PROCESS_TERMME:
                /* Check if other process is terminating */
                if ( !PCD_process_find_by_state(procList, PCD_PROCESS_KILLME) )
                {
                    if ( p->pid > 0 )
                    {
                        /* Check if corresponding process was already spawned */
                        p->state = PCD_PROCESS_KILLME;

                        /* Wait for 7 iterations (~10 seconds) before sending SIGKILL to a non-responsive process */
                        p->tm = 7;
                        PCD_PRINTF_STDOUT(  "Terminating process %s (%d) (Rule %s_%s)", rule->command, p->pid, rule->ruleId.groupName, rule->ruleId.ruleName );
                        p->signaled = True;
                        if( kill(p->pid, SIGTERM) < 0 )
						{
							p->state = PCD_PROCESS_STOPPED;
						}
                    }
                    else
                    {
                        p->state = PCD_PROCESS_STOPPED;
                    }
                }
                break;

            case PCD_PROCESS_KILLME:
                if ( p->tm == 0 )
                {
                    if ( p->pid > 0 )
                    {
                        PCD_PRINTF_STDOUT(  "Killing process %s (%d) (Rule %s_%s)", rule->command, p->pid, rule->ruleId.groupName, rule->ruleId.ruleName );
                        p->state = PCD_PROCESS_STOPPED;
                        p->signaled = True;
                        kill(p->pid, SIGKILL);
                    }
                    else
                    {
                        PCD_PRINTF_STDERR( "No process %s spawned, removing (Rule %s_%s)", rule->command, rule->ruleId.groupName, rule->ruleId.ruleName );
                        p->state = PCD_PROCESS_STOPPED;
                    }
                }
                else
                {
                    p->tm--;
                }
                break;

            case PCD_PROCESS_RUNME:
                /* Special case on quit */
                p->state = PCD_PROCESS_STOPPED;
                break;

            default:
                break;
        }
        p = p->next;
    }

    return PCD_STATUS_OK;
}


PCD_status_e PCD_process_signal_by_rule( rule_t *rule, int sig )
{
    procObj_t *proc;

    if ( !rule )
    {
        return PCD_STATUS_NOK;
    }

    proc = rule->proc;

    if ( !proc )
    {
        PCD_PRINTF_WARNING_STDOUT( "Cannot signal process, process %s is not running (Rule %s_%s)", rule->command, rule->ruleId.groupName, rule->ruleId.ruleName );
        return PCD_STATUS_INVALID_RULE;
    }

    /* Allow to signal only processes which are actually running */
    if ( ( proc->state == PCD_PROCESS_STARTING ) || ( proc->state == PCD_PROCESS_RUNNING ) )
    {
        /* Signal the process */
        kill(proc->pid, sig);
    }
    else
    {
        PCD_PRINTF_WARNING_STDOUT( "Cannot signal process, process %s is not running (Rule %s_%s)", rule->command, rule->ruleId.groupName, rule->ruleId.ruleName );
        return PCD_STATUS_INVALID_RULE;
    }

    return PCD_STATUS_OK;
}


static PCD_status_e PCD_process_trigger_action( rule_t *rule )
{
    rule_t *tmpRule;
    PCD_status_e retval;

    /* Dequeue the rule from the timer queue, we are triggering failure action now. */
    PCD_timer_dequeue_rule( rule, True );

    /* This calls a failure action callback and enqueues new timer object if required */
    tmpRule = ( PCD_failure_action_get_function( rule->failureAction.action ) )( rule );

    if ( tmpRule )
    {
        if ( ( retval = PCD_timer_enqueue_rule( tmpRule ) ) < 0 )
        {
            PCD_PRINTF_STDERR( "Failed to enqueue rule %s_%s", rule->ruleId.groupName, rule->ruleId.ruleName );
            return retval;
        }
    }

    return PCD_STATUS_OK;
}

rule_t *PCD_process_get_rule_by_pid( pid_t pid )
{
    procObj_t *p = procList;

    while ( p )
    {
        if ( p->pid == pid )
        {
            /* Found */
            return p->rule;
        }

        p = p->next;
    }

    return NULL;
}

void PCD_process_reboot( void )
{
    if ( debugMode == False )
    {
        /* Terminate all monitored processes */
        kill( 0, SIGTERM );

		/* Terminate init, reboot the system */
        kill( 1, SIGTERM );
    }
    else
    {
        const char msg[]= "pcd: Reboot disabled in debug mode, exiting.\n";
        int32_t i;
		
        /* Display reboot message */
        i = write( STDERR_FILENO, msg, sizeof(msg) );

        /* Exit abnormally */
        exit( 1 );
    }
}

