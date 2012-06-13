#ifndef _DVB_DESC_SHORT_EV_H_
#define _DVB_DESC_SHORT_EV_H_ 1

#include "bits/bits-stream.h"
#include "descriptors.h"

namespace dvb {

namespace si {


  class short_event_descriptor : public descriptor_syntax {
    public:
    const static unsigned char tag = 0x4d;
    std::string ISO_639_language_code;
    std::string event_name;
    std::string text;
    virtual void read(descriptor & d, bits::bitstream & source);
    virtual void write(descriptor & d, bits::bitstream & dst);
    virtual std::string to_string() const;
    virtual unsigned get_length(); 

  };

}

}

#endif
