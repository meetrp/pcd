/*
 * misc.c
 * Description:
 * PCD miscellaneous functions implementation file
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
#include <unistd.h>
#include <fcntl.h>
#include <sys/resource.h>
#include <sched.h>
#include "system_types.h"
#include "pcdapi.h"
#include "pcd.h"

static pid_t netRxPid = 0;
static u_int32_t netRxPriority = 0;

PCD_status_e PCD_misc_reduce_net_rx_priority( int32_t priority )
{
    struct sched_param setParam;

    netRxPid = PCD_api_find_process_id( "softirq-net-rx/" );

    if ( netRxPid )
    {
        /* Get net rx priority */
        if ( sched_getparam( netRxPid, &setParam ) == 0 )
        {
            netRxPriority = setParam.sched_priority;
        }
        else
        {
            /* Default value */
            netRxPriority = 50;
        }

        PCD_PRINTF_STDOUT( "Setting low priority for softirq-net-rx task" );

        /* Setup a new low priority to enable system startup */
        setParam.sched_priority = 0;
        sched_setscheduler( netRxPid, SCHED_OTHER, &setParam );
        setpriority( PRIO_PROCESS, netRxPid, priority );
        return PCD_STATUS_OK;
    }

    return PCD_STATUS_NOK;
}

PCD_status_e PCD_misc_restore_net_rx_priority( void )
{
    struct sched_param setParam;

    if ( netRxPid )
    {
        PCD_PRINTF_STDOUT( "Restoring priority for softirq-net-rx task" );

        /* Restore priority */
        setParam.sched_priority = netRxPriority;
        sched_setscheduler( netRxPid, SCHED_FIFO, &setParam );
        netRxPid = 0;
    }

    return PCD_STATUS_OK;
}

