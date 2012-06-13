#ifndef _DVB_SECTIONS_SDT
#define _DVB_SECTIONS_SDT_ 1

#include "sections.h"

namespace dvb {

namespace si {

  class sdt_section : public section {
     protected:

     virtual void read_contents (bits::bitstream & source);
     virtual void write_contents (bits::bitstream & dest);

     public:

     typedef struct {
        unsigned service_id;
        unsigned EIT_schedule_flag;
        unsigned EIT_present_following_flag;
        unsigned running_status;
        unsigned free_CA_mode;
        descriptors_v descriptors;
     } service_info;
     typedef std::vector<Poco::SharedPtr<service_info> > service_info_v;


     unsigned transport_stream_id;
     unsigned version_number;
     unsigned current_next_indicator;
     unsigned section_number;
     unsigned last_section_number;
     unsigned original_network_id;

     service_info_v services;

  };

}

}

#endif
