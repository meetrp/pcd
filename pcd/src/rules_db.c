/*
 * rules_db.c
 * Description:
 * PCD Rules database implementation file
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
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "system_types.h"
#include "rules_db.h"
#include "process.h"
#include "timer.h"
#include "pcd.h"

/**************************************************************************/
/*      LOCAL DEFINITIONS AND VARIABLES                                   */
/**************************************************************************/

static ruleGroup_t *rulesListHead = NULL;
static ruleGroup_t *lastReturnedGroup = NULL;
static rule_t *lastReturnedRule = NULL;

/**************************************************************************/
/*      IMPLEMENTATION                                                    */
/**************************************************************************/

static PCD_status_e PCD_rulesdb_add_rule_to_group( ruleGroup_t *group, rule_t *newRule )
{
    char *ruleName = newRule->ruleId.ruleName;
    rule_t *ruleList = group->firstRule;
    rule_t *prevRule;

    if ( !ruleList )
    {
         ruleList = newRule;
         ruleList->next = NULL;

         return PCD_STATUS_OK;
    }

    prevRule = NULL;


    while ( 1 )
    {
         int32_t val;

         val = strcmp( ruleName, ruleList->ruleId.ruleName );

         if ( val == 0 )
         {
              PCD_PRINTF_STDERR( "Multiple definitions of rule %s_%s, ignoring", newRule->ruleId.groupName, newRule->ruleId.ruleName );
              return PCD_STATUS_NOK;
         }

         /* Search for the right place to put the rule */
         if ( val > 0 )
         {
              if ( ruleList->next )
              {
                   /* Move to next rule in the list */
                   prevRule = ruleList;
                   ruleList = ruleList->next;
                   continue;
              }
              else
              {
                   /* Add rule to the end of the list */
                   ruleList->next = newRule;
                   newRule->next = NULL;
                   break;
              }
         }
         else
         {
              if ( !prevRule )
              {
                   /* Update the first rule */
                   newRule->next = group->firstRule;
                   group->firstRule = newRule;
                   break;
              }
              else
              {
                   /* Add new rule between the two elements */
                   newRule->next = ruleList;
                   prevRule->next = newRule;
                   break;
              }
         }
    }

    return PCD_STATUS_OK;
}

PCD_status_e PCD_rulesdb_add_rule( rule_t *newrule )
{
    ruleGroup_t *searchList = rulesListHead;
    rule_t *rule;

    if( !newrule )
         return PCD_STATUS_NOK;

    rule = malloc( sizeof( rule_t ) );

    if ( !rule )
    {
         PCD_PRINTF_STDERR( "failed to allocate memory" );
         return PCD_STATUS_NOK;
    }

    memcpy( rule, newrule, sizeof( rule_t ) );

    while ( searchList )
    {
         /* Find if the group already exists */
         if ( strcmp( searchList->groupName, rule->ruleId.groupName ) == 0 )
         {
              if ( PCD_rulesdb_add_rule_to_group( searchList, rule ) == PCD_STATUS_NOK )
              {
                   free( rule );
                   rule = NULL;
              }
              return PCD_STATUS_OK;
         }

         searchList = searchList->next;
    }

    /* Create a new group */
    if ( !searchList )
    {
         searchList = malloc( sizeof( ruleGroup_t ) );

         if ( !searchList )
         {
              free( rule );
              PCD_PRINTF_STDERR(  "failed to allocate memory" );
              return PCD_STATUS_NOK;
         }

         searchList->groupName = malloc( strlen( rule->ruleId.groupName ) + 1 );

         if ( !searchList->groupName )
         {
              free( rule );
              free( searchList );
              PCD_PRINTF_STDERR(  "failed to allocate memory" );
              return PCD_STATUS_NOK;
         }

         strcpy( searchList->groupName, rule->ruleId.groupName );

         /* Add the first rule to the new group */
         searchList->firstRule = rule;
         searchList->next = NULL;

         /* Check if this is the first group */
         if ( !rulesListHead )
         {
              rulesListHead = searchList;
         }
         else
         {
              ruleGroup_t *newSearchList = rulesListHead;

              /* Find the next free space */
              while ( newSearchList->next )
              {
                   newSearchList = newSearchList->next;
              }

              /* Add a new element */
              newSearchList->next = searchList;
         }
    }

    return PCD_STATUS_OK;
}

rule_t *PCD_rulesdb_get_rule_by_id( ruleId_t *ruleId )
{
    ruleGroup_t *searchList = rulesListHead;
    rule_t *searchRuleList;
    rule_t *tmpRule = NULL;

    if( !ruleId )
         return NULL;

    while ( searchList )
    {
         if( strcmp( ruleId->groupName, searchList->groupName ) == 0 )
              break;
         else
              searchList = searchList->next;
    }

    /* Not found...? */
    if( !searchList )
         return NULL;

    searchRuleList = searchList->firstRule;

    /* Note, the search can be optimized because the list is sorted */
    while ( searchRuleList )
    {
         /* Found */
         if( strcmp( searchRuleList->ruleId.ruleName, ruleId->ruleName ) == 0 )
         {
              return searchRuleList;
         }
         else if( ( searchRuleList->indexed ) && ( strncmp( searchRuleList->ruleId.ruleName, ruleId->ruleName, strlen(searchRuleList->ruleId.ruleName) ) == 0 ) )
         {
              /* Save the last match as a temporary rule */
              tmpRule = searchRuleList;
         }

         searchRuleList = searchRuleList->next;
    }

    if( tmpRule )
    {
         rule_t newRule;

         /* If we are here, so we need to create a new rule */
         memcpy( &newRule, tmpRule, sizeof( rule_t ) );
         newRule.indexed = False;
         newRule.optionalParams = NULL;
         strcpy( newRule.ruleId.ruleName, ruleId->ruleName );

         if( PCD_rulesdb_add_rule( &newRule ) == PCD_STATUS_OK )
         {
              return PCD_rulesdb_get_rule_by_id( ruleId );
         }
    }

    return NULL;
}

rule_t *PCD_rulesdb_get_rule_by_pid( int32_t pid )
{
    ruleGroup_t *searchList = rulesListHead;

    while ( searchList )
    {
         rule_t *searchRuleList;

         searchRuleList = searchList->firstRule;

         while ( searchRuleList )
         {
              /* Found */
              if( searchRuleList->proc->pid == pid )
                   return searchRuleList;

              searchRuleList = searchRuleList->next;
         }

         searchList = searchList->next;
    }

    return NULL;
}

rule_t *PCD_rulesdb_get_first( void )
{
    if( !rulesListHead )
         return NULL;

    lastReturnedGroup = rulesListHead;
    lastReturnedRule = rulesListHead->firstRule;

    return lastReturnedRule;
}

rule_t *PCD_rulesdb_get_next( void )
{
    if( ( !lastReturnedGroup ) || ( !lastReturnedRule ) )
         return NULL;

    if ( lastReturnedRule->next )
    {
         lastReturnedRule = lastReturnedRule->next;
         return lastReturnedRule;
    }
    else
    {
         lastReturnedGroup = lastReturnedGroup->next;

         if( !lastReturnedGroup )
              return NULL;
         else
         {
              lastReturnedRule = lastReturnedGroup->firstRule;
              return lastReturnedRule;
         }
    }

    return NULL;
}

PCD_status_e PCD_rulesdb_activate( void )
{
    rule_t *rule;

    /* Get the first rule */
    rule = PCD_rulesdb_get_first();

    while ( rule )
    {
         /* Find only active rules */
         if ( rule->ruleState == PCD_RULE_ACTIVE )
         {
              /* Enqueue the rule in the timer module */
              if ( PCD_timer_enqueue_rule( rule ) != PCD_STATUS_OK )
              {
                   PCD_PRINTF_STDERR( "Failed to enqueue rule %s_%s", rule->ruleId.groupName, rule->ruleId.ruleName );
                   return PCD_STATUS_NOK;
              }
         }

         /* Fetch the next rule */
         rule = PCD_rulesdb_get_next();
    }

    return PCD_STATUS_OK;
}

PCD_status_e PCD_rulesdb_setup_optional_params( rule_t *rule, const char *optionalParams )
{
    if( ( !rule ) || ( !optionalParams ) )
         return PCD_STATUS_NOK;

    if( rule->optionalParams )
    {
         free( rule->optionalParams );
    }

    rule->optionalParams = malloc( strlen( optionalParams ) + 1 );

    if( !rule->optionalParams )
         return PCD_STATUS_NOK;

    strcpy( rule->optionalParams, optionalParams );
    return PCD_STATUS_OK;
}

PCD_status_e PCD_rulesdb_clear_optional_params( rule_t *rule )
{
    if( !rule )
         return PCD_STATUS_NOK;

    if ( rule->optionalParams )
    {
         free( rule->optionalParams );
         rule->optionalParams = NULL;
    }

    return PCD_STATUS_OK;
}



