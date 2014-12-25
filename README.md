G60 Linux support
=================

This program replaces <code>g60.ko</code> shipped with the Fujitsu G60 SDK.
<code>g60.ko</code> is a simple character device driver that proxies USB
bulk transfers.  We can do the same thing in userland with libusb, avoiding
the need for kernel dependencies and all of the complications they cause.

g60cuse uses CUSE to create a fully functional <code>/dev/g60</code> node,
so that the existing Fujitsu software (F\_G60DP2.exe and F\_G60DP1.so) can
be used without modifications.

Compiling
---------

Prerequisites:

 * gcc/make/etc.
 * libusb 1.0 (will not work with the 0.1 API)
 * libfuse
 * autotools, git (for building from GitHub)

This was originally tested on Fedora 13, as that is the distribution targeted
by the G60 SDK.  The long term goal is to be able to use the G60 with any
reasonably modern x86 Linux distribution.  On Fedora use:

    yum groupinstall "Development Tools"
    yum install libusb1-devel fuse-devel autoconf automake git-all

On Debian/Ubuntu use:

    apt-get install build-essential libfuse-dev libusb-1.0-0-dev autoconf automake pkg-config git

Build commands:

    git clone git://github.com/cernekee/g60
    cd g60
    ./autogen.sh
    ./configure
    make

Usage
-----

This starts g60cuse in the foreground, with USB tracing enabled:

    sudo -s
    killall -9 F_G60DP2.exe g60cuse
    rmmod g60
    ./g60cuse -f --trace

Then run the G60 sample programs or applications.

If it cannot find the G60 device, run <code>lsusb</code> (from the
<code>usbutils</code> package) and ensure that you see a line that looks
like:

    Bus 002 Device 007: ID 04c5:124a Fujitsu, Ltd

In production you probably want to run the G60 software as an unprivileged
user; instructions can be found at the top of <code>55-g60-cuse.rules</code>.

Credits
-------

Copyright 2014 Kevin Cernekee &lt;cernekee@gmail.com&gt;

License: GPLv2

This program was derived from the CUSE example program, cusexmp.c, shipped
with FUSE.  Original copyright notices are at the top of the file.
