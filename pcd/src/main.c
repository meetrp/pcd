/*
 * main.c
 * Description:
 * PCD main implementation file
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
 * Copyright (C) 2011 PCD Project - http://www.rt-embedded.com/pcd
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
#include <fcntl.h>
#include <getopt.h>
#include <pthread.h>
#include <sched.h>
#include "system_types.h"
#include "rules_db.h"
#include "parser.h"
#include "timer.h"
#include "process.h"
#include "timer.h"
#include "pcd_api.h"
#include "except.h"
#include "pcd.h"
#include "pcdapi.h"
#include "errlog.h"

/**************************************************************************/
/*      LOCAL DEFINITIONS AND VARIABLES                                   */
/**************************************************************************/

/* PCD Process tick: 1500ms */
#define PCD_PROCESS_TICK  ( 1500 )

static char *rulesFilename = NULL;
static bool_t crashDaemonMode = False;

static void PCD_main_usage( char *execname );
bool_t verboseOutput = False;
bool_t debugMode = False;

/* PCD Timer tick in ms */
u_int32_t PCD_TIMER_TICK = PCD_TIMER_TICK_DEFAULT;

/**************************************************************************/
/*      IMPLEMENTATION                                                    */
/**************************************************************************/

static void PCD_main_usage( char *execname )
{
    printf( "Usage: %s [options]\nOptions:\n\n", execname );
    printf( "-f FILE, --file=FILE\t\tSpecify PCD rules file.\n" );
    printf( "-p, --print\t\t\tPrint parsed configuration.\n" );
    printf( "-v, --verbose\t\t\tVerbose display.\n" );
    printf( "-t tick, --timer-tick=tick\tSetup timer ticks in ms (default 200ms).\n" );
    printf( "-e FILE, --errlog=FILE\t\tSpecify error log file (in nvram).\n" );
    printf( "-c, --crashd\t\t\tEnable crash-daemon only mode (no rules file).\n" );
    printf( "-h, --help\t\t\tPrint this message and exit.\n" );
    exit(0);
}

void PCD_main_parse_params( int32_t argc, char *argv[] )
{
    int c;

    opterr = 0;

    while ( 1 )
    {
        struct option long_options[] =
        {
            /* These options set a flag. */
            {"verbose",     no_argument,        0, 'v'},
            {"print",       no_argument,        0, 'p'},
            {"help",        no_argument,        0, 'h'},
            {"file",        required_argument,  0, 'f'},
            {"timer-tick",  required_argument,  0, 't'},
            {"debug",       no_argument,        0, 'd'},
            {"errlog",      required_argument,  0, 'e'},
            {"crashd",      no_argument,        0, 'c'},		
            {"version",     no_argument,        0, 'V'},
			{0, 0, 0, 0}
        };

        /* getopt_long stores the option index here. */
        int option_index = 0;

        c = getopt_long( argc, argv, "dpvVhcf:t:e:", long_options, &option_index );

        /* Detect the end of the options. */
        if ( c == -1 )
            break;

        switch ( c )
        {
            case 0:
                /* If this option set a flag, do nothing else now. */
                if ( long_options[option_index].flag != 0 )
                    break;
            case 'd':
                debugMode = True;
                break;

            case 'c':
			    if( rulesFilename )
				{
				    PCD_PRINTF_WARNING_STDOUT( "Rule file already specified, crash daemon mode is disabled" );	
				}
				else
				{
                    crashDaemonMode = True;
				}
                break;

            case 'p':
                PCD_parser_enable_verbose( True );
                break;

            case 'v':
                verboseOutput = True;
                break;

            case 'f':
			    if( crashDaemonMode )
				{
                    verboseOutput = True;			    
					PCD_PRINTF_STDERR( "Rules are disabled in crash daemon mode (either specify a rule file or enable crash daemon mode)" );	
					exit(1);
				}
                rulesFilename = optarg;
                break;

            case 'h':
                PCD_main_usage( argv[ 0 ] );
                break;

            case 'e':
                PCD_errlog_init( optarg );
                break;

            case 't':
                PCD_TIMER_TICK = atoi( optarg );

                /* Sanity checking 10-500ms is ok */
                if ( ( PCD_TIMER_TICK < 10 ) || ( PCD_TIMER_TICK > 500 ) )
                {
                    PCD_PRINTF_WARNING_STDOUT( "Invalid tick requested, using default of %dms", PCD_TIMER_TICK_DEFAULT );
                    PCD_TIMER_TICK = PCD_TIMER_TICK_DEFAULT;
                }
                break;

            case 'V':
				printf( "Process Control Daemon v%s\nCopyright (C) 2010 Texas Instruments Incorporated\nCopyright (C) 2011 PCD Project - http://www.rt-embedded.com/pcd", PCD_VERSION );
				exit(0);
				break;
			
			case '?':
                /* getopt_long already printed an error message. */
                break;

            default:
                abort( );
        }
    }
}

void PCD_main_set_self_priority( int32_t priority, int32_t policy )
{
    struct sched_param setParam;

    if ( sched_getparam( 0, &setParam ) == 0 )
    {
        setParam.sched_priority = (int32_t)priority;
        sched_setscheduler( 0, policy, &setParam );
    }
}

void PCD_main_init( void )
{
    /* Initialize the whole PCD subsystems. Exit abnormally in case something fails to init. */

    /* Initialize the timer module */
    if ( PCD_timer_init() != PCD_STATUS_OK )
    {
        exit(1);
    }

    /* Initialize process module */
    if ( PCD_process_init() != PCD_STATUS_OK )
    {
        exit(1);
    }

    /* Parse the configuration file, and initialize rules database */
    if ( !crashDaemonMode && PCD_parser_parse( rulesFilename ) != PCD_STATUS_OK )
    {
        exit(1);
    }

    /* Initialize the API module */
    if ( PCD_api_init() != PCD_STATUS_OK )
    {
        exit(1);
    }

    /* Activate all rules in database */
    if ( PCD_rulesdb_activate() != PCD_STATUS_OK )
    {
        exit(1);
    }

    /* Start the timer tick */
    if ( PCD_timer_start() != PCD_STATUS_OK )
    {
        exit(1);
    }

    /* Initialize exception handler */
    if ( PCD_exception_init() != PCD_STATUS_OK )
    {
        exit(1);
    }

    PCD_PRINTF_STDOUT( "Initialization complete" );
}

void PCD_main_loop( void )
{
    u_int32_t pcdTimerTick;
    u_int32_t tickCounter = PCD_PROCESS_TICK; /* Init tickCounter to perform an iteration */

    /* Setup PCD tick */
    pcdTimerTick = PCD_TIMER_TICK * 1000; /* Convert to uSeconds */

    /* An endless loop */
    while ( 1 )
    {
        fflush( stdout );
        fflush( stderr );

        /* Check incoming messages */
        PCD_api_check_messages();

        usleep( pcdTimerTick );

        /* Iterate on timer loop */
        if ( ( PCD_timer_iterate() ) || ( tickCounter >= PCD_PROCESS_TICK ) )
        {
            /* Iterate on process loop */
            PCD_process_iterate_start();
            PCD_process_iterate_stop();

            /* Check for incoming exceptions */
            PCD_exception_listen();

            tickCounter = 0;
        }

        /* Advance the tick counter */
        tickCounter += PCD_TIMER_TICK;
    }
}

int main( int32_t argc, char *argv[] )
{
    /* Setup FIFO_SCHED level 1. Boost the priority immediately */
    PCD_main_set_self_priority( CONFIG_PCD_PRIORITY, SCHED_FIFO );

    if ( PCD_api_find_process_id( argv[0] ) > 0 )
    {
        verboseOutput = True;
        PCD_PRINTF_STDERR( "Another instance of PCD is already running, aborting" );
        exit( 1 );
    }

    if( daemon( 1, 1 ) < 0 )
    {
        verboseOutput = True;
        PCD_PRINTF_STDERR( "Failed to daemonize" );
        exit( 1 );       
    }

    /* Parse the command line parameters */
    PCD_main_parse_params( argc, argv );

    if ( !rulesFilename && !crashDaemonMode )
    {
        verboseOutput = True;
        PCD_PRINTF_STDERR( "Please specify rules filename" );
        exit(1);
    }

    printf( "Starting Process Control Daemon v%s\nCopyright (C) 2010 Texas Instruments Incorporated\nCopyright (C) 2011 PCD Project - http://www.rt-embedded.com/pcd\n", PCD_VERSION );

    /* Init PCD subsystems */
    PCD_main_init();

    /* Loop forever... */
    PCD_main_loop();

    return 0;
}
