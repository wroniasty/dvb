#include "Poco/ByteOrder.h"
#include "sections.h"

namespace dvb {

namespace si {
    
  unsigned peek_table_id (bits::bitstream & source) {
      return source.read<unsigned> (8);
  }

  unsigned peek_table_id (std::vector<unsigned char> & buffer) {
      return (unsigned) buffer[0];
  }

  section::section() : table_id(0), section_syntax_indicator(1),
			 section_length(0), _valid(true),
                         _check_crc(true)
  {
  }
  
  unsigned section::peek_section_length(bits::bitstream & source) {
      //unsigned offset = source.read_at<unsigned> (source.position(), 8);
      unsigned pos = source.position();

      unsigned offset = source.read<unsigned> (8);
      source.skip ( offset * 8 );
      table_id = source.read<unsigned char> ( 8 );
      section_syntax_indicator = source.read<unsigned char> (1);
      source.skip (3);
      section_length = source.read<unsigned> (12);
      source.seek(pos);
      return section_length; //source.read_at<unsigned> (source.position() + 8 + offset*8 + 12, 12);
  }

  int section::read (bits::bitstream & source) {
    _valid = true;
    /* source should be byte-aligned */  
    if (source.position() % 8 != 0) { 
        _valid = false; return SECTION_ALIGNMENT_ERROR; 
    }
    
    if (_check_crc) {
      unsigned length = peek_section_length (source);
      unsigned offset = source.peek<unsigned>(8);

      dvb::mpeg::crc32_mpeg crc;
      crc.process_bytes(source.ptr() + source.position()/8 + 1 + offset, length-1);     
      unsigned actualcrc = source.read_at<unsigned> ( source.position() + 8 + offset*8 + length*8 - 8, 32 );

      if (actualcrc != crc.checksum()) {
        _valid = false; return SECTION_CRC_ERROR;
      }
    }
    read_header(source);
    read_contents(source);
    if (check_validity())
        return SECTION_OK;
  }

  int section::read (std::vector<unsigned char> & buffer) {
    bits::bitstream stream ( (unsigned char *) &buffer[0] );
    return read ( stream );
  }

  void section::read_header (bits::bitstream & source) {
    unsigned offset = source.read<unsigned> (8);
    source.skip ( offset * 8 );
    _read_offset_0 = source.position();
    table_id = source.read<unsigned char> ( 8 );
    section_syntax_indicator = source.read<unsigned char> (1);
    source.skip (3);
    section_length = source.read<unsigned> (12);
  }

  void section::read_contents (bits::bitstream & source) {
    source.skip (section_length*8);
  }

  void section::write_header (bits::bitstream & dest) {
    dest.write (8, 0);   /* pointer offset == 0 */  
    _write_offset_0 = dest.position();
    dest.write (8, table_id);
    dest.write (1, section_syntax_indicator);
    dest.write (3, 0);
    dest.write (12, get_section_length() );
  }

  void section::write_contents (bits::bitstream & dest) {
  }

  void section::write (bits::bitstream & dest) {
    write_header(dest);
    write_contents(dest);
  }

  unsigned section::get_section_length() {
    return section_length;
  }
  
  bool section::check_validity() {
      return _valid;
  }

  bool section::is_valid() {
    return _valid;
  }

  void pat_section::read_contents (bits::bitstream & source) {
    transport_stream_id = source.read<unsigned> (16);
    source.skip(2);
    version_number = source.read<unsigned> (5);
    current_next_indicator = source.read<unsigned> (1);
    section_number = source.read<unsigned> (8);
    last_section_number = source.read<unsigned> (8);
    programs.clear();
    while ( ((source.position() - _read_offset_0) / 8) < section_length - 1) {
        program * p = new program;
        p->program_number = source.read<unsigned>(16);
        source.skip(3);
        p->program_map_pid = source.read<unsigned>(13);
        programs.push_back ( Poco::SharedPtr<program> (p) );
    }
    crc32 = source.read<unsigned> (32); /* CRC32 */
  }

  void pat_section::write_contents (bits::bitstream & dest) {
      dest.write (16, transport_stream_id);
      dest.write (1, 0); dest.write (2, 0x3);
      dest.write (5, version_number);
      dest.write (1, current_next_indicator);
      dest.write (8, section_number);
      dest.write (8, last_section_number);
      
      for (programs_v::iterator it = programs.begin(); it != programs.end(); it++) {
          dest.write (16, (*it)->program_number);
          dest.write (3, 0x7);
          dest.write (13, (*it)->program_map_pid);
      }
  }

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

  void eit_section::read_contents (bits::bitstream & source) {
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
    while ( ((source.position() - _read_offset_0) / 8) < (section_length - 1) ) {
        event * e = new event;
        e->event_id = source.read<unsigned> (16);
        source.readstring(e->start_time, 40);
        source.readstring(e->duration, 24);
        e->running_status = source.read<unsigned> (3);
        e->free_CA_mode = source.read<unsigned> (1);
        e->descriptors_loop_length = source.read<unsigned> (12);
        std::size_t descriptors_start = source.position();
        while ((source.position() - descriptors_start) / 8 < e->descriptors_loop_length) {
            dvb::si::descriptor * d = new dvb::si::descriptor(); d->read(source);
            e->descriptors.push_back ( Poco::SharedPtr <dvb::si::descriptor> (d) );
        }
        events.push_back ( Poco::SharedPtr<event> (e) );
    }
    crc32 = source.read<unsigned> (32); /* CRC32 */

  }

  void eit_section::write_contents (bits::bitstream & dest) {
  }


}

}
