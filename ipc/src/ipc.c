/*
 * ipc.c
 * Description:
 * Inter process communication (IPC) library implementation file
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
 * Author:
 * Hai Shalom - hai@rt-embedded.com
 * 
 * * Change log:
 * - Hai Shalom: Use standard system types
 * - Hai Shalom: Added deinit function
 * - Hai Shalom: Added a lock to protect from concurrent accesses from 
 *               the same context (threads).
 */

#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <syslog.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <pthread.h>
#include <errno.h>
#include <sys/shm.h>
#include <sys/un.h>
#include "system_types.h"
#include "ipc.h"

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

*/

/*! \def IPC_MAX_LIST_SIZE
 *  \brief Maximum size of the IPC clients list
 */
#ifndef IPC_MAX_LIST_SIZE
#define IPC_MAX_LIST_SIZE   32
#endif /* IPC_MAX_LIST_SIZE */

/*! \def IPC_MAX_BUFFER_SIZE
 *  \brief Maximum IPC message size
 */
#ifndef IPC_MAX_BUFFER_SIZE
#define IPC_MAX_BUFFER_SIZE 1024
#endif /* IPC_MAX_BUFFER_SIZE */

/*! \def IPC_SOCKET_PATH
 *  \brief The path for the IPC sockets (platform depended - can be overridden by the makefile)
 */
#ifndef IPC_SOCKET_PATH
#define IPC_SOCKET_PATH     "/var/tmp"
#endif /* IPC_SOCKET_PATH */

/*! \def IPC_UNIX_PATH_MAX
 *  \brief Maximum path name of a socket
 */
#define IPC_UNIX_PATH_MAX   108

/*! \def IPC_MESSAGE_MAGIC
 *  \brief IPC message magic number
 */
#define IPC_MESSAGE_MAGIC   0x78AC39D1

/*! \struct IPC_client_t
 *  \brief IPC client record
 */
typedef struct
{
    int32_t   fd;
    char    path[ IPC_UNIX_PATH_MAX ];
    u_int32_t  flags;
    u_int32_t  owner;
    pid_t   pid;

} IPC_client_t;

/*! \struct IPC_list_t
 *  \brief IPC clients list and lock
 */
typedef struct
{
    pthread_mutex_t lock;
    IPC_client_t    list[ IPC_MAX_LIST_SIZE ];

} IPC_list_t;

/*! \struct IPC_info_t
 *  \brief IPC shared memory information
 */
typedef struct
{
    key_t   i_key;
    int32_t i_shmid;
    void    *i_shmaddr;
    pthread_mutex_t lock;    

} IPC_info_t;

static IPC_list_t *IPC_Clients = NULL;

static IPC_info_t info = { 0, 0, NULL };

/* A small macro to determine if library was initialized */
#define initDone ( info.i_shmaddr )

/* Enable this definition for debug prints */
#ifdef IPC_DEBUG_ENABLE
#define ENTER_FUNC      fprintf( stdout, "Entering function %s.\n", __FUNCTION__ )
#else
#define ENTER_FUNC
#endif

/*!\fn IPC_init
 * \brief Initialize the IPC module. To be used in case it requires general init.
 * \return          IPC_STATUS_OK - Success, IPC_STATUS_NOK - Error
 */
IPC_status_e IPC_init( u_int32_t flags )
{
    int32_t newdb = 0;

    ENTER_FUNC;

    if ( !initDone )
    {
        /* Generate an IPC key */
        if ((info.i_key = ftok("/proc/version", 124)) == -1)
        {
            return IPC_STATUS_NOK;
        }

        /* Try to get an existing shm */
        if ((info.i_shmid = shmget(info.i_key, 0, 0666 )) < 0 )
        {
            newdb = 1;

            /* If not exists, create it */
            if ((info.i_shmid = shmget(info.i_key, sizeof( IPC_list_t ), IPC_CREAT | 0666 )) < 0 )
            {
                IPC_PRINTF_ERROR_STDERR( "Shared memory failure" );
                return IPC_STATUS_NOK;
            }
        }

        /* Get the shm address */
        if ((info.i_shmaddr = (char *)shmat(info.i_shmid, NULL, 0)) == NULL)
        {
            IPC_PRINTF_ERROR_STDERR( "Shared memory failure" );
            return IPC_STATUS_NOK;
        }

        /* Create a lock for info, for IPC concurrent accesses in the same context */
        pthread_mutex_init( &info.lock, 0 );

        IPC_Clients = (IPC_list_t *)info.i_shmaddr;

        if( newdb )
        {
            pthread_mutexattr_t mutex_attr;
            
            /* Create a lock and clear the list in the very first time */            
            memset( IPC_Clients, 0, sizeof( IPC_list_t ) );
            
            /* Make the lock shared across all processes */
            pthread_mutexattr_init( &mutex_attr );
            pthread_mutexattr_setpshared( &mutex_attr, PTHREAD_PROCESS_SHARED );
            pthread_mutex_init( &IPC_Clients->lock, &mutex_attr );
        }
    }
    
    return IPC_STATUS_OK;
}

/*!\fn IPC_deinit
 * \brief Deinitialize the IPC module. IPC will be unavailable after this function call.
 * \return          IPC_STATUS_OK - Success, IPC_STATUS_NOK - Error
 */
IPC_status_e IPC_deinit( u_int32_t flags )
{
    u_int32_t i;
    
    ENTER_FUNC;

    if ( initDone )
    {        
        void *shmaddr = info.i_shmaddr;
        
        /* Wait for the lock */
        pthread_mutex_lock( &IPC_Clients->lock );
        
        /* Clear the shared memory address */
        info.i_shmaddr = NULL;
        
        for( i = 0; i < IPC_MAX_LIST_SIZE; i++ )
        {
            if( IPC_Clients->list[ i ].fd ) 
            {
                /* Remove open sockets, might not be enough if owner did not close it */
                unlink( IPC_Clients->list[ i ].path );
            }
        }

        /* Clear the list just in case */            
        memset( IPC_Clients->list, 0, sizeof( IPC_Clients->list ) );

        /* Unlock and destroy the lock */
        pthread_mutex_unlock( &IPC_Clients->lock );
        pthread_mutex_destroy( &IPC_Clients->lock );

        /* Delete shared memory segment */
        if( shmdt( shmaddr ) < 0 )
        {
            IPC_PRINTF_ERROR_STDERR( "Shared memory failure" );
            return IPC_STATUS_NOK;
        }
        
        /* Unlock and destroy the global context lock */
        pthread_mutex_destroy( &info.lock );
        
        /* Clear the info structure */            
        memset( &info, 0, sizeof( IPC_info_t ) );
    }
    return IPC_STATUS_OK;
}

/*!\fn IPC_start
 * \brief Start a communication channel.
 * \return          IPC_STATUS_OK - Success, IPC_STATUS_NOK - Error
 */
IPC_status_e IPC_start( char *myName, IPC_context_t *myContext, u_int32_t flags )
{
    struct sockaddr_un sun;
    int32_t fd;
    int32_t i = 0;

    ENTER_FUNC;

    /* Sanity checks */
    if( !initDone || !myName || !myContext )
    {
        IPC_PRINTF_ERROR_STDERR( "Failed to start IPC library" );
        return IPC_STATUS_NOK;
    }

    pthread_mutex_lock( &IPC_Clients->lock );

    /* Find an empty spot */
    while ( i < IPC_MAX_LIST_SIZE )
    {
        if ( IPC_Clients->list[ i ].fd == 0 )
        {
            break;
        }

        i++;
    }

    if ( i == IPC_MAX_LIST_SIZE )
    {
        pthread_mutex_unlock( &IPC_Clients->lock );

        IPC_PRINTF_ERROR_STDERR( "Maximum amount of clients has reached, consider enlarging the list" );

        /* No more space in list */
        return IPC_STATUS_NOK;
    }

    flags |= MSG_DONTWAIT;
    fd = socket( AF_UNIX, SOCK_DGRAM, 0 );

    if ( fd == -1 )
    {
        IPC_PRINTF_ERROR_STDERR( "Low level socket error" );

        pthread_mutex_unlock( &IPC_Clients->lock );
        return IPC_STATUS_NOK;
    }
    sun.sun_family = AF_UNIX;
    snprintf( sun.sun_path, IPC_UNIX_PATH_MAX, "%s/%s.ctl", IPC_SOCKET_PATH, myName );

    if ( bind( fd, (struct sockaddr*)&sun, sizeof(struct sockaddr_un) ) < 0 )
    {
        /* Ok, maybe the socket already exists try connecting
         * to see if another instance is present
         */
        if ( connect( fd, (struct sockaddr*)&sun,
                      sizeof(struct sockaddr_un) )==0 )
        {
            IPC_PRINTF_ERROR_STDERR( "Second instance already running" );
            pthread_mutex_unlock( &IPC_Clients->lock );
            return IPC_STATUS_NOK;
        }
        /* That wasn't it, lets try removing the socket from the filesystem */
        if ( unlink( sun.sun_path ) < 0 )
        {
            IPC_PRINTF_ERROR_STDERR( "Error removing old socket" );
            pthread_mutex_unlock( &IPC_Clients->lock );
            return IPC_STATUS_NOK;
        }
        /* Ok, if we are here, then we unlinked the old socket. Lets bind again */
        if ( bind( fd, (struct sockaddr*)&sun, sizeof(struct sockaddr_un) )<0 )
        {
            IPC_PRINTF_ERROR_STDERR( "Error binding socket" );
            close( fd );
            pthread_mutex_unlock( &IPC_Clients->lock );
            return IPC_STATUS_NOK;
        }
    }
    *myContext = ( IPC_context_t )i;

    /* Initialize the client record */
    IPC_Clients->list[ i ].fd = fd;
    IPC_Clients->list[ i ].flags = flags;
    IPC_Clients->list[ i ].owner = IPC_NO_OWNER;
    IPC_Clients->list[ i ].pid = getpid();
    strcpy( IPC_Clients->list[ i ].path, sun.sun_path );

    pthread_mutex_unlock( &IPC_Clients->lock );
    return IPC_STATUS_OK;
}

/*!\fn IPC_stop
 * \brief Stop a communication channel, free all the resources.
 * \return          IPC_STATUS_OK - Success, IPC_STATUS_NOK - Error
 */
IPC_status_e IPC_stop( IPC_context_t myContext )
{
    int32_t i = (int32_t)myContext;

    ENTER_FUNC;

    /* Sanity checks */
    if( !initDone || i >= IPC_MAX_LIST_SIZE || IPC_Clients->list[ i ].fd == 0 )
    {
        return IPC_STATUS_NOK;
    }

    pthread_mutex_lock( &IPC_Clients->lock );

    /* Close the socket */
    if( IPC_Clients->list[ i ].pid == getpid() )
    {
        close( IPC_Clients->list[ i ].fd );
    }

    /* Clear the client record */
    unlink( IPC_Clients->list[ i ].path );
    IPC_Clients->list[ i ].path[ 0 ] = '\0';
    IPC_Clients->list[ i ].fd = 0;
    IPC_Clients->list[ i ].owner = IPC_NO_OWNER;
    IPC_Clients->list[ i ].pid = 0;
    
    pthread_mutex_unlock( &IPC_Clients->lock );
    return IPC_STATUS_OK;
}

/*!\fn IPC_alloc_msg
 * \brief Allocate memory for a message. Note that this pointer is not usable, it is a pointer for
 * \return          IPC_STATUS_OK - Success, IPC_STATUS_NOK - Error
 */
IPC_message_t *IPC_alloc_msg( IPC_context_t myContext, u_int32_t size )
{
    IPC_message_t *msg;
    int32_t i = (int32_t)myContext;

    ENTER_FUNC;

    /* Sanity checks */
    if ( size + sizeof( IPC_message_t ) > IPC_MAX_BUFFER_SIZE || i >= IPC_MAX_LIST_SIZE || IPC_Clients->list[ i ].fd == 0 )
    {
        IPC_PRINTF_ERROR_STDERR( "IPC message to large" );
        return NULL;
    }

    /* Allocate a message */
    msg = malloc( size + sizeof( IPC_message_t ) );

    if ( !msg )
    {
        IPC_PRINTF_ERROR_STDERR( "Failed to allocate IPC message memory" );
        return NULL;
    }

    /* Setup message header */
    msg->magic = IPC_MESSAGE_MAGIC;
    msg->size = size + sizeof( IPC_message_t );
    msg->context = i;

    return msg;
}

/*!\fn IPC_get_msg
 * \brief Get a pointer to the data in the message body. Required in case the IPC module encapsulates infromation in the message body.
 * \return          IPC_STATUS_OK - Success, IPC_STATUS_NOK - Error
 */
void *IPC_get_msg( IPC_message_t *msg )
{
    ENTER_FUNC;

    /* Sanity checks */
    if ( !msg || msg->magic != IPC_MESSAGE_MAGIC )
    {
        IPC_PRINTF_ERROR_STDERR( "Invalid IPC message" );
        return NULL;
    }
    return( void *)msg->data;
}

/*!\fn IPC_free_msg
 * \brief Free message memory.
 * \return          IPC_STATUS_OK - Success, IPC_STATUS_NOK - Error
 */
IPC_status_e IPC_free_msg( IPC_message_t *msg )
{
    ENTER_FUNC;

    /* Sanity checks */
    if ( !msg || msg->magic != IPC_MESSAGE_MAGIC )
    {
        IPC_PRINTF_ERROR_STDERR( "Invalid IPC message" );
        return IPC_STATUS_NOK;
    }

    /* Just in case... */
    msg->magic = ~IPC_MESSAGE_MAGIC;

    free( msg );
    return IPC_STATUS_OK;
}

/*!\fn IPC_send_msg
 * \brief Send a message to a destination. Use either the destination's context or name.
 * \return          IPC_STATUS_OK - Success, IPC_STATUS_NOK - Error
 */
IPC_status_e IPC_send_msg( IPC_context_t destContext, IPC_message_t *msg )
{
    struct sockaddr_un to;
    int32_t destIdx = (int32_t)destContext;

    ENTER_FUNC;

    /* Sanity checks */
    if ( !initDone || !msg || msg->magic != IPC_MESSAGE_MAGIC || destIdx >= IPC_MAX_LIST_SIZE || IPC_Clients->list[ destIdx ].fd == 0 )
    {
        return IPC_STATUS_NOK;
    }

    to.sun_family = AF_UNIX;
    strcpy( to.sun_path, IPC_Clients->list[ destIdx ].path );

    /* Wait for the lock */
    pthread_mutex_lock( &info.lock );

    /* Send the message to the destination */
    if ( sendto( IPC_Clients->list[ msg->context ].fd, msg, msg->size, IPC_Clients->list[ msg->context ].flags, (struct sockaddr *)&to, sizeof(struct sockaddr_un) ) < 0 )
    {
        pthread_mutex_unlock( &info.lock );
        IPC_PRINTF_ERROR_STDERR( "Send IPC messaged failed" );
        return IPC_STATUS_NOK;
    }

    pthread_mutex_unlock( &info.lock );

    /* Message was copied by the kernel, we can now free the message */
    IPC_free_msg( msg );

    return IPC_STATUS_OK;
}

/*!\fn IPC_wait_msg
 * \brief Wait a specific amount of time for an incoming message.
 * \return          IPC_STATUS_OK - Success, IPC_STATUS_NOK - Error
 */
IPC_status_e IPC_wait_msg( IPC_context_t myContext, IPC_message_t **msgBuffer, IPC_timeout_e timeout )
{
    struct timeval to, *pto;
    fd_set rdset;
    int32_t i = (int32_t)myContext;
    int32_t ret;
    int32_t fd;
    void *localMsgBuffer = NULL;

    /* Sanity checks */
    if( !initDone || i >= IPC_MAX_LIST_SIZE || !msgBuffer || IPC_Clients->list[ i ].fd == 0 )
    {
        return IPC_STATUS_NOK;
    }

    fd = (int32_t)IPC_Clients->list[ i ].fd;

    /* Setup timeout */
    if( timeout == IPC_TIMEOUT_FOREVER )
    {
        pto = NULL;
    }
    else
    {
        pto = &to;

        if( timeout == IPC_TIMEOUT_IMMEDIATE )
        {
            to.tv_sec = 0;
            to.tv_usec = 0;
        }
        else
        {
            /* Define the timeout while waiting for the message */
            to.tv_sec = timeout / 1000;
            to.tv_usec = timeout - ( to.tv_sec * 1000 );
        }
    }

    /* Init file descriptors */
    FD_ZERO(&rdset);
    FD_SET(fd, &rdset);

    /* Wait for the lock */
    pthread_mutex_lock( &info.lock );

    /* Wait for incoming messages. Deal with signals correctly */
    do
    {
        ret = select( fd+1, &rdset, 0, 0, pto );

    } while( ret == -1 && errno == EINTR );

    if(ret <= 0)
    {
        /* timeout or error, return with error */
        pthread_mutex_unlock( &info.lock );
        return IPC_STATUS_NOK;
    }
    else
    {
        /* context listening socket operations */
        if(FD_ISSET(fd, &rdset))
        {
            /* Allocate memory for the incoming message */
            localMsgBuffer = malloc( IPC_MAX_BUFFER_SIZE );

            if( !localMsgBuffer )
            {
                IPC_PRINTF_ERROR_STDERR( "Failed to allocate IPC message memory" );
                goto wait_msg_fail;
            }

            /* Receive the message */
            ret = recv(fd, localMsgBuffer, IPC_MAX_BUFFER_SIZE, MSG_DONTWAIT | MSG_NOSIGNAL);
            
            pthread_mutex_unlock( &info.lock );
            
            /* Read error */
            if( ret < 0 )
                 goto wait_msg_fail;

            /* Initialize caller pointer with the buffer */
            *msgBuffer = localMsgBuffer;
        }
        else
        {
            return IPC_STATUS_NOK;
        }
    }

    return IPC_STATUS_OK;

    wait_msg_fail:
    free( localMsgBuffer );

    return IPC_STATUS_NOK;
}

/*!\fn IPC_reply_msg
 * \brief Reply to an incoming message. Incoming message needs to be freed after replying.
 * \return          IPC_STATUS_OK - Success, IPC_STATUS_NOK - Error
 */
IPC_status_e IPC_reply_msg( IPC_message_t *incomingMsg, IPC_message_t *replyMsg )
{
    struct sockaddr_un to;

    ENTER_FUNC;

    /* Sanity checks */
    if ( !initDone || !incomingMsg || incomingMsg->magic != IPC_MESSAGE_MAGIC
         || !replyMsg || replyMsg->magic != IPC_MESSAGE_MAGIC )
    {
        return IPC_STATUS_NOK;
    }

    to.sun_family = AF_UNIX;
    strcpy( to.sun_path, IPC_Clients->list[ (int32_t)incomingMsg->context ].path );

    /* Wait for the lock */
    pthread_mutex_lock( &info.lock );

    /* Send a reply */
    if ( sendto( IPC_Clients->list[ (int32_t)replyMsg->context ].fd, replyMsg, replyMsg->size, IPC_Clients->list[ (int32_t)replyMsg->context ].flags, (struct sockaddr *)&to, sizeof(struct sockaddr_un) ) < 0 )
    {
        pthread_mutex_unlock( &info.lock );
        
        IPC_PRINTF_ERROR_STDERR( "Send IPC messaged failed" );
        return IPC_STATUS_NOK;
    }

    pthread_mutex_unlock( &info.lock );
    
    /* Message was copied by the kernel, we can now free the message */
    IPC_free_msg( replyMsg );

    return IPC_STATUS_OK;

}

/*!\fn IPC_get_msg_context
 * \brief Get the owner index of the message
 * \return          IPC_STATUS_OK - Success, IPC_STATUS_NOK - Error
 */
IPC_status_e IPC_get_msg_context( IPC_message_t *msg, IPC_context_t *msgContext )
{
    ENTER_FUNC;

    /* Sanity checks */
    if ( !msg || !msgContext || msg->magic != IPC_MESSAGE_MAGIC  )
    {
        return IPC_STATUS_NOK;
    }

    *msgContext = (IPC_context_t)msg->context;
    return IPC_STATUS_OK;
}

/*!\fn IPC_cleanup_proc
 * \brief Cleanup IPC resources of a specific process (optional).
 * \return          IPC_STATUS_OK - Success, IPC_STATUS_NOK - Error
 */
IPC_status_e IPC_cleanup_proc( pid_t pid )
{
    int32_t i = 0;

    ENTER_FUNC;

    if( !initDone )
    {
        return IPC_STATUS_NOK;
    }
   
    while ( i < IPC_MAX_LIST_SIZE )
    {
        /* Remove all entries of a specific pid */
        if ( ( IPC_Clients->list[ i ].fd != 0 ) && ( IPC_Clients->list[ i ].pid == pid ) )
        {
            return IPC_stop( (IPC_context_t)i );
        }

        i++;
    }
   
    return IPC_STATUS_OK;
}

/*!\fn IPC_set_owner
 * \brief Set ownership (index value) on an IPC resource (optional).
 * \return          IPC_STATUS_OK - Success, IPC_STATUS_NOK - Error
 */
IPC_status_e IPC_set_owner( IPC_context_t myContext, u_int32_t owner )
{
    int32_t i = (int32_t)myContext;

    ENTER_FUNC;

    /* Sanity checks */
    if ( !initDone || i >= IPC_MAX_LIST_SIZE )
        return IPC_STATUS_NOK;

    pthread_mutex_lock( &IPC_Clients->lock );

    IPC_Clients->list[ i ].owner = owner;

    pthread_mutex_unlock( &IPC_Clients->lock );
    return IPC_STATUS_OK;
}

/*!\fn IPC_get_context_by_owner
 * \brief Get an index value of an IPC resource (optional).
 * \return          IPC_STATUS_OK - Success, IPC_STATUS_NOK - Error
 */
IPC_status_e IPC_get_context_by_owner( IPC_context_t *destContext, u_int32_t owner )
{
    int32_t i = 0;

    ENTER_FUNC;

    /* Sanity checks */
    if( !initDone || !destContext )
    {
        return IPC_STATUS_NOK;
    }

    pthread_mutex_lock( &IPC_Clients->lock );

    while ( i < IPC_MAX_LIST_SIZE )
    {
        if ( ( IPC_Clients->list[ i ].fd != 0 ) && ( IPC_Clients->list[ i ].owner == owner ) )
        {
            *destContext = ( IPC_context_t )i;

            pthread_mutex_unlock( &IPC_Clients->lock );
            return IPC_STATUS_OK;
        }

        i++;
    }

    pthread_mutex_unlock( &IPC_Clients->lock );
    return IPC_STATUS_NOK;
}

/*!\fn IPC_general_func
 * \brief Optional general function for any extension required.
 * \return          IPC_STATUS_OK - Success, IPC_STATUS_NOK - Error
 */
IPC_status_e IPC_general_func( u_int32_t value, void *data, u_int32_t dataSize )
{
    ENTER_FUNC;
    return IPC_STATUS_OK;
}

