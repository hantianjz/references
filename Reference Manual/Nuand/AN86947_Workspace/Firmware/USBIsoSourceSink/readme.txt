
                        CYPRESS SEMICONDUCTOR CORPORATION
                                    FX3 SDK

USB ISO SOURCE SINK EXAMPLE
-------------------------------

  This example illustrates the use of the FX3 firmware APIs to implement
  a data source and data sink over a pair of USB Iso endpoints.

  The device enumerates as a vendor specific USB device with a pair of Iso
  endpoints (3-OUT and 3-IN). The OUT endpoint acts as data sink and the IN
  endpoint acts as data source to the PC Host.

  The source and sink is achieved with the help of a a DMA MANUAL_IN channel 
  and a DMA MANUAL_OUT channel. Any data received from the host through the 
  DMA MANUAL_IN channel is discarded. A constant data pattern is continuously
  loaded into the DMA MANUAL_OUT channel and sent to the host. 

  Files:

    * cyfx_gcc_startup.S   : Start-up code for the ARM-9 core on the FX3 device.
      This assembly source file follows the syntax for the GNU assembler.

    * cyfxisosrcsink.h     : Constant definitions for the iso source sink 
      application. The USB connection speed, numbers and properties of the 
      endpoints etc. can be selected through definitions in this file.

    * cyfxisodscr.  c      : C source file containing the USB descriptors that
      are used by this firmware example.

    * cyfxtx.c             : ThreadX RTOS wrappers and utility functions required
      by the FX3 API library.

    * cyfxisosrcsink.c     : Main C source file that implements the iso source sink
      example.

    * makefile             : GNU make compliant build script for compiling this
      example.

  PERFORMANCE OPTIMIZATIONS

  The data transfer rate obtained with this example depends on the USB host's capabilities;
  as well as on transfer parameters such as the burst size, number of bursts per service
  interval, size of DMA buffer and number of DMA buffers. The following constants defined in
  the cyfxisosrcsink.h header file can be used to modify these parameters.

  1. CY_FX_ISO_PKTS                 : Number of burst transfers per service interval.
                                      This is set to 3 by default.
  2. CY_FX_ISO_BURST                : Burst size for the endpoint in 1 KB units.
                                      This is set to 15 by default.
  3. CY_FX_ISOSRCSINK_DMA_BUF_SIZE  : Size of each DMA buffer.
                                      This is set to 45 KB by default.
  4. CY_FX_ISOSRCSINK_DMA_BUF_COUNT : Number of DMA buffers used per endpoint.
                                      This is set to 1 by default.

[]

