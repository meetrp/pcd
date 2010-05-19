/*
 * system_types.h
 * Description:
 * General common primitive system types header file
 *
 * Copyright (C) 2010 Texas Instruments Incorporated - http://www.ti.com/
 *
 *
 *  Redistribution and use in source and binary forms, with or without
 *  modification, are permitted provided that the following conditions
 *  are met:
 *
 *    Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 *
 *    Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the
 *    distribution.
 *
 *    Neither the name of Texas Instruments Incorporated nor the names of
 *    its contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
 *
 *  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 *  "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 *  LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 *  A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 *  OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 *  SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 *  LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 *  DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 *  THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 *  (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 *  OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 */

#ifndef _SYSTEM_TYPES_H_
#define _SYSTEM_TYPES_H_

#ifndef _SYS_PTYPES_H_
#define _SYS_PTYPES_H_

/* Bool - Boolean Type */
typedef enum { False = 0, True = 1 } Bool;

/* Character */
typedef char            Char;

/* Signed integer definitions */
typedef long long       Int64;
typedef int             Int32;
typedef short           Int16;
typedef signed char     Int8;

/* Unsigned integer definitions */
typedef unsigned long long  Uint64;
typedef unsigned int        Uint32;
typedef unsigned short      Uint16;
typedef unsigned char       Uint8;

#endif

#ifndef _STATUS_H_
#define _STATUS_H_

typedef enum
{
    STATUS_OK = 0,      /* success */
    STATUS_NOK = -1,     /* error   */

    STATUS_INTERNAL_ERROR = 0x100,
    STATUS_SOCKET_ERROR,
    STATUS_LIBRARY_ERROR,
    STATUS_PROTOCOL_ERROR,
    STATUS_FILE_ERROR,

    STATUS_ERROR = 0x200,
    STATUS_ERROR_ABORT,
	STATUS_ERROR_FILE_EXCEEDS_MAX_SIZE,
    STATUS_ERROR_INVALID_ENTRY,
    STATUS_ERROR_MISSING_OPTION,
    STATUS_ERROR_OUT_OF_MEMORY,
    STATUS_ERROR_NO_SUCH_DEVICE_OR_ADDRESS,
    STATUS_ERROR_OPERATION_NOT_PERMITTED,
    STATUS_ERROR_PERMISSION_DENIED,
    STATUS_ERROR_INVALID_ARGUMENT,
    STATUS_ERROR_RESOURCE_BUSY,
    STATUS_ERROR_NOT_SUPPORTED,
    STATUS_ERROR_OVERFLOW,
    STATUS_ERROR_DESTINATION_UNREACHABLE,
    STATUS_ERROR_FILE_NOT_FOUND,
    STATUS_ERROR_NO_ANSWER,
	STATUS_ERROR_DATA_ERROR,

    STATUS_BAD_PARAMS = 0x300,
    STATUS_BAD_OPTION_FORMAT,
    STATUS_BAD_SERVER,
    STATUS_BAD_FLOW,

	STATUS_PASS_WITH_WARNINGS = 0x400,
	STATUS_PASS_INCOMPLETE,

} STATUS;

#endif

#endif /* _SYSTEM_TYPES_H_ */

