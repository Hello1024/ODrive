 
#ifndef __USB_HANDLERS_H__
#define __USB_HANDLERS_H__


/* Includes ------------------------------------------------------------------*/
#include <fibre/protocol.hpp>
#include "lrndis/lwip-1.4.1/apps/httpserver_raw/fs.h"

#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>

/* Exported types ------------------------------------------------------------*/
/* Exported constants --------------------------------------------------------*/
/* Exported variables --------------------------------------------------------*/
/* Exported macro ------------------------------------------------------------*/
/* Exported functions --------------------------------------------------------*/

extern "C" {
int fs_open_custom(struct fs_file *file, const char *name);
void fs_close_custom(struct fs_file *file);
}



/* Exported functions --------------------------------------------------------*/


#endif /* __USB_HANDLERS_H__ */

