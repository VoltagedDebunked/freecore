============================
FreeCore Kernel Architecture
============================

========
Overview
========
FreeCore is a modular operating system kernel designed to support multiple CPU architectures. The kernel is written in C with some assembly components where necessary for architecture-specific operations.

===================
Directory Structure
===================
- ``kernel/src/core/``: Core kernel functionality
- ``kernel/src/lib/``: Library functions and utilities
- ``kernel/src/drivers/``: Device drivers
- ``kernel/src/arch/``: Architecture-specific code

=================
Kernel Components
=================

----
Core
----
- Initialization and main entry point
- Memory management
- Process scheduling
- Interrupt handling

-------
Drivers
-------
- Input drivers (keyboard)
- Character device drivers (serial)
- Block device drivers
- Network interface drivers

---------------------
Architecture-specific
---------------------
- Boot code
- CPU initialization (GDT, IDT, TSS on x86)
- Architecture-specific memory management

============
Boot Process
============
1. Limine bootloader loads the kernel
2. Architecture-specific initialization (GDT, IDT setup on x86)
3. Main kernel initialization
4. Driver initialization
5. Shell or init process startup

==================
Interrupt Handling
==================
The kernel uses a modular interrupt handling system that routes hardware and software interrupts to appropriate handlers.

=================
Memory Management
=================
The kernel implements virtual memory management with paging for memory protection and isolation.
