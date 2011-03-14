/*
 * outputhdr.c
 * Description:
 * PCD output header generation implementation file
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

#define MAX_FILENAME_LEN      255
#define MAX_GROUPS            16

static char groupName[ MAX_GROUPS ][ 32 ];

/*! \fn PCD_status_e PCD_output_header_create_file
 *  \brief open header file
 *  \param[in] headerFilename, headerHandle.
 *  \param[out] graphHandle.
 *  \return OK or error status.
 */
PCD_status_e PCD_output_header_create_file( const char *headerFilename, void **headerHandle )
{
    char header[ MAX_FILENAME_LEN ];
    char *ptr, *last;
    u_int32_t i;
    FILE *myHeaderHandle;

    myHeaderHandle = fopen( headerFilename, "w" );

    if ( !myHeaderHandle )
    {
        return PCD_STATUS_NOK;
    }

    *headerHandle = myHeaderHandle;
    memset( groupName, 0, sizeof( groupName ) );

    /* Generate header headerHandle */
    ptr = ( char *)headerFilename;

    do
    {
        last = strchr( ptr, '/' );
        if ( last )
            ptr = last + 1;

    } while ( last );

    i = 0;
    last = ptr;
    memset( header, 0, MAX_FILENAME_LEN );

    header[ i++ ] = '_';

    /* Create a string for header definition from header headerHandle */
    while ( ( *ptr ) && ( i < MAX_FILENAME_LEN - 2 ) )
    {
        if ( *ptr >= 'a' && *ptr <= 'z' )
        {
            header[ i++ ] = *ptr -'a'+'A';
        }
        else if ( ( *ptr == '.' ) || ( *ptr == '_' ) )
        {
            header[ i++ ] = '_';
        }

        ptr++;
    }

    header[ i ] = '_';

    /* Write the header */
    fprintf( myHeaderHandle, "/**************************************************************************\n" );
    fprintf( myHeaderHandle, "*\n" );
    fprintf( myHeaderHandle, "*   FILE:  %s\n", last );
    fprintf( myHeaderHandle, "*   PURPOSE: PCD definitions file (auto generated).\n" );
    fprintf( myHeaderHandle, "*\n" );
    fprintf( myHeaderHandle, "**************************************************************************/\n\n" );
    fprintf( myHeaderHandle, "#ifndef %s\n", header );
    fprintf( myHeaderHandle, "#define %s\n\n", header );
    fprintf( myHeaderHandle, "#include \"pcdapi.h\"\n\n" );

    return PCD_STATUS_OK;
}

/*! \fn PCD_status_e PCD_output_header_close_file
 *  \brief close header file
 *  \param[in] headerFilename, headerHandle.
 *  \param[out] no output.
 *  \return OK or error status.
 */
PCD_status_e PCD_output_header_close_file( const char *headerFilename, const void *headerHandle )
{
    u_int32_t i = 0;
    FILE *myHeaderHandle;

    if ( !headerHandle )
    {
        return PCD_STATUS_NOK;
    }

    myHeaderHandle = (FILE *)headerHandle;

    /* Generate macros for all groups */
    while ( ( i < MAX_GROUPS ) && ( groupName[ i ][ 0 ] ) )
    {
        fprintf( myHeaderHandle, "\n/*! \\def %s_DECLARE_PCD_RULEID()\n", groupName[ i ] );
        fprintf( myHeaderHandle, " *  \\brief Define a ruleId easily when calling PCD API\n" );
        fprintf( myHeaderHandle, "*/\n" );
        fprintf( myHeaderHandle, "#define %s_DECLARE_PCD_RULEID( ruleId, RULE_NAME ) \\\n", groupName[ i ] );
        fprintf( myHeaderHandle, "\tPCD_DECLARE_RULEID( ruleId, %s_PCD_GROUP_NAME, RULE_NAME )\n", groupName[ i ] );
        i++;
    }

    fprintf( myHeaderHandle, "\n#endif\n\n" );

    if ( fclose(myHeaderHandle) < 0 )
    {
        PCD_PRINTF_STDERR( "Failed to close output file %s", headerFilename );
        return PCD_STATUS_NOK;
    }

    PCD_PRINTF_STDOUT( "Generated output file %s", headerFilename );
    return PCD_STATUS_OK;
}

/*! \fn PCD_status_e PCD_output_header_update_file
 *  \brief Add a new item in graph
 *  \param[in] rule, headerHandle.
 *  \param[out] no output.
 *  \return OK or error status.
 */
PCD_status_e PCD_output_header_update_file( rule_t *newrule, const void *headerHandle )
{
    u_int32_t i = 0;
    bool_t found = False;
    FILE *myHeaderHandle;

    if ( !headerHandle )
    {
        return PCD_STATUS_NOK;
    }

    myHeaderHandle = (FILE *)headerHandle;

    /* Look for existing group */
    while ( ( i < MAX_GROUPS ) && ( groupName[ i ][ 0 ] ) )
    {
        if ( strcmp( groupName[ i ], newrule->ruleId.groupName ) == 0 )
        {
            found = True;
            break;
        }
        i++;
    }

    if ( found == False )
    {
        /* Write a new group definition */
        strcpy( groupName[ i ], newrule->ruleId.groupName );
        fprintf( myHeaderHandle, "\n/*! \\def %s_PCD_GROUP_NAME\n", groupName[ i ] );
        fprintf( myHeaderHandle, " *  \\brief Define group ID string for %s\n", groupName[ i ] );
        fprintf( myHeaderHandle, "*/\n" );
        fprintf( myHeaderHandle, "#define %s_PCD_GROUP_NAME\t\"%s\"\n\n", newrule->ruleId.groupName, newrule->ruleId.groupName );
    }

    /* Write the rule */
    fprintf( myHeaderHandle, "#define %s_PCD_RULE_%s\t\"%s\"\n", newrule->ruleId.groupName, newrule->ruleId.ruleName, newrule->ruleId.ruleName );

    return PCD_STATUS_OK;
}

