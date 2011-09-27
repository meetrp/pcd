/*
 * ipc.h
 * Description:
 * Inter process communication (IPC) library header file
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
 * Author:
 * Hai Shalom - hai@rt-embedded.com
 */

#ifndef __IPC_H__
#define __IPC_H__

#include "system_types.h"

/* IPC Library

    The IPC library utilizes the UNIX sockets to create client destination points,
    where the actual implementation and socket path is encapsulated.
    The destination points database is managed using a shared memory segment,
    which lists the clients details. A well known process can own a destination
    point using a predefined index number. Clients who wish to communicate with it
    can use the predefined number without knowing the actual path of the socket.

    The IPC message format guarantees a certain level of message integrity by adding
    magic and size fields.

    The flow of usage is as follows:

    To start the IPC:
    1. IPC_init     -> Initialize the library.
    2. IPC_start    -> Start a destination point. The IPC can now be used.
    3. IPC_set_owner    -> (Optional) Own a destination point.

    To wait for a message (Server):
    1. IPC_wait_msg -> Wait for incoming message.
    2. IPC_get_msg_context -> (Optional) Get the message's destination point.
    3. IPC_get_msg      -> Get a pointer for the data segment in the message.
    4. IPC_reply_msg    -> Reply using the incoming message which needs to be freed later.
    5. IPC_free_msg     -> Free the incoming message

    To send a message (Client):
    1. IPC_alloc_msg    -> Allocate memory for a message.
    2. IPC_get_msg      -> Get a pointer for the data segment in the message.
    3. IPC_get_context_by_owner -> Get the destination point of a certain owner.
    4. IPC_send_msg     -> Send a message. The recipient must free the message when done.

    To stop the IPC on a specific destination point:
    1. IPC_stop         -> Stop the IPC. Free the resources.

    To free the IPC resources after all destination points were stopped:
    1. IPC_deinit       -> Deinitialize the library, free all resources.
  
    General functions:
    1. IPC_cleanup_proc -> A general function to cleanup resources of a context. Can be used by a process monitor.
    2. IPC_general_func -> A general purpose function. Not used currently.
 
 * Copyright (C) 2011 PCD Project - http://www.rt-embedded.com/pcd
 * 
 * Change log:
 * - Use standard system types
 */

/* IPC Status enum
 * 
 */
typedef enum
{
	IPC_STATUS_OK = 0,
	IPC_STATUS_NOK = -1
	
} IPC_status_e;

/*! \IPC_message_t
 *  \brief IPC message structure
 */
typedef struct
{
    u_int32_t  magic;
    u_int32_t  size;
    int32_t   context;
    u_int8_t   data[0];

} IPC_message_t;

typedef enum
{
	IPC_TIMEOUT_IMMEDIATE = 0,
	IPC_TIMEOUT_FOREVER = ~0

} IPC_timeout_e;

enum { IPC_NO_OWNER = ~0U };

/*! \def IPC_PRINTF_ERROR_STDERR
 *  \brief Print an error message to standard error
 */
#define IPC_PRINTF_ERROR_STDERR( format, args... )		fprintf( stderr, "ipc: Error: " format ".\n", ## args )

typedef u_int32_t IPC_context_t;

/*!\fn IPC_init
 * \brief Initialize the IPC module. To be used in case it requires general init.
 * \param[in] 		flags: Special handling flags
 * \return			IPC_STATUS_OK - Success, IPC_STATUS_NOK - Error
 */
IPC_status_e IPC_init( u_int32_t flags );

/*!\fn IPC_deinit
 * \brief Deinitialize the IPC module. IPC will be unavailable after this function call.
 * \return          IPC_STATUS_OK - Success, IPC_STATUS_NOK - Error
 */
IPC_status_e IPC_deinit( u_int32_t flags );

/*!\fn IPC_start
 * \brief Start a communication channel.
 * \param[in] 		myName: Context socket identifier
 * \param[in] 		flags: Special handling flags
 * \param[out]      myContext: Context handle, to be used with the IPC API
 * \return			IPC_STATUS_OK - Success, IPC_STATUS_NOK - Error
 */
IPC_status_e IPC_start( char *myName, IPC_context_t *myContext, u_int32_t flags );

/*!\fn IPC_stop
 * \brief Stop a communication channel, free all the resources.
 * \param[in] 		myContext: Context handle
 * \return			IPC_STATUS_OK - Success, IPC_STATUS_NOK - Error
 */
IPC_status_e IPC_stop( IPC_context_t myContext );

/*!\fn IPC_alloc_msg
 * \brief Allocate memory for a message
 * \param[in] 		myContext: Context handle
 * \param[in] 	    size: Buffer size
 * \return			Pointer to an IPC message - Success, NULL - Error
 */
IPC_message_t *IPC_alloc_msg( IPC_context_t myContext, u_int32_t size );

/*!\fn IPC_get_msg
 * \brief Get a pointer to the data in the message body. Required in case the IPC module encapsulates infromation in the message body.
 * \param[in] 		msg: Pointer to an IPC message
 * \return			Pointer to the data - Success, NULL - Error
 */
void *IPC_get_msg( IPC_message_t *msg );

/*!\fn IPC_free_msg
 * \brief Free message memory.
 * \param[in] 		msg: Pointer to an IPC message
 * \return			IPC_STATUS_OK - Success, IPC_STATUS_NOK - Error
 */
IPC_status_e IPC_free_msg( IPC_message_t *msg );

/*!\fn IPC_send_msg
 * \brief Send a message to a destination. Use either the destination's context or name.
 * \param[in] 		destContext: Destination context (message target)
 * \param[in] 		msg: Pointer to an IPC message
 * \return			IPC_STATUS_OK - Success, IPC_STATUS_NOK - Error
 */
IPC_status_e IPC_send_msg( IPC_context_t destContext, IPC_message_t *msg );

/*!\fn IPC_wait_msg
 * \brief Wait a specific amount of time for an incoming message.
 * \param[in] 		myContext: Context handle
 * \param[in] 	    msgBuffer: Address of a pointer to an IPC message. Upon message reception, this pointer will be populated.
 * \param[in] 		timeout: Define the maximum time to wait in ms, or using IPC_timeout_e
 * \return			IPC_STATUS_OK - Success, IPC_STATUS_NOK - Error
 */
IPC_status_e IPC_wait_msg( IPC_context_t myContext, IPC_message_t **msgBuffer, IPC_timeout_e timeout );

/*!\fn IPC_reply_msg
 * \brief Reply to an incoming message. Incoming message needs to be freed after replying.
 * \param[in]       incomingMsg: The IPC message which we want to reply to.
 * \param[in] 	    replyMsg: The IPC reply message
 * \return			IPC_STATUS_OK - Success, IPC_STATUS_NOK - Error
 */
IPC_status_e IPC_reply_msg( IPC_message_t *incomingMsg, IPC_message_t *replyMsg );

/*!\fn IPC_get_msg_owner
 * \brief Get a pointer to the data in the message body. Required in case the IPC module encapsulates infromation in the message body.
 * \param[in]
 * \param[in,out]
 * \return			IPC_STATUS_OK - Success, IPC_STATUS_NOK - Error
 */
void *IPC_get_msg( IPC_message_t *msg );

/*!\fn IPC_get_msg_context
 * \brief Get the owner index of the message
 * \param[in] 		msg: Pointer to an IPC message
 * \param[out] 	    msgContext: The message context
 * \return			IPC_STATUS_OK - Success, IPC_STATUS_NOK - Error
 */
IPC_status_e IPC_get_msg_context( IPC_message_t *msg, IPC_context_t *msgContext );

/*!\fn IPC_cleanup_proc
 * \brief Cleanup IPC resources of a specific process (optional).
 * \param[in]       pid: Process ID
 * \return			IPC_STATUS_OK - Success, IPC_STATUS_NOK - Error
 */
IPC_status_e IPC_cleanup_proc( pid_t pid );

/*!\fn IPC_set_owner
 * \brief Set ownership (index value) on an IPC resource (optional).
 * \param[in] 		myContext: Context handle
 * \param[in] 	    owner: Owner's ID, a constant value which *always* represents this context
 * \return			IPC_STATUS_OK - Success, IPC_STATUS_NOK - Error
 */
IPC_status_e IPC_set_owner( IPC_context_t myContext, u_int32_t owner );

/*!\fn IPC_get_context_by_owner
 * \brief Get an index value of an IPC resource (optional).
 * \param[in] 		owner: Owner's ID, a constant value which *always* represents this context
 * \param[out] 	    destContext: Destination context which is identified as the owner
 * \return			IPC_STATUS_OK - Success, IPC_STATUS_NOK - Error
 */
IPC_status_e IPC_get_context_by_owner( IPC_context_t *destContext, u_int32_t owner );

/*!\fn IPC_general_func
 * \brief Optional general function for any extension required.
 * \param[in]       value: Some value
 * \param[in]       dataSize: Used to determine the data size
 * \param[in,out] 	data: Some data
 * \return			IPC_STATUS_OK - Success, IPC_STATUS_NOK - Error
 */
IPC_status_e IPC_general_func( u_int32_t value, void *data, u_int32_t dataSize );

#endif /*__IPC_H__*/

