# Introductory guide

Code base on github: https://github.com/PeppeRu96/freertos-fault-injector  
All the other materials/resources can be found in the Dropbox shared folder RTOS_Project

# General

*#TODO things which are not dependent on the host operating system*

Since FreeRTOS has been designed to be cross-compiled for different platforms (hardware/simulator operating systems..) in a way that clearly distinguishes the code folders
which are common to all platforms from the code folders which are platform-specific, we must try to develop the code in a platform transparent way.
Indeed, we have to build:
- A FreeRTOS Simulator which runs a configurable set of different tasks;
- A FaultInjector program which is in charge of hooking to the FreeRTOS Simulator process in order to allow for fault injections and tracking features (C or C++?)

We have two (three) different target platforms, Windows, Linux, (Mac) for our two programs to build. Therefore, we have to build the platform-specific code for the two (maybe three)
different targets, clearly distinguishing the platform-specific code from the common code.
As a result, a good directory structure could be as follows:
- For the FreeRTOS Simulator:
	- The official FreeRTOS directory structure which already splits portable code and non-portable code;
	- Add to the already existing directory the platform-specific code, indeed, one for windows, one for linux/osx (to be understand if it is required to port code differently for osx);
- For the FaultInjector program (C or C++?):
	- A single project code-base with:
		- Common code (all the logic for the fault injection system)
		- Platform-specific code:
			- Of course, we have to develop a console-application for two/three different operating systems.  
			  As a consequence, each operating system requires a different compiler and different libraries in order to compile and build a console application.  
			  	- For linux, maybe it is wiser to compile the console application with the same type of compiler used to compile FreeRTOS Simulator (gcc).  
			  	- For the Mac, it is very likely that the code for the linux os (which is a simple console application) will run on the Mac too, so no need for a different code. (This may be not the case because the processes might be managed differently under OSX)
			  	- For Windows, maybe it is better to stick with the same compiler used for compiling the FreeRTOS Simulator, for simplicity and for having the same underlying C standard library compiled in the same way. (MSVC)

Having separated the portable-code and non-portable code either in the FreeRTOS Simulator or in the FaultInjector program allows us to build two unique projects for the three different operating systems we are supposed to build the programs for.  
### Example usage
For example, if in our file we require a platform-specific function and so we have to include a header file, we simply include it by its name (which will be the same for all operating systems).  
`#include "somefeature.h"`  
After that, in some Portable folder inside our directories we will have:
 - One *"somefeature.h"* inside the windows folder;
 - One *"somefeature.h"* inside the unix folder;  

Finally, we have three different **configuration settings** in our IDEs (suggested CLion (with GCC) for Linux/Osx building and Visual Studio 2019 community (with MSVC) for Windows building):
 - One configuration for Windows (of course there will be only this configuration in the Visual Studio IDE..) which will target Windows and so it must include only the files under Portable/Windows ... ignoring the others;
 - One configuration for Linux (of course there will be this and the one for osx in CLion on Linux..) which will target Linux and so it must include only the files under Portable/Unix .. ignoring the others;
 - One configuration for OSX which will target OSX and so it must include only the files under Portable/Unix (we may have some file which requires to be implemented differently for Linux and OSX, so we adopt the following strategy).
For the Linux and OSX difference, since there will be very small changes to adopt for OSX, we may use the following strategy.
#### Linux/OSX specialization
 - Only for the files which require a different implementation for OSX, we have a standard file which is used as a wrapper (which will be directly included from other code). The wrapper simply checks a define (kind of #define OSX 0/1) and include, according to the define:
	+ A specific file for Linux (e.g. *some_feature_linux.h*) if the define OSX is set to 0;
	+ A specific file for OSX (e.g. *some_feature_osx.h*) if the define OSX is set to 1.
 - The other files which work well either for Linux or OSX in the same way and do not require any change, we simply use them normally.

### IPC - InterProcess Communication
In order to build a FaultInjector program, we must be able to access the FreeRTOS Simulator underlying memory; thus, it becomes vital to define a good IPC strategy.  
A good reference to understand and learn IPC methods can be found on the book *Operating Systems Concepts - Tenth edition on chapters 3, 4, 6, 7, 13*, where there can be found a lot of IPC strategies along with synchronization techniques needed to overcome the synch problems arised by accessing the same portion of memory from two processes concurrently as we can imagine.
Up to now, the most promising IPC approach seems to be the so called **Shared Memory**.
#### Shared Memory
In short words, a simple but effective technique may be to share all the FreeRTOS Simulator memory used between the simulator and the fault injector.  
Although it may seem simple at a first look, it is much harder that one can think. There are some obvious problems related to technical concerns.
- Multi-platform problem  
  Unix-based operating systems can use POSIX compliant shared memory management, which in theory is working on MacOS too. However, there are significant implementation differences made by MacOS, which leads to the need of making two different implementations for Linux and MacOS;  
  Windows, of course, uses completely different libraries to make available the sharing memory model.  
  	- One straight-forward solution could be using BOOST library, but it is built for C++ only.. and the FreeRTOS is written in C.. -> May it be possible to switch and build a FreeRTOS Simulator in C++?
	- Another solution to avoid the complexity of managing three different implementations could be to use a portable library in C which offers a portable shared memory model implementation. -> #TODO: find a good portable library which offers that.

The portable library solution seems to be the quickest solution to avoid wasting time in implementing low-level details of the host operating system.
  


## Windows

Initial setup
  1) Follow this beginner guide https://www.freertos.org/simple-freertos-demos.html  
     **Note: All the code and the downloaded distribution coming from the guide on the link is needed just for experimenting and understanding.
            The code for the process (goal of the project) of the Simulator FreeRTOS, which has to be built, will be on github.**
  2) Suggested IDE: Visual Studio 2019 Community (pack for c++ development)
  3) Understand how the source code is organized (very simple directory organization) https://www.freertos.org/a00017.html
  
    FreeRTOS
        |
        +-Source        The core FreeRTOS kernel files
            |
            +-include   The core FreeRTOS kernel header files
            |
            +-Portable  Processor specific code (we are interested only in the folders MSVC-MingW and MemMang, all other folders may be deleted or just ignored)
                |
                +-MSVC-MingW    All the code specific for windows and in particular compiled with MSVC or MingW
                +-MemMang       The sample heap implementations (the reason why the memory is hardware-dependent is at https://www.freertos.org/a00111.html)
        +-Demo		Contains the demo application project (the only useful folders are WIN32-* and Common (others may be deleted or just ignored)
            |
            +-WIN32-MSVC-Static-Allocation-Only		Demo applications for Windows (32 bit) and compiled with MSVC, this variant allocates only statically and not dynamically (Usable with VS2019)
            +-WIN32-MSVC				Demo applications for Windows (32 bit) and compiled with MSVC (Usable with VS2019)
            +-WIN32-MingW				Demo applications for Windows (32 bit) and compiled with MingW (Usable with Eclipse, do not use other IDEs because they need to be configured)
            +-Common					The demo application files that are used by all the demos regardless of the platform
            
  4) Simply build and run the Blinky demo following the guide and read the comments on the code to understand how it is working. (The code is very well commented)

*#TODO: understand what is needed to explore in the demo (probably the memory management first) and list them here*

*#TODO: understand how a building toolchain can be made on Windows (with VisualStudio), can we use CMake? How can we build a configuration system, which should be as simple as possible to use, for the tasks to be executed? (building time configuration, not runtime)*


## Linux

There are two Linux simulators which seem to be kept "updated" with the new versions of the official FreeRTOS (official github).
Therefore, we may have to choose which Linux FreeRTOS simulator implementation to use for our project, Savino suggests in the official pdf two repositories:

1) FreeRTOS-Sim		https://github.com/megakilo/FreeRTOS-Sim/tree/master/Source  
	It seems to be very basic and provide the kernel functions along with the portable file implementations for a POSIX based operating system (Linux / OSX) (They must be compiled with GCC)

2) FreeRTOS-Emulator	https://github.com/alxhoff/FreeRTOS-Emulator  
	It seems very complex (even though it is much more documented) and provide, according to github description:  
		*"POSIX based FreeRTOS emulator with SDL2 graphics interface and multiple async communications interfaces,
		aiming to make it possible to teach FreeRTOS without embedded hardware using similar processes."*  
	So, it is capable of drawing things with graphics while emulating FreeRTOS, it may be interesting knowing why Savino has put this repository in the suggested ones.  
	Are we supposed to build something with graphics? Probably no.  
	Maybe Savino suggested this because we have to reuse some ideas or interfaces to build our "fault injector process"? -> We need to understand what this emulator is actually thought for..  

 
*#TODO: understand how a building toolchain can be made on Linux (with CLion, GCC and CMake)? How can we build a configuration system, which should be as simple as possible to use, for the tasks to be executed? (building time configuration, not runtime)*

# Building
The project is fully based on CMake as it makes cross-platform compiling easy.
## Linux
Recommended IDE: CLion
### Quick guide
You can use CLion or do it manually.
#### Through CLion
- Open CLion on the root folder of the project
- It automatically detects CMakeLists.txt
- Select the target
- Build/Play
- If you want to modify some options, open cmake-build-debug/CMakeCache.txt
#### Manual cmake
- go to the root folder
- `cmake -S . -B build`
- `cd build`
- `make <target-name>`

### Debug under Linux
In order to debug under Linux (with CLion + gdb), you have to follow these steps:
- Modify or create .gdbinit file in the home of the current user *~/.gdbinit*
- Write the following code:  
`set auto-load local-gdbinit on`  
`add-auto-load-safe-path [full path to the project root]/.gdbinit`

## Windows
Recommended IDE: Visual Studio Community 2019
### Quick guide
- Open Visual Studio on the root folder of the project
- It automatically detects CMakeLists.txt and CMakeSettings.json and generate everything automatically
- Open the main CMakeLists.txt
- Play after having selected a target
- You can modify CMake options and targets directly opening CMakeSettings
- Use WIN32-Debug-Blinky-Demo to use the blinky demo
- Use WIN32-Debug-Full-Demo to use the full demo


# Useful links
- https://www.freertos.org/FreeRTOS-simulator-for-Linux.html
- https://www.freertos.org/FreeRTOS-Windows-Simulator-Emulator-for-Visual-Studio-and-Eclipse-MingW.html
- https://github.com/megakilo/FreeRTOS-Sim
- https://github.com/alxhoff/FreeRTOS-Emulator
- [Read/Write memory under Windows and Linux](https://nullprogram.com/blog/2016/09/03/)
- https://man7.org/linux/man-pages/man2/process_vm_readv.2.html - Read and write memory under Linux
- https://www.youtube.com/watch?v=8XpVsb44YHA - Read-Write into another process memory (Windows)
