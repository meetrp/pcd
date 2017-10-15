[intro](index.md) | [how to build](build.md) | [usage](cli.md) | [scripts](script.md) | [handle exception](except.md) | [dependency graph](depend.md) | [header file](header.md) | [system startup](startup.md) | **~~api~~**

Using the PCD API
=================
The PCD provides additional API for every application in order to request services from the PCD, or retrieve information. The services that the PCD provides encapsulate the actual process name and id (pid might vary). The controlling application only needs to know the rule name in order to request a service.

When requesting a rule-related service from the PCD, the rule name needs to be provided. In order to facilitate this service and avoid run-time errors that can be caused by misspelled rule names, the [pcdparser](header.md) provides a means to generate a header file which contains all the rule names, according to the PCD scripts. When calling the PCD API, use the generated defines.
Requirements

In order to use the PCD API, apply the following steps:
- Include **pcdapi.h** in the required source file.
- Include the **pcdparser** generated header file with rule names (not mandatory, but recommended).
- Link your application with pcdapi library (add **-lpcd -lipc**, in some platforms **-lrt**, to your **LDFLAGS**).
- Rule ID declaration (only for rule related actions).

## Declare a Rule ID
##### PCD_DECLARE_RULEID( ruleId, GROUP_NAME, RULE_NAME )
The Rule ID is an identifier which is required in order to perform rule related actions (start, terminate, signal, etc.). Prior to such calls, it is recommended to use the mentioned macro which will generate the Rule ID from the provided group name and rule name. Note that the names are strings, and should be inside `”` signs.

Here is an example that shows how to use the [example generated header](header.md) to manually start a process:
```
int start_timer_process( void )
{
    /* Declare a rule ID variable for the timer process */
    SYSTEM_DECLARE_PCD_RULEID( ruleId, SYSTEM_PCD_RULE_TIMER );

    /* Start the timer process manually */
    if( PCD_api_start_process( &ruleId, NULL ) != 0 )
    {
        fprintf( stderr, "Failed to start a new process!\n" );
        return -1;
    }

    return 0;
}
```

## Start a Process
##### STATUS PCD_api_start_process( const struct ruleId_t *ruleId, const Char *optionalParams );
The PCD provides API to activate a given rule, which will trigger a new process spawn. This API is used by applications that need to start **passive** (i.e. non-active) rules. Passive rules, are rules that the PCD does not start automatically, probably because their start condition requires some logic which is known only to the application, or because they need to be started only in some cases and not always. In this case, the application can use the start process API when it determines that its the right time.

The start process API is a better alternative than the fork/exec method.  Processes that are spawned privately using fork and exec are not monitored by the PCD and won’t be recovered in case of a crash. Instead of forking, define a rule and let PCD to spawn and monitor it for you. There are many advantages for replacing fork/execs with this API; Monitored processes can be started in the appropriate priority, can be  recovered in case of a crash, and crashes are detailed and logged.

The start process API allows you to send custom parameters to the process. In other words, the parameter list could be dynamic and not hard-coded from the rule. If the application does not specify any parameters, the list will be taken from the rule. Examples for such a scenario: A configuration application that starts a DHCP server on the box only if it was enabled by the user. A Telnet server that needs to spawn a new instance per each incoming connection.

The PCD also allows to define “one-to-many rule”, where one rule is used for starting multiple processes. Rule name that ends with a dollar sign ($) marks such a rule.

This API is non-blocking. It means that the application sends this request and continues to run immediately, before the required process has actually started to run.

## Terminate a process
##### STATUS PCD_api_terminate_process( const struct ruleId_t *ruleId );
##### STATUS PCD_api_terminate_process_non_blocking( const struct ruleId_t *ruleId );
The PCD provides API to terminate a running process gracefully. This action instructs the PCD to send a termination signal (SIGTERM) to the corresponding process, which triggers its termination. In case the process has a signal handler for SIGTERM it will be activated.

In case a daemon process is required to be terminated, this API is the only valid way to do that because normally, the PCD will not allow the termination of processes which are marked as daemons in their rule.

In case a process refuses to terminate gracefully, the PCD will initiate a kill signal (SIGKILL) to the non-responsive process after 10 seconds, thus forcing it to terminate.

There are two flavors of this function. A non-blocking version and a blocking version. There are cases where an application just needs to terminate another process and continue to work as usual, and in other cases, it needs to wait until the confirmation of the process termination arrives.

## Kill a process
##### STATUS PCD_api_kill_process( const struct ruleId_t *ruleId );
The PCD provides API to kill a running process brutally. This action instructs the PCD to send a kill signal (SIGKILL) to the corresponding process, which triggers its immediate termination. This action does not trigger the process’s termination handler function. Use this option with caution because it could cause data loss or corruption.

## Send a “process ready” event
##### STATUS PCD_api_send_process_ready( void );
This option is used by applications to send an event to the PCD during the system startup sequence. This event tells the PCD that the application has finished its initialization and the next depended rule(s) can be started.  This API is very useful and documented in detail in here.

## Signal a process
##### STATUS PCD_api_signal_process( const struct ruleId_t *ruleId, Int32 sig );
The PCD provides API to send User Signals (SIGUSR1 and SIGUSR2) to a process, where the sending application does not have to know what is the process id of the receiver which may vary, but only the rule name. In this way, applications can send signals to each other while knowing only the rule names which stay constant. The PCD does not allow to send other signals which might cause the receiver to terminate.

## Register to PCD default exception handlers
##### STATUS PCD_api_register_exception_handlers( Char *name, Cleanup_func cleanup );
##### PCD_API_REGISTER_EXCEPTION_HANDLERS()
One of the PCD’s strong features is its exception handlers. Once an application registers to the PCD’s exception handlers, the PCD will be able to provide useful debug information in case of a crash. This information can help the programmers to find the root cause more quickly.

It is possible to specify a cleanup callback function that the PCD will invoke in the crashed process upon a crash event. If no cleanup function is required, it is possible to use the simplified version of the API using the macro specified under the function, from the program’s main function.

This API is very useful and documented in detail [here](except.md).

## Get rule state
##### STATUS PCD_api_get_rule_state( const struct ruleId_t *ruleId, pcdApiRuleState_e *ruleState );
The PCD provides API to query a given rule’s state. This API is used by applications that want to know what is the current state of the rule, and act upon it. For example, an application can request the PCD to start a rule, and then query and wait until the process is running.

The following states represent the states that a rule can be in:
- PCD_API_RULE_IDLE: Rule is idle, never been run.
- PCD_API_RULE_RUNNING: Rule is running; waiting for start or end condition.
- PCD_API_RULE_COMPLETED_PROCESS_RUNNING: Rule completed successfully, process is running (daemon).
- PCD_API_RULE_COMPLETED_PROCESS_EXITED: Rule completed successfully, process exited.
- PCD_API_RULE_NOT_COMPLETED: Rule failed due to timeout, failure in end condition.
- PCD_API_RULE_FAILED: Rule failed due to process unexpected failure.

## Find another instance of a process
##### pid_t PCD_api_find_process_id( Char *name );
The PCD provides API to find another instance of the started process. This is a general purpose function and it is also used by the PCD to make sure there is only one instance of it running.

## Reboot the system (with a given reason)
##### void PCD_api_reboot( const Char *reason, Bool force );
##### PCD_API_REBOOT()
##### PCD_API_REBOOT_ALWAYS()
The PCD provides API to reboot the system with a given reason. There are cases where an application decides that it has encountered a fatal error and needs to reboot the system. In complex systems, it is not always clear who initiated the reboot. When using this API, the PCD allows the application to send a “reason for reboot” string,  and will display it as well as the process name and id before it reboots the system. The simplified macro versions display a standard  message with the requesting function and line. Note that when the PCD is in debug mode, it will not perform a system reboot and it will allow the user to examine the system. However, if the program is using the reboot always macro, the PCD will reboot the system in any case.

