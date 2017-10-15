PCD - Process Control Daemon
============================

| Documentation | Examples |

# What is PCD?
PCD (**Process Control Daemon**), is an open source, light-weight, system level process manager for Embedded Linux based products (Consumer electronics, network devices, and more). With PCD in control, it is guaranteed that the system’s boot up time will be reduced, and its reliability, availability and robustness will be enhanced.

# Why do you need PCD in your Embedded Linux based product?
With PCD driving your product, you will gain:
- Enhanced control and monitoring on all the processes in the system.
- Simple, yet powerfull means to configure each process and service in the system:
	- When to start each process, configure its dependencies.
	- What resource or condition to check, per each process, in order to verify its successful startup.
	- What recovery action to take when a process fails to start or crashes during its work.
- Reduced system startup time
	- Rules are started as soon as their start condition is satisfied.
	- No need for non-deterministic delays. Dependencies are well defined and enforced.
	- Rules without inter-dependency are started in parallel.
- Enhanced debug capabilities of faulty processes
	- Exception handlers and logs provide useful and detailed information about process crashes, such as the process name and id, exception description, date and time, fault address, register dump, map file and more.
	- Crash log storage in non-volatile memory for post-mortem/offline analysis.
- Improvement in system stability and robustness.
- Generation of a graphic representation of the system startup sequence for further analysis.

