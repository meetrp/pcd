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
#ifdef CONFIG_ARM /* ARM registers */
#include <asm/sigcontext.h>
#endif
#ifdef CONFIG_X86 /* X86 processor context */
#define __USE_GNU
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
    Uint32 magic;

    Char process_name[ PCD_EXCEPTION_MAX_PROCESS_NAME ];

    struct timespec time;

    pid_t process_id;

    /* The number of the exception signal */
    Uint32 signal_number;

    /* The signal code from siginfo_t. Provides exception reason */
    Uint32 signal_code;

    /* Fault address, if relevant */
    void *fault_address;

    /* The last error as reported via siginfo_t. Seems to be always 0 */
    Uint32 signal_errno;

    /* The last error in errno when the exception handler got called. */
    Uint32 handler_errno;

#ifdef CONFIG_ARM /* ARM registers */
    struct sigcontext regs;
#endif

#ifdef CONFIG_X86 /* x86 processor context */
    mcontext_t uc_mctx;
#endif

} exception_t;


#define PCD_EXCEPTION_FILE  "/var/pcd_except"

/**************************************************************************/
/*      INTERFACE FUNCTIONS Prototypes:                                   */
/**************************************************************************/

/*! \fn             PCD_exception_init
 *  \brief          Init PCD exception handler
 *  \param[in]      None
 *  \param[in,out]  None
 *  \return         STATUS_OK - Success, Otherwise - Error
 */
STATUS PCD_exception_init( void );

/*! \fn             PCD_exception_close
 *  \brief          Close PCD exception handler
 *  \param[in]      None
 *  \param[in,out]  None
 *  \return         STATUS_OK - Success, Otherwise - Error
 */
STATUS PCD_exception_close( void );

/*! \fn             PCD_exception_listen
 *  \brief          PCD exception handler
 *  \param[in]      None
 *  \param[in,out]  None
 *  \return         None
 */
void PCD_exception_listen( void );

#endif /* _EXCEPT_H_ */
