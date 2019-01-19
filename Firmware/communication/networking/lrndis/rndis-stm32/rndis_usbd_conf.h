/**
  ******************************************************************************
  * @file    usbd_conf.h
  * @author  MCD Application Team
  * @version V1.1.0
  * @date    19-March-2012
  * @brief   USB Device configuration file
  ******************************************************************************
  * @attention
  *
  * <h2><center>&copy; COPYRIGHT 2012 STMicroelectronics</center></h2>
  *
  * Licensed under MCD-ST Liberty SW License Agreement V2, (the "License");
  * You may not use this file except in compliance with the License.
  * You may obtain a copy of the License at:
  *
  *        http://www.st.com/software_license_agreement_liberty_v2
  *
  * Unless required by applicable law or agreed to in writing, software 
  * distributed under the License is distributed on an "AS IS" BASIS, 
  * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
  * See the License for the specific language governing permissions and
  * limitations under the License.
  *
  ******************************************************************************
  */

#ifndef __RNDIS_USBD_CONF__H__
#define __RNDIS_USBD_CONF__H__

// #include "usb_conf.h"

// These need to not overlap with interface and EP numbers 
// used by CDC and ODrive interfaces
#define RNDIS_DATA_IN_EP         0x83
#define RNDIS_DATA_OUT_EP        0x01

// Nonfunctional.  Th STM32F405 in FS mode only allows a
// maximum of 0x83.  It is used as normal, but never works.
#define RNDIS_NOTIFICATION_IN_EP 0x82

#define RNDIS_NOTIFICATION_IN_SZ 0x08
#define RNDIS_DATA_IN_SZ         0x40
#define RNDIS_DATA_OUT_SZ        0x40

#define RNDIS_CONTROL_IFACE      0x0
#define RNDIS_DATA_IFACE         (RNDIS_CONTROL_IFACE+1)


#define USBD_CFG_MAX_NUM         1
#define USB_MAX_STR_DESC_SIZ     64
           

#endif
