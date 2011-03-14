/*
 * except.h
 * Description:
 * PCD exception handler header file
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

#ifndef _EXCEPT_H_
#define _EXCEPT_H_

/***************************************************************************/
/*! \file excet.h
 *  \brief exception header file
****************************************************************************/

/**************************************************************************/
/*      INCLUDES                                                          */
/**************************************************************************/
#ifdef CONFIG_PCD_PLATFORM_ARM /* ARM registers */
#include <asm/sigcontext.h>
#endif

#ifdef CONFIG_PCD_PLATFORM_X86 /* x86 processor context */
#ifndef __USE_GNU
#define __USE_GNU
#endif
#include <sys/ucontext.h>
#endif

#ifdef CONFIG_PCD_PLATFORM_X64 /* x64 registers */
#include <sys/ucontext.h>
#endif

#ifdef CONFIG_PCD_PLATFORM_MIPS /* MIPS registers */
#include <sys/ucontext.h>
#endif
#include "process.h"


#define PCD_EXCEPTION_MAX_PROCESS_NAME      32

#define PCD_EXCEPTION_MAGIC                 0x09CD0D0D

/**************************************************************************/
/*      INTERFACE TYPES and STRUCT Definitions                            */
/**************************************************************************/

typedef struct exception_t
{
    /* Magic number */
    u_int32_t magic;

    char process_name[ PCD_EXCEPTION_MAX_PROCESS_NAME ];

    struct timespec time;

    pid_t process_id;

    /* The number of the exception signal */
    u_int32_t signal_number;

    /* The signal code from siginfo_t. Provides exception reason */
    u_int32_t signal_code;

    /* Fault address, if relevant */
    void *fault_address;

    /* The last error as reported via siginfo_t. Seems to be always 0 */
    u_int32_t signal_errno;

    /* The last error in errno when the exception handler got called. */
    u_int32_t handler_errno;

#ifdef CONFIG_PCD_PLATFORM_ARM /* ARM registers */
    struct sigcontext regs;
#endif

    /* x86 processor context */ /* MIPS registers */
#if defined(CONFIG_PCD_PLATFORM_X86) || defined(CONFIG_PCD_PLATFORM_MIPS)
    mcontext_t uc_mctx;
#endif
    
#if defined(CONFIG_PCD_PLATFORM_X64) /* x64 registers */
    mcontext_t uc_mcontext;
#endif

} exception_t;


#define PCD_EXCEPTION_FILE  CONFIG_PCD_TEMP_PATH"/pcd_except"

/**************************************************************************/
/*      INTERFACE FUNCTIONS Prototypes:                                   */
/**************************************************************************/

/*! \fn             PCD_exception_init
 *  \brief          Init PCD exception handler
 *  \param[in]      None
 *  \param[in,out]  None
 *  \return         PCD_STATUS_OK - Success, Otherwise - Error
 */
PCD_status_e PCD_exception_init( void );

/*! \fn             PCD_exception_close
 *  \brief          Close PCD exception handler
 *  \param[in]      None
 *  \param[in,out]  None
 *  \return         PCD_STATUS_OK - Success, Otherwise - Error
 */
PCD_status_e PCD_exception_close( void );

/*! \fn             PCD_exception_listen
 *  \brief          PCD exception handler
 *  \param[in]      None
 *  \param[in,out]  None
 *  \return         None
 */
void PCD_exception_listen( void );

#endif /* _EXCEPT_H_ */
