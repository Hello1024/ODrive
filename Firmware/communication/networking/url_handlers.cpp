#include "url_handlers.hpp"

#include <string.h>

static const char response[] = "HTTP/1.0 200 OK\r\nContent-type: text/plain\r\n\r\n";


// Discards all bytes give except between start and end given.  Writes resulting bytes to given buffer.
class PartitioningStreamSink : public StreamSink {
public:
    PartitioningStreamSink(size_t start, size_t len, uint8_t* buf) :
        start_(start),
        len_(len),
        buf_(buf)
    {}

    int process_bytes(const uint8_t* buffer, size_t length, size_t* processed_bytes) {
    	if (processed_bytes)
            *processed_bytes += length;
        int return_val = (int)length;

    	if (start_>length) {
    		start_ -= length;
    		return return_val;
    	}
    	length -= start_;
    	buffer += start_;

        if (length>len_) length = len_;

    	// Got to the data we want!
    	memcpy(buf_, buffer, length);

    	buf_ += length;
    	len_ -= length;

    	if (len_==0)
    		return -1;
    	return return_val;
    }

    size_t get_free_space() { return SIZE_MAX; }
    size_t len() { return len_; }

private:
    size_t start_;
    size_t len_;
    uint8_t* buf_;
};


int fs_open_custom(struct fs_file *file, const char *name) {
    Endpoint* endpoint = application_endpoints_->get_by_name(name+1, strlen(name+1));
	if (!endpoint) {
	    return 0;
	} else {
		int bufsize = 800;

	    char* data = new char[bufsize];
	    file->data = data;
	    
	    strncpy(data, response, 80);
	    data += strlen(response);
	    int len = bufsize - strlen(response);

        PartitioningStreamSink a(0, len, (uint8_t*)data);

	    application_endpoints_->write_json(0, &a);

    	file->len = bufsize - a.len();
        file->index = file->len;
        file->pextension = NULL;
        file->http_header_included = 1;

        return 1;
	}

}

void fs_close_custom(struct fs_file *file) {
	delete file->data;
}