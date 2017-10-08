/*
 * parser.c
 * Description:
 * PCD rules files parser implementation file
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
 * Change log:
 * - Nick Stay, nlstay@gmail.com, Added optional USER field so that processes 
 *   can be executed as an arbitrary user.
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
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#include <errno.h>

#include <sys/ioctl.h>
#include <unistd.h>
#include <fcntl.h>
#include <pwd.h>
#include <sys/utsname.h>
#include "system_types.h"
#include "rules_db.h"
#include "ruleid.h"
#include "condchk.h"
#include "schedtype.h"
#include "parser.h"
#include "pcd.h"

/**************************************************************************/
/*      LOCAL DEFINITIONS AND VARIABLES                                   */
/**************************************************************************/

#define SET_HANDLER_FUNC(x)   PCD_parser_handle_##x
#define STR(x)   #x
#define XSTR(x)  STR(x)

typedef struct configKeywordHandler_t
{
    char      *name;
    int32_t     (*handler)(char *line);
    u_int32_t    parse_flag;        /* set at run time. */
    u_int32_t    mandatory_flag;    /* indicate if this is a mandatory field. */

} configKeywordHandler_t;

/**************************************************************************
 * Declarations for the keyword handlers.
 **************************************************************************/
#define PCD_PARSER_KEYWORD( keyword, mandatory )\
    static int32_t SET_HANDLER_FUNC( keyword ) ( char *line );

PCD_PARSER_KEYWORDS

#undef PCD_PARSER_KEYWORD

/**************************************************************************
 * Initialize keyword array
 **************************************************************************/
#define PCD_PARSER_KEYWORD( keyword, mandatory ) { XSTR( keyword ), SET_HANDLER_FUNC( keyword ), 0, mandatory },

configKeywordHandler_t keywordHandlersList[] =
{
    PCD_PARSER_KEYWORDS
    { NULL,       NULL,          0, 0},
};

#undef PCD_PARSER_KEYWORD

/**************************************************************************
 * Keyword enumeration
 **************************************************************************/
#define SET_KEYWORD_ENUM(x)   PCD_PARSER_KEYWORD_##x

#define PCD_PARSER_KEYWORD( keyword, mandatory ) SET_KEYWORD_ENUM( keyword ),

typedef enum parserKeywords_e
{
    PCD_PARSER_KEYWORDS

} parserKeywords_e;

#undef PCD_PARSER_KEYWORD

#define PCD_START_COND_KEYWORD( keyword ) \
    XSTR( keyword ),

static const char *startCondKeywords[] =
{
    PCD_START_COND_KEYWORDS
    NULL
};

#undef PCD_START_COND_KEYWORD

#define PCD_END_COND_KEYWORD( keyword ) \
    XSTR( keyword ),

static const char *endCondKeywords[] =
{
    PCD_END_COND_KEYWORDS
    NULL
};

#undef PCD_END_COND_KEYWORD

#define PCD_FAILURE_ACTION_KEYWORD( keyword ) \
    XSTR( keyword ),

static const char *failureActionKeywords[] =
{
    PCD_FAILURE_ACTION_KEYWORDS
    NULL
};

#undef PCD_FAILURE_ACTION_KEYWORD

#define PCD_PARSER_DELIMITERS     ", \t"
#define PCD_PARSER_MAX_LINE_SIZE    256

/**************************************************************************
 * Global definitions
 **************************************************************************/
static rule_t     rule;
static int32_t      fileVersion = -1;
static u_int32_t     readParseStatus = 0;   /* Did we read all the neccessary fields to populate rule? */
static u_int32_t     writableParseStatus = 0;   /* Must have fields to populate the rule. */
static u_int32_t     lineNumber = 0;   /* The line number of input file which we are reading. */
static u_int32_t     verbose = 0;    /* Show rules after parsing */
static u_int32_t     totalRuleRecords = 0;   /* The number of records written into the database. */

static void PCD_parser_dump_config( rule_t *rule );

/**************************************************************************/
/*      IMPLEMENTATION                                                    */
/**************************************************************************/

/**************************************************************************
 * Parser core functions
 **************************************************************************/
static void PCD_parser_dump_config( rule_t *rule )
{
    printf( "-------------------------------------------\n" );
    printf( "Rule ID: %s_%s\n", rule->ruleId.groupName, rule->ruleId.ruleName );
    printf( "Start condition: %s\n", startCondKeywords[ rule->startCondition.type ] );
    printf( "Command: %s\n", rule->command );
    printf( "Parameters: %s\n", rule->params ? rule->params : "None" );
    printf( "End condition: %s\n", endCondKeywords[ rule->endCondition.type ] );
    printf( "Timeout: ");
    if ( rule->timeout == ~0 )
        printf( "None\n" );
    else
        printf( "%dms\n", (u_int32_t)rule->timeout);
    printf( "Scheduling: %s (%d)\n", rule->sched.type == PCD_SCHED_TYPE_FIFO ? "FIFO" : "NICE", rule->sched.niceSched );
    printf( "Daemon: %s\n", rule->daemon ? "YES":"NO"  );
    printf( "Active: %s\n", rule->ruleState == PCD_RULE_ACTIVE ? "YES":"NO"  );
    printf( "User id: " );
    if( rule->uid )
        printf( "%d\n", rule->uid );
    else
        printf( "Same as pcd\n" );
}

static int32_t PCD_parser_is_parse_status_set( parserKeywords_e kwId )
{
    configKeywordHandler_t *kwPtr = &keywordHandlersList[kwId];

    PCD_FUNC_ENTER_PRINT

    if ( !kwPtr->name )
        return( 0 );

    PCD_DEBUG_PRINTF( "%s: readParseStatus=0x%x, kwPtr->parse_flag=0x%x, return %d", __FUNCTION__, readParseStatus, kwPtr->parse_flag, ( (readParseStatus & kwPtr->parse_flag)? 1:0) );

    return( (readParseStatus & kwPtr->parse_flag)? 1:0);
}

static int32_t PCD_parser_update_parse_status( parserKeywords_e kwId )
{
    configKeywordHandler_t *kwPtr = &keywordHandlersList[0];

    PCD_FUNC_ENTER_PRINT

    while ( kwPtr->name )
    {
        if ( !(readParseStatus & kwPtr->parse_flag) )
            break;

        kwPtr++;
    }

    if ( !kwPtr->name )
        return( -1 );

    PCD_DEBUG_PRINTF( "%s: kwPtr->name=%s", __FUNCTION__, kwPtr->name );

    if ( strcmp( kwPtr->name, keywordHandlersList[ kwId ].name ) )
    {
        while ( 1 )
        {
            PCD_DEBUG_PRINTF( "%s: Inside while, kwPtr->name=%s, kwPtr->mandatory_flag=%d", __FUNCTION__, kwPtr->name, kwPtr->mandatory_flag );

            if ( kwPtr->mandatory_flag )
            {
                PCD_PRINTF_STDERR( "Missing input: expected \"%s\" but found \"%s\" at line# %d",
                                   kwPtr->name, keywordHandlersList[ kwId ].name, lineNumber );
                break;
            }

            kwPtr++;
            if ( !kwPtr->name )
                kwPtr = &keywordHandlersList[0];
        }

        return( -1 );
    }

    readParseStatus |= kwPtr->parse_flag;
    PCD_DEBUG_PRINTF( "%s: readParseStatus=0x%x, kwPtr->parse_flag=0x%x", __FUNCTION__, readParseStatus, kwPtr->parse_flag );

    return( 0 );
}

static int32_t PCD_parser_clear_parse_status( parserKeywords_e kwId )
{
    configKeywordHandler_t *kwPtr = &keywordHandlersList[kwId];
    int ret_val = -1;

    PCD_FUNC_ENTER_PRINT

    while ( kwPtr->name )
    {
        ret_val = 0;
        readParseStatus &= ~(kwPtr->parse_flag);
        kwPtr++;
    }

    return( ret_val );
}

static int32_t PCD_parser_add_rule(rule_t *rule)
{
    PCD_FUNC_ENTER_PRINT

    if ( PCD_rulesdb_add_rule( rule ) == PCD_STATUS_OK )
    {
        if ( verbose )
            PCD_parser_dump_config( rule );

        totalRuleRecords++;
    }
    else
    {
        PCD_PRINTF_STDERR( "Failed to add rule to database" );
        return -1;
    }

    return( 0 );
}

static int32_t PCD_parser_write_rule_to_db( parserKeywords_e kwId )
{
    PCD_FUNC_ENTER_PRINT

    if ( readParseStatus != writableParseStatus )
        return( -1 );

    if ( PCD_parser_add_rule( &rule ) )
        return( -1 );

    PCD_parser_clear_parse_status( kwId );

    return( 0 );
}

static int32_t PCD_parser_print_error(configKeywordHandler_t *kwPtr)
{
    PCD_FUNC_ENTER_PRINT

    PCD_PRINTF_STDERR( "Handling the keyword %s", kwPtr->name);
    return 0;
}

#ifdef PCD_HOST_BUILD
extern char hostPrefix[ 128 ];
#endif

/**************************************************************************
 * File readers and initializers.
 **************************************************************************/
static int32_t PCD_parser_read_config( const char *filename, bool_t toplevel )
{
    FILE *in;
    char buffer[PCD_PARSER_MAX_LINE_SIZE], orig[PCD_PARSER_MAX_LINE_SIZE], *token, *line;
    int32_t i, ret_val = 0;
    configKeywordHandler_t *kwPtr;
#ifdef PCD_HOST_BUILD
    {
        char hostFilename[ 255 ];

        PCD_FUNC_ENTER_PRINT

        if ( toplevel == False )
        {
            sprintf( hostFilename, "%s/%s", hostPrefix, filename );
        }
        else
        {
            strcpy( hostFilename, filename );
        }

        if ( !(in = fopen( hostFilename, "r" )) )
        {
            PCD_PRINTF_STDERR( "Unable to open configuration file %s", hostFilename );
            return -1;
        }
    }
#else
    PCD_FUNC_ENTER_PRINT

    if ( !(in = fopen( filename, "r" )) )
    {
        PCD_PRINTF_STDERR( "Unable to open configuration file %s", filename);
        return -1;
    }
#endif

    while ( fgets( buffer, PCD_PARSER_MAX_LINE_SIZE, in ) )
    {
        lineNumber++;

        if ( strchr( buffer, '\n' ) )
            *(strchr( buffer, '\n' )) = '\0';

        strncpy( orig, buffer, PCD_PARSER_MAX_LINE_SIZE );

        if ( strchr( buffer, '#' ) )
            *(strchr( buffer, '#' )) = '\0';

        token = buffer + strspn( buffer, " \t" );

        if ( *token == '\0' )
            continue;

        line = token + strcspn( token, " \t=" );

        if ( *line == '\0' )
            continue;

        *line = '\0';
        line++;

        /* eat leading whitespace */
        line = line + strspn( line, " \t=" );

        /* eat trailing whitespace */
        for ( i = strlen( line ) ; i > 0 && isspace( line[i-1] ); i-- );
        line[i] = '\0';

        for ( kwPtr = &keywordHandlersList[0] ; kwPtr->name ; kwPtr++ )
        {
            if ( !strcasecmp( token, kwPtr->name ) )
            {
                if ( kwPtr->handler( line ) )
                {
                    PCD_PRINTF_STDERR( "Unable to parse %s", line );
                    PCD_parser_print_error( kwPtr );
                    ret_val = -1;
                    goto read_config_exit;
                }
            }
        }
    }

    read_config_exit:

    /* Flush out the outstanding entry. */
    if ( PCD_parser_is_parse_status_set( PCD_PARSER_KEYWORD_RULE ) )
    {
        if ( PCD_parser_write_rule_to_db( PCD_PARSER_KEYWORD_RULE ) )
        {
            PCD_PRINTF_STDERR( "Input file did not have complete information, premature termination" );
            ret_val = -1;
        }
    }

    fclose( in );

    return( ret_val );
}

static int32_t PCD_parser_generate_config( const char *filename )
{
    int ret_val = -1;

    PCD_FUNC_ENTER_PRINT

    if ( PCD_parser_read_config( filename, True ) )
    {
        PCD_PRINTF_STDERR( "Reading the input configuration" );
        goto generate_config_exit;
    }

    ret_val = 0;

    generate_config_exit:

    if ( !ret_val )
        PCD_PRINTF_STDOUT( "Loaded %d rules", totalRuleRecords );

    return( ret_val );
}

static int32_t PCD_parser_init_kwHandlersList(void)
{
    int index;
    configKeywordHandler_t *kwHandlersListPtr = &keywordHandlersList[0];
    writableParseStatus = 0;

    PCD_FUNC_ENTER_PRINT

    for ( kwHandlersListPtr = &keywordHandlersList[0], index = 0;
        kwHandlersListPtr->name;
        kwHandlersListPtr++, index++ )
    {
        if ( kwHandlersListPtr->mandatory_flag )
        {
            kwHandlersListPtr->parse_flag   = (1 << index);
            writableParseStatus |= kwHandlersListPtr->parse_flag;
        }

        PCD_DEBUG_PRINTF("%s: name: %s, parse_flag=0x%x, mandatory=%d", __FUNCTION__, kwHandlersListPtr->name, kwHandlersListPtr->parse_flag, kwHandlersListPtr->mandatory_flag );
    }

    memset( &rule, 0, sizeof( rule_t ) );

    return 0;
}

/**************************************************************************
 * Implementation of the handlers.
 **************************************************************************/
static int32_t PCD_parser_parse_rule_id( ruleId_t *ruleId, char *line )
{
    char *token1, *token2;
    u_int32_t cp;

    PCD_FUNC_ENTER_PRINT

    /* Clear rule id */
    memset( ruleId, 0, sizeof( ruleId_t ) );

    token1 = line;
    token2 = strchr( line, '_' );

    if ( ( !token1 ) || ( !token2 ) )
    {
        PCD_PRINTF_STDERR( "parsing rule missing id" );
        return -1;
    }

    /* Calculate how many bytes to copy */
    cp = token2 - token1;
    token2++;

    if ( cp > PCD_RULEID_MAX_GROUP_NAME_SIZE - 1 )
    {
        cp = PCD_RULEID_MAX_GROUP_NAME_SIZE - 1;
    }

    strncpy( ruleId->groupName, token1, cp );
    strncpy( ruleId->ruleName, token2, PCD_RULEID_MAX_RULE_NAME_SIZE - 1 );

    return 0;
}


static int32_t PCD_parser_handle_VERSION( char *line )
{
    PCD_FUNC_ENTER_PRINT

    /* Update file version */
    fileVersion = atoi( line );

    return 0;
}

static int32_t PCD_parser_handle_ACTIVE( char *line )
{
    PCD_FUNC_ENTER_PRINT

    if ( PCD_parser_update_parse_status( PCD_PARSER_KEYWORD_ACTIVE ) )
        return(-1);

    if ( strcmp( line, "YES" ) == 0 )
        rule.ruleState = PCD_RULE_ACTIVE;
    else
        rule.ruleState = PCD_RULE_IDLE;

    return 0;
}

static int32_t PCD_parser_handle_INCLUDE( char *line )
{
    u_int32_t    local_read_parse_status = readParseStatus;   /* Did we read all the neccessary fields to populate rule? */
    u_int32_t    local_line_num = lineNumber;   /* The line number of input file which we are reading. */
    rule_t    local_rule = rule;

    PCD_FUNC_ENTER_PRINT

    /* After saving the current values, clear them and call read config function again */
    memset( &rule, 0, sizeof( rule_t ) );
    lineNumber = 0;
    readParseStatus = 0;

    /* Parse the include file */
    PCD_parser_read_config( line, False );

    /* Restore values */
    readParseStatus = local_read_parse_status;
    lineNumber = local_line_num;
    rule = local_rule;

    return 0;
}

static int32_t PCD_parser_handle_RULE( char *line )
{
    PCD_status_e retval;

    PCD_FUNC_ENTER_PRINT

    if ( PCD_parser_is_parse_status_set( PCD_PARSER_KEYWORD_RULE ) )
    {
        if ( PCD_parser_write_rule_to_db( PCD_PARSER_KEYWORD_RULE ) )
        {
            return( -1 );
        }
        else
        {
            memset( &rule, 0, sizeof( rule_t ) );

        }
    }

    if ( PCD_parser_update_parse_status( PCD_PARSER_KEYWORD_RULE ) )
        return(-1);

    retval = PCD_parser_parse_rule_id( &rule.ruleId, line );

    if ( retval == PCD_STATUS_OK )
    {
        char *ptr;

        if ( ( ptr = strchr( rule.ruleId.ruleName, '$' ) ) != NULL )
        {
            *ptr = '\0';
            rule.indexed = True;
        }
    }

    return retval;
}

static int32_t PCD_parser_handle_START_COND( char *line )
{
    char *token1, *token2;
    u_int32_t i = 0;

    PCD_FUNC_ENTER_PRINT

    token1 = strtok(line, PCD_PARSER_DELIMITERS);

    if ( PCD_parser_update_parse_status( PCD_PARSER_KEYWORD_START_COND ) )
        return(-1);

    if ( !token1 )
    {
        PCD_PRINTF_STDERR( "Invalid start condition keyword");
        return -1;
    }

    while ( startCondKeywords[ i ] != NULL )
    {
        if ( strcmp( startCondKeywords[ i ], token1 ) == 0 )
        {
            rule.startCondition.type = i;
            break;
        }
        i++;
    }

    if ( startCondKeywords[ i ] == NULL )
    {
        PCD_PRINTF_STDERR( "Invalid start condition keyword");
        return -1;
    }

    rule.startCondition.type = i;

    if ( i == PCD_START_COND_KEYWORD_NONE )
        return 0;

    /* Special care here because we don't need token2, but we find tokens in a loop */
    if ( i == PCD_START_COND_KEYWORD_RULE_COMPLETED )
    {
        u_int32_t j = 0;
        char *token;
        char tempToken[ PCD_RULEID_MAX_GROUP_NAME_SIZE+PCD_RULEID_MAX_RULE_NAME_SIZE+2 ];

        /* Clear the structure */
        memset( rule.startCondition.ruleCompleted, 0, sizeof( ruleCache_t ) * PCD_START_COND_MAX_IDS );

        /* Parse all rules */
        while ( ( j < PCD_START_COND_MAX_IDS ) && ( ( token = strtok(NULL, PCD_PARSER_DELIMITERS) ) != NULL ) )
        {
            memset( tempToken, 0, sizeof( tempToken ) );
            strncpy( tempToken, token, sizeof( tempToken ) - 1 );

            if ( PCD_parser_parse_rule_id( &rule.startCondition.ruleCompleted[ j ].ruleId, tempToken ) != PCD_STATUS_OK )
            {
                return -1;
            }

            PCD_DEBUG_PRINTF( "Parsed rule %s_%s, index %d", rule.startCondition.ruleCompleted[ j ].ruleId.groupName, rule.startCondition.ruleCompleted[ j ].ruleId.ruleName, j );
            j++;
        }

        /* We did not get any rule */
        if ( j == 0 )
        {
            PCD_PRINTF_STDERR( "Invalid or missing start condition token for %s", startCondKeywords[ j ] );
            return -1;
        }

        return 0;
    }

    /* Get the second token */
    token2 = strtok(NULL, PCD_PARSER_DELIMITERS);

    if ( !token2 )
    {
        PCD_PRINTF_STDERR( "Invalid or missing start condition token for %s", startCondKeywords[ i ] );
        return -1;
    }

    switch ( i )
    {
        case PCD_START_COND_KEYWORD_FILE:
			memset( rule.startCondition.filename, 0, sizeof( rule.startCondition.filename ) );
            strncpy( rule.startCondition.filename, token2, PCD_COND_MAX_SIZE - 1 );
            break;
        case PCD_START_COND_KEYWORD_NETDEVICE:
		    memset( rule.startCondition.netDevice, 0, sizeof( rule.startCondition.netDevice ) );
            strncpy( rule.startCondition.netDevice, token2, IF_NAMESIZE - 1 );
            break;
        case PCD_START_COND_KEYWORD_IPC_OWNER:
            rule.startCondition.ipcOwner = atoi( token2 );
            break;
        case PCD_START_COND_KEYWORD_ENV_VAR:
            {
                char *token3;

                token3 = strtok(NULL, PCD_PARSER_DELIMITERS);
                if ( !token3 )
                {
                    PCD_PRINTF_STDERR( "Invalid or missing start condition token for %s", startCondKeywords[ i ] );
                    return -1;
                }

                strncpy( rule.startCondition.envVar.envVarName, token2, PCD_COND_MAX_SIZE );
                strncpy( rule.startCondition.envVar.envVarValue, token3, PCD_COND_MAX_SIZE );
            }
            break;
        default:
            break;
    }

    return 0;
}

static int32_t PCD_parser_handle_COMMAND( char *line )
{
    char *params;

    PCD_FUNC_ENTER_PRINT

    if ( PCD_parser_update_parse_status( PCD_PARSER_KEYWORD_COMMAND ) )
        return(-1);

    /* Find the first space, seperator from command to parameters */
    params = strchr( line, ' ' );

    if ( params )
    {
        *params = '\0';
        params++;

        if ( ( rule.params = malloc( strlen( params ) + 1 ) ) == NULL )
        {
            PCD_PRINTF_STDERR( "Memory allocation error" );
            return -1;
        }

        /* Copy parameters */
        strcpy( rule.params, params );
    }

    if ( ( rule.command = malloc( strlen( line ) + 1 ) ) == NULL )
    {
        PCD_PRINTF_STDERR( "Memory allocation error" );
        return -1;
    }

    /* Copy command */
    strcpy( rule.command, line );
    return 0;
}

static int32_t PCD_parser_handle_SCHED( char *line )
{
    char *token1, *token2;
    int32_t value;

    PCD_FUNC_ENTER_PRINT

    token1 = strtok(line, PCD_PARSER_DELIMITERS);
    token2 = strtok(NULL, PCD_PARSER_DELIMITERS);

    if ( ( !token1 ) || ( !token2 ) )
    {
        PCD_PRINTF_STDERR( "missing scheduling keyword");
        return -1;
    }

    value = atoi( token2 );

    if ( strcmp( token1, "NICE" ) == 0 )
    {
        if ( ( value > 19 ) || ( value < -20 ) )
        {
            PCD_PRINTF_STDERR( "warning, invalid NICE value %d, setting to 0", value );
            value = 0;
        }
        rule.sched.type = PCD_SCHED_TYPE_NICE;
        rule.sched.niceSched = value;
    }
    else if ( strcmp( token1, "FIFO" ) == 0 )
    {
        if ( ( value > 99 ) || ( value < 0 ) )
        {
            PCD_PRINTF_STDERR( "warning, invalid FIFO value %d, setting to 0", value );
            value = 0;
        }

        rule.sched.type = PCD_SCHED_TYPE_FIFO;
        rule.sched.fifoSched = value;
    }
    else
    {
        PCD_PRINTF_STDERR( "invalid scheduling keyword");
        return -1;
    }

    return 0;
}

static int32_t PCD_parser_handle_DAEMON( char *line )
{
    PCD_FUNC_ENTER_PRINT

    if ( strcmp( line, "YES" ) == 0 )
        rule.daemon = True;
    else
        rule.daemon = False;

    return 0;
}

static int32_t PCD_parser_handle_USER( char *line )
{
    PCD_FUNC_ENTER_PRINT

    /* Check if the value is a string or number. If it is a string, then 
     * we'll attempt to convert it to the correct UID */
    char *linecopy = line;
    int isnum = 1;
    for( ; *linecopy != '\0'; ++linecopy )
    {
        if( isalpha( *linecopy ) )
        {
            isnum = 0;
            break;
        }
    }
    if ( isnum )
    {
        /* USER is a number, assume it is a direct UID */
        errno = 0;
        rule.uid = strtoul( line, (char**)NULL, 0 );
        if ( errno != 0 )
        {
             PCD_PRINTF_STDERR( "USER numeric value is invalid for rule = %s",
                               rule.ruleId.ruleName );
        }
    }
    else
    {
        /* USER is a login name, attempt to convert it to the UID */
        struct passwd* pw = getpwnam( line );
        if( pw )
        {
            rule.uid = pw->pw_uid;
        }
        else
        {
#ifdef PCD_HOST_BUILD
            /* On host, warn user that UID cannot be determined, but allow 
             * parser to continue (since this user should exist on target) */
            PCD_PRINTF_WARNING_STDOUT( "Cannot determine UID from USER field for rule = %s",
                                       rule.ruleId.ruleName );
#else
            /* On target, this user must exist or it is an error */
            PCD_PRINTF_STDERR( "Cannot determine UID from USER field for rule = %s",
                               rule.ruleId.ruleName );
            return -1;
#endif
        }
    }
    
    return 0;
}

static int32_t PCD_parser_handle_END_COND( char *line )
{
    char *token1, *token2;
    u_int32_t i = 0;

    PCD_FUNC_ENTER_PRINT

    token1 = strtok(line, PCD_PARSER_DELIMITERS);
    token2 = strtok(NULL, PCD_PARSER_DELIMITERS);

    if ( PCD_parser_update_parse_status( PCD_PARSER_KEYWORD_END_COND ) )
        return(-1);

    if ( !token1 )
    {
        PCD_PRINTF_STDERR( "invalid end condition keyword");
        return -1;
    }

    while ( endCondKeywords[ i ] != NULL )
    {
        if ( strcmp( endCondKeywords[ i ], token1 ) == 0 )
        {
            rule.endCondition.type = i;
            break;
        }
        i++;
    }

    if ( endCondKeywords[ i ] == NULL )
    {
        PCD_PRINTF_STDERR( "invalid end condition keyword");
        return -1;
    }

    rule.endCondition.type = i;

    if ( ( i == PCD_END_COND_KEYWORD_NONE ) || ( i == PCD_END_COND_KEYWORD_PROCESS_READY ) )
        return 0;

    if ( !token2 )
    {
        PCD_PRINTF_STDERR( "invalid or missing end condition token for %s", endCondKeywords[ i ] );
        return -1;
    }

    switch ( i )
    {
        case PCD_END_COND_KEYWORD_FILE:
            memset( rule.endCondition.filename, 0, sizeof( rule.endCondition.filename ) );
			strncpy( rule.endCondition.filename, token2, PCD_COND_MAX_SIZE - 1 );
            break;
        case PCD_END_COND_KEYWORD_NETDEVICE:
		    memset( rule.endCondition.netDevice, 0, sizeof( rule.endCondition.netDevice ) );
            strncpy( rule.endCondition.netDevice, token2, IF_NAMESIZE - 1 );
            break;
        case PCD_END_COND_KEYWORD_IPC_OWNER:
            rule.endCondition.ipcOwner = atoi( token2 );
            break;
        case PCD_END_COND_KEYWORD_EXIT:
            rule.endCondition.exitStatus = atoi( token2 );
            break;
        case PCD_END_COND_KEYWORD_WAIT:
            rule.endCondition.delay[0] = rule.endCondition.delay[1] = atoi( token2 );
            break;
        default:
            break;
    }

    return 0;
}

static int32_t PCD_parser_handle_END_COND_TIMEOUT( char *line )
{
    int32_t i = atoi( line );

    PCD_FUNC_ENTER_PRINT

    if ( PCD_parser_update_parse_status( PCD_PARSER_KEYWORD_END_COND_TIMEOUT ) )
        return(-1);

    if ( ( i == -1 ) || ( rule.endCondition.type == PCD_END_COND_KEYWORD_WAIT ) )
    {
        rule.timeout = ~0;
    }
    else
    {
        rule.timeout = i;
    }

    return 0;
}

static int32_t PCD_parser_handle_FAILURE_ACTION( char *line )
{
    char *token1, *token2;
    u_int32_t i = 0;

    PCD_FUNC_ENTER_PRINT

    token1 = strtok(line, PCD_PARSER_DELIMITERS);
    token2 = strtok(NULL, PCD_PARSER_DELIMITERS);

    if ( PCD_parser_update_parse_status( PCD_PARSER_KEYWORD_FAILURE_ACTION ) )
        return(-1);

    if ( !token1 )
    {
        PCD_PRINTF_STDERR( "invalid failure condition keyword");
        return -1;
    }

    while ( failureActionKeywords[ i ] != NULL )
    {
        if ( strcmp( failureActionKeywords[ i ], token1 ) == 0 )
        {
            rule.failureAction.action = i;
            break;
        }
        i++;
    }

    if ( failureActionKeywords[ i ] == NULL )
    {
        PCD_PRINTF_STDERR( "invalid failure condition keyword");
        return -1;
    }

    if ( i == PCD_FAILURE_ACTION_KEYWORD_EXEC_RULE )
    {
        if ( !token2 )
        {
            PCD_PRINTF_STDERR( "missing failure condition keyword");
            return -1;
        }

        return PCD_parser_parse_rule_id( &rule.failureAction.ruleId, token2 );
    }

    return 0;
}

PCD_status_e PCD_parser_parse( const char *filename )
{
    PCD_FUNC_ENTER_PRINT

    if ( PCD_parser_init_kwHandlersList( ) )
        return( -1 );

    if ( PCD_parser_generate_config( filename ) )
    {
        PCD_PRINTF_STDERR( "Error in generating configuration" );
        return PCD_STATUS_NOK;
    }

    return PCD_STATUS_OK;
}

PCD_status_e PCD_parser_enable_verbose( bool_t enable )
{
    if ( enable == True )
    {
        verbose = 1;
    }
    else
    {
        verbose = 0;
    }

    return PCD_STATUS_OK;
}
