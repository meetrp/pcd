/*
 * graph.h
 * Description:
 * PCD parser utility main file
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
#include <fcntl.h>
#include <getopt.h>
#include "system_types.h"
#include "rules_db.h"
#include "parser.h"
#include "condchk.h"
#include "graph.h"
#include "outputhdr.h"

#define MAX_FILENAME_LEN      255
#define MAX_GROUPS            16

static char *rulesFilename = NULL;
bool_t verboseOutput = True;

static char *headerFilename = NULL;
static void *headerHandle = NULL;

static void *graphHandle = NULL;
static char *graphFilename = NULL;

char hostPrefix[ 128 ] = { "\0"};

static void PCD_main_usage( char *execname );

static void PCD_main_usage( char *execname )
{
    printf( "Usage: %s [options]\nOptions:\n\n", execname );
    printf( "-f FILE, --file=FILE\t\tSpecify PCD rules file.\n" );
    printf( "-g FILE, --graph=FILE\t\tGenerate a graph file.\n" );
    printf( "-d [0|1|2], --display=[0|1|2]\tItems to display in graph file (Active|All|Inactive).\n" );
    printf( "-o FILE, --output=FILE\t\tGenerate an output header file with rules definitions.\n" );
    printf( "-b DIR, --base-dir=DIR\t\tSpecify base directory on the host.\n" );
    printf( "-v, --verbose\t\t\tPrint parsed configuration.\n" );
    printf( "-h, --help\t\t\tPrint this message and exit.\n" );
    exit(0);
}

/* Stub */
void PCD_errlog_log( char *buffer, bool_t timeStamp )
{
}

PCD_status_e PCD_rulesdb_add_rule( rule_t *newrule )
{
    if ( headerHandle )
    {
        /* Add a line in the header file */
        if ( PCD_output_header_update_file( newrule, headerHandle ) != PCD_STATUS_OK )
        {
            return PCD_STATUS_NOK;
        }
    }

    if ( graphHandle )
    {
        /* Add an entry in the graph file */
        if ( PCD_graph_update_file( newrule, graphHandle ) != PCD_STATUS_OK )
        {
            return PCD_STATUS_NOK;
        }
    }

    return PCD_STATUS_OK;
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
            {"verbose",    no_argument,       0, 'v'},
            {"help",       no_argument,       0, 'h'},
            {"file",       required_argument, 0, 'f'},
            {"output",     required_argument, 0, 'o'},
            {"graph",      required_argument, 0, 'g'},
            {"display",    required_argument, 0, 'd'},
            {"base-dir",    required_argument, 0, 'b'},
            {0, 0, 0, 0}
        };

        /* getopt_long stores the option index here. */
        int option_index = 0;

        c = getopt_long( argc, argv, "vhf:t:o:g:d:b:", long_options, &option_index );

        /* Detect the end of the options. */
        if ( c == -1 )
            break;

        switch ( c )
        {
            case 0:
                /* If this option set a flag, do nothing else now. */
                if ( long_options[option_index].flag != 0 )
                    break;

            case 'v':
                PCD_parser_enable_verbose( True );
                break;

            case 'f':
                rulesFilename = optarg;
                break;

            case 'h':
                PCD_main_usage( argv[ 0 ] );
                break;

            case 'o':
                headerFilename = optarg;
                /*
                Create the output file */
                if ( PCD_output_header_create_file( headerFilename, &headerHandle ) == PCD_STATUS_NOK )
                {
                    PCD_PRINTF_STDERR( "Failed to create output file %s", headerFilename );
                    exit(1);
                }
                break;

            case 'g':
                graphFilename = optarg;

                /* Create the output file */
                if ( PCD_graph_create_file( graphFilename, &graphHandle ) == PCD_STATUS_NOK )
                {
                    PCD_PRINTF_STDERR( "Failed to create graph file %s", graphFilename );
                    exit(1);
                }
                break;

            case 'd':
                {
                    pcdGraph_e graphDisplay;

                    graphDisplay = atoi( optarg );
                    if ( graphDisplay > PCD_MAIN_GRAPH_LAST )
                    {
                        graphDisplay = PCD_MAIN_GRAPH_ACTIVE_ONLY;
                    }
                    PCD_graph_set_display_items( graphDisplay );
                }
                break;

            case 'b':
                memset( hostPrefix, 0, sizeof( hostPrefix ) );
                strncpy( hostPrefix, optarg, sizeof( hostPrefix ) -1 );
                break;

            case '?':
                /* getopt_long already printed an error message. */
                break;

            default:
                abort( );
        }
    }
}


int main( int32_t argc, char *argv[] )
{
    u_int32_t ret = 0;

    PCD_main_parse_params( argc, argv );

    if ( !rulesFilename )
    {
        PCD_PRINTF_STDERR( "Please specify rules filename , or --help for help" );
        ret = 1;
        goto cleanup;
    }

    /* Parse the configuration file, and initialize rules database */
    if ( PCD_parser_parse( rulesFilename ) != PCD_STATUS_OK )
    {
        ret = 1;
        goto cleanup;
    }

    cleanup:

    /* Close the file in case it was open */
    if ( headerHandle )
    {
        PCD_output_header_close_file( headerFilename, headerHandle );
        if ( ret )
        {
            unlink( headerFilename );
        }
    }

    /* Close the file in case it was open */
    if ( graphHandle )
    {
        PCD_graph_close_file( graphFilename, graphHandle );
        if ( ret )
        {
            unlink( graphFilename );
        }
    }

    return ret;
}

