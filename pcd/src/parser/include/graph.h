/*
 * graph.h
 * Description:
 * PCD graph generation header file
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

/***************************************************************************/

/*! \file graph.h
    \brief Header file for graph generation

     The script graph file uses the DOT language syntax.
     For reference: http://graphviz.org/doc/info/lang.html
     The script is converted to graphical layout using the Graphviz tool
     (Available for Windows/Linux): http://graphviz.org/Download.php

****************************************************************************/

#ifndef _GRAPH_H_
#define _GRAPH_H_

#include "system_types.h"

/*! \enum pcdGraph_e
    \brief Select the items to display in graph
*/
typedef enum
{
    PCD_MAIN_GRAPH_ACTIVE_ONLY,
    PCD_MAIN_GRAPH_FULL_DISPLAY,
    PCD_MAIN_GRAPH_INACTIVE_ONLY,

    PCD_MAIN_GRAPH_LAST,

} pcdGraph_e;

/**************************************************************************/
/*      INTERFACE FUNCTIONS Prototypes:                                   */
/**************************************************************************/

/*! \fn PCD_status_e PCD_graph_set_display_items
 *  \brief Setup display items
 *  \param[in] graphDisplayItems.
 *  \param[out] None.
 *  \return OK or error status.
 */
PCD_status_e PCD_graph_set_display_items( pcdGraph_e graphDisplayItems );

/*! \fn PCD_status_e PCD_graph_create_file
 *  \brief open graph file
 *  \param[in] graphFilename, graphDisplay.
 *  \param[out] graphHandle.
 *  \return OK or error status.
 */
PCD_status_e PCD_graph_create_file( const char *graphFilename, void **graphHandle );

/*! \fn PCD_status_e PCD_graph_close_file
 *  \brief close graph file
 *  \param[in] graphFilename, graphHandle.
 *  \param[out] no output.
 *  \return OK or error status.
 */
PCD_status_e PCD_graph_close_file( const char *graphFilename, const void *graphHandle );

/*! \fn PCD_status_e PCD_graph_update_file
 *  \brief Add a new item in graph
 *  \param[in] rule, graphHandle.
 *  \param[out] no output.
 *  \return OK or error status.
 */
PCD_status_e PCD_graph_update_file( const rule_t *newrule, const void *graphHandle );

#endif /* _GRAPH_H_ */

