[intro](index.md) | [how to build](build.md) | [usage](cli.md) | **~~scripts~~** | [handle exception](except.md) | [dependency graph](depend.md) | [header file](header.md) | [system startup](startup.md) | [api](api.md)

WRITING PCD SCRIPTS
===================

The PCD script, or Rule file is the actual input to the PCD. The script file is a human readable text file, and it is composed of “Rule Blocks”. Each Rule block defines which process to start, when to start it, in which priority to run it, what recovery action to take when it fails, and more. A rule block is associated with a single process. The Rule Blocks have a uniform structure and composed of a set of commands, according to the following syntax, where lines that start with a # mark are remark lines which the PCD ignores.

```shell
################################################################# 
# Index of the rule RULE = <GROUP>_<DESCRIPTION>[$] 
# Condition to start rule, existence of one of the following 
START_COND = {NONE | FILE,[filename] | RULE_COMPLETED,[rule],..
 | NET_DEVICE,[netdev] | IPC_OWNER,[owner] | ENV_VAR,[variable,value]}

# Command with parameters
 COMMAND = <Full path>   [parameters] [$variable] 

# Scheduling (priority) of the process 
SCHED = {NICE,-19..19 | FIFO,1..99} 

# Daemon flag - Process must not end 
DAEMON = {YES | NO} 

# Condition to end rule and move to next rule, wait for: 
END_COND = {NONE | FILE,[filename] | NET_DEVICE,[netdevice]
 | WAIT,[delay] | EXIT,[status] |   IPC_OWNER,[owner]} 

# Timeout for end condition. Fail if timeout expires 
END_COND_TIMEOUT = {-1 | 0..99999} 

# Action upon failure, do one of the following actions upon failure 
FAILURE_ACTION = {NONE | REBOOT | RESTART | EXEC_RULE} 

# Active rule, start automatically or manually 
ACTIVE = {YES | NO} 

# User id for the process
USER = { UID | User name }
################################################################
```
Each Rule must contain these commands, otherwise, it’s a syntax error.

## Rule commands in detail
##### RULE
The Rule name. The name is composed of two parts, the group name and rule description with an underscore sign between them (for example, SYSTEM_LOGGER).

##### START_COND
Defines the start condition. Describes what is the condition that will make the PCD logic decide to start the process associated to this rule. The following start conditions are supported:
- NONE – No start condition, application is spawn immediately
- FILE filename – The existence of a file
- RULE_COMPLETED id – Rule id completed successfully
- NETDEVICE netdev – The existence of a networking device
- IPC_OWNER owner – The existence of an IPC destination point
- ENV_VAR name,value – Value of a variable

##### COMMAND
Describes the process name (full path) and an optional list of arguments. It is possible to specify a variable name with a $ sign, and the PCD will fetch the argument list from the contents of the file in PCD_TEMP_PATH/variable (for example, given the command: “COMMAND = /usr/sbin/logger $myvars”, the PCD will start /usr/sbin/logger with the list of arguments that are written in the file PCD_TEMP_PATH/myvars, where PCD_TEMP_PATH is defined in pcd.h). The command can be NONE. In this case, no process is actually spawned, and this rule is referred as a “Synchronization Rule”. Such rules can be used as a means to mark group events. For example, suppose there are two groups of rules, where one needs to be started only after the first has finished. The last rule of the first group can be defined as a synchronization rule which depends on all the group’s rules completion. The second group’s first rule can depend on this synchronization rule, and in this way, the second group will only be started after the first has finished to initialize.

##### SCHED
Defines the process scheduling policy and priority. Normal processes should be used with NICE scheduling, and high priority processes should be used with FIFO scheduling. The NICE values vary from 20 (lowest) to -19 (highest), and the FIFO values vary from 1 (lowest) to 99 (highest).

##### DAEMON
Set this flag to YES when the associated process is a daemon, and should never terminate. In case a daemon terminates, the PCD treats this even as a failure and triggers a recovery action.

##### END_CONDITION
Defines the end condition. Describes what is the condition that will make the PCD logic to decide that the Rule has completed successfully, and to start the next depended rules. The following end conditions are supported:
- NONE – No end condition
- FILE filename – The existence of a file
- EXIT status – The application exited with status. Other statuses are considered failure
- PROCESS_READY – The process sent a READY event though PCD API.
- WAIT msecs – Delay, ignore END_COND_TIMEOUT
- NET_DEVICE netdev – The existence of a networking device
- IPC_OWNER owner – The existence of an IPC destination point

##### END_COND_TIMEOUT
Defines the maximum amount of milliseconds the PCD waits for the end condition completion. In case a rule fails to complete within the configured timeout, the PCD triggers a recovery action. In case no timeout is required, use -1.

##### FAILURE_ACTION
Defines which failure/recovery action to trigger in case a rule fails or the associated process crashes. The following Failure actions are supported:
- NONE – Take no action
- REBOOT - Reboot the system
- RESTART – Restart the rule
- EXEC_RULE id - Execute a rule

##### ACTIVE
Defines whether the rule is active or passive. In case it is active, the PCD will automatically activate it, as soon as its start condition is satisfied. Passive rules are started manually by the applications, using the PCD API. This option is useful when a process needs to be started as a result of an event that only the application is aware of. In this case, use a Passive rule, and let the PCD to start it for you instead of using fork/exec.

##### USER (Optional)
Defines the user id to start the process. In embedded system, the PCD runs usually as root. Some processes must not have root privileges. Specify either required  UID or user name, which will be converted to UID in run-time.

The PCD supports a special Passive Rule format, which allows to use a single pseudo rule to start multiple copies of the same processes. If a $ sign is specified in the end of the Rule name, the PCD can be instructed to start this rule as many times as required, where it will replace the $ sign with an index. Each copy of the process can be started with different parameters. An example for this option could be a system that has 3 DHCP clients for different networks. The same Rule could be used to activate all three. This is done using the PCD API.

## Notes and warnings
- There should not be a rule with a NONE start condition, unless this is the first rule that runs in the system, or the rule is Passive, and started manually by an application (using the PCD API). All Active Rules with NONE start condition will be started in parallel as soon as the PCD finishes the parsing of the rules!
- There should not be a rule with a NONE end condition, unless no other rules depend on the successful initialization of this specific rule.
- There should not be a rule with a NONE failure action, because your system will remain unstable without any recovery action. Unless you really don’t care if a process crashes, select one of the other failure actions.

