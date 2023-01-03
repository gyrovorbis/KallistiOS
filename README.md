# KallistiOS


KOS (pronounced "chaos") is an unofficial development environment for the SEGA Dreamcast game console with some support for the NAOMI and NAOMI 2 arcade boards.  

KOS was developed from scratch over the internet by a group of free software developers, and has no relation to the official Katana or WinCE development kits for the DC. This has allowed it to fuel a thriving Dreamcast homebrew scene, powering many commercial releases for the platform over the years. It supports a signficiant portion of the Dreamcast's hardware capabilities and a wide variety of peripherals, accessories, and add-ons for the console, including custom hardware modifications that have been created by the scene. 

Despite the console's age, KOS offers an extremely modern, programmer-friendly development platform, supporting C17 and C++17, with the majority of their standard libraries fully supported and additional support for many POSIX APIs. Additionally, KOS-ports offers a rich set of add-on libraries such as SDL, OpenGL, and Lua for the platform. 

## Features
##### Core Functionality
* Concurrency with Kernel Threads, C11 Threads, PThreads
* Virtual filesystem abstraction 
* IPv4/IPv6 Network stack
* Dynamically loading libraries/modules
* GDB Debug stubs

##### Dreamcast Hardware Support
* GD-Rom driver
* Low-level 3D PowerVR Graphics 
* SH4 ASM-Optimized Math Routines
* SH4 SCIF Serial I/O
* DMA Controller 
* Flashrom Access 
* AICA SPU Sound Processor Driver
* Cache and Store Queue Management
* Timer Peripherals and Real-Time Clock
* MMU Management API 
* BIOS Font Rendering

##### Peripherals and Accessory Support
* Visual Memory Unit
* Puru Puru Vibration Pack
* Seaman Microphone
* Dreameye Webcam
* Lightgun 
* Keyboard
* Mouse
* Dial-up Modem
* Broadband Adapter
* LAN Adapter
* VGA Adapter
* SD Card Reader

##### Hardware Modification Support
* IDE Hard Drive
* 32MB RAM Upgrade
* Custom BIOS Flashroms

## Getting Started 
A beginner's guide to development for the Sega Dreamcast along with detailed instructions for installing KOS and the required toolchains can be found on the  [Dreamcast Wiki](https://dreamcast.wiki/Getting_Started_with_Dreamcast_development). Additional documentation can be found in the docs folder. Once you've set up the environment and are ready to begin developing, a good place to start learning is the examples directory, which provides demos for interacting with the various KOS APIs and aspects of the Dreamcast's hardware. 

## Toolchains
Build scripts have been provided for the following toolchains which are used when building the environment and developing for the platform. More information as well as scripts for automatically building them can be found within utils/dc-chain.
* Dreamcast SH4 CPU
  * sh-elf GCC 9.3.0
  * sh-elf GCC 4.2.0
* AICA SPU ARM7 GPU
  *	arm-eabi GCC 8.4.0
  * arm-eabi GCC 4.7.4

## Resources
[DCEmulation Forums](http://dcemulation.org/phpBB/viewforum.php?f=29): Goldmine of Dreamcast development information and history  
[Dreamcast Wiki](http://dreamcast.wiki): Large collection of tutorials and articles for beginners  
[Simulant Discord Chat](https://discord.gg/bpDZHT78PA): Home to the official Discord channel of KOS  
irc.libera.chat #dreamcastdev 

