PCD - Process Control Daemon
=============

PCD (**Process Control Daemon**), is an open source, light-weight, system level process manager for Embedded Linux based products (Consumer electronics, network devices, and more). With PCD in control, it is guaranteed that the system’s boot up time will be reduced, and its reliability, availability and robustness will be enhanced.

## Why do you need PCD in your Embedded Linux based product?
With PCD driving your product, you will gain:
- Enhanced control and monitoring on all the processes in the system.
- Simple, yet powerful means to configure each process and service in the system:
- Reduced system startup time
- Enhanced debug capabilities of faulty processes
- Improvement in system stability and robustness.
- Generation of a graphic representation of the system startup sequence for further analysis.

## Some Background
The PCD was originally designed and developed by [Hai Shalom](https://il.linkedin.com/in/haishalom) as a proprietary solution for Texas Instruments. During the last stages of the development, the PCD project was released by TI as an open source software and the project’s community is in process of growing. New programmers which find the PCD useful and interesting project are starting to contribute to the project with their ideas and innovations.

## Supported Architectures
The PCD is a user space application, therefore its exposure to the actual architecture is limited. However, supported platforms have an enhanced and detailed crash dump which displays all registers, thus allowing an easier debug.

The PCD fully supports the following architectures:
- ARM
- MIPS
- x86
- x64

Non-supported architectures should also work, but without the enhanced exception details.
If PCD does not support your platform, it is possible to support it if you’ll send the hardware and BSP.

## Already have a process controller in your product?
You can still use the PCD in “**crash-daemon only**” mode. In this mode, the PCD will not start your system nor monitor its processes. Instead, it will remain idle until one of your application crashes, and when that happens, the PCD will generate and log the detailed crash dump which could help you isolate and fix the bug.

## PCD Licensing
- The PCD project is licensed under the GNU Lesser General Public License version 2.1 (**LGPLv2.1**), as published by the Free Software Foundation.
- To view a copy of this license, visit [http://www.gnu.org/licenses/lgpl-2.1.html#SEC1](http://www.gnu.org/licenses/lgpl-2.1.html#SEC1)
- There are no purchase costs or royalty fees.
- This license allows commercial use without the need to expose proprietary source code. Any enhancements or features that are added to the PCD must be submitted back to the community.

## PCD Resources
All the relevant documents are available at [PCD Documentation](https://meetrp.github.io/pcd/)

**Specific Help Topics**
- [Introduction to PCD](https://meetrp.github.io/pcd/index.md)
- [Configure & Build the PCD](https://meetrp.github.io/pcd/build.md)
- [PCD Command Line Paramters](https://meetrp.github.io/pcd/cli.md)
- [Writing PCD Scripts](https://meetrp.github.io/pcd/script.md)
- [Registering to the PCD exception handlers](https://meetrp.github.io/pcd/except.md)
- [Generating Dependency Graphs](https://meetrp.github.io/pcd/depend.md)
- [Generating Header Files with Rule Names](https://meetrp.github.io/pcd/header.md)
- [Tight Control on your System Startup](https://meetrp.github.io/pcd/startup.md)
- [Using the PCD API](https://meetrp.github.io/pcd/api.md)

## Contributing
### Bug Reports & Feature Request
Please use the [issue tracker](https://github.com/meetrp/pcd/issues) to report bugs or feature requests.

### Development
Always welcome. To begin developing, do this:
```
$> git clone git@github.com:meetrp/pcd.git
```

## What is my role?
I use PCD. I needed a fix to make it work on x64. Intially I was planning to send the patch fix to be merged to the orginal. That is when I noticed the original [sourceforce page](http://sourceforge.net/projects/pcd/) was not being maintained (*or, at least, it appeared so!*). So, I decided to import the source files and maintain it in my free time.

## What is my hope?
As a community, if we can maintain PCD it would be great. This is a very useful process monitoring tool & this should not die.
