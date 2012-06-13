#ifndef _DVB_SECTIONS_PAT_
#define _DVB_SECTIONS_PAT_ 1

#include "sections.h"

namespace dvb {

namespace si {

  class pat_section : public section {
     protected:

     virtual void read_contents (bits::bitstream & source);
     virtual void write_contents (bits::bitstream & dest);

     public:

     pat_section();
         
     typedef struct { unsigned program_number, program_map_pid; } program;
     typedef std::vector<Poco::SharedPtr<program> > programs_v;

     programs_v programs;

     unsigned transport_stream_id;
     unsigned version_number;
     unsigned current_next_indicator;
     unsigned section_number;
     unsigned last_section_number;

     unsigned calculate_section_length();
     int add_program ( unsigned program_number, unsigned program_map_pid );
     void remove_program ( unsigned program_number );
  };

}

}

#endif
