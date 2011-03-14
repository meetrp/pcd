/*
 * process.h
 * Description:
 * PCD Process management header file
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

#ifndef _PROCESS_H_
#define _PROCESS_H_

/***************************************************************************/
/*! \file process.h
 *  \brief Process module header file
****************************************************************************/

/**************************************************************************/
/*      INCLUDES                                                          */
/**************************************************************************/
#include "system_types.h"
#include <time.h>
#include <sys/types.h>

struct rule_t;

/**************************************************************************/
/*      INTERFACE TYPES and STRUCT Definitions                            */
/**************************************************************************/

/*! \enum procState_e
 *  \brief Process state enumeration
 */
typedef enum procState_e
{
    PCD_PROCESS_NOTHING,
    PCD_PROCESS_RUNME,
    PCD_PROCESS_STARTING,
    PCD_PROCESS_RUNNING,
    PCD_PROCESS_TERMME,
    PCD_PROCESS_KILLME,
    PCD_PROCESS_STOPPING,
    PCD_PROCESS_STOPPED,

} procState_e;

/*! \enum procExit_e
 *  \brief Process exit status enumeration
 */
typedef enum procExit_e
{
    PCD_PROCESS_RETNOTHING,
    PCD_PROCESS_RETEXITED,
    PCD_PROCESS_RETSIGNALED,
    PCD_PROCESS_RETSTOPPED,

} procExit_e;

/*! \struct procExit_e
 *  \brief Process list object definition
 */
typedef struct procObj_t
{
    struct rule_t   *rule;
    pid_t           pid;
    procState_e     state;
    procExit_e      retstat;
    int32_t           retcode;
    u_int32_t          tm;
    bool_t            signaled;
    void            *cookie;

    struct procObj_t *prev;
    struct procObj_t *next;

} procObj_t;

/**************************************************************************/
/*      INTERFACE FUNCTIONS Prototypes:                                   */
/**************************************************************************/

/*! \fn             PCD_process_init
 *  \brief          Module's init function
 *  \param[in]      None
 *  \param[in,out]  None
 *  \return         PCD_STATUS_OK - Success, Otherwise - Error
 */
PCD_status_e PCD_process_init( void );

/*! \fn             PCD_process_enqueue
 *  \brief          Enqueue a process
 *  \param[in]      Rule
 *  \param[in,out]  None
 *  \return         PCD_STATUS_OK - Success, Otherwise - Error
 */
PCD_status_e PCD_process_enqueue( rule_t *rule );

/*! \fn             PCD_process_stop
 *  \brief          Stop a process
 *  \param[in]      Rule, Brutal; True to kill, cookie; caller message
 *  \param[in,out]  None
 *  \return         PCD_STATUS_OK - Success, Otherwise - Error
 */
PCD_status_e PCD_process_stop( rule_t *rule, bool_t brutal, void *cookie );

/*! \fn             PCD_process_iterate_start
 *  \brief          Start iteration function
 *  \param[in]      None
 *  \param[in,out]  None
 *  \return         PCD_STATUS_OK - Success, Otherwise - Error
 */
PCD_status_e PCD_process_iterate_start( void );

/*! \fn             PCD_process_iterate_stop
 *  \brief          Stop iteration function
 *  \param[in]      None
 *  \param[in,out]  None
 *  \return         PCD_STATUS_OK - Success, Otherwise - Error
 */
PCD_status_e PCD_process_iterate_stop( void );

/*! \fn             PCD_process_signal_by_rule
 *  \brief          Signal a process, find it by its rule
 *  \param[in]      Rule, Signal
 *  \param[in,out]  None
 *  \return         PCD_STATUS_OK - Success, Otherwise - Error
 */
PCD_status_e PCD_process_signal_by_rule( rule_t *rule, int sig );

/*! \fn             PCD_process_get_rule_by_pid
 *  \brief          Get rule by its process ID
 *  \param[in]      Process ID
 *  \param[in,out]  None
 *  \return         Pointer to rule, NULL - Error
 */
rule_t *PCD_process_get_rule_by_pid( pid_t pid );

/*! \fn             PCD_process_reboot
 *  \brief          Reboot the system
 *  \param[in]      None
 *  \param[in,out]  None
 *  \return         Never returns. Either reboots or exits (in debug mode).
 */
void PCD_process_reboot( void );

#endif /* _PROCESS_H_ */
