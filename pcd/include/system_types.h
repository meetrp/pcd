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
 * Copyright (C) 2011 PCD Project - http://www.rt-embedded.com/pcd
 * 
 * Change log:
 * - Use standard system types
 */

#ifndef _SYSTEM_TYPES_H_
#define _SYSTEM_TYPES_H_

#include <sys/types.h>

#if !defined( False ) && !defined( True )
#define False 	(0)
#define True 	(!False)
#endif

#ifndef bool_t
/* Boolean system type */
typedef u_int32_t bool_t;

#endif

/* PCD return status */
typedef enum
{
	PCD_STATUS_OK = 0,
	PCD_STATUS_WAIT = 1,
	PCD_STATUS_NOK = -1,
	PCD_STATUS_INVALID_RULE = -2,
	PCD_STATUS_BAD_PARAMS = -3,
	PCD_STATUS_TIMEOUT = -4
	
} PCD_status_e;

#endif /* _SYSTEM_TYPES_H_ */
