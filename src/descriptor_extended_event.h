#ifndef _DVB_DESC_EXT_EV_H_
#define _DVB_DESC_EXT_EV_H_ 1
#include "bits/bits-stream.h"
#include "descriptors.h"

namespace dvb {

namespace si {

  class extended_event_descriptor : public descriptor_syntax {
    public:
    const static unsigned char tag = 0x4e;

    typedef struct {
        std::string description;
        std::string item;
    } item;
    typedef std::vector< Poco::SharedPtr<item> > items_v;

    unsigned descriptor_number;
    unsigned last_descriptor_number;
    std::string ISO_639_language_code;
    items_v items;
    std::string text;

    virtual void read(descriptor & d, bits::bitstream & source);
    virtual void write(descriptor & d, bits::bitstream & dst);
    virtual std::string to_string() const;

    void add_item ( std::string descriptiom, std::string item);
    
    virtual unsigned get_length(); 

    //virtual void read(descriptor & d, bits::bitstream & source);
  };


}

}

#endif
