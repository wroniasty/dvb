#include <boost/foreach.hpp>
#include "sections.h"

namespace dvb {

namespace si {
   eit_section::event::event()
    : event_id(0), running_status(1), free_CA_mode(1), descriptors_loop_length(0)           
   {}
   
   eit_section::event::event(unsigned _event_id, Poco::DateTime _start_time, 
         Poco::Timespan _duration, std::string language,
         std::string title, std::string short_text, std::string long_text)
    : event_id (_event_id), start_time(_start_time), duration(_duration),
      running_status(1), free_CA_mode(1), descriptors_loop_length(0) {

      dvb::si::descriptor_p d_short ( new dvb::si::descriptor );
      Poco::SharedPtr<dvb::si::short_event_descriptor> d_short_data =
              d_short->set_data<dvb::si::short_event_descriptor> ();
      
      d_short_data->ISO_639_language_code = language;
      d_short_data->event_name = title;
      d_short_data->text = short_text;
      descriptors.push_back(d_short);

      std::size_t offset = 0;
      /*
//      while (offset < long_text.size()) {
          dvb::si::descriptor_p d_ext ( new dvb::si::descriptor );
          Poco::SharedPtr<dvb::si::extended_event_descriptor> d_ext_data =
              d_ext->set_data<dvb::si::extended_event_descriptor> ();
          d_ext_data->descriptor_number = 0;
          d_ext_data->last_descriptor_number = 0;
          d_ext_data->ISO_639_language_code = language;
          d_ext_data->add_item("2012", "rok");
          d_ext_data->add_item("sensacha", "rodzaj");
          d_ext_data->text = dvb::string_encoding::encode("HELLO HELLO HELLO");
          descriptors.push_back(d_ext);          
//          offset += 253;
//      }
*/
       
   }
   
   

  void eit_section::event::read(bits::bitstream& source) {
     event_id = source.read<unsigned> (16);
     start_time = dvb::read_mjd_datetime(source);
     duration = dvb::read_bcd_time(source);

     running_status = source.read<unsigned> (3);
     free_CA_mode = source.read<unsigned> (1);
     descriptors_loop_length = source.read<unsigned> (12);

     dvb::util::position counter(source); 
     std::size_t descriptors_start = source.position();
     descriptors.clear();
     while (counter() < descriptors_loop_length) {
       dvb::si::descriptor * d = new dvb::si::descriptor(); d->read(source);
       descriptors.push_back ( Poco::SharedPtr <dvb::si::descriptor> (d) );
     }
     
  }

  void eit_section::event::write(bits::bitstream& dest) {      
      dest.write(16, event_id);
      dvb::write_mjd_datetime(dest, start_time);
      dvb::write_bcd_time(dest, duration);
      dest.write(3, running_status);
      dest.write(1, free_CA_mode);
      dest.skip(12);
      std::size_t descriptors_start = dest.position();
      dvb::util::position counter(dest); 
      BOOST_FOREACH ( Poco::SharedPtr<dvb::si::descriptor> d, descriptors) {
          d->write( dest );
      };
      descriptors_loop_length = counter();
      dest.write_at(descriptors_start - 12, 12, descriptors_loop_length);
  }

  unsigned eit_section::event::calculate_length() {
      unsigned l = 12;
      BOOST_FOREACH ( Poco::SharedPtr<dvb::si::descriptor> d, descriptors) {
          l += d->get_length();
      };
      return l;      
  }

  eit_section::eit_section() : section(),
          segment_last_section_number(0),
          service_id(0), version_number(0), current_next_indicator(0),
          section_number(0), last_section_number(0), transport_stream_id(0),
          original_network_id(0), 
          last_table_id(0)
  {
      max_length = 4096; 
  }

  
  void eit_section::read_contents (bits::bitstream & source) {
    dvb::util::position counter(source);  
    service_id = source.read<unsigned> (16);
    source.skip(2);
    version_number = source.read<unsigned> (5);
    current_next_indicator = source.read<unsigned> (1);
    section_number = source.read<unsigned> (8);
    last_section_number = source.read<unsigned> (8);
    transport_stream_id = source.read<unsigned> (16);
    original_network_id = source.read<unsigned> (16);
    segment_last_section_number = source.read<unsigned> (8);
    last_table_id = source.read<unsigned> (8);

    events.clear();
    while ( counter() < (section_length - 4) ) {
        event_p e ( new event );
        e->read(source);
        events.push_back ( e );
    }
    _valid = ( counter() == section_length - 4 );
    crc32 = source.read<unsigned> (32); /* CRC32 */

  }

  void eit_section::write_contents (bits::bitstream & dest) {
      dvb::util::position counter(dest);
      dest.write(16, service_id);
      dest.write (2, 0xff);
      dest.write (5, version_number);
      dest.write (1, current_next_indicator);
      dest.write (8, section_number);
      dest.write (8, last_section_number);
      dest.write (16, transport_stream_id);
      dest.write (16, original_network_id);
      dest.write (8, segment_last_section_number);
      dest.write (8, last_table_id);
      
      BOOST_FOREACH( Poco::SharedPtr<event> e, events) {
         e->write( dest );  
      };
      assert ( counter() == calculate_section_length() - 4 );
  }

  int eit_section::add_event(event_p e) {      
      if (calculate_section_length() + e->calculate_length() > max_length - 2) {
          return 0;
      } else {
          events.push_back(e);
      }
      return 1;
  }
  
  int eit_section::add_event(unsigned event_id, Poco::DateTime start_time, 
          Poco::Timespan duration, std::string language, 
          std::string title, std::string short_text, std::string long_text, 
          std::string charset) {
      event_p e = make_event ( event_id, start_time, duration, language,
              title, short_text, long_text, charset);
      return add_event (e);
  } 
  
  eit_section::event_p eit_section::make_event ( 
        unsigned event_id, Poco::DateTime start_time, 
        Poco::Timespan duration, std::string language,
        std::string title, std::string short_text, std::string long_text        
        )
  {
      return make_event (event_id, start_time, duration, language, title, short_text, long_text, "iso-8859-2");
  }

  eit_section::event_p eit_section::make_event ( 
        unsigned event_id, Poco::DateTime start_time, 
        Poco::Timespan duration, std::string language,
        std::string title, std::string short_text, std::string long_text,
        std::string charset
        )
  {      

      event_p e ( new event (event_id, start_time, duration, language, 
              dvb::string_encoding::encode(charset, title), 
              dvb::string_encoding::encode(charset, short_text), 
              dvb::string_encoding::encode(charset, long_text) )
      );
      
      return e;
  }

  unsigned eit_section::calculate_section_length() {
      unsigned l = 15;
      BOOST_FOREACH ( Poco::SharedPtr<event> e, events) {
          l += e->calculate_length();
      };
      return l;      
  }
  
  eit_section_v eit_prepare_present_following ( 
          unsigned service_id, 
          unsigned version_number, 
          unsigned current_next,
          unsigned transport_stream_id,
          unsigned original_network_id,
          eit_section::event_p present_event,
          eit_section::event_p following_event          
          ) 
   {
      eit_section_v pf;
      
      eit_section_p present ( new eit_section );      
      present->table_id = 0x4e;
      present->last_table_id = 0x4e;
      present->service_id = service_id;
      present->version_number = version_number;
      present->current_next_indicator = current_next;
      present->section_syntax_indicator = 1;
      present->section_number = 0;
      present->last_section_number = 1;
      present->original_network_id = original_network_id;
      if (present_event)
         present->add_event(present_event);

      eit_section_p following ( new eit_section );      
      following->table_id = 0x4e;
      following->last_table_id = 0x4e;
      following->service_id = service_id;
      following->version_number = version_number;
      following->current_next_indicator = current_next;
      following->section_syntax_indicator = 1;
      following->section_number = 1;
      following->last_section_number = 1;
      following->original_network_id = original_network_id;
      
      if (following_event)
        following->add_event ( following_event );

      pf.push_back (present);
      pf.push_back (following);
      
      return pf;      
  }

  bool compare_event_p ( eit_section::event_p e1, eit_section::event_p e2 ) {
      return e1->start_time < e2->start_time;
  }
  
  eit_section_v eit_prepare_schedule ( 
          unsigned service_id, 
          unsigned version_number, 
          unsigned current_next,
          unsigned transport_stream_id,
          unsigned original_network_id,
          eit_section::event_v events
          ) 
   {
      eit_section_v sched;      
      
      std::sort ( events.begin(), events.end(), compare_event_p );
      
      
      
      
      return sched;
   }
  
}

}
