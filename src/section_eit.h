#ifndef _DVB_SECTIONS_EIT
#define _DVB_SECTIONS_EIT_ 1

#include "sections.h"

namespace dvb {

namespace si {


  class eit_section : public section {
     protected:

     virtual void read_contents (bits::bitstream & source);
     virtual void write_contents (bits::bitstream & dest);

     public:

     class event {
     public:
         void write (bits::bitstream & dest);
         void read (bits::bitstream & read);
         unsigned calculate_length();

         unsigned event_id;
         Poco::DateTime start_time;
         Poco::Timespan duration;
         unsigned running_status;
         unsigned free_CA_mode;
         unsigned descriptors_loop_length;
         descriptors_v descriptors;
     };
     typedef std::vector<Poco::SharedPtr<event> > events_v;

     events_v events;

     unsigned service_id;
     unsigned version_number;
     unsigned current_next_indicator;
     unsigned section_number;
     unsigned last_section_number;
     unsigned transport_stream_id;
     unsigned original_network_id;
     unsigned segment_last_section_number;
     unsigned last_table_id;

     const static unsigned max_length = 4096;

     unsigned calculate_section_length();

  };
  
}

}

#endif
