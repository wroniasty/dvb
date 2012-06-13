#ifndef _DVB_SECTIONS_TOT_
#define _DVB_SECTIONS_TOT_ 1

#include "sections.h"

namespace dvb {

namespace si {


  class tot_section : public section {
     protected:

     virtual void read_contents (bits::bitstream & source);
     virtual void write_contents (bits::bitstream & dest);

     public:

     tot_section();
         
     Poco::DateTime utc;
     descriptors_v descriptors;
     unsigned crc32;
     
     const static unsigned default_table_id = 0x73;

     virtual unsigned calculate_section_length();

     void add_offset ( std::string code, unsigned region_id=0, 
        unsigned polarity=0, unsigned offset=0, unsigned long long time_of_change=0, 
        unsigned next_time_offset=0 );
     
  };

}

}

#endif
