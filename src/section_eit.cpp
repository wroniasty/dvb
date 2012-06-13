#include <boost/foreach.hpp>
#include "sections.h"

namespace dvb {

namespace si {
  
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
        event * e = new event;
        e->read(source);
        events.push_back ( Poco::SharedPtr<event> (e) );
    }
    crc32 = source.read<unsigned> (32); /* CRC32 */

  }

  void eit_section::write_contents (bits::bitstream & dest) {
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
  }

  unsigned eit_section::calculate_section_length() {
      unsigned l = 14;
      BOOST_FOREACH ( Poco::SharedPtr<event> e, events) {
          l += e->calculate_length();
      };
      return l;      
  }
  

}

}
