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

static Int32 fd = -1;
static fd_set rdset;

/* This translates a signal code into a readable string */
static inline char *PCD_code2str(Int32 code, Int32 signal)
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

char *strsignal( Int32 sig );

static void PCD_dump_maps_file( pid_t pid )
{
    struct stat fbuf;
    Char mapsFile[ 22 ];
    Int32 fd;

    sprintf( mapsFile, "%s/%d.maps", CONFIG_PCD_TEMP_PATH, pid );

    /* Try to open the file */
    if ( stat(mapsFile, &fbuf) )
        return;

    fd = open( mapsFile, O_RDONLY );

    if ( fd > 0 )
    {
        Char buffer[ 512 ];
        Int32 readBytes = 0;

        write( STDERR_FILENO, "\nMaps file:\n\n", 13 );

        /* Read the maps file and display it on the console */
        while ( ( readBytes = read( fd, buffer, sizeof( buffer ) ) ) > 0 )
        {
            write( STDERR_FILENO, buffer, readBytes );
        }

        close( fd );

        /* Delete the file */
        unlink( mapsFile );
    }
}

static void PCD_dump_fault_info( exception_t *exception )
{
    Char buffer[ PCD_ERRLOG_BUF_SIZE ];
    Int32 i;

    memset( buffer, 0, PCD_ERRLOG_BUF_SIZE );

    write( STDERR_FILENO, "\n**************************************************************************\n", 76 );
    write( STDERR_FILENO, "**************************** Exception Caught ****************************", 74 );
    write( STDERR_FILENO, "\n**************************************************************************\n", 76 );

    i = snprintf( buffer, PCD_ERRLOG_BUF_SIZE - 1, "\nSignal information:\n\nTime: %sProcess name: %s\nPID: %d\nFault Address: %p\nSignal: %s\nSignal Code: %s\nLast error: %s (%d)\nLast error (by signal): %d\n",
                  ctime(&(exception->time.tv_sec)),
                  exception->process_name, exception->process_id, exception->fault_address,
                  strsignal( exception->signal_number ), PCD_code2str( exception->signal_code, exception->signal_number ),
                  strerror( exception->handler_errno ), exception->handler_errno, exception->signal_errno );

    if ( i<0 )
        return;

    write( STDERR_FILENO, buffer, i );

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
                  "fpregs.fp_r.fp_dregs=0x%08f\n"
                  "fpregs.fp_r.fp_fregs[NFPREG]._fp_fregs=0x%08f\n"
                  "fpregs.fp_r.fp_fregs[NFPREG]._fp_pad=0x%08x\n"
                  "fp_owned=0x%08x\n"
                  "fpc_csr=0x%08x\n"
                  "fpc_eir=0x%08x\n"
                  "used_math=0x%08x\n"
                  "dsp=0x%08x\n"
                  "mdhi=0x%08llx\n"
                  "mdlo=0x%08llx\n"
                  "hi1=0x%08lx\n"
                  "lo1=0x%08lx\n"
                  "hi2=0x%08lx\n"
                  "lo2=0x%08lx\n"
                  "hi3=0x%08lx\n"
                  "lo3=0x%08lx\n",
                  exception->uc_mctx.regmask,
                  exception->uc_mctx.status,
                  exception->uc_mctx.pc,
                  exception->uc_mctx.fpregs.fp_r.fp_dregs[NFPREG],
                  exception->uc_mctx.fpregs.fp_r.fp_fregs[NFPREG]._fp_fregs,
                  exception->uc_mctx.fpregs.fp_r.fp_fregs[NFPREG]._fp_pad,
                  exception->uc_mctx.fp_owned,
                  exception->uc_mctx.fpc_csr,
                  exception->uc_mctx.fpc_eir,
                  exception->uc_mctx.used_math,
                  exception->uc_mctx.dsp,
                  exception->uc_mctx.mdhi,
                  exception->uc_mctx.mdlo,
                  exception->uc_mctx.hi1,
                  exception->uc_mctx.lo1,
                  exception->uc_mctx.hi2,
                  exception->uc_mctx.lo2,
                  exception->uc_mctx.hi3,
                  exception->uc_mctx.lo3);


    if ( i<0 )
        return;
#endif

#ifdef CONFIG_PCD_PLATFORM_X86 /* Print X86 registers */
    i = snprintf( buffer, PCD_ERRLOG_BUF_SIZE - 1, "\nX86 registers:\n\n"
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

    write( STDERR_FILENO, buffer, i );
    PCD_errlog_log( buffer, False );

    PCD_dump_maps_file( exception->process_id );

    write( STDERR_FILENO, "\n**************************************************************************\n", 76 );
}

STATUS PCD_exception_init( void )
{
    /* Create a FIFO stream that PCD will listen to */
    if ( mkfifo(PCD_EXCEPTION_FILE, 0644) < 0 )
    {
        if ( errno != EEXIST )
        {
            PCD_PRINTF_STDERR( "Failed to create FIFO exception file %s",  PCD_EXCEPTION_FILE );
            return STATUS_NOK;
        }
    }

    /* Open it */
    fd = open( PCD_EXCEPTION_FILE, O_RDONLY | O_NONBLOCK );

    if ( fd < 0 )
    {
        PCD_PRINTF_STDERR( "Failed to open exception file %s",  PCD_EXCEPTION_FILE );
        return STATUS_NOK;
    }

    /* Clear read fd */
    FD_ZERO(&rdset);
    FD_SET(fd, &rdset);

    return STATUS_OK;
}

STATUS PCD_exception_close( void )
{
    if ( fd > 0 )
    {
        /* Close FIFO */
        close( fd );
        fd = -1;
        unlink( PCD_EXCEPTION_FILE );
    }

    return STATUS_OK;
}

void PCD_exception_listen( void )
{
    Int32 ret;
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
        Char *buffer = ( Char *)&exception;
        Uint32 remainingBytes = sizeof( exception_t );

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

