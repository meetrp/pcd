/*
 * schedtype.h
 * Description:
 * PCD scheduling type header file
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

#ifndef _SCHEDTYPE_H_
#define _SCHEDTYPE_H_

/***************************************************************************/
/*! \file schedtype.h
 *  \brief Scheduling type header file
****************************************************************************/

/**************************************************************************/
/*      INCLUDES                                                          */
/**************************************************************************/
#include "system_types.h"

/**************************************************************************/
/*      INTERFACE TYPES and STRUCT Definitions                            */
/**************************************************************************/

/*! \enum schedType_e
 *  \brief Scheduling type enumeration
 */
typedef enum schedType_e
{
    PCD_SCHED_TYPE_FIFO,
    PCD_SCHED_TYPE_NICE,

} schedType_e;

/*! \struct schedType_t
 *  \brief Sched type structure
 */
typedef struct schedType_t
{
    schedType_e type;
    union
    {
        int32_t   fifoSched;
        u_int32_t  niceSched;
    };

} schedType_t;

#endif /* _SCHEDTYPE_H_ */
