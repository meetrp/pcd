/*
 * graph.c
 * Description:
 * PCD graph generation implementation file
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
#include "system_types.h"
#include "rules_db.h"
#include "parser.h"
#include "condchk.h"
#include "graph.h"

static bool_t graphNoStartCondCreated;
static pcdGraph_e graphDisplay = PCD_MAIN_GRAPH_ACTIVE_ONLY;

/*! \fn PCD_status_e PCD_graph_set_display_items
 *  \brief Setup display items
 *  \param[in] graphDisplayItems.
 *  \param[out] None.
 *  \return OK or error status.
 */
PCD_status_e PCD_graph_set_display_items( pcdGraph_e graphDisplayItems )
{
    graphDisplay = graphDisplayItems;

    return PCD_STATUS_OK;
}

/*! \fn PCD_status_e PCD_graph_create_file
 *  \brief open graph file
 *  \param[in] graphFilename, graphDisplay.
 *  \param[out] graphHandle.
 *  \return OK or error status.
 */
PCD_status_e PCD_graph_create_file( const char *graphFilename, void **graphHandle )
{
    FILE *myGraphHandle;

    /* Create a new graph file */
    myGraphHandle = fopen( graphFilename, "w" );

    if ( !myGraphHandle )
    {
        return PCD_STATUS_NOK;
    }

    /* Init variables */
    *graphHandle = (void *)myGraphHandle;
    graphNoStartCondCreated = False;
    graphDisplay = graphDisplay;

    /* Write the header */
    fprintf( myGraphHandle, "digraph G {\n\n" );

    return PCD_STATUS_OK;
}

/*! \fn PCD_status_e PCD_graph_close_file
 *  \brief close graph file
 *  \param[in] graphFilename, graphHandle.
 *  \param[out] no output.
 *  \return OK or error status.
 */
PCD_status_e PCD_graph_close_file( const char *graphFilename, const void *graphHandle )
{
    FILE *myGraphHandle;

    if ( !graphHandle )
    {
        return PCD_STATUS_NOK;
    }

    myGraphHandle = (FILE *)graphHandle;

    fprintf( myGraphHandle, "\n}\n" );

    if ( fclose(myGraphHandle) < 0 )
    {
        PCD_PRINTF_STDERR( "Failed to close graph file %s", graphFilename );
        return PCD_STATUS_NOK;
    }

    PCD_PRINTF_STDOUT( "Generated graph file %s", graphFilename );
    return PCD_STATUS_OK;
}

/*! \fn PCD_status_e PCD_graph_update_file
 *  \brief Add a new item in graph
 *  \param[in] rule, graphHandle.
 *  \param[out] no output.
 *  \return OK or error status.
 */
PCD_status_e PCD_graph_update_file( const rule_t *newrule, const void *graphHandle )
{
    FILE *myGraphHandle;

    if ( !graphHandle )
    {
        return PCD_STATUS_NOK;
    }

    myGraphHandle = (FILE *)graphHandle;

    if ( ( ( newrule->ruleState == PCD_RULE_ACTIVE ) &&
           (( graphDisplay == PCD_MAIN_GRAPH_ACTIVE_ONLY ) || ( graphDisplay == PCD_MAIN_GRAPH_FULL_DISPLAY )) )
         ||
         ( ( newrule->ruleState == PCD_RULE_IDLE ) &&
           (( graphDisplay == PCD_MAIN_GRAPH_INACTIVE_ONLY ) || ( graphDisplay == PCD_MAIN_GRAPH_FULL_DISPLAY )) )
       )
    {
        char *shape;

        if ( strcmp( newrule->command, "NONE" ) == 0 )
        {
            /* Mark sync rules in diamond shape */
            shape = "diamond";
        }
        else
        {
            shape = "ellipse";
        }

        fprintf( myGraphHandle, "%s_%s [ label = \"%s\\n%s\", shape = %s ];\n", newrule->ruleId.groupName, newrule->ruleId.ruleName, newrule->ruleId.groupName, newrule->ruleId.ruleName, shape );

        if ( newrule->startCondition.type == PCD_START_COND_KEYWORD_RULE_COMPLETED )
        {
            u_int32_t i = 0;

            while ( i < PCD_START_COND_MAX_IDS )
            {
                if ( !newrule->startCondition.ruleCompleted[ i ].ruleId.groupName[ 0 ] )
                {
                    break;
                }

                fprintf( myGraphHandle, "%s_%s -> %s_%s;\n", newrule->startCondition.ruleCompleted[ i ].ruleId.groupName, newrule->startCondition.ruleCompleted[ i ].ruleId.ruleName, newrule->ruleId.groupName, newrule->ruleId.ruleName );

                i++;
            }
        }
        else if ( newrule->startCondition.type == PCD_START_COND_KEYWORD_NONE )
        {
            if ( graphNoStartCondCreated == False )
            {
                graphNoStartCondCreated = True;
                fprintf( myGraphHandle, "NoStartCondition [ label = \"No Start\\nCondition\", shape = diamond ];\n" );
            }
            fprintf( myGraphHandle, "NoStartCondition -> %s_%s;\n", newrule->ruleId.groupName, newrule->ruleId.ruleName );
        }
        else
        {
            char *typeStr = NULL;
            char condStr[ 32 ];

            switch ( newrule->startCondition.type )
            {
                case PCD_START_COND_KEYWORD_FILE:
                    typeStr = "File";
                    strcpy( condStr, newrule->startCondition.filename );
                    break;

                case PCD_START_COND_KEYWORD_NETDEVICE:
                    typeStr = "Net deivce";
                    strcpy( condStr, newrule->startCondition.netDevice );
                    break;

                case PCD_START_COND_KEYWORD_IPC_OWNER:
                    typeStr = "IPC";
                    sprintf( condStr, "%d", newrule->startCondition.ipcOwner );
                    break;

                case PCD_START_COND_KEYWORD_ENV_VAR:
                    typeStr = "Variable";
                    strcpy( condStr, newrule->startCondition.envVar.envVarName );
                    break;

                default:
                    break;
            }

            if ( typeStr )
            {
                fprintf( myGraphHandle, "%s_%s [ label = \"%s:\\n%s\", shape = box ];\n", typeStr, condStr, newrule->ruleId.groupName, newrule->ruleId.ruleName );
            }
        }
    }

    return PCD_STATUS_OK;
}

