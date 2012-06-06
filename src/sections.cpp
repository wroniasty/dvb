#include <string.h>

#include "sections.h"

namespace dvb {

namespace si {

  section::section() : table_id(0), section_syntax_indicator(1),
			 section_length(0)
  {
  }

  void section::read (bits::bitstream & source) {
    read_header(source);
    read_contents(source);
  }

  void section::read (std::vector<unsigned char> & buffer) {
    bits::bitstream stream ( (unsigned char *) &buffer[0] );
    read ( stream );
  }

  void section::read_header (bits::bitstream & source) {
    unsigned offset = source.read<unsigned> (8);
    source.skip ( offset * 8 );
    table_id = source.read<unsigned char> ( 8 );
    section_syntax_indicator = source.read<unsigned char> (1);
    source.skip (3);
    section_length = source.read<unsigned> (12);
  }

  void section::read_contents (bits::bitstream & source) {
    source.skip (section_length*8);
  }

  void section::write_header (bits::bitstream & dest) {
    dest.write (8, table_id);
    dest.write (1, section_syntax_indicator);
    dest.write (3, 0);
    dest.write (12, get_section_length() );
  }

  void section::write_contents (bits::bitstream & dest) {
  }

  void section::write (bits::bitstream & dest) {
    write_header(dest);
  }

  unsigned section::get_section_length() {
    return section_length;
  }

  bool section::is_valid() {
    return true;
  }

  void pat_section::read_contents (bits::bitstream & source) {
    transport_stream_id = source.read<unsigned> (16);
    source.skip(2);
    version_number = source.read<unsigned> (5);
    current_next_indicator = source.read<unsigned> (1);
    section_number = source.read<unsigned> (8);
    last_section_number = source.read<unsigned> (8);
    programs.clear();
    while (source.position() / 8 < section_length - 4) {
        program * p = new program;
        p->program_number = source.read<unsigned>(16);
        source.skip(3);
        p->program_map_pid = source.read<unsigned>(13);
        programs.push_back ( Poco::SharedPtr<program> (p) );
    }
  }

  void pat_section::write_contents (bits::bitstream & dest) {
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
    while (source.position() / 8 < section_length - 4) {
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
  }

  void eit_section::write_contents (bits::bitstream & dest) {
  }


}

}
