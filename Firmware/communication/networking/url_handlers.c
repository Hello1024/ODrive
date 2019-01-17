#include "url_handlers.h"



static const tPath root_uri_table[] =
{
    { "/api", state_cgi_handler, api_uri_table },
};

static const tPath api_uri_table[] =
{
    { "/api", state_cgi_handler },
};

int fs_open_custom(struct fs_file *file, const char *name) {


}

void fs_close_custom(struct fs_file *file) {

}