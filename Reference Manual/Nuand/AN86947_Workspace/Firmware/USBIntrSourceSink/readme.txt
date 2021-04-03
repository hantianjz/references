
                        CYPRESS SEMICONDUCTOR CORPORATION
                                    FX3 SDK

USB INTR SOURCE SINK EXAMPLE
-------------------------------

  This example illustrates the use of the FX3 firmware APIs to implement
  a data source and data sink over a pair of USB intr endpoints.

  The device enumerates as a vendor specific USB device with a pair of intr
  endpoints (3-OUT and 3-IN). The OUT endpoint acts as data sink and the IN
  endpoint acts as data source to the PC Host.

  The source and sink is achieved with the help of a a DMA MANUAL_IN channel 
  and a DMA MANUAL_OUT channel. Any data received from the host through the 
  DMA MANUAL_IN channel is discarded. A constant data pattern is continuously
  loaded into the DMA MANUAL_OUT channel and sent to the host. 

  Files:

    * cyfx_gcc_startup.S   : Start-up code for the ARM-9 core on the FX3 device.
      This assembly source file follows the syntax for the GNU assembler.

    * cyfxintrsrcsink.h     : Constant definitions for the intr source sink 
      application. The USB connection speed, numbers and properties of the 
      endpoints etc. can be selected through definitions in this file.

    * cyfxintrdscr.  c      : C source file containing the USB descriptors that
      are used by this firmware example.

    * cyfxtx.c             : ThreadX RTOS wrappers and utility functions required
      by the FX3 API library.

    * cyfxintrsrcsink.c     : Main C source file that implements the intr source sink
      example.

    * makefile             : GNU make compliant build script for compiling this
      example.

  PERFORMANCE OPTIMIZATIONS

  Interrupt transfers are low performance transfers in general. The actual performance
  obtained in this case will depend primarily on the Burst size used for the endpoint.
  This can be defined using the CY_FX_INTR_BURST_SIZE constant in the cyfxintrsrcsink.h
  header file. This constant can take the values from 1 to 3, as per the USB 3.0
  specification.

[]

