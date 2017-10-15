[introduction](index.md) | [build pcd](build.md) | [pcd cli](cli.md) | [pcd scripts](script.md) | **~~exception handlers~~** | [dependency graph](depend.md) | [generate header file](header.md) | [api](api.md)

REGISTERING TO THE PCD EXCEPTION HANDLERS
=========================================
The PCD provides a powerful means for debugging process/thread crashes: The PCD **default exception handlers**.
These exception handlers provide, in a safe way, the following information in case a process crashes:
- Process name and id.
- Signal description, origin and id.
- Date and time of the exception.
- Last known *errno*.
- Fault address (The address which caused the crash).
- Detailed register dump.
- Detailed map file (all accessible address spaces).

When a process performs an illegal operation, an exception takes place. The OS sends a fault signal (like SIGSEGV) to the faulting process. In order to handle it in your application, you must register to handle this signal. In most cases, the application does not register to handle any fault signal, so a default handler will be invoked upon generation of the signal. The default handler (if you don’t specify anything) is an empty function that prints “Segmentation Fault” and terminates the process. Unfortunately, it doesn’t provide any other information, and it is impossible to understand what was the problem, and actually, in complex systems, it is not trivial to understand which process has crashed, because this information is not provided as well.

## The PCD exception handlers in action
```
pcd: Starting process /usr/sbin/alignment (Rule TEST_ALIGNMENT).
Alignment trap: alignment (157) PC=0x000087cc Instr=0xe5901000 Addr=0x00010b3a FSR 0x011
pcd: Rule TEST_ALIGNMENT: Success (Process /usr/sbin/alignment (157)).

**************************************************************************
**************************** Exception Caught ****************************
**************************************************************************

Signal information:
Time: Thu Jan 1 00:03:10 1970
Process name: /usr/sbin/alignment
PID: 157
Fault Address: 0x00000000
Signal: Bus error
Signal Code: Kernel
Last error: Success (0)
Last error (by signal): 0

ARM registers:

trap_no=0x00000000
error_code=0x00000000
oldmask=0x00000000
r0=0x00010b3a
r1=0x0ea5ab84
r2=0x00000000
r3=0xfffff000
r4=0x0000000c
r5=0x0ea5aeb4
r6=0x0004cfa9
r7=0x00010b24
r8=0x00000000
r9=0x00000000
r10=0x00000000
fp=0x00000000
ip=0x00000000
sp=0x0ea5acd0
lr=0x000087cc
pc=0x000087cc
cpsr=0x40000010
fault_address=0x00000000

Maps file:

00008000-00009000   r-xp 00000000 1f:07 57 /usr/sbin/alignment
00010000-00011000 rw-p 00000000 1f:07 57 /usr/sbin/alignment
04000000-04005000 r-xp 00000000 1f:05 282 /lib/ld-uClibc-0.9.29.so
04005000-04007000 rw-p 04005000 00:00 0
0400c000-0400d000 r--p 00004000 1f:05 282 /lib/ld-uClibc-0.9.29.so
0400d000-0400e000 rw-p 00005000 1f:05 282 /lib/ld-uClibc-0.9.29.so
0400e000-04023000 r-xp 00000000 1f:05 213 /lib/libpcdapi.so
04023000-0402a000 ---p 04023000 00:00 0
0402a000-0402c000 rw-p 00014000 1f:05 213 /lib/libpcdapi.so
0402c000-04067000 r-xp 00000000 1f:05 241 /lib/libuClibc-0.9.29.so
04067000-0406e000 ---p 04067000 00:00 0
0406e000-0406f000 r--p 0003a000 1f:05 241 /lib/libuClibc-0.9.29.so
0406f000-04070000 rw-p 0003b000 1f:05 241 /lib/libuClibc-0.9.29.so
04070000-04075000 rw-p 04070000 00:00 0
04075000-0407f000 r-xp 00000000 1f:05 171 /lib/libgcc_s.so.1
0407f000-04086000 ---p 0407f000 00:00 0
04086000-04087000 rw-p 00009000 1f:05 171 /lib/libgcc_s.so.1
0ea46000-0ea5b000 rwxp 0ea46000 00:00 0 [stack]
**************************************************************************
pcd: Error: Process /usr/sbin/alignment (157) exited unexpectedly (Rule TEST_ALIGNMENT).
pcd: Terminating PCD, rebooting system...
The system is going down NOW!
Sent SIGTERM to all processes
Sent SIGKILL to all processes
Restarting system.
```

## What do I need to do to enable this feature?
The PCD provides signal registration function that registers all the fault signals for you. This function is provided in the  PCD API  library. In order to use this facility, implement the following steps:
- Include pcdapi.h in your main source file.
- Call the **PCD_API_REGISTER_EXCEPTION_HANDLERS()** macro in your *main()* function, as a **first operation**, right after the variable declaration (if any).
- Link your application with the PCD API and IPC libraries: add “**-lpcd -lipc**” to your *LDFLAGS*. You may also need to add “**-L<library-installation-dir>**” to tell the linker the location of the libraries.

## PCD’s Safe exception handlers
POSIX has the concept of “safe function”. If a signal interrupts an unsafe function, and the signal handler calls an unsafe function, then the behavior is undefined. Furthermore, in case of an exception, there is no guarantee for the integrity of the heap or stack, so it is even unsafe to use printf( ). Safe functions are listed explicitly in the various standards. Read [here](http://linux.die.net/man/2/signal) about the list of safe functions.

The PCD exception handlers are safe. When you register to the PCD exception handlers, the library actually registers signal handlers for you. These signal handlers are only gathering and collecting all the available crash information, and send it, in a safe manner using a FIFO, to the PCD. Once the PCD receives this information, it formats it in a human readable way, and prints this information on the console. Furthermore, it also logs this information in the non-volatile memory storage for offline analysis (if this option was enabled).

## Recommendation for your system’s health
It is recommended that all the processes in the system will register to the PCD exception handlers. In this way, you’ll gain plenty of useful debug information in case of a crash, which eventually increase your system’s quality, robustness, and availability. Please note, that even if a process is started and monitored by the PCD, the detailed exception information will not be displayed in case of a crash, if the process will not actively register to the service (The PCD can not do it for the process because all signal registrations are reset when a new process is executed).

## Examples
There are two articles that demonstrate the PCD exception handlers in action:
- [Resolving segmentation faults](segfault.md)
- [Resolving alignment traps](align.md)

