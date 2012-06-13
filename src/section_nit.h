#ifndef _DVB_SECTIONS_NIT
#define _DVB_SECTIONS_NIT_ 1

#include "sections.h"

namespace dvb {

namespace si {

  class nit_section : public section {
     protected:

     virtual void read_contents (bits::bitstream & source);
     virtual void write_contents (bits::bitstream & dest);

     public:

     typedef struct {
        unsigned transport_stream_id;
        unsigned original_network_id;
        descriptors_v descriptors;
     } transport_stream;
     typedef std::vector<Poco::SharedPtr<transport_stream> > transport_stream_v;

     transport_stream_v transport_streams;

     unsigned network_id;
     unsigned version_number;
     unsigned current_next_indicator;
     unsigned section_number;
     unsigned last_section_number;
     
     descriptors_v network_descriptors;
     
  };

}

}

#endif
