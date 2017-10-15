[intro](index.md) | [how to build](build.md) | [usage](cli.md) | [scripts](script.md) | [handle exception](except.md) | [dependency graph](depend.md) | [header file](header.md) | **~~system startup~~** | [api](api.md)

TIGHT CONTROL ON YOUR SYSTEM STARTUP
===================================
The system startup process with PCD is event driven. Unlike starting the system with scripts in a non-deterministic manner, the PCD provides you tools to synchronize your startup sequence in the most deterministic and efficient way. In order to utilize this feature, you need to configure the **start** and **end** conditions of each rule correctly. It is guaranteed, that if you’ll configure the conditions correctly, your system startup time will be reduced.

When you specify the proper start condition, it is guaranteed that the rule will be started only after this condition has been satisfied. The associated process will be started in the correct time. However, the PCD’s strength is the ability to monitor the **end** condition. By defining the correct end condition, it is guaranteed that depended processes will not be started until the end condition is satisfied.  This feature is extremely important when a process initializes a service or a resource, and there are other processes which consume or use it. They will fail if they are started before the service or resource is truly available.

The PCD can be configured to check various standard end conditions, as mentioned [here](script.md), but it also provides a dedicated means to send a “process ready event“. This event, which is sent by the monitored process, tells the PCD that it has completed all its initialization and it is now available and ready to provide service. Sending this event from your application is the most efficient and effective means of synchronization. You, as the author of the code, know best when your application is ready. In that particular spot, all you need to do is to send the event back to the PCD, which will trigger the activation of the next depended rules.

## How do I use this feature in my application?
In order to send a process ready event in one of your source files, use the following steps:
- Create your rule, and use **PROCESS_READY** as your end condition.
- Include **pcdapi.h** in the required source file.
- Use the **PCD_api_send_process_ready()** function to send the event right after your application has finished the last init function call. This function is a simple void function, and there is no need for any parameters.
- Link your application with libpcdapi.so library (add **-lpcdapi** to your LDFLAGS).
- Create rules for depended applications, and use **RULE_COMPLETED**,<your rule name> as their start condition.

The steps mentioned above will guarantee that your application and its depended applications will be started in the most deterministic and efficient way.

