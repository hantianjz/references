/*
 ## Cypress USB 3.0 Platform header file (cyfxintrsrcsink.h)
 ## ===========================
 ##
 ##  Copyright Cypress Semiconductor Corporation, 2010-2011,
 ##  All Rights Reserved
 ##  UNPUBLISHED, LICENSED SOFTWARE.
 ##
 ##  CONFIDENTIAL AND PROPRIETARY INFORMATION
 ##  WHICH IS THE PROPERTY OF CYPRESS.
 ##
 ##  Use of this file is governed
 ##  by the license agreement included in the file
 ##
 ##     <install>/license/license.txt
 ##
 ##  where <install> is the Cypress software
 ##  installation root directory path.
 ##
 ## ===========================
*/

/* This file contains the constants used by the intr source sink application example */

#ifndef _INCLUDED_CYFXINTRSRCSINK_H_
#define _INCLUDED_CYFXINTRSRCSINK_H_

#include "cyu3types.h"
#include "cyu3usbconst.h"
#include "cyu3externcstart.h"

#define CY_FX_INTRSRCSINK_DMA_TX_SIZE        (0)        /* DMA transfer size is set to infinite */
#define CY_FX_INTRSRCSINK_THREAD_STACK       (0x1000)   /* Thread stack size */
#define CY_FX_INTRSRCSINK_THREAD_PRIORITY    (8)        /* Thread priority */
#define CY_FX_INTRSRCSINK_PATTERN            (0xAA)     /* 8-bit pattern to be loaded to the source buffers. */

/* Endpoint and socket definitions for the Interrupt Source Sink application */

/* To change the producer and consumer EP enter the appropriate EP numbers for the #defines.
 * In the case of IN endpoints enter EP number along with the direction bit.
 * For eg. EP 6 IN endpoint is 0x86
 *     and EP 6 OUT endpoint is 0x06.
 * To change sockets mention the appropriate socket number in the #defines. */

/* Note: For USB 2.0 the endpoints and corresponding sockets are one-to-one mapped
         i.e. EP 1 is mapped to UIB socket 1 and EP 2 to socket 2 so on */

#define CY_FX_EP_PRODUCER               0x03    /* EP 1 OUT */
#define CY_FX_EP_CONSUMER               0x83    /* EP 1 IN */

#define CY_FX_EP_PRODUCER_SOCKET        CY_U3P_UIB_SOCKET_PROD_3    /* Socket 3 is producer */
#define CY_FX_EP_CONSUMER_SOCKET        CY_U3P_UIB_SOCKET_CONS_3    /* Socket 3 is consumer */

/* Burst length in 1 KB packets. Permitted values are 1, 2 and 3. */
#ifndef CY_FX_INTR_BURST_SIZE
#define CY_FX_INTR_BURST_SIZE           (3)
#endif

/* Number of DMA buffers used per channel. */
#ifndef CY_FX_INTRSRCSINK_DMA_BUF_COUNT
#define CY_FX_INTRSRCSINK_DMA_BUF_COUNT (1)
#endif

/* Extern definitions for the USB Descriptors */
extern const uint8_t CyFxUSB20DeviceDscr[];
extern const uint8_t CyFxUSB30DeviceDscr[];
extern const uint8_t CyFxUSBDeviceQualDscr[];
extern const uint8_t CyFxUSBFSConfigDscr[];
extern const uint8_t CyFxUSBHSConfigDscr[];
extern const uint8_t CyFxUSBBOSDscr[];
extern const uint8_t CyFxUSBSSConfigDscr[];
extern const uint8_t CyFxUSBStringLangIDDscr[];
extern const uint8_t CyFxUSBManufactureDscr[];
extern const uint8_t CyFxUSBProductDscr[];

#include "cyu3externcend.h"

#endif /* _INCLUDED_CYFXINTRSRCSINK_H_ */

/*[]*/
