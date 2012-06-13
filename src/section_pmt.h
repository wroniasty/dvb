#ifndef _DVB_SECTIONS_PMT
#define _DVB_SECTIONS_PMT_ 1

#include "sections.h"

namespace dvb {

namespace si {

  class pmt_section : public section {
     protected:

     virtual void read_contents (bits::bitstream & source);
     virtual void write_contents (bits::bitstream & dest);
     public:
     
     pmt_section();
         
     typedef struct {
        unsigned stream_type;
        unsigned elementary_PID;
        unsigned info_length;
        descriptors_v info;
     } es_info;

     typedef std::vector<Poco::SharedPtr<es_info> > es_info_v;
     unsigned program_number;
     unsigned version_number;
     unsigned current_next_indicator;
     unsigned section_number;
     unsigned last_section_number;
     unsigned PCR_PID;
     unsigned program_info_length;
     descriptors_v program_info;
     es_info_v info;

  };

}

}

#endif
