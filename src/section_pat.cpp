#include "section_pat.h"

namespace dvb {

namespace si {
  
  pat_section::pat_section () : 
        section(), 
        transport_stream_id(0), 
        version_number(0), current_next_indicator(0),
        section_number(0), last_section_number(0) {
      
  }
 

  void pat_section::read_contents (bits::bitstream & source) {
    dvb::util::position counter(source);
    transport_stream_id = source.read<unsigned> (16);
    source.skip(2);
    version_number = source.read<unsigned> (5);
    current_next_indicator = source.read<unsigned> (1);
    section_number = source.read<unsigned> (8);
    last_section_number = source.read<unsigned> (8);
    programs.clear();
    while ( counter() < section_length - 4) {
        Poco::SharedPtr<program> p (new program);
        p->program_number = source.read<unsigned>(16);
        source.skip(3);
        p->program_map_pid = source.read<unsigned>(13);
        programs.push_back ( p );
    }
    crc32 = source.read<unsigned> (32); /* CRC32 */
  }

  void pat_section::write_contents (bits::bitstream & dest) {
      dest.write (16, transport_stream_id);
      dest.write (2, 0x3);
      dest.write (5, version_number);
      dest.write (1, current_next_indicator);
      dest.write (8, section_number);
      dest.write (8, last_section_number);
      /* size = 5 */
      for (programs_v::iterator it = programs.begin(); it != programs.end(); it++) {
          dest.write (16, (*it)->program_number);
          dest.write (3, 0x7);
          dest.write (13, (*it)->program_map_pid);
      }
  }

  unsigned pat_section::calculate_section_length() {
      unsigned l = 5 + 4 * programs.size() + 4; /*crc*/;
      return l;      
  }

  int pat_section::add_program(unsigned program_number, unsigned program_map_pid) {
      if (calculate_section_length() + 4 < max_length ) {
        program * p = new program;
        p->program_number = program_number;
        p->program_map_pid = program_map_pid;
        programs.push_back ( Poco::SharedPtr<program> (p) );
        return 1;
      } else {
        return 0;
      }
  }

  void pat_section::remove_program(unsigned program_number) {
      for ( programs_v::iterator i = programs.begin(); 
              i != programs.end(); i++) {
          if ( (*i)->program_number == program_number) {
              i = programs.erase(i);
          }
      }
  }
  

}

}
