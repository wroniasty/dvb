#ifndef _DVB_DESC_CABLE_DEL_H_
#define _DVB_DESC_CABLE_DEL_H_ 1

#include "bits/bits-stream.h"
#include "descriptors.h"

namespace dvb {

namespace si {


  class cable_delivery_system_descriptor : public descriptor_syntax {
    public:
    const static unsigned char tag = 0x44;

    unsigned frequency;
    unsigned FEC_outer;
    unsigned modulation;
    unsigned symbol_rate;
    unsigned FEC_inner;

    virtual void read(descriptor & d, bits::bitstream & source);
    virtual void write(descriptor & d, bits::bitstream & dst);

    virtual std::string to_string() const;        
    virtual unsigned get_length(); 

    //virtual void read(descriptor & d, bits::bitstream & source);
  };

}

}

#endif
