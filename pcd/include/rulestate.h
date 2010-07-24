/*
 * rulestate.h
 * Description:
 * PCD Rules state type header file
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

#ifndef _RULESTATE_H_
#define _RULESTATE_H_

/***************************************************************************/
/*! \file rulestate.h
 *  \brief Rules state header file
****************************************************************************/

/**************************************************************************/
/*      INTERFACE TYPES and STRUCT Definitions                            */
/**************************************************************************/

/*! \enum pcdRuleState_e
 *  \brief Rule state enumeration
 */
typedef enum
{
    PCD_RULE_IDLE,                      /* Rule is idle */
    PCD_RULE_ACTIVE,                    /* Rule is active, will soon be changed to waiting */
    PCD_RULE_START_CONDITION_WAITING,   /* Rule is waiting for start condition to be satisfied */
    PCD_RULE_END_CONDITION_WAITING,     /* Rule is waiting for end condition to be satisfied */
    PCD_RULE_COMPLETED,                 /* Rule completed successfully */
    PCD_RULE_NOT_COMPLETED,             /* Rule not completed, either failed end condition, or timeout */
    PCD_RULE_FAILED,                    /* Rule failed, either terminated or exited unexpectedly */

} pcdRuleState_e;

/*! \def PCD_RULE_ACTIVE
 *  \brief Macro to check if a rule is active or not
 */
#define PCD_RULE_ACTIVE( rule ) ( ( rule->ruleState >= PCD_RULE_START_CONDITION_WAITING ) && ( rule->ruleState <= PCD_RULE_END_CONDITION_WAITING ) )

#endif /* _RULESTATE_H_ */
