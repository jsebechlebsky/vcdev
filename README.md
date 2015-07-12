Virtual V4L2 Linux Kernel driver
================================

Author: Jan Sebechlebsky <sebecjan@fit.cvut.cz>

This module implements simple virtual camera device with raw input from proc file.

The driver is highly experimental, use it at your own risk.

## Building module
After running `make` in root directory of cloned repository, the `bin` folder should be created containing two files:
* vcmod.ko - Kernel module binary
* vcctrl   - Minimalistic utility to configure virtual camera devices

## Usage

The module can be loaded to kernel by runnning
```
insmod vcmod.ko
```
as superuser.

By default two device nodes will be created in `/dev`:
* videoX - V4L2 device
* vcdev - Control device for virtual cameras used by control utility

In `/proc` folder `fbX` file will be created.

The device if initialy configured to process 640x480 RGB24 image format.
By writing 640x480 RGB24 raw frame data to `/proc/fbX` file the resulting video stream will appear on corresponding `/dev/videoX` V4L2 device.

 Run `vcctrl --help` for more information about how to configure,add or remove virtual camera devices.
