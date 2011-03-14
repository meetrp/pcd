/*
 * misc.h
 * Description:
 * PCD Miscellaneous functions header file
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

#ifndef _MISC_H_
#define _MISC_H_

/***************************************************************************/
/*! \file misc.h
 *  \brief Miscellaneous process control daemon API header file
****************************************************************************/

/**************************************************************************/
/*      INCLUDES                                                          */
/**************************************************************************/
#include <unistd.h>
#include "system_types.h"

extern pid_t netRxPid;

/**************************************************************************/
/*      INTERFACE FUNCTIONS Prototypes:                                   */
/**************************************************************************/
/*! \def			PCD_misc_net_rx_low_priority
 *  \brief 			Check if the softirq-net-rx task priority is low
*/
#define PCD_misc_net_rx_low_priority()  (netRxPid)

/*! \fn				PCD_misc_reduce_net_rx_priority
 *  \brief 			Reduce the softirq-net-rx task priority, to enable system
 *                  boot during high traffic
 *  \param[in] 		priority
 *  \param[in,out] 	None
 *  \return			PCD_STATUS_OK - Success, Otherwise - Error
 */
PCD_status_e PCD_misc_reduce_net_rx_priority( int32_t priority );

/*! \fn				PCD_misc_restore_net_rx_priority
 *  \brief 			restore softirq-net-rx priority
 *  \param[in] 		None
 *  \param[in,out] 	None
 *  \return			PCD_STATUS_OK - Success, Otherwise - Error
 */
PCD_status_e PCD_misc_restore_net_rx_priority( void );

#endif /* _MISC_H_ */
