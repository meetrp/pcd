/*
 * pcd_api.h
 * Description:
 * PCD API header file
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

#ifndef _PCD_API_H_
#define _PCD_API_H_

/***************************************************************************/
/*! \file pcd_api.h
 *  \brief Process control daemon API header file
****************************************************************************/

/**************************************************************************/
/*      INCLUDES                                                          */
/**************************************************************************/
#include "system_types.h"
#include "ruleid.h"
#include "pcdapi.h"

/**************************************************************************/
/*      INTERFACE TYPES and STRUCT Definitions                            */
/**************************************************************************/

/*! \enum pcdApi_e
 *  \brief PCD API enumeration
 */
typedef enum
{
    PCD_API_START_PROCESS,
    PCD_API_TERMINATE_PROCESS,
    PCD_API_TERMINATE_PROCESS_SYNC,
    PCD_API_KILL_PROCESS,
    PCD_API_PROCESS_READY,
    PCD_API_SIGNAL_PROCESS,
    PCD_API_GET_RULE_STATE,
    PCD_API_REDUCE_NETRX_PRIORITY,
    PCD_API_RESTORE_NETRX_PRIORITY,

} pcdApi_e;

/*! \struct pcdApiMessage_t
 *  \brief PCD API message structure
 */
typedef struct pcdApiMessage_t
{
    pcdApi_e    type;
    union
    {
        pid_t       pid;
        int32_t       sig;
        int32_t       priority;
    };
    ruleId_t    ruleId;
    u_int32_t      msgId;
    char        params[ CONFIG_PCD_MAX_PARAM_SIZE ];   /* Optional parameters */

} pcdApiMessage_t;

/*! \struct pcdApiReplyMessage_t
 *  \brief PCD API reply message structure
 */
typedef struct pcdApiReplyMessage_t
{
    u_int32_t      msgId;
    union
    {
        pcdApiRuleState_e   ruleState;
    };
    PCD_status_e      retval;

} pcdApiReplyMessage_t;

/**************************************************************************/
/*      INTERFACE FUNCTIONS Prototypes:                                   */
/**************************************************************************/

/*! \fn             PCD_api_init
 *  \brief          Module's init function
 *  \param[in]      None
 *  \param[in,out]  None
 *  \return         PCD_STATUS_OK - Success, Otherwise - Error
 */
PCD_status_e PCD_api_init( void );

/*! \fn             PCD_api_deinit
 *  \brief          Module's deinit function - Call before end of life
 *  \param[in]      None
 *  \param[in,out]  None
 *  \return         PCD_STATUS_OK - Success, Otherwise - Error
 */
PCD_status_e PCD_api_deinit( void );

/*! \fn             PCD_api_check_messages
 *  \brief          Check for incoming messages
 *  \param[in]      None
 *  \param[in,out]  None
 *  \return         PCD_STATUS_OK - Success, Otherwise - Error
 */
PCD_status_e PCD_api_check_messages( void );

/*! \fn             PCD_api_reply_message
 *  \brief          Reply a termination request message
 *  \param[in]      cookie: caller encapsulated message, retval: return status
 *  \param[in,out]  None
 *  \return         None
 */
void PCD_api_reply_message( void *cookie, PCD_status_e retval );

#endif /* _PCD_API_H_ */
