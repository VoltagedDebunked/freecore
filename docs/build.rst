=================
Building FreeCore
=================

============
Prerequisites
============
- GCC or Clang compiler
- NASM assembler (for x86 architecture)
- Make build system
- xorriso for ISO creation

==============
Build Commands
==============
- ``make``: Build the kernel for the default architecture (x86_64)
- ``make ARCH=aarch64``: Build for a specific architecture
- ``make run``: Build and run the kernel in QEMU
- ``make clean``: Clean build artifacts
- ``make distclean``: Remove all generated files

===================
Build Configuration
===================
You can customize the build by setting environment variables:

- ``ARCH``: Target architecture (x86_64, aarch64, riscv64, loongarch64)
- ``QEMUFLAGS``: Additional flags to pass to QEMU

=======
Example
=======

.. code-block:: bash

   # Build for x86_64 with 4GB of RAM in QEMU
   make ARCH=x86_64 QEMUFLAGS="-m 4G"

=================
Cross-Compilation
=================
To cross-compile for a different architecture, ensure you have the appropriate cross-compiler installed and set the CC environment variable:

.. code-block:: bash

   make ARCH=aarch64 CC=aarch64-linux-gnu-gcc
