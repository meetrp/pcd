[introduction](index.md) | [build pcd](build.md) | **~~pcd cli~~** | [pcd scripts](script.md) | [exception handlers](except.md) | [dependency graph](depend.md) | [generate header file](header.md) | [api](api.md)

PCD COMMAND LINE PARAMETERS
===========================

## Starting PCD
The PCD should be started by the system initialization script (rcS). It is recommended that it would be the first service that the system starts. The following sections describe the command line parameters that can be used.

## Command line Parameters
The following parameters are supported by the PCD:
```
-f FILE, --file=FILE    : Specify PCD rules file.
-p, --print             : Print parsed configuration.
-v, --verbose           : Verbose display.
-t tick, --timer-tick=t : Setup timer ticks in ms (default 200ms).
-e FILE, --errlog=FILE  : Specify error log file (in nvram)
-c, --crashd            : Crash-daemon only mode (no rules file).
-d, --debug             : Debug mode
-h, --help              : Print usage screen
```

## Command line Parameters in Detail
### File
This option specifies the top level PCD script file, which contains the top level system/product rules. There is no need to specify other PCD script files. Instead, use the top level PCD script file to **include** other PCD scripts. The best practice is to define a top level script and a dedicated script per each component or sub-system. In this way, there is less dependency between the rules, and it is easier to maintain products with several flavors, where the variety of components may differ. The PCD will not start unless given at least one script file.
### Print
This option prints all the parsed rules on the console. Usually, there is no need to do that on a software version that goes to the field. The only use for it is during development and debug, where there is uncertainty about the rules integrity. A better way to verify the rules integrity and syntax is to use the pcdparser utility on the host machine. This option is for debug purpose only and should not be used on the target.
### Verbose
This option enables verbose mode, which causes the PCD to show a log per each event and failure, such as starting rule, success messages, and crash messages. It is recommended to enable this option at all times.
### Tick
This option specifies the PCD ticks. If not specified, the default tick value is 200ms. The ticks are the time units that drive the PCD logic. During the system start up, the PCD will perform the rules condition checks according to these ticks, and during the system life, the PCD will monitor all the processes in a period which is a multiplication of the tick (which results in once each 2-3 seconds). Specifying a short tick might reduce the system boot up time, but result in higher CPU consumption. Long ticks will cause the boot up time to be longer. It is recommended to use ticks in the range of 20ms – 200ms.
### Error log
The PCD can log all the system errors in a non-volatile storage for offline/post mortem analysis. This option specifies the full path of a file which will be filled with the error logs. The maximum size of the file is 4KB, and the logs are stored as a cyclic buffer. It is recommended to enable this option for field deployment.
### Crash-Daemon only mode
Do you already have a process monitor that starts and monitors the system? That’s OK, you can still use PCD as a crash-daemon that will remain idle until a crash occurs. When it occurs, it will provide and log all the useful crash information, in the most reliable and safe way. In crash-daemon only mode, there is no need for rules file or any other configuration. However, applications that require a detailed crash log must register to the [PCD exception handlers](except.md) in order to use this feature. Note that in this mode, PCD will not take any action upon a crash.
### Debug
Enables debug mode.  In normal system operation, the PCD should never terminate. In case a crash has occurred and a system reboot was requested as a recovery action, or the PCD has terminated for any reason, the following message will appear:
```
Terminating PCD, rebooting system...
```
And a system reboot will follow. Debug mode disables the “Reboot” recovery action, and does not reboot the system in case the PCD terminates, but leave it as is. This is helpful when need to debug a crash on the spot, where the developer can extract more information from the device. This option also helps when the system is not stable during the first stages of the development and will prevent the system from rebooting continuously in case of a fatal error exists. It is recommended to keep this option enabled during the development stages and remove it for field deployment.

## Other information
- The PCD will reboot the system in case it is terminated for any reason (unless it is in debug mode).
- Only one instance of PCD can run in the system. The PCD will not permit more than one instance.

