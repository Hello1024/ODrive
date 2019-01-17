 
#ifndef __USB_HANDLERS_H__
#define __USB_HANDLERS_H__


#include "lrndis/lwip-1.4.1/apps/httpserver_raw/fs.h"



typedef const char *(*tPathHandler)(void* SelectedObject, void* HandlerContext, const char *remaining_path, struct fs_file *file);

/*
 * Structure defining the base filename (URL) of a CGI and the associated
 * function which is to be called when that URL is requested.
 */
typedef struct
{
    const char *pcPathPrefix;
    tPathHandler pfnPathHandler;
    const void* HandlerContext;
} tPath;


#endif
