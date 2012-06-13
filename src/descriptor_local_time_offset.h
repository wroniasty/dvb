#ifndef _DVB_DESC_LOC_TIM_OFF_H_
#define _DVB_DESC_LOC_TIM_OFF_H_ 1

#include "bits/bits-stream.h"
#include "descriptors.h"

namespace dvb {

namespace si {


  class local_time_offset_descriptor : public descriptor_syntax {
    public:
    const static unsigned char tag = 0x58;

    typedef struct {
      std::string code;
      unsigned region_id;
      unsigned local_time_offset_polarity;
      unsigned local_time_offset;
      unsigned long long time_of_change;
      unsigned next_time_offset;
    } _offset_t;
    
    typedef Poco::SharedPtr <_offset_t> offset_p;
    typedef vector<offset_p> offset_v;

    offset_v offsets;

    virtual void read(descriptor & d, bits::bitstream & source);
    virtual void write(descriptor & d, bits::bitstream & dst);

    virtual std::string to_string() const;        
    virtual unsigned get_length(); 

    offset_p add_offset ( std::string code, unsigned region_id=0, 
        unsigned polarity=0, unsigned offset=0, unsigned long long time_of_change=0, 
        unsigned next_time_offset=0 );
    
    //virtual void read(descriptor & d, bits::bitstream & source);
  };

}

}

#endif
