/*
 * pcdapi.h
 * Description:
 * PCD API header file
 *
 * Copyright (C) 2010 Texas Instruments Incorporated - http://www.ti.com/
 *
 * This library/application is free software; you can redistribute it and/or
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
 * Copyright (C) 2010-2012 PCD Project - http://www.rt-embedded.com/pcd
 * 
 * Change log:
 * - Support dynamic configurations from kconfig
 * - Added support for C++ applications
 */

/* Author:
 * Hai Shalom, hai@rt-embedded.com 
 *
 * PCD Homepage: http://www.rt-embedded.com/pcd/
 * PCD Project at SourceForge: http://sourceforge.net/projects/pcd/
 *  
 */

/***************************************************************************/
/*! \file pcdapi.h
 *  \brief PCD API header file
****************************************************************************/

#ifndef _PCDAPI_H_
#define _PCDAPI_H_

#ifdef __cplusplus /* Support for C++ applications */
extern "C" 
{ 
#endif 

/**************************************************************************/
/*      INCLUDES                                                          */
/**************************************************************************/
#include "system_types.h"
#include "ruleid.h"
#include "pcd_autoconf.h"

struct ruleId_t;

/* 	Optional Cleanup callback function. Use when the process wants to free
	or cleanup resources before exiting. */
typedef void( *Cleanup_func )( int signo );


/*! \def PCD_DECLARE_RULEID()
 *  \brief 		Define a ruleId easily when calling PCD API
*/
#define PCD_DECLARE_RULEID( ruleId, GROUP_NAME, RULE_NAME ) \
    ruleId_t ruleId = { GROUP_NAME, RULE_NAME }

typedef enum
{
    PCD_API_RULE_IDLE,                      /* Rule is idle, never been run */
    PCD_API_RULE_RUNNING,                   /* Rule is running; waiting for start or end condition */
    PCD_API_RULE_COMPLETED_PROCESS_RUNNING, /* Rule completed successfully, process is running (daemon) */
    PCD_API_RULE_COMPLETED_PROCESS_EXITED,  /* Rule completed successfully, process exited */
    PCD_API_RULE_NOT_COMPLETED,             /* Rule failed due to timeout, failure in end condition */
    PCD_API_RULE_FAILED,                    /* Rule failed due to process unexpected failure */

} pcdApiRuleState_e;

/**************************************************************************/
/*      INTERFACE FUNCTIONS Prototypes:                                   */
/**************************************************************************/

/*! \fn PCD_api_start_process()
 *  \brief 		Start a process associated with a rule
 *  \param[in] 		ruleId, optinal parameters
 *  \param[in,out] 	None
 *  \return			PCD_STATUS_OK - Success, <0 - Error
 */
PCD_status_e PCD_api_start_process( const struct ruleId_t *ruleId, const char *optionalParams );

/*! \fn PCD_api_signal_process()
 *  \brief 		Signal a process associated with a rule
 *  \param[in] 		ruleId, signal id
 *  \param[in,out] 	None
 *  \return			PCD_STATUS_OK - Success, <0 - Error
 */
PCD_status_e PCD_api_signal_process( const struct ruleId_t *ruleId, int32_t sig );

/*! \fn PCD_api_terminate_process()
 *  \brief 		Terminate a process associated with a rule, block until process dies
 *  \param[in] 		ruleId
 *  \param[in,out] 	None
 *  \return			PCD_STATUS_OK - Success, <0 - Error
 */
PCD_status_e PCD_api_terminate_process( const struct ruleId_t *ruleId );

/*! \fn PCD_api_terminate_process_non_blocking()
 *  \brief 		Terminate a process associated with a rule in non blocking mode
 *  \param[in] 		ruleId
 *  \param[in,out] 	None
 *  \return			PCD_STATUS_OK - Success, <0 - Error
 */
PCD_status_e PCD_api_terminate_process_non_blocking( const struct ruleId_t *ruleId );

/*! \fn PCD_api_kill_process()
 *  \brief 		Kill a process associated with a rule
 *  \param[in] 		ruleId
 *  \param[in,out] 	None
 *  \return			PCD_STATUS_OK - Success, <0 - Error
 */
PCD_status_e PCD_api_kill_process( const struct ruleId_t *ruleId );

/*! \fn PCD_api_send_process_ready()
 *  \brief 		Send PROCESS_READY event to PCD
 *  \param[in] 		None
 *  \param[in,out] 	None
 *  \return			PCD_STATUS_OK - Success, <0 - Error
 */
PCD_status_e PCD_api_send_process_ready( void );

/*! \fn PCD_api_get_rule_state()
 *  \brief 		Get rule state
 *  \param[in] 		ruleId
 *  \param[in,out] 	ruleState, see pcdApiRuleState_e
 *  \return			PCD_STATUS_OK - Success, <0 - Error
 */
PCD_status_e PCD_api_get_rule_state( const struct ruleId_t *ruleId, pcdApiRuleState_e *ruleState );

/*! \fn PCD_api_register_exception_handlers()
 *  \brief 		Register default PCD exception handler
 *  \param[in] 		argv[0]
 *  \param[in,out] 	None
 *  \return			PCD_STATUS_OK - Success, <0 - Error
 */
PCD_status_e PCD_api_register_exception_handlers( char *name, Cleanup_func cleanup );

/*! \fn PCD_main_find_process_id( char *cliname )
 *  \brief Find process ID, detects if another instance alrady running.
 *  \param[in] 		Process name
 *  \return pid on success, or 0 if not found
 */
pid_t PCD_api_find_process_id( char *name );

/*! \fn PCD_api_reduce_net_rx_priority
 *  \brief Reduce net-rx task priority to a given priority value (non preemtive mode)
 *  \param[in] 		New priority: 19 (lowest) to -19 (highest)
 *  \return 		PCD_STATUS_OK - Success, <0 - Error
 */
PCD_status_e PCD_api_reduce_net_rx_priority( int32_t priority );

/*! \fn PCD_api_restore_net_rx_priority
 *  \brief Restore net-rx task original priority
 *  \param[in,out] 	None
 *  \return			PCD_STATUS_OK - Success, <0 - Error
 */
PCD_status_e PCD_api_restore_net_rx_priority( void );

/*! \fn PCD_api_reboot( char *reason )
 *  \brief Display a reboot reason (optional) and reboot the system.
 *  \param[in] 		Reboot reason (optinal)
 *  \param[in] 		Force: Force reboot even if PCD is in debug mode.
 *  \return Never returns
 */
void PCD_api_reboot( const char *reason, bool_t force );

/*! \def PCD_API_REBOOT_MACRO
 *  \brief Helper macro with default reboot request message
 */
#define PCD_API_REBOOT_MACRO( force ) ({ char buffer[ 128 ]; snprintf( buffer, sizeof( buffer ), "Reboot requested by process %d in %s line %d", getpid(), __FUNCTION__, __LINE__ ); PCD_api_reboot( buffer, force ); })

/*! \def PCD_API_REBOOT
 *  \brief Reboot the system through the PCD, use a standard reason message stating the function and line
 */
#define PCD_API_REBOOT() ( PCD_API_REBOOT_MACRO(False) )

/*! \def PCD_API_REBOOT_ALWAYS
 *  \brief Reboot the system through the PCD, use a standard reason message stating the function and line
 *         Reboot always, even if PCD is in debug mode.
 */
#define PCD_API_REBOOT_ALWAYS() ( PCD_API_REBOOT_MACRO(True) )

/*! \def PCD_API_REGISTER_EXCEPTION_HANDLERS
 *  \brief Register	my process to PCD's default exception handlers
 */
#define PCD_API_REGISTER_EXCEPTION_HANDLERS() ( PCD_api_register_exception_handlers( argv[ 0 ], NULL ) )

/*! \def PCD_API_REGISTER_EXCEPTION_HANDLERS
 *  \brief Register	my process to PCD's default exception handlers
 */
#define PCD_API_REGISTER_EXCEPTION_HANDLERS() ( PCD_api_register_exception_handlers( argv[ 0 ], NULL ) )

/*! \def PCD_API_REGISTER_EXCEPTION_HANDLERS
 *  \brief Find if another copy of my process is already running.
 */
#define PCD_API_FIND_PROCESS_ID() ( PCD_api_find_process_id( argv[ 0 ] ) )

#ifdef __cplusplus 
} 
#endif 
#endif /* _PCDAPI_H_ */
