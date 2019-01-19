/**
  ******************************************************************************
  * @file           : usb_device.c
  * @version        : v1.0_Cube
  * @brief          : This file implements the USB Device
  ******************************************************************************
  * This notice applies to any and all portions of this file
  * that are not between comment pairs USER CODE BEGIN and
  * USER CODE END. Other portions of this file, whether 
  * inserted by the user or by software development tools
  * are owned by their respective copyright owners.
  *
  * Copyright (c) 2019 STMicroelectronics International N.V. 
  * All rights reserved.
  *
  * Redistribution and use in source and binary forms, with or without 
  * modification, are permitted, provided that the following conditions are met:
  *
  * 1. Redistribution of source code must retain the above copyright notice, 
  *    this list of conditions and the following disclaimer.
  * 2. Redistributions in binary form must reproduce the above copyright notice,
  *    this list of conditions and the following disclaimer in the documentation
  *    and/or other materials provided with the distribution.
  * 3. Neither the name of STMicroelectronics nor the names of other 
  *    contributors to this software may be used to endorse or promote products 
  *    derived from this software without specific written permission.
  * 4. This software, including modifications and/or derivative works of this 
  *    software, must execute solely and exclusively on microcontroller or
  *    microprocessor devices manufactured by or for STMicroelectronics.
  * 5. Redistribution and use of this software other than as permitted under 
  *    this license is void and will automatically terminate your rights under 
  *    this license. 
  *
  * THIS SOFTWARE IS PROVIDED BY STMICROELECTRONICS AND CONTRIBUTORS "AS IS" 
  * AND ANY EXPRESS, IMPLIED OR STATUTORY WARRANTIES, INCLUDING, BUT NOT 
  * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY, FITNESS FOR A 
  * PARTICULAR PURPOSE AND NON-INFRINGEMENT OF THIRD PARTY INTELLECTUAL PROPERTY
  * RIGHTS ARE DISCLAIMED TO THE FULLEST EXTENT PERMITTED BY LAW. IN NO EVENT 
  * SHALL STMICROELECTRONICS OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
  * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
  * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, 
  * OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF 
  * LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING 
  * NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE,
  * EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
  *
  ******************************************************************************
  */

/* Includes ------------------------------------------------------------------*/

#include "usb_device.h"
#include "usbd_core.h"
#include "usbd_desc.h"
#include "usbd_cdc.h"
#include "usbd_cdc_if.h"

/* USER CODE BEGIN Includes */
#include "communication/networking/lrndis/rndis-stm32/rndis_usbd_conf.h"
extern USBD_ClassTypeDef usbd_rndis_cb;

/* USER CODE END Includes */

/* USER CODE BEGIN PV */
/* Private variables ---------------------------------------------------------*/

/* USER CODE END PV */

/* USER CODE BEGIN PFP */
/* Private function prototypes -----------------------------------------------*/

/* USER CODE END PFP */

/* USB Device Core handle declaration. */
USBD_HandleTypeDef hUsbDeviceFS;

int usbd_composite_next_ep0_recv_for_cdc = 0;

/*
 * -- Insert your variables declaration here --
 */
/* USER CODE BEGIN 0 */

#define USB_COMPOSITE_CONFIG_DESC_SIZ  (67 + 39 + (/* for rndis */8+9+5+5+4+5+7+9+7+7) )


// This descriptor is hand crafted from descriptors of all the sub-device types
/* USB COMPOSITE device Configuration Descriptor */
__ALIGN_BEGIN uint8_t USBD_COMPOSITE_CfgDesc[USB_COMPOSITE_CONFIG_DESC_SIZ] __ALIGN_END =
{
  /*Configuration Descriptor*/
  0x09,   /* bLength: Configuration Descriptor size */
  USB_DESC_TYPE_CONFIGURATION,      /* bDescriptorType: Configuration */
  USB_COMPOSITE_CONFIG_DESC_SIZ,                /* wTotalLength:no of returned bytes */
  0x00,
  0x05,   /* bNumInterfaces: 5 interfaces (2 for CDC, 1 custom, 2 for rndis) */
  0x01,   /* bConfigurationValue: Configuration value */
  0x00,   /* iConfiguration: Index of string descriptor describing the configuration */
  0xC0,   /* bmAttributes: self powered */
  0x32,   /* MaxPower 0 mA */

  ///////////////////////////////////////////////////////////////////////////////

  /* Interface Association Descriptor: CDC device (virtual com port) */
  0x08,   /* bLength: IAD size */
  0x0B,   /* bDescriptorType: Interface Association Descriptor */
  CDC_COMM_IFACE,   /* bFirstInterface */
  0x02,   /* bInterfaceCount */
  0x02,   /* bFunctionClass: Communication Interface Class */
  0x02,   /* bFunctionSubClass: Abstract Control Model */
  0x01,   /* bFunctionProtocol: Common AT commands */
  0x00,   /* iFunction */
  
  /*---------------------------------------------------------------------------*/

  /*Interface Descriptor */
  0x09,   /* bLength: Interface Descriptor size */
  USB_DESC_TYPE_INTERFACE,  /* bDescriptorType: Interface */
  /* Interface descriptor type */
  CDC_COMM_IFACE,   /* bInterfaceNumber: Number of Interface */
  0x00,   /* bAlternateSetting: Alternate setting */
  0x01,   /* bNumEndpoints: One endpoints used */
  0x02,   /* bInterfaceClass: Communication Interface Class */
  0x02,   /* bInterfaceSubClass: Abstract Control Model */
  0x01,   /* bInterfaceProtocol: Common AT commands */
  0x00,   /* iInterface: */

  /*Header Functional Descriptor*/
  0x05,   /* bLength: Endpoint Descriptor size */
  0x24,   /* bDescriptorType: CS_INTERFACE */
  0x00,   /* bDescriptorSubtype: Header Func Desc */
  0x10,   /* bcdCDC: spec release number */
  0x01,
  
  /*Call Management Functional Descriptor*/
  0x05,   /* bFunctionLength */
  0x24,   /* bDescriptorType: CS_INTERFACE */
  0x01,   /* bDescriptorSubtype: Call Management Func Desc */
  0x00,   /* bmCapabilities: D0+D1 */
  0x01,   /* bDataInterface: 1 */
  
  /*ACM Functional Descriptor*/
  0x04,   /* bFunctionLength */
  0x24,   /* bDescriptorType: CS_INTERFACE */
  0x02,   /* bDescriptorSubtype: Abstract Control Management desc */
  0x02,   /* bmCapabilities */
  
  /*Union Functional Descriptor*/
  0x05,   /* bFunctionLength */
  0x24,   /* bDescriptorType: CS_INTERFACE */
  0x06,   /* bDescriptorSubtype: Union func desc */
  CDC_COMM_IFACE,   /* bMasterInterface: Communication class interface */
  CDC_DATA_IFACE,   /* bSlaveInterface0: Data Class Interface */

  /*Endpoint 2 Descriptor*/
  0x07,                           /* bLength: Endpoint Descriptor size */
  USB_DESC_TYPE_ENDPOINT,   /* bDescriptorType: Endpoint */
  CDC_CMD_EP,                     /* bEndpointAddress */
  0x03,                           /* bmAttributes: Interrupt */
  LOBYTE(CDC_CMD_PACKET_SIZE),     /* wMaxPacketSize: */
  HIBYTE(CDC_CMD_PACKET_SIZE),
  0x10,                           /* bInterval: */ 
  /*---------------------------------------------------------------------------*/
  
  /*Data class interface descriptor*/
  0x09,   /* bLength: Endpoint Descriptor size */
  USB_DESC_TYPE_INTERFACE,  /* bDescriptorType: */
  CDC_DATA_IFACE,   /* bInterfaceNumber: Number of Interface */
  0x00,   /* bAlternateSetting: Alternate setting */
  0x02,   /* bNumEndpoints: Two endpoints used */
  0x0A,   /* bInterfaceClass: CDC */
  0x00,   /* bInterfaceSubClass: */
  0x00,   /* bInterfaceProtocol: */
  0x00,   /* iInterface: */
  
  /*Endpoint OUT Descriptor*/
  0x07,   /* bLength: Endpoint Descriptor size */
  USB_DESC_TYPE_ENDPOINT,      /* bDescriptorType: Endpoint */
  CDC_OUT_EP,                        /* bEndpointAddress */
  0x02,                              /* bmAttributes: Bulk */
  LOBYTE(CDC_DATA_HS_MAX_PACKET_SIZE),  /* wMaxPacketSize: */
  HIBYTE(CDC_DATA_HS_MAX_PACKET_SIZE),
  0x00,                              /* bInterval: ignore for Bulk transfer */
  
  /*Endpoint IN Descriptor*/
  0x07,   /* bLength: Endpoint Descriptor size */
  USB_DESC_TYPE_ENDPOINT,      /* bDescriptorType: Endpoint */
  CDC_IN_EP,                         /* bEndpointAddress */
  0x02,                              /* bmAttributes: Bulk */
  LOBYTE(CDC_DATA_HS_MAX_PACKET_SIZE),  /* wMaxPacketSize: */
  HIBYTE(CDC_DATA_HS_MAX_PACKET_SIZE),
  0x00,                              /* bInterval: ignore for Bulk transfer */

  ///////////////////////////////////////////////////////////////////////////////
  
  /* Interface Association Descriptor: custom device */
  0x08,   /* bLength: IAD size */
  0x0B,   /* bDescriptorType: Interface Association Descriptor */
  CDC_ODRIVE_IFACE,   /* bFirstInterface */
  0x01,   /* bInterfaceCount */
  0x00,   /* bFunctionClass: */
  0x00,   /* bFunctionSubClass: */
  0x00,   /* bFunctionProtocol: */
  0x06,   /* iFunction */

  /*---------------------------------------------------------------------------*/
  
  /*Data class interface descriptor*/
  0x09,   /* bLength: Endpoint Descriptor size */
  USB_DESC_TYPE_INTERFACE,  /* bDescriptorType: */
  CDC_ODRIVE_IFACE,   /* bInterfaceNumber: Number of Interface */
  0x00,   /* bAlternateSetting: Alternate setting */
  0x02,   /* bNumEndpoints: Two endpoints used */
  0x00,   /* bInterfaceClass: vendor specific */
  0x01,   /* bInterfaceSubClass: ODrive Communication */
  0x00,   /* bInterfaceProtocol: */
  0x00,   /* iInterface: */
  
  /*Endpoint OUT Descriptor*/
  0x07,   /* bLength: Endpoint Descriptor size */
  USB_DESC_TYPE_ENDPOINT,      /* bDescriptorType: Endpoint */
  ODRIVE_OUT_EP,                        /* bEndpointAddress */
  0x02,                              /* bmAttributes: Bulk */
  LOBYTE(CDC_DATA_HS_MAX_PACKET_SIZE),  /* wMaxPacketSize: */
  HIBYTE(CDC_DATA_HS_MAX_PACKET_SIZE),
  0x00,                              /* bInterval: ignore for Bulk transfer */
  
  /*Endpoint IN Descriptor*/
  0x07,   /* bLength: Endpoint Descriptor size */
  USB_DESC_TYPE_ENDPOINT,      /* bDescriptorType: Endpoint */
  ODRIVE_IN_EP,                         /* bEndpointAddress */
  0x02,                              /* bmAttributes: Bulk */
  LOBYTE(CDC_DATA_HS_MAX_PACKET_SIZE),  /* wMaxPacketSize: */
  HIBYTE(CDC_DATA_HS_MAX_PACKET_SIZE),
  0x00,                              /* bInterval: ignore for Bulk transfer */

  
  /*
   *  RNDIS device descriptors.
   *  Consists of an association descriptor, two interfaces (3&4) and associated descriptors
   */

    /* IAD descriptor */

  0x08, /* bLength */
  0x0B, /* bDescriptorType */
  RNDIS_CONTROL_IFACE, /* bFirstInterface */
  0x02, /* bInterfaceCount */
  0xE0, /* bFunctionClass (Wireless Controller) */
  0x01, /* bFunctionSubClass */
  0x03, /* bFunctionProtocol */
  0x00, /* iFunction */

  /* Interface 3 descriptor */
  
  9,                             /* bLength */
  USB_DESC_TYPE_INTERFACE,       /* bDescriptorType = INTERFACE */
  RNDIS_CONTROL_IFACE,           /* bInterfaceNumber */
  0x00,                          /* bAlternateSetting */
  1,                             /* bNumEndpoints */
  0xE0,                          /* bInterfaceClass: Wireless Controller */
  0x01,                          /* bInterfaceSubClass */
  0x03,                          /* bInterfaceProtocol */
  0,                             /* iInterface */

  /* Interface 3 functional descriptor */

  /* Header Functional Descriptor */
  0x05, /* bFunctionLength */
  0x24, /* bDescriptorType = CS Interface */
  0x00, /* bDescriptorSubtype */
  0x10, /* bcdCDC = 1.10 */
  0x01, /* bcdCDC = 1.10 */

  /* Call Management Functional Descriptor */
  0x05, /* bFunctionLength */
  0x24, /* bDescriptorType = CS Interface */
  0x01, /* bDescriptorSubtype = Call Management */
  0x00, /* bmCapabilities */
  0x04, /* bDataInterface */

  /* Abstract Control Management Functional Descriptor */
  0x04, /* bFunctionLength */
  0x24, /* bDescriptorType = CS Interface */
  0x02, /* bDescriptorSubtype = Abstract Control Management */
  0x00, /* bmCapabilities = Device supports the notification Network_Connection */

  /* Union Functional Descriptor */
  0x05, /* bFunctionLength */
  0x24, /* bDescriptorType = CS Interface */
  0x06, /* bDescriptorSubtype = Union */
  RNDIS_CONTROL_IFACE, /* bControlInterface = "RNDIS Communications Control" */
  RNDIS_DATA_IFACE, /* bSubordinateInterface0 = "RNDIS Ethernet Data" */

  /* Endpoint descriptors for Communication Class Interface */

  7,                            /* bLength         = 7 bytes */
  USB_DESC_TYPE_ENDPOINT,       /* bDescriptorType = ENDPOINT */
  RNDIS_NOTIFICATION_IN_EP,     /* bEndpointAddr   = IN - EP3 */
  0x03,                         /* bmAttributes    = Interrupt endpoint */
  8, 0,                         /* wMaxPacketSize */
  0x01,                         /* bInterval       = 1 ms polling from host */

  /* Interface 4 descriptor */

  9,                             /* bLength */
  USB_DESC_TYPE_INTERFACE,       /* bDescriptorType */
  RNDIS_DATA_IFACE,                /* bInterfaceNumber */
  0x00,                          /* bAlternateSetting */
  2,                             /* bNumEndpoints */
  0x0A,                          /* bInterfaceClass: CDC */
  0x00,                          /* bInterfaceSubClass */
  0x00,                          /* bInterfaceProtocol */
  0x00,                          /* uint8  iInterface */

  /* Endpoint descriptors for Data Class Interface */

  7,                            /* bLength         = 7 bytes */
  USB_DESC_TYPE_ENDPOINT, /* bDescriptorType = ENDPOINT [IN] */
  RNDIS_DATA_IN_EP,             /* bEndpointAddr   = IN EP */
  0x02,                         /* bmAttributes    = BULK */
  RNDIS_DATA_IN_SZ, 0,          /* wMaxPacketSize */
  0,                            /* bInterval       = ignored for BULK */

  7,                            /* bLength         = 7 bytes */
  USB_DESC_TYPE_ENDPOINT, /* bDescriptorType = ENDPOINT [OUT] */
  RNDIS_DATA_OUT_EP,            /* bEndpointAddr   = OUT EP */
  0x02,                         /* bmAttributes    = BULK */
  RNDIS_DATA_OUT_SZ, 0,         /* wMaxPacketSize */
  0                             /* bInterval       = ignored for BULK */

} ;


static uint8_t  USBD_COMPOSITE_Init (USBD_HandleTypeDef *pdev, 
                               uint8_t cfgidx) {
  USBD_CDC.Init(pdev, cfgidx);
  return usbd_rndis_cb.Init(pdev, cfgidx);
}

static uint8_t  USBD_COMPOSITE_DeInit (USBD_HandleTypeDef *pdev, 
                                 uint8_t cfgidx) {
  return USBD_CDC.DeInit(pdev, cfgidx)
      || usbd_rndis_cb.DeInit(pdev, cfgidx);
}

static inline uint8_t ep_is_for_cdc(uint8_t epnum) {
  switch (epnum) {
    case CDC_IN_EP:
    case CDC_OUT_EP:
    case CDC_CMD_EP:
    case ODRIVE_IN_EP:
    case ODRIVE_OUT_EP:
      return 1;
    default:
      return 0;
  }
}
static uint8_t  USBD_COMPOSITE_Setup (USBD_HandleTypeDef *pdev, 
                                USBD_SetupReqTypedef *req) {
  // Se need to see who this setup is for...
  //return USBD_OK;

  switch (req->bmRequest & USB_REQ_RECIPIENT_MASK) {
    case USB_REQ_RECIPIENT_DEVICE:
      // hopefully lower layers deal with this for us.
      return USBD_OK;
    case USB_REQ_RECIPIENT_INTERFACE:
      switch (req->wIndex) {
        case CDC_COMM_IFACE:
        case CDC_DATA_IFACE:
        case CDC_ODRIVE_IFACE:
          usbd_composite_next_ep0_recv_for_cdc = 1;
          return USBD_CDC.Setup(pdev, req);
        case RNDIS_CONTROL_IFACE:
        case RNDIS_DATA_IFACE:
          usbd_composite_next_ep0_recv_for_cdc = 0;
          return usbd_rndis_cb.Setup(pdev, req);
        default:
          return USBD_OK;
      }
    case USB_REQ_RECIPIENT_ENDPOINT:
      if (ep_is_for_cdc(req->wIndex)) {
          usbd_composite_next_ep0_recv_for_cdc = 1;
          return USBD_CDC.Setup(pdev, req);
      } else {
          // Also handles default case.
          usbd_composite_next_ep0_recv_for_cdc = 0;
          return usbd_rndis_cb.Setup(pdev, req);
      }
    default:
      return USBD_OK;
  }
}

static uint8_t  USBD_COMPOSITE_EP0_RxReady (USBD_HandleTypeDef *pdev) {
  // This approach might not be valid if the next
  // EP0 packet gets setup before this one gets received.
  if (usbd_composite_next_ep0_recv_for_cdc)
    return USBD_CDC.EP0_RxReady(pdev);
  else
    return usbd_rndis_cb.EP0_RxReady(pdev);
}

static uint8_t  USBD_COMPOSITE_DataIn (USBD_HandleTypeDef *pdev, 
                                       uint8_t epnum) {
  if (ep_is_for_cdc(epnum | 0x80))
    return USBD_CDC.DataIn(pdev, epnum);
  return usbd_rndis_cb.DataIn(pdev, epnum);
}

static uint8_t  USBD_COMPOSITE_DataOut (USBD_HandleTypeDef *pdev, 
                                        uint8_t epnum) {
  if (ep_is_for_cdc(epnum))
    return USBD_CDC.DataOut(pdev, epnum);
  return usbd_rndis_cb.DataOut(pdev, epnum);
}

static uint8_t  USBD_COMPOSITE_SOF (USBD_HandleTypeDef *pdev) {
  return usbd_rndis_cb.SOF(pdev);
}

static uint8_t  USBD_COMPOSITE_IsoINIncomplete (USBD_HandleTypeDef *pdev, 
                                 uint8_t epnum) {
  return usbd_rndis_cb.IsoINIncomplete(pdev, epnum);
}

static uint8_t  USBD_COMPOSITE_IsoOUTIncomplete (USBD_HandleTypeDef *pdev, 
                                 uint8_t epnum) {
  return usbd_rndis_cb.IsoOUTIncomplete(pdev, epnum);
}


static uint8_t  *USBD_COMPOSITE_GetCfgDescriptor (uint16_t *length) {
  USBD_COMPOSITE_CfgDesc[2] = sizeof(USBD_COMPOSITE_CfgDesc) & 0xFF;
  USBD_COMPOSITE_CfgDesc[3] = (sizeof(USBD_COMPOSITE_CfgDesc) >> 8) & 0xFF;
    
  *length = sizeof (USBD_COMPOSITE_CfgDesc);
  return USBD_COMPOSITE_CfgDesc;
}

static uint8_t  *USBD_COMPOSITE_GetDeviceQualifierDescriptor (uint16_t *length) {
  return USBD_CDC.GetDeviceQualifierDescriptor(length);
}

static uint8_t * USBD_COMPOSITE_UsrStrDescriptor(struct _USBD_HandleTypeDef *pdev, uint8_t index,  uint16_t *length) {
  return USBD_CDC.GetUsrStrDescriptor(pdev, index, length);
}

/* COMPOSITE interface class callbacks structure */
USBD_ClassTypeDef  USBD_COMPOSITE = 
{
  USBD_COMPOSITE_Init,
  USBD_COMPOSITE_DeInit,
  USBD_COMPOSITE_Setup,
  NULL,                 /* EP0_TxSent, */
  USBD_COMPOSITE_EP0_RxReady,
  USBD_COMPOSITE_DataIn,
  USBD_COMPOSITE_DataOut,
  USBD_COMPOSITE_SOF,
  USBD_COMPOSITE_IsoINIncomplete,
  USBD_COMPOSITE_IsoOUTIncomplete, 
  USBD_COMPOSITE_GetCfgDescriptor,  
  USBD_COMPOSITE_GetCfgDescriptor,    
  USBD_COMPOSITE_GetCfgDescriptor, 
  USBD_COMPOSITE_GetDeviceQualifierDescriptor,
  USBD_COMPOSITE_UsrStrDescriptor
};


/* USER CODE END 0 */

/*
 * -- Insert your external function declaration here --
 */
/* USER CODE BEGIN 1 */

/* USER CODE END 1 */

/**
  * Init USB device Library, add supported class and start the library
  * @retval None
  */
void MX_USB_DEVICE_Init(void)
{
  /* USER CODE BEGIN USB_DEVICE_Init_PreTreatment */
  
  /* USER CODE END USB_DEVICE_Init_PreTreatment */
  
  /* Init Device Library, add supported class and start the library. */
  USBD_Init(&hUsbDeviceFS, &FS_Desc, DEVICE_FS);

  USBD_RegisterClass(&hUsbDeviceFS, &USBD_COMPOSITE);

  USBD_CDC_RegisterInterface(&hUsbDeviceFS, &USBD_Interface_fops_FS);

  USBD_Start(&hUsbDeviceFS);

  /* USER CODE BEGIN USB_DEVICE_Init_PostTreatment */
  
  /* USER CODE END USB_DEVICE_Init_PostTreatment */
}

/**
  * @}
  */

/**
  * @}
  */

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
