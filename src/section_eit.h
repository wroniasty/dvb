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
         
         event();
         event(unsigned _event_id, Poco::DateTime _start_time, 
               Poco::Timespan _duration, std::string language,
               std::string title, std::string short_text, std::string long_text);
         
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
     typedef Poco::SharedPtr<event> event_p;
     typedef std::vector<event_p > event_v;
     typedef std::vector<event_p > events_v;

     event_v events; 

     eit_section();
     
     unsigned service_id;
     unsigned version_number;
     unsigned current_next_indicator;
     unsigned section_number;
     unsigned last_section_number;
     unsigned transport_stream_id;
     unsigned original_network_id;
     unsigned segment_last_section_number;
     unsigned last_table_id;

     int add_event ( event_p e );
     int add_event ( unsigned event_id, Poco::DateTime start_time, 
        Poco::Timespan duration, std::string language,
        std::string title, std::string short_text, std::string long_text,
        std::string charset );
     
          
     static event_p make_event ( unsigned event_id, Poco::DateTime start_time, 
        Poco::Timespan duration, std::string language,
        std::string title, std::string short_text, std::string long_text        
        );

     static event_p make_event ( unsigned event_id, Poco::DateTime start_time, 
        Poco::Timespan duration, std::string language,
        std::string title, std::string short_text, std::string long_text,
        std::string charset
        );
     
     unsigned calculate_section_length();

  };
  
  typedef Poco::SharedPtr<eit_section> eit_section_p;
  typedef std::vector<eit_section_p> eit_section_v;
  
  eit_section_v eit_prepare_present_following ( 
          unsigned service_id, 
          unsigned version_number, 
          unsigned current_next,
          unsigned transport_stream_id,
          unsigned original_network_id,
          eit_section::event_p present,
          eit_section::event_p following          
          );

  eit_section_v eit_prepare_schedule ( 
          unsigned service_id, 
          unsigned version_number, 
          unsigned current_next,
          unsigned transport_stream_id,
          unsigned original_network_id,
          eit_section::event_v events
  );

}

}

#endif
