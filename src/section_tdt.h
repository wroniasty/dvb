#ifndef _DVB_SECTIONS_TDT
#define _DVB_SECTIONS_TDT_ 1

#include "sections.h"

namespace dvb {

namespace si {


  class tdt_section : public section {
     protected:
    
     virtual void read_contents (bits::bitstream & source);
     virtual void write_contents (bits::bitstream & dest);

     public:
 
     Poco::DateTime utc;
     tdt_section(); 
     const static unsigned default_table_id = 0x70;

     virtual unsigned calculate_section_length();
     
  };

}

}

#endif
