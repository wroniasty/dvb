#include "sections.h"

namespace dvb {

namespace si {

  void nit_section::read_contents (bits::bitstream & source) {
    network_id = source.read<unsigned> (16);
    source.skip(2);
    version_number = source.read<unsigned> (5);
    current_next_indicator = source.read<unsigned> (1);
    section_number = source.read<unsigned> (8);
    last_section_number = source.read<unsigned> (8);
    source.skip(4);

    unsigned network_descriptors_length = source.read<unsigned> (12);
    network_descriptors.clear();    
    while (network_descriptors_length > 0) {
        dvb::si::descriptor * d = new dvb::si::descriptor(); d->read(source);
        network_descriptors.push_back ( Poco::SharedPtr <dvb::si::descriptor> (d) );
        network_descriptors_length -= d->get_length();
    }
    
    source.skip(4);
    unsigned transport_stream_loop_length = source.read<unsigned> (12);
    while (transport_stream_loop_length > 0) {
        transport_stream * ts = new transport_stream;
        ts->transport_stream_id = source.read<unsigned> (16);
        ts->original_network_id = source.read<unsigned> (16);
        source.skip(4);
        unsigned transport_descriptors_length = source.read<unsigned> (12);
        transport_stream_loop_length -= (transport_descriptors_length + 6);
        while (transport_descriptors_length > 0) {
          dvb::si::descriptor * d = new dvb::si::descriptor(); d->read(source);
          ts->descriptors.push_back( Poco::SharedPtr<dvb::si::descriptor> (d) );
          transport_descriptors_length -=  d->get_length();
        }
    }        
    crc32 = source.read<unsigned> (32); /* CRC32 */
  }

  void nit_section::write_contents (bits::bitstream & dest) {
  }

}

}
