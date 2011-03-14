/*
 * except.c
 * Description:
 * PCD exception handler implementation file
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
 * Copyright (C) 2010 PCD Project - http://www.rt-embedded.com/pcd
 * 
 * Change log:
 * - Support MIPS, x86 and x64 Platforms
 * - Bug fixes
 */

/* Author:
 * Hai Shalom, hai@rt-embedded.com 
 *
 * PCD Homepage: http://www.rt-embedded.com/pcd/
 * PCD Project at SourceForge: http://sourceforge.net/projects/pcd/
 *  
 */

/* Required for some ucontetx.h headers */
#define _GNU_SOURCE

/**************************************************************************/
/*      INCLUDES                                                          */
/**************************************************************************/
#include <unistd.h>
#include <time.h>
#include <malloc.h>
#include <signal.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <pty.h>
#include <errno.h>
#include <syslog.h>
#include <sys/wait.h>
#include <semaphore.h>
#include <fcntl.h>
#include <sys/stat.h>
#include "rules_db.h"
#include "process.h"
#include "timer.h"
#include "pcd.h"
#include "except.h"
#include "errlog.h"

#define PCD_ERRLOG_BUF_SIZE             1024

static int32_t fd = -1;
static fd_set rdset;

/* This translates a signal code into a readable string */
static inline char *PCD_code2str(int32_t code, int32_t signal)
{
    switch ( code )
    {
        case SI_USER:
            return "Kill, sigsend or raise ";
        case SI_KERNEL:
            return "Kernel";
        case SI_QUEUE:
            return "sigqueue";
    }

    if ( signal == SIGILL )
    {
        switch ( code )
        {
            case ILL_ILLOPC:
                return "Illegal opcode";
            case ILL_ILLOPN:
                return "Illegal operand";
            case ILL_ILLADR:
                return "Illegal addressing mode";
            case ILL_ILLTRP:
                return "Illegal trap";
            case ILL_PRVOPC:
                return "Privileged register";
            case ILL_COPROC:
                return "Coprocessor error";
            case ILL_BADSTK:
                return "Internal stack error";
        }
    }
    else if ( signal == SIGFPE )
    {
        switch ( code )
        {
            case FPE_INTDIV:
                return "Integer divide by zero";
            case FPE_INTOVF:
                return "Integer overflow";
            case FPE_FLTDIV:
                return "Floating point divide by zero";
            case FPE_FLTOVF:
                return "Floating point overflow";
            case FPE_FLTUND:
                return "Floating point underflow";
            case FPE_FLTRES:
                return "Floating point inexact result";
            case FPE_FLTINV:
                return "Floating point invalid operation";
            case FPE_FLTSUB:
                return "Subscript out of range";
        }
    }
    else if ( signal == SIGSEGV )
    {
        switch ( code )
        {
            case SEGV_MAPERR:
                return "Address not mapped to object";
            case SEGV_ACCERR:
                return "Invalid permissions for mapped object";
        }
    }
    else if ( signal == SIGBUS )
    {
        switch ( code )
        {
            case BUS_ADRALN:
                return "Invalid address alignment";
            case BUS_ADRERR:
                return "Non-existent physical address";
            case BUS_OBJERR:
                return "Object specific hardware error";
        }
    }
    else if ( signal == SIGTRAP )
    {
        switch ( code )
        {
            case TRAP_BRKPT:
                return "Process breakpoint";
            case TRAP_TRACE:
                return "Process trace trap";
        }
    }

    return "Unhandled signal handler";
}

char *strsignal( int32_t sig );

static void PCD_dump_maps_file( pid_t pid )
{
    struct stat fbuf;
    char mapsFile[ 22 ];
    int32_t fd;
	int32_t i;

    sprintf( mapsFile, "%s/%d.maps", CONFIG_PCD_TEMP_PATH, pid );

    /* Try to open the file */
    if ( stat(mapsFile, &fbuf) )
        return;

    fd = open( mapsFile, O_RDONLY );

    if ( fd > 0 )
    {
        char buffer[ 512 ];
        int32_t readBytes = 0;

        i = write( STDERR_FILENO, "\nMaps file:\n\n", 13 );

        /* Read the maps file and display it on the console */
        while ( ( readBytes = read( fd, buffer, sizeof( buffer ) ) ) > 0 )
        {
            i = write( STDERR_FILENO, buffer, readBytes );
        }

        close( fd );

        /* Delete the file */
        unlink( mapsFile );
    }
}

static void PCD_dump_fault_info( exception_t *exception )
{
    char buffer[ PCD_ERRLOG_BUF_SIZE ];
    int32_t i;

    memset( buffer, 0, PCD_ERRLOG_BUF_SIZE );

    /* Adding i for return value to avoid warnings on newer versions of gcc */
	i = write( STDERR_FILENO, "\n**************************************************************************\n", 76 );
    i = write( STDERR_FILENO, "**************************** Exception Caught ****************************", 74 );
    i = write( STDERR_FILENO, "\n**************************************************************************\n", 76 );

    i = snprintf( buffer, PCD_ERRLOG_BUF_SIZE - 1, "\nSignal information:\n\nTime: %sProcess name: %s\nPID: %d\nFault Address: %p\nSignal: %s\nSignal Code: %s\nLast error: %s (%d)\nLast error (by signal): %d\n",
                  ctime(&(exception->time.tv_sec)),
                  exception->process_name, exception->process_id, exception->fault_address,
                  strsignal( exception->signal_number ), PCD_code2str( exception->signal_code, exception->signal_number ),
                  strerror( exception->handler_errno ), exception->handler_errno, exception->signal_errno );

    if ( i<0 )
        return;

    i = write( STDERR_FILENO, buffer, i );

    PCD_errlog_log( buffer, False );

#ifdef CONFIG_PCD_PLATFORM_ARM /* Print ARM registers */
    i = snprintf( buffer, PCD_ERRLOG_BUF_SIZE - 1, "\nARM registers:\n\n"
                  "trap_no=0x%08lx\n"
                  "error_code=0x%08lx\n"
                  "oldmask=0x%08lx\n"
                  "r0=0x%08lx\n"
                  "r1=0x%08lx\n"
                  "r2=0x%08lx\n"
                  "r3=0x%08lx\n"
                  "r4=0x%08lx\n"
                  "r5=0x%08lx\n"
                  "r6=0x%08lx\n"
                  "r7=0x%08lx\n"
                  "r8=0x%08lx\n"
                  "r9=0x%08lx\n"
                  "r10=0x%08lx\n"
                  "fp=0x%08lx\n"
                  "ip=0x%08lx\n"
                  "sp=0x%08lx\n"
                  "lr=0x%08lx\n"
                  "pc=0x%08lx\n"
                  "cpsr=0x%08lx\n"
                  "fault_address=0x%08lx\n",
                  exception->regs.trap_no,
                  exception->regs.error_code,
                  exception->regs.oldmask,
                  exception->regs.arm_r0,
                  exception->regs.arm_r1,
                  exception->regs.arm_r2,
                  exception->regs.arm_r3,
                  exception->regs.arm_r4,
                  exception->regs.arm_r5,
                  exception->regs.arm_r6,
                  exception->regs.arm_r7,
                  exception->regs.arm_r8,
                  exception->regs.arm_r9,
                  exception->regs.arm_r10,
                  exception->regs.arm_fp,
                  exception->regs.arm_ip,
                  exception->regs.arm_sp,
                  exception->regs.arm_lr,
                  exception->regs.arm_pc,
                  exception->regs.arm_cpsr,
                  exception->regs.fault_address );

    if ( i<0 )
        return;
#endif

#ifdef CONFIG_PCD_PLATFORM_MIPS /* Print MIPS registers */
    i = snprintf( buffer, PCD_ERRLOG_BUF_SIZE - 1, "\nMIPS registers:\n\n"
				"regmask=0x%08x\n"
				"status=0x%08x\n"
				"pc=0x%08llx\n"
				"zero=0x%08x\n"
				"at=0x%08x\n"
    			"v0=0x%08x\n"
    			"v1=0x%08x\n"
				"a0=0x%08x\n"
				"a1=0x%08x\n"
				"a2=0x%08x\n"
				"a3=0x%08x\n"
    			"t0=0x%08x\n"
				"t1=0x%08x\n"
				"t2=0x%08x\n"
				"t3=0x%08x\n"
				"t4=0x%08x\n"
				"t5=0x%08x\n"
				"t6=0x%08x\n"
				"t7=0x%08x\n"
				"s0=0x%08x\n"
				"s1=0x%08x\n"
				"s2=0x%08x\n"
				"s3=0x%08x\n"
				"s4=0x%08x\n"
				"s5=0x%08x\n"
				"s6=0x%08x\n"
				"s7=0x%08x\n"
				"t8=0x%08x\n"
				"t9=0x%08x\n"
				"k0=0x%08x\n"
				"k1=0x%08x\n"
				"gp=0x%08x\n"
				"sp=0x%08x\n"
				"fp=0x%08x\n"
				"ra=0x%08x\n",
				exception->uc_mctx.regmask,
				exception->uc_mctx.status,
				exception->uc_mctx.pc,
				(u_int32_t)exception->uc_mctx.gregs[0],
				(u_int32_t)exception->uc_mctx.gregs[1],
				(u_int32_t)exception->uc_mctx.gregs[2],
				(u_int32_t)exception->uc_mctx.gregs[3],
				(u_int32_t)exception->uc_mctx.gregs[4],
				(u_int32_t)exception->uc_mctx.gregs[5],
				(u_int32_t)exception->uc_mctx.gregs[6],
				(u_int32_t)exception->uc_mctx.gregs[7],
				(u_int32_t)exception->uc_mctx.gregs[8],
				(u_int32_t)exception->uc_mctx.gregs[9],
				(u_int32_t)exception->uc_mctx.gregs[10],
				(u_int32_t)exception->uc_mctx.gregs[11],
				(u_int32_t)exception->uc_mctx.gregs[12],
				(u_int32_t)exception->uc_mctx.gregs[13],
				(u_int32_t)exception->uc_mctx.gregs[14],
				(u_int32_t)exception->uc_mctx.gregs[15],
				(u_int32_t)exception->uc_mctx.gregs[16],
				(u_int32_t)exception->uc_mctx.gregs[17],
				(u_int32_t)exception->uc_mctx.gregs[18],
				(u_int32_t)exception->uc_mctx.gregs[19],
				(u_int32_t)exception->uc_mctx.gregs[20],
				(u_int32_t)exception->uc_mctx.gregs[21],
				(u_int32_t)exception->uc_mctx.gregs[22],
				(u_int32_t)exception->uc_mctx.gregs[23],
				(u_int32_t)exception->uc_mctx.gregs[24],
				(u_int32_t)exception->uc_mctx.gregs[25],
				(u_int32_t)exception->uc_mctx.gregs[26],
				(u_int32_t)exception->uc_mctx.gregs[27],
				(u_int32_t)exception->uc_mctx.gregs[28],
				(u_int32_t)exception->uc_mctx.gregs[29],
				(u_int32_t)exception->uc_mctx.gregs[30],
				(u_int32_t)exception->uc_mctx.gregs[31]);

    if ( i<0 )
        return;
#endif

#ifdef CONFIG_PCD_PLATFORM_X86 /* Print X86 registers */
    i = snprintf( buffer, PCD_ERRLOG_BUF_SIZE - 1, "\nx86 registers:\n\n"
                  "cr2=0x%08lx\n"
                  "oldmask=0x%08lx\n"
                  "GS=0x%08x\n"
                  "FS=0x%08x\n"
                  "ES=0x%08x\n"
                  "DS=0x%08x\n"
                  "EDI=0x%08x\n"
                  "ESI=0x%08x\n"
                  "EBP=0x%08x\n"
                  "ESP=0x%08x\n"
                  "EBX=0x%08x\n"
                  "EDX=0x%08x\n"
                  "ECX=0x%08x\n"
                  "EAX=0x%08x\n"
                  "TRAPNO=0x%08x\n"
                  "ERR=0x%08x\n"
                  "EIP=0x%08x\n"
                  "CS=0x%08x\n"
                  "EFL=0x%08x\n"
                  "UESP=0x%08x\n"
                  "SS=0x%08x\n",
                  exception->uc_mctx.cr2,
                  exception->uc_mctx.oldmask,
                  exception->uc_mctx.gregs[REG_GS],
                  exception->uc_mctx.gregs[REG_FS],
                  exception->uc_mctx.gregs[REG_ES],
                  exception->uc_mctx.gregs[REG_DS],
                  exception->uc_mctx.gregs[REG_EDI],
                  exception->uc_mctx.gregs[REG_ESI],
                  exception->uc_mctx.gregs[REG_EBP],
                  exception->uc_mctx.gregs[REG_ESP],
                  exception->uc_mctx.gregs[REG_EBX],
                  exception->uc_mctx.gregs[REG_EDX],
                  exception->uc_mctx.gregs[REG_ECX],
                  exception->uc_mctx.gregs[REG_EAX],
                  exception->uc_mctx.gregs[REG_TRAPNO],
                  exception->uc_mctx.gregs[REG_ERR],
                  exception->uc_mctx.gregs[REG_EIP],
                  exception->uc_mctx.gregs[REG_CS],
                  exception->uc_mctx.gregs[REG_EFL],
                  exception->uc_mctx.gregs[REG_UESP],
                  exception->uc_mctx.gregs[REG_SS]);

    if ( i<0 )
        return;
#endif
#if defined(CONFIG_PCD_PLATFORM_X64) /* x64 registers */
    i = snprintf( buffer, PCD_ERRLOG_BUF_SIZE - 1, "\nx64 registers:\n\n"
                  "R8=0x%016lx\n"
                  "R9=0x%016lx\n"
                  "R10=0x%016lx\n"
                  "R11=0x%016lx\n"
                  "R12=0x%016lx\n"
                  "R13=0x%016lx\n"
                  "R14=0x%016lx\n"
                  "R15=0x%016lx\n"
                  "RDI=0x%016lx\n"
                  "RSI=0x%016lx\n"
                  "RBP=0x%016lx\n"
                  "RBX=0x%016lx\n"
                  "RDX=0x%016lx\n"
                  "RAX=0x%016lx\n"
                  "RCX=0x%016lx\n"
                  "RSP=0x%016lx\n"
                  "RIP=0x%016lx\n"
                  "EFL=0x%016lx\n"
                  "CSGSFS=0x%016lx\n"
                  "ERR=0x%016lx\n"
                  "TRAPNO=0x%016lx\n"
                  "OLDMASK=0x%016lx\n"
                  "CR2=0x%016lx\n",
                  (u_int64_t)exception->uc_mcontext.gregs[REG_R8],
                  (u_int64_t)exception->uc_mcontext.gregs[REG_R9],
                  (u_int64_t)exception->uc_mcontext.gregs[REG_R10],
                  (u_int64_t)exception->uc_mcontext.gregs[REG_R11],
                  (u_int64_t)exception->uc_mcontext.gregs[REG_R12],
                  (u_int64_t)exception->uc_mcontext.gregs[REG_R13],
                  (u_int64_t)exception->uc_mcontext.gregs[REG_R14],
                  (u_int64_t)exception->uc_mcontext.gregs[REG_R15],
                  (u_int64_t)exception->uc_mcontext.gregs[REG_RDI],
                  (u_int64_t)exception->uc_mcontext.gregs[REG_RSI],
                  (u_int64_t)exception->uc_mcontext.gregs[REG_RBP],
                  (u_int64_t)exception->uc_mcontext.gregs[REG_RBX],
                  (u_int64_t)exception->uc_mcontext.gregs[REG_RDX],
                  (u_int64_t)exception->uc_mcontext.gregs[REG_RAX],
                  (u_int64_t)exception->uc_mcontext.gregs[REG_RCX],
                  (u_int64_t)exception->uc_mcontext.gregs[REG_RSP],
                  (u_int64_t)exception->uc_mcontext.gregs[REG_RIP],
                  (u_int64_t)exception->uc_mcontext.gregs[REG_EFL],
                  (u_int64_t)exception->uc_mcontext.gregs[REG_CSGSFS],
                  (u_int64_t)exception->uc_mcontext.gregs[REG_ERR],
                  (u_int64_t)exception->uc_mcontext.gregs[REG_TRAPNO],
                  (u_int64_t)exception->uc_mcontext.gregs[REG_OLDMASK],
                  (u_int64_t)exception->uc_mcontext.gregs[REG_CR2]);

    if ( i<0 )
        return;

#endif

    i = write( STDERR_FILENO, buffer, i );
    PCD_errlog_log( buffer, False );

    PCD_dump_maps_file( exception->process_id );

    i = write( STDERR_FILENO, "\n**************************************************************************\n", 76 );
}

PCD_status_e PCD_exception_init( void )
{
    /* Create a FIFO stream that PCD will listen to */
    if ( mkfifo(PCD_EXCEPTION_FILE, 0644) < 0 )
    {
        if ( errno != EEXIST )
        {
            PCD_PRINTF_STDERR( "Failed to create FIFO exception file %s",  PCD_EXCEPTION_FILE );
            return PCD_STATUS_NOK;
        }
    }

    /* Open it */
    fd = open( PCD_EXCEPTION_FILE, O_RDONLY | O_NONBLOCK );

    if ( fd < 0 )
    {
        PCD_PRINTF_STDERR( "Failed to open exception file %s",  PCD_EXCEPTION_FILE );
        return PCD_STATUS_NOK;
    }

    /* Clear read fd */
    FD_ZERO(&rdset);
    FD_SET(fd, &rdset);

    return PCD_STATUS_OK;
}

PCD_status_e PCD_exception_close( void )
{
    if ( fd > 0 )
    {
        /* Close FIFO */
        close( fd );
        fd = -1;
        unlink( PCD_EXCEPTION_FILE );
    }

    return PCD_STATUS_OK;
}

void PCD_exception_listen( void )
{
    int32_t ret;
    struct timeval timeout = { 0, 0}; /* Do not block */

    ret = select(fd+1, &rdset, NULL, NULL, &timeout );

    /* Wait for incoming messages. Deal with signals correctly */
    while ( ret == -1 && errno == EINTR )
    {
        ret = select(fd+1, &rdset, NULL, NULL, &timeout );
    }

    if ( ret < 0 )
    {
        return;
    }
    else
    {
        exception_t exception;
        char *buffer = ( char *)&exception;
        u_int32_t remainingBytes = sizeof( exception_t );

        /* Read the incoming message. Might arrive in parts, and we read until we get
           the whole exception structure, or an error has occurred. */

        do
        {

            ret = read(fd, buffer, remainingBytes);

            /* No more information */
            if ( ret == 0 )
                break;

            /* Handle random signals */
            if ( ret == -1 && errno == EINTR )
                continue;

            /* Read error */
            if ( ret < 0 )
                break;

            buffer += ret;
            remainingBytes -= ret;

        } while ( ret && (remainingBytes > 0) );

        /* Go process the crash */
        if (ret > 0)
            PCD_dump_fault_info( &exception );
    }
}

