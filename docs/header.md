[intro](index.md) | [how to build](build.md) | [usage](cli.md) | [scripts](script.md) | [handle exception](except.md) | [dependency graph](depend.md) | **~~header file~~** | [system startup](startup.md) | [api](api.md)

GENERATING HEADER FILES WITH RULE NAMES
=======================================
After you understood the PCD scripts syntax, and wrote your own rules, you can now use the **pcdparser utility** to generate a header file that contains the rule names, as defined in the PCD script you wrote. This header file is very useful when calling the PCD API, and avoids run time errors which are caused by misspelling the rule names (The PCD API requires the rule name as an input parameter).

For this matter, let’s take the example system.pcd file from the dependency graph generation example:
```
#################################################################
RULE = SYSTEM_WATCHDOG
START_COND = NONE
COMMAND = /usr/sbin/watchdog -t 10 /dev/watchdog -n
SCHED = FIFO,99
DAEMON = YES
END_COND = NONE
END_COND_TIMEOUT = -1
FAILURE_ACTION = RESTART
ACTIVE=YES

#################################################################
RULE = SYSTEM_LOGGER
START_COND = NONE
COMMAND = /usr/sbin/logger –no-fork
SCHED = NICE,19
DAEMON = YES
END_COND = PROCESS_READY
END_COND_TIMEOUT = -1
FAILURE_ACTION = RESTART
ACTIVE = YES

#################################################################
RULE = SYSTEM_TIMER
START_COND = RULE_COMPLETED,SYSTEM_LOGGER
COMMAND = /usr/sbin/timer –timer-tick=100
SCHED = FIFO,1
DAEMON = YES
END_COND = PROCESS_READY
END_COND_TIMEOUT = -1
FAILURE_ACTION = REBOOT
ACTIVE = YES

#################################################################
RULE = SYSTEM_INIT
START_COND = RULE_COMPLETED,SYSTEM_TIMER
COMMAND = /usr/sbin/sys_init
SCHED = NICE,3
DAEMON = NO
END_COND = EXIT,0
END_COND_TIMEOUT = -1
FAILURE_ACTION = RESTART
ACTIVE = YES

#################################################################
RULE = SYSTEM_LASTRULE
START_COND = RULE_COMPLETED,SYSTEM_TIMER,SYSTEM_INIT,SYSTEM_LOGGER
COMMAND = NONE
SCHED = NICE,0
DAEMON = NO
END_COND = NONE
END_COND_TIMEOUT = -1
FAILURE_ACTION = NONE
ACTIVE = YES

#################################################################
```

And now, let’s generate a header file, in the name of pcd_system.h:
```
# ./pcd/src/parser/src/pcdparser -f system.pcd -o pcd_system.h

pcd: Loaded 5 rules.
pcd: Generated output file pcd_system.h.
```

The generated header file looks like that:

```
/*********************************************************
*
*   FILE:  pcd_system.h
*   PURPOSE: PCD definitions file (auto generated).
*
**********************************************************/

#ifndef _PCD_SYSTEM_H_
#define _PCD_SYSTEM_H_

#include "pcdapi.h"

/*! \def SYSTEM_PCD_GROUP_NAME
 *  \brief Define group ID string for SYSTEM
*/
#define SYSTEM_PCD_GROUP_NAME   "SYSTEM"

#define SYSTEM_PCD_RULE_WATCHDOG    "WATCHDOG"
#define SYSTEM_PCD_RULE_LOGGER  "LOGGER"
#define SYSTEM_PCD_RULE_TIMER   "TIMER"
#define SYSTEM_PCD_RULE_INIT    "INIT"
#define SYSTEM_PCD_RULE_LASTRULE    "LASTRULE"

/*! \def SYSTEM_DECLARE_PCD_RULEID()
 *  \brief Define a ruleId easily when calling PCD API
*/
#define SYSTEM_DECLARE_PCD_RULEID( ruleId, RULE_NAME ) \
    PCD_DECLARE_RULEID( ruleId, SYSTEM_PCD_GROUP_NAME, RULE_NAME )

#endif
```

As we can see in the header file:
- The group name has a dedicated define (SYSTEM_PCD_GROUP_NAME).
- Each rule that we created has a dedicated define (SYSTEM_PCD_RULE_XXX).
- A dedicated macro has been created to create a ruleId variable which is required by the PCD API (SYSTEM_DECLARE_PCD_RULEID). Use this macro to declare and initialize a ruleId variable. Send this ruleId variable as is when calling the PCD API.

It is most recommended to use the generated macro along with the required rule name definition when requesting a service from the PCD. This is the only way to ensure that there are no misspells.

Use this option in the **pcdparser** for each and every PCD script you have. Actually, each controlled group should have its own header file for best practice.

Click [here](api.md) to jump to the PCD API user guide.
