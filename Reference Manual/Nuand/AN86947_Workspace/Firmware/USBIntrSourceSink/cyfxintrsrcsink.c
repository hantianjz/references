/*
 ## Cypress USB 3.0 Platform source file (cyfxintrsrcsink.c)
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

/* This file illustrates the Intr source sink Application example using the DMA MANUAL_IN
 * and DMA MANUAL_OUT mode */

/*
   This example illustrates USB endpoint data source and data sink mechanism. The example
   comprises of vendor class USB enumeration descriptors with two Intr endpoints. An Intr OUT
   endpoint acts as the producer of data and acts as the sink to the host. An intr IN endpoint
   acts as the consumer of data and acts as the source to the host.

   The data source and sink is achieved with the help of a DMA MANUAL_IN channel and a DMA
   MANUAL_OUT channel. A DMA MANUAL_IN channel is created between the producer USB intr endpoint
   and the CPU. A DMA MANUAL_OUT channel is created between the CPU and the consumer USB intr
   endpoint. Data is received in the IN channel DMA buffer from the host through the producer 
   endpoint. CPU is signalled of the data reception using DMA callbacks. The CPU discards this
   buffer. This leads to the sink mechanism.

   A constant patern data is loaded onto the OUT channel DMA buffer whenever the buffer is available.
   CPU issues commit of the DMA data transfer to the consumer endpoint which then gets transferred 
   to the host. This leads to a constant source mechanism.

   The DMA buffer size is defined based on the USB speed. 64 for full speed, 1024 for high speed
   and 1024 for super speed. CY_FX_INTRSRCSINK_DMA_BUF_COUNT in the header file defines the number
   of DMA buffers.

   For performance optimizations refer the readme.txt.
 */

#include "cyu3system.h"
#include "cyu3os.h"
#include "cyu3dma.h"
#include "cyu3error.h"
#include "cyfxintrsrcsink.h"
#include "cyu3usb.h"
#include "cyu3uart.h"

CyU3PThread     IntrSrcSinkAppThread;	 /* Intr loop application thread structure */
CyU3PDmaChannel glChHandleIntrSink;       /* DMA MANUAL_IN channel handle */
CyU3PDmaChannel glChHandleIntrSrc;        /* DMA MANUAL_OUT channel handle */

CyBool_t glIsApplnActive = CyFalse;      /* Whether the loopback application is active or not. */
uint32_t glDMARxCount = 0;               /* Counter to track the number of buffers received. */
uint32_t glDMATxCount = 0;               /* Counter to track the number of buffers transmitted. */

/* Application Error Handler */
void
CyFxAppErrorHandler (
        CyU3PReturnStatus_t apiRetStatus    /* API return status */
        )
{
    /* Application failed with the error code apiRetStatus */

    /* Add custom debug or recovery actions here */

    /* Loop Indefinitely */
    for (;;)
    {
        /* Thread sleep : 100 ms */
        CyU3PThreadSleep (100);
    }
}

/* This function initializes the debug module. The debug prints
 * are routed to the UART and can be seen using a UART console
 * running at 115200 baud rate. */
void
CyFxIntrSrcSinkApplnDebugInit (void)
{
    CyU3PUartConfig_t uartConfig;
    CyU3PReturnStatus_t apiRetStatus = CY_U3P_SUCCESS;

    /* Initialize the UART for printing debug messages */
    apiRetStatus = CyU3PUartInit();
    if (apiRetStatus != CY_U3P_SUCCESS)
    {
        /* Error handling */
        CyFxAppErrorHandler(apiRetStatus);
    }

    /* Set UART configuration */
    CyU3PMemSet ((uint8_t *)&uartConfig, 0, sizeof (uartConfig));
    uartConfig.baudRate = CY_U3P_UART_BAUDRATE_115200;
    uartConfig.stopBit = CY_U3P_UART_ONE_STOP_BIT;
    uartConfig.parity = CY_U3P_UART_NO_PARITY;
    uartConfig.txEnable = CyTrue;
    uartConfig.rxEnable = CyFalse;
    uartConfig.flowCtrl = CyFalse;
    uartConfig.isDma = CyTrue;

    apiRetStatus = CyU3PUartSetConfig (&uartConfig, NULL);
    if (apiRetStatus != CY_U3P_SUCCESS)
    {
        CyFxAppErrorHandler(apiRetStatus);
    }

    /* Set the UART transfer to a really large value. */
    apiRetStatus = CyU3PUartTxSetBlockXfer (0xFFFFFFFF);
    if (apiRetStatus != CY_U3P_SUCCESS)
    {
        CyFxAppErrorHandler(apiRetStatus);
    }

    /* Initialize the debug module. */
    apiRetStatus = CyU3PDebugInit (CY_U3P_LPP_SOCKET_UART_CONS, 8);
    if (apiRetStatus != CY_U3P_SUCCESS)
    {
        CyFxAppErrorHandler(apiRetStatus);
    }
}

/* Callback funtion for the DMA event notification. */
void
CyFxIntrSrcSinkDmaCallback (
        CyU3PDmaChannel   *chHandle, /* Handle to the DMA channel. */
        CyU3PDmaCbType_t  type,      /* Callback type.             */
        CyU3PDmaCBInput_t *input)    /* Callback status.           */
{
    CyU3PDmaBuffer_t buf_p;
    CyU3PReturnStatus_t status = CY_U3P_SUCCESS;

    if (type == CY_U3P_DMA_CB_PROD_EVENT)
    {
        /* This is a produce event notification to the CPU. This notification is 
         * received upon reception of every buffer. We have to discard the buffer
         * as soon as it is received to implement the data sink. */
        status = CyU3PDmaChannelDiscardBuffer (chHandle);
        if (status != CY_U3P_SUCCESS)
        {
            CyU3PDebugPrint (4, "CyU3PDmaChannelDiscardBuffer failed, Error code = %d\n", status);
        }

        /* Increment the counter. */
        glDMARxCount++;
    }
    if (type == CY_U3P_DMA_CB_CONS_EVENT)
    {
        /* This is a consume event notification to the CPU. This notification is 
         * received when a buffer is sent out from the device. We have to commit
         * a new buffer as soon as a buffer is available to implement the data
         * source. The data is preloaded into the buffer at that start. So just
         * commit the buffer. */
        status = CyU3PDmaChannelGetBuffer (chHandle, &buf_p, CYU3P_NO_WAIT);
        if (status == CY_U3P_SUCCESS)
        {
            /* Commit the full buffer with default status. */
            status = CyU3PDmaChannelCommitBuffer (chHandle, buf_p.size, 0);
            if (status != CY_U3P_SUCCESS)
            {
                CyU3PDebugPrint (4, "CyU3PDmaChannelCommitBuffer failed, Error code = %d\n", status);
            }
        }
        else
        {
            CyU3PDebugPrint (4, "CyU3PDmaChannelGetBuffer failed, Error code = %d\n", status);
        }

        /* Increment the counter. */
        glDMATxCount++;
    }
}

/* This function starts the Intr loop application. This is called
 * when a SET_CONF event is received from the USB host. The endpoints
 * are configured and the DMA pipe is setup in this function. */
void
CyFxIntrSrcSinkApplnStart (
        void)
{
    uint16_t size = 0, index = 0;
    CyU3PEpConfig_t epCfg;
    CyU3PDmaBuffer_t buf_p;
    CyU3PDmaChannelConfig_t dmaCfg;
    CyU3PReturnStatus_t apiRetStatus = CY_U3P_SUCCESS;
    CyU3PUSBSpeed_t usbSpeed = CyU3PUsbGetSpeed();

    /* First identify the usb speed. Once that is identified,
     * create a DMA channel and start the transfer on this. */

    /* Based on the Bus Speed configure the endpoint packet size */
    switch (usbSpeed)
    {
        case CY_U3P_FULL_SPEED:
            size = 64;
            break;

        case CY_U3P_HIGH_SPEED:
            size = 1024;
            break;

        case  CY_U3P_SUPER_SPEED:
            size = 1024;
            break;

        default:
            CyU3PDebugPrint (4, "Error! Invalid USB speed.\n");
            CyFxAppErrorHandler (CY_U3P_ERROR_FAILURE);
            break;
    }

    CyU3PMemSet ((uint8_t *)&epCfg, 0, sizeof (epCfg));
    epCfg.enable   = CyTrue;
    epCfg.epType   = CY_U3P_USB_EP_INTR;
    epCfg.burstLen = (usbSpeed == CY_U3P_SUPER_SPEED) ? (CY_FX_INTR_BURST_SIZE) : 1;
    epCfg.streams  = 0;
    epCfg.pcktSize = size;
    epCfg.isoPkts  = 1;

    /* Producer endpoint configuration */
    apiRetStatus = CyU3PSetEpConfig(CY_FX_EP_PRODUCER, &epCfg);
    if (apiRetStatus != CY_U3P_SUCCESS)
    {
        CyU3PDebugPrint (4, "CyU3PSetEpConfig failed, Error code = %d\n", apiRetStatus);
        CyFxAppErrorHandler (apiRetStatus);
    }

    /* Consumer endpoint configuration */
    apiRetStatus = CyU3PSetEpConfig(CY_FX_EP_CONSUMER, &epCfg);
    if (apiRetStatus != CY_U3P_SUCCESS)
    {
        CyU3PDebugPrint (4, "CyU3PSetEpConfig failed, Error code = %d\n", apiRetStatus);
        CyFxAppErrorHandler (apiRetStatus);
    }

    /* Flush the endpoint memory */
    CyU3PUsbFlushEp(CY_FX_EP_PRODUCER);
    CyU3PUsbFlushEp(CY_FX_EP_CONSUMER);

    /* Set the buffer size based on constants defined in the header file. */
    dmaCfg.size  = CY_FX_INTR_BURST_SIZE * 1024;
    dmaCfg.count = CY_FX_INTRSRCSINK_DMA_BUF_COUNT;
    dmaCfg.prodSckId = CY_FX_EP_PRODUCER_SOCKET;
    dmaCfg.consSckId = CY_U3P_CPU_SOCKET_CONS;
    dmaCfg.dmaMode = CY_U3P_DMA_MODE_BYTE;
    dmaCfg.notification = CY_U3P_DMA_CB_PROD_EVENT;
    dmaCfg.cb = CyFxIntrSrcSinkDmaCallback;
    dmaCfg.prodHeader = 0;
    dmaCfg.prodFooter = 0;
    dmaCfg.consHeader = 0;
    dmaCfg.prodAvailCount = 0;

    apiRetStatus = CyU3PDmaChannelCreate (&glChHandleIntrSink,
            CY_U3P_DMA_TYPE_MANUAL_IN, &dmaCfg);
    if (apiRetStatus != CY_U3P_SUCCESS)
    {
        CyU3PDebugPrint (4, "CyU3PDmaChannelCreate failed, Error code = %d\n", apiRetStatus);
        CyFxAppErrorHandler(apiRetStatus);
    }

    /* Create a DMA MANUAL_OUT channel for the consumer socket. */
    dmaCfg.notification = CY_U3P_DMA_CB_CONS_EVENT;
    dmaCfg.prodSckId = CY_U3P_CPU_SOCKET_PROD;
    dmaCfg.consSckId = CY_FX_EP_CONSUMER_SOCKET;
    apiRetStatus = CyU3PDmaChannelCreate (&glChHandleIntrSrc,
            CY_U3P_DMA_TYPE_MANUAL_OUT, &dmaCfg);
    if (apiRetStatus != CY_U3P_SUCCESS)
    {
        CyU3PDebugPrint (4, "CyU3PDmaChannelCreate failed, Error code = %d\n", apiRetStatus);
        CyFxAppErrorHandler(apiRetStatus);
    }

    /* Set DMA Channel transfer size */
    apiRetStatus = CyU3PDmaChannelSetXfer (&glChHandleIntrSink, CY_FX_INTRSRCSINK_DMA_TX_SIZE);
    if (apiRetStatus != CY_U3P_SUCCESS)
    {
        CyU3PDebugPrint (4, "CyU3PDmaChannelSetXfer Failed, Error code = %d\n", apiRetStatus);
        CyFxAppErrorHandler(apiRetStatus);
    }

    apiRetStatus = CyU3PDmaChannelSetXfer (&glChHandleIntrSrc, CY_FX_INTRSRCSINK_DMA_TX_SIZE);
    if (apiRetStatus != CY_U3P_SUCCESS)
    {
        CyU3PDebugPrint (4, "CyU3PDmaChannelSetXfer Failed, Error code = %d\n", apiRetStatus);
        CyFxAppErrorHandler(apiRetStatus);
    }

    /* Now preload all buffers in the MANUAL_OUT pipe with the required data. */
    for (index = 0; index < CY_FX_INTRSRCSINK_DMA_BUF_COUNT; index++)
    {
        apiRetStatus = CyU3PDmaChannelGetBuffer (&glChHandleIntrSrc, &buf_p, CYU3P_NO_WAIT);
        if (apiRetStatus != CY_U3P_SUCCESS)
        {
            CyU3PDebugPrint (4, "CyU3PDmaChannelGetBuffer failed, Error code = %d\n", apiRetStatus);
            CyFxAppErrorHandler(apiRetStatus);
        }
        CyU3PMemSet (buf_p.buffer, CY_FX_INTRSRCSINK_PATTERN, buf_p.size);
        apiRetStatus = CyU3PDmaChannelCommitBuffer (&glChHandleIntrSrc, buf_p.size, 0);
        if (apiRetStatus != CY_U3P_SUCCESS)
        {
            CyU3PDebugPrint (4, "CyU3PDmaChannelCommitBuffer failed, Error code = %d\n", apiRetStatus);
            CyFxAppErrorHandler(apiRetStatus);
        }
    }

    /* Update the flag so that the application thread is notified of this. */
    glIsApplnActive = CyTrue;
}

/* This function stops the Intr loop application. This shall be called whenever
 * a RESET or DISCONNECT event is received from the USB host. The endpoints are
 * disabled and the DMA pipe is destroyed by this function. */
void
CyFxIntrSrcSinkApplnStop (
        void)
{
    CyU3PEpConfig_t epCfg;
    CyU3PReturnStatus_t apiRetStatus = CY_U3P_SUCCESS;

    /* Update the flag so that the application thread is notified of this. */
    glIsApplnActive = CyFalse;

    /* Destroy the channels */
    CyU3PDmaChannelDestroy (&glChHandleIntrSink);
    CyU3PDmaChannelDestroy (&glChHandleIntrSrc);

    /* Flush the endpoint memory */
    CyU3PUsbFlushEp(CY_FX_EP_PRODUCER);
    CyU3PUsbFlushEp(CY_FX_EP_CONSUMER);

    /* Disable endpoints. */
    CyU3PMemSet ((uint8_t *)&epCfg, 0, sizeof (epCfg));
    epCfg.enable = CyFalse;

    /* Producer endpoint configuration. */
    apiRetStatus = CyU3PSetEpConfig(CY_FX_EP_PRODUCER, &epCfg);
    if (apiRetStatus != CY_U3P_SUCCESS)
    {
        CyU3PDebugPrint (4, "CyU3PSetEpConfig failed, Error code = %d\n", apiRetStatus);
        CyFxAppErrorHandler (apiRetStatus);
    }

    /* Consumer endpoint configuration. */
    apiRetStatus = CyU3PSetEpConfig(CY_FX_EP_CONSUMER, &epCfg);
    if (apiRetStatus != CY_U3P_SUCCESS)
    {
        CyU3PDebugPrint (4, "CyU3PSetEpConfig failed, Error code = %d\n", apiRetStatus);
        CyFxAppErrorHandler (apiRetStatus);
    }
}

/* Callback to handle the USB setup requests. */
CyBool_t
CyFxIntrSrcSinkApplnUSBSetupCB (
        uint32_t setupdat0, /* SETUP Data 0 */
        uint32_t setupdat1  /* SETUP Data 1 */
    )
{
    /* Fast enumeration is used. Only requests addressed to the interface, class,
     * vendor and unknown control requests are received by this function.
     * This application does not support any class or vendor requests. */

    uint8_t  bRequest, bReqType;
    uint8_t  bType, bTarget;
    uint16_t wValue, wIndex;
    CyBool_t isHandled = CyFalse;

    /* Decode the fields from the setup request. */
    bReqType = (setupdat0 & CY_U3P_USB_REQUEST_TYPE_MASK);
    bType    = (bReqType & CY_U3P_USB_TYPE_MASK);
    bTarget  = (bReqType & CY_U3P_USB_TARGET_MASK);
    bRequest = ((setupdat0 & CY_U3P_USB_REQUEST_MASK) >> CY_U3P_USB_REQUEST_POS);
    wValue   = ((setupdat0 & CY_U3P_USB_VALUE_MASK)   >> CY_U3P_USB_VALUE_POS);
    wIndex   = ((setupdat1 & CY_U3P_USB_INDEX_MASK)   >> CY_U3P_USB_INDEX_POS);

    if (bType == CY_U3P_USB_STANDARD_RQT)



    {
        /* Handle SET_FEATURE(FUNCTION_SUSPEND) and CLEAR_FEATURE(FUNCTION_SUSPEND)
         * requests here. It should be allowed to pass if the device is in configured
         * state and failed otherwise. */
        if ((bTarget == CY_U3P_USB_TARGET_INTF) && ((bRequest == CY_U3P_USB_SC_SET_FEATURE)
                    || (bRequest == CY_U3P_USB_SC_CLEAR_FEATURE)) && (wValue == 0))
        {
            if (glIsApplnActive)
                CyU3PUsbAckSetup ();
            else
                CyU3PUsbStall (0, CyTrue, CyFalse);

            isHandled = CyTrue;
        }

        /* CLEAR_FEATURE request for endpoint is always passed to the setup callback
         * regardless of the enumeration model used. When a clear feature is received,
         * the previous transfer has to be flushed and cleaned up. This is done at the
         * protocol level. Since this is just a loopback operation, there is no higher
         * level protocol. So flush the EP memory and reset the DMA channel associated
         * with it. If there are more than one EP associated with the channel reset both
         * the EPs. The endpoint stall and toggle / sequence number is also expected to be
         * reset. Return CyFalse to make the library clear the stall and reset the endpoint
         * toggle. Or invoke the CyU3PUsbStall (ep, CyFalse, CyTrue) and return CyTrue.
         * Here we are clearing the stall. */
        if ((bTarget == CY_U3P_USB_TARGET_ENDPT) && (bRequest == CY_U3P_USB_SC_CLEAR_FEATURE)
                && (wValue == CY_U3P_USBX_FS_EP_HALT))
        {
            if (glIsApplnActive)
            {
                if (wIndex == CY_FX_EP_PRODUCER)
                {
                    CyU3PDmaChannelReset (&glChHandleIntrSink);
                    CyU3PUsbFlushEp(CY_FX_EP_PRODUCER);
                    CyU3PUsbResetEp (CY_FX_EP_PRODUCER);
                    CyU3PDmaChannelSetXfer (&glChHandleIntrSink, CY_FX_INTRSRCSINK_DMA_TX_SIZE);
                    CyU3PUsbStall (wIndex, CyFalse, CyTrue);
                    isHandled = CyTrue;
                }

                if (wIndex == CY_FX_EP_CONSUMER)
                {
                    CyU3PDmaChannelReset (&glChHandleIntrSrc);
                    CyU3PUsbFlushEp(CY_FX_EP_CONSUMER);
                    CyU3PUsbResetEp (CY_FX_EP_CONSUMER);
                    CyU3PDmaChannelSetXfer (&glChHandleIntrSrc, CY_FX_INTRSRCSINK_DMA_TX_SIZE);
                    CyU3PUsbStall (wIndex, CyFalse, CyTrue);
                    isHandled = CyTrue;
                }
            }
        }
    }

    return isHandled;
}

/* This is the callback function to handle the USB events. */
void
CyFxIntrSrcSinkApplnUSBEventCB (
    CyU3PUsbEventType_t evtype, /* Event type */
    uint16_t            evdata  /* Event data */
    )
{
    switch (evtype)
    {
        case CY_U3P_USB_EVENT_SETCONF:
            /* Stop the application before restarting. */
            if (glIsApplnActive)
            {
                CyFxIntrSrcSinkApplnStop ();
            }
            /* Start the loop back function. */
            CyFxIntrSrcSinkApplnStart ();
            break;

        case CY_U3P_USB_EVENT_RESET:
        case CY_U3P_USB_EVENT_DISCONNECT:
            /* Stop the loop back function. */
            if (glIsApplnActive)
            {
                CyFxIntrSrcSinkApplnStop ();
            }
            break;

        default:
            break;
    }
}

/* Callback function to handle LPM requests from the USB 3.0 host. This function is invoked by the API
   whenever a state change from U0 -> U1 or U0 -> U2 happens. If we return CyTrue from this function, the
   FX3 device is retained in the low power state. If we return CyFalse, the FX3 device immediately tries
   to trigger an exit back to U0.

   This application does not have any state in which we should not allow U1/U2 transitions; and therefore
   the function always return CyTrue.
 */
CyBool_t
CyFxIntrSrcSinkApplnLPMRqtCB (
        CyU3PUsbLinkPowerMode link_mode)
{
    return CyTrue;
}

/* This function initializes the USB Module, sets the enumeration descriptors.
 * This function does not start the Intr streaming and this is done only when
 * SET_CONF event is received. */
void
CyFxIntrSrcSinkApplnInit (void)
{
    CyU3PReturnStatus_t apiRetStatus = CY_U3P_SUCCESS;

    /* Start the USB functionality. */
    apiRetStatus = CyU3PUsbStart();
    if (apiRetStatus != CY_U3P_SUCCESS)
    {
        CyU3PDebugPrint (4, "CyU3PUsbStart failed to Start, Error code = %d\n", apiRetStatus);
        CyFxAppErrorHandler(apiRetStatus);
    }

    /* The fast enumeration is the easiest way to setup a USB connection,
     * where all enumeration phase is handled by the library. Only the
     * class / vendor requests need to be handled by the application. */
    CyU3PUsbRegisterSetupCallback(CyFxIntrSrcSinkApplnUSBSetupCB, CyTrue);

    /* Setup the callback to handle the USB events. */
    CyU3PUsbRegisterEventCallback(CyFxIntrSrcSinkApplnUSBEventCB);

    /* Register a callback to handle LPM requests from the USB 3.0 host. */
    CyU3PUsbRegisterLPMRequestCallback(CyFxIntrSrcSinkApplnLPMRqtCB);
    
    /* Set the USB Enumeration descriptors */

    /* Super speed device descriptor. */
    apiRetStatus = CyU3PUsbSetDesc(CY_U3P_USB_SET_SS_DEVICE_DESCR, NULL, (uint8_t *)CyFxUSB30DeviceDscr);
    if (apiRetStatus != CY_U3P_SUCCESS)
    {
        CyU3PDebugPrint (4, "USB set device descriptor failed, Error code = %d\n", apiRetStatus);
        CyFxAppErrorHandler(apiRetStatus);
    }

    /* High speed device descriptor. */
    apiRetStatus = CyU3PUsbSetDesc(CY_U3P_USB_SET_HS_DEVICE_DESCR, NULL, (uint8_t *)CyFxUSB20DeviceDscr);
    if (apiRetStatus != CY_U3P_SUCCESS)
    {
        CyU3PDebugPrint (4, "USB set device descriptor failed, Error code = %d\n", apiRetStatus);
        CyFxAppErrorHandler(apiRetStatus);
    }

    /* BOS descriptor */
    apiRetStatus = CyU3PUsbSetDesc(CY_U3P_USB_SET_SS_BOS_DESCR, NULL, (uint8_t *)CyFxUSBBOSDscr);
    if (apiRetStatus != CY_U3P_SUCCESS)
    {
        CyU3PDebugPrint (4, "USB set configuration descriptor failed, Error code = %d\n", apiRetStatus);
        CyFxAppErrorHandler(apiRetStatus);
    }

    /* Device qualifier descriptor */
    apiRetStatus = CyU3PUsbSetDesc(CY_U3P_USB_SET_DEVQUAL_DESCR, NULL, (uint8_t *)CyFxUSBDeviceQualDscr);
    if (apiRetStatus != CY_U3P_SUCCESS)
    {
        CyU3PDebugPrint (4, "USB set device qualifier descriptor failed, Error code = %d\n", apiRetStatus);
        CyFxAppErrorHandler(apiRetStatus);
    }

    /* Super speed configuration descriptor */
    apiRetStatus = CyU3PUsbSetDesc(CY_U3P_USB_SET_SS_CONFIG_DESCR, NULL, (uint8_t *)CyFxUSBSSConfigDscr);
    if (apiRetStatus != CY_U3P_SUCCESS)
    {
        CyU3PDebugPrint (4, "USB set configuration descriptor failed, Error code = %d\n", apiRetStatus);
        CyFxAppErrorHandler(apiRetStatus);
    }

    /* High speed configuration descriptor */
    apiRetStatus = CyU3PUsbSetDesc(CY_U3P_USB_SET_HS_CONFIG_DESCR, NULL, (uint8_t *)CyFxUSBHSConfigDscr);
    if (apiRetStatus != CY_U3P_SUCCESS)
    {
        CyU3PDebugPrint (4, "USB Set Other Speed Descriptor failed, Error Code = %d\n", apiRetStatus);
        CyFxAppErrorHandler(apiRetStatus);
    }

    /* Full speed configuration descriptor */
    apiRetStatus = CyU3PUsbSetDesc(CY_U3P_USB_SET_FS_CONFIG_DESCR, NULL, (uint8_t *)CyFxUSBFSConfigDscr);
    if (apiRetStatus != CY_U3P_SUCCESS)
    {
        CyU3PDebugPrint (4, "USB Set Configuration Descriptor failed, Error Code = %d\n", apiRetStatus);
        CyFxAppErrorHandler(apiRetStatus);
    }

    /* String descriptor 0 */
    apiRetStatus = CyU3PUsbSetDesc(CY_U3P_USB_SET_STRING_DESCR, 0, (uint8_t *)CyFxUSBStringLangIDDscr);
    if (apiRetStatus != CY_U3P_SUCCESS)
    {
        CyU3PDebugPrint (4, "USB set string descriptor failed, Error code = %d\n", apiRetStatus);
        CyFxAppErrorHandler(apiRetStatus);
    }

    /* String descriptor 1 */
    apiRetStatus = CyU3PUsbSetDesc(CY_U3P_USB_SET_STRING_DESCR, 1, (uint8_t *)CyFxUSBManufactureDscr);
    if (apiRetStatus != CY_U3P_SUCCESS)
    {
        CyU3PDebugPrint (4, "USB set string descriptor failed, Error code = %d\n", apiRetStatus);
        CyFxAppErrorHandler(apiRetStatus);
    }

    /* String descriptor 2 */
    apiRetStatus = CyU3PUsbSetDesc(CY_U3P_USB_SET_STRING_DESCR, 2, (uint8_t *)CyFxUSBProductDscr);
    if (apiRetStatus != CY_U3P_SUCCESS)
    {
        CyU3PDebugPrint (4, "USB set string descriptor failed, Error code = %d\n", apiRetStatus);
        CyFxAppErrorHandler(apiRetStatus);
    }

    /* Connect the USB Pins with super speed operation enabled. */
    apiRetStatus = CyU3PConnectState(CyTrue, CyTrue);
    if (apiRetStatus != CY_U3P_SUCCESS)
    {
        CyU3PDebugPrint (4, "USB Connect failed, Error code = %d\n", apiRetStatus);
        CyFxAppErrorHandler(apiRetStatus);
    }
}

/* Entry function for the IntrSrcSinkAppThread. */
void
IntrSrcSinkAppThread_Entry (
        uint32_t input)
{
    /* Initialize the debug module */
    CyFxIntrSrcSinkApplnDebugInit();

    /* Initialize the Intr loop application */
    CyFxIntrSrcSinkApplnInit();

    for (;;)
    {
        CyU3PThreadSleep (1000);
        if (glIsApplnActive)
        {
            /* Print the number of buffers received / transmitted so far from the USB host. */
            CyU3PDebugPrint (6, "Data tracker: buffers received: %d, buffers sent: %d\n", glDMARxCount, glDMATxCount);
        }
    }
}

/* Application define function which creates the threads. */
void
CyFxApplicationDefine (
        void)
{
    void *ptr = NULL;
    uint32_t retThrdCreate = CY_U3P_SUCCESS;

    /* Allocate the memory for the threads */
    ptr = CyU3PMemAlloc (CY_FX_INTRSRCSINK_THREAD_STACK);

    /* Create the thread for the application */
    retThrdCreate = CyU3PThreadCreate (&IntrSrcSinkAppThread,           /* Intr loop App Thread structure */
                          "21:Intr_loop_MANUAL_IN_OUT",            /* Thread ID and Thread name */
                          IntrSrcSinkAppThread_Entry,                   /* Intr loop App Thread Entry function */
                          0,                                      /* No input parameter to thread */
                          ptr,                                    /* Pointer to the allocated thread stack */
                          CY_FX_INTRSRCSINK_THREAD_STACK,               /* Intr loop App Thread stack size */
                          CY_FX_INTRSRCSINK_THREAD_PRIORITY,            /* Intr loop App Thread priority */
                          CY_FX_INTRSRCSINK_THREAD_PRIORITY,            /* Intr loop App Thread priority */
                          CYU3P_NO_TIME_SLICE,                    /* No time slice for the application thread */
                          CYU3P_AUTO_START                        /* Start the Thread immediately */
                          );

    /* Check the return code */
    if (retThrdCreate != 0)
    {
        /* Thread Creation failed with the error code retThrdCreate */

        /* Add custom recovery or debug actions here */

        /* Application cannot continue */
        /* Loop indefinitely */
        while(1);
    }
}

/*
 * Main function
 */
int
main (void)
{
    CyU3PIoMatrixConfig_t io_cfg;
    CyU3PReturnStatus_t status = CY_U3P_SUCCESS;

    /* Initialize the device */
    status = CyU3PDeviceInit (NULL);
    if (status != CY_U3P_SUCCESS)
    {
        goto handle_fatal_error;
    }

    /* Initialize the caches. Enable both Instruction and Data Caches. */
    status = CyU3PDeviceCacheControl (CyTrue, CyTrue, CyTrue);
    if (status != CY_U3P_SUCCESS)
    {
        goto handle_fatal_error;
    }

    /* Configure the IO matrix for the device. On the FX3 DVK board, the COM port 
     * is connected to the IO(53:56). This means that either DQ32 mode should be
     * selected or lppMode should be set to UART_ONLY. Here we are choosing
     * UART_ONLY configuration. */
    io_cfg.isDQ32Bit = CyFalse;
    io_cfg.useUart   = CyTrue;
    io_cfg.useI2C    = CyFalse;
    io_cfg.useI2S    = CyFalse;
    io_cfg.useSpi    = CyFalse;
    io_cfg.lppMode   = CY_U3P_IO_MATRIX_LPP_UART_ONLY;
    /* No GPIOs are enabled. */
    io_cfg.gpioSimpleEn[0]  = 0;
    io_cfg.gpioSimpleEn[1]  = 0;
    io_cfg.gpioComplexEn[0] = 0;
    io_cfg.gpioComplexEn[1] = 0;
    status = CyU3PDeviceConfigureIOMatrix (&io_cfg);
    if (status != CY_U3P_SUCCESS)
    {
        goto handle_fatal_error;
    }

    /* This is a non returnable call for initializing the RTOS kernel */
    CyU3PKernelEntry ();

    /* Dummy return to make the compiler happy */
    return 0;

handle_fatal_error:

    /* Cannot recover from this error. */
    while (1);
}

/* [ ] */

