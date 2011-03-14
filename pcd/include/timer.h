/*
 * timer.h
 * Description:
 * PCD timer header file
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

#ifndef _TIMER_H_
#define _TIMER_H_

/***************************************************************************/
/*! \file timer.h
 *  \brief PCD timer header file
****************************************************************************/

/**************************************************************************/
/*      INCLUDES                                                          */
/**************************************************************************/
#include "system_types.h"
#include "rules_db.h"
#include "condchk.h"
#include "failact.h"
#include <time.h>

/**************************************************************************/
/*      INTERFACE TYPES and STRUCT Definitions                            */
/**************************************************************************/

/*! \def PCD_TIMER_TICK_DEFAULT
 *  \brief Default timer tick value, 200ms
 */
#define PCD_TIMER_TICK_DEFAULT       200

extern u_int32_t PCD_TIMER_TICK;       /* PCD Timer tick in ms */

/*! \struct timerObj_t
 *  \brief Timer object structure
 */
typedef struct timerObj_t
{
    rule_t              *rule;
    condCheckFunc       startCondCheckFunc;
    condCheckFunc       endCondCheckFunc;
    failActionFunc      failureActionFunc;
    u_int32_t              timeout;

    struct timerObj_t   *prev;
    struct timerObj_t   *next;

} timerObj_t;

/**************************************************************************/
/*      INTERFACE FUNCTIONS Prototypes:                                   */
/**************************************************************************/

/*! \fn             PCD_timer_init
 *  \brief          Module's init function
 *  \param[in]      None
 *  \param[in,out]  None
 *  \return         PCD_STATUS_OK - Success, Otherwise - Error
 */
PCD_status_e PCD_timer_init( void );

/*! \fn             PCD_timer_iterate
 *  \brief          Timer iteration function
 *  \param[in]      None
 *  \param[in,out]  None
 *  \return         True - Process spawn required, False - No process spawn now
 */
bool_t PCD_timer_iterate( void );

/*! \fn             PCD_timer_start
 *  \brief          Start the timer
 *  \param[in]      None
 *  \param[in,out]  None
 *  \return         PCD_STATUS_OK - Success, Otherwise - Error
 */
PCD_status_e PCD_timer_start( void );

/*! \fn             PCD_timer_stop
 *  \brief          Stop the timer
 *  \param[in]      None
 *  \param[in,out]  None
 *  \return         PCD_STATUS_OK - Success, Otherwise - Error
 */
PCD_status_e PCD_timer_stop( void );

/*! \fn             PCD_timer_enqueue_rule
 *  \brief          Enqueue a rule from the timer list
 *  \param[in]      Rule
 *  \param[in,out]  None
 *  \return         PCD_STATUS_OK - Success, Otherwise - Error
 */
PCD_status_e PCD_timer_enqueue_rule( rule_t *rule );

/*! \fn             PCD_timer_dequeue_rule
 *  \brief          Dequeue a rule from the timer list
 *  \param[in]      Rule, failed flag
 *  \param[in,out]  None
 *  \return         PCD_STATUS_OK - Success, Otherwise - Error
 */
PCD_status_e PCD_timer_dequeue_rule( rule_t *rule, bool_t failed );

#endif /* _TIMER_H_ */
