#include <cstring>
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
      table_id = default_table_id;
  }
  
  section::~section() {}
  
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

      if (length + 3 > max_length) { _valid = false; return SECTION_SIZE_INVALID; }
      
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
    else
        return SECTION_INVALID;
  }

  int section::read (std::vector<unsigned char> & buffer) {
    bits::bitstream stream ( (unsigned char *) &buffer[0] );
    return read ( stream );
  }
  
  void section::read_header (bits::bitstream & source) {
    unsigned offset = source.read<unsigned> (8);
    source.skip ( offset * 8 );
//    _read_offset_0 = source.position();
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
//    _write_offset_0 = dest.position();
    dest.write (8, table_id);
    dest.write (1, section_syntax_indicator);
    dest.write (3, 0xf);
    dest.write (12, section_length = calculate_section_length() );
  }

  void section::write_contents (bits::bitstream & dest) {
  }

  void section::write (bits::bitstream & dest) {
    dvb::util::position counter(dest);
    
    unsigned p0 = dest.position();
    write_header(dest);
    write_contents(dest);
    
    if (has_crc) {
      dvb::mpeg::crc32_mpeg crc;
      crc.process_bytes(dest.ptr() + p0/8 + 1, section_length+3-4);     
      dest.write (32, crc.checksum());
    }
    
  }

  dvb::mpeg::packet_v section::serialize_to_mpegts( unsigned pid ) {
      unsigned char buffer[max_length+1];
      memset (buffer, 0, max_length+1);

      dvb::mpeg::packet_v packets;
      bits::bitstream stream(buffer);
      write ( stream );
      int offset = 0;
      unsigned cc = 0;
      
      while (offset <= calculate_section_length() + 3) {
          dvb::mpeg::packet_p p ( new dvb::mpeg::packet );
          p->PID = pid; p->continuityCounter = ( cc++ % 16 );
          p->payloadSize = p->max_payload_size();          
          p->copy_payload ( buffer + offset, std::min((unsigned) p->payloadSize, calculate_section_length() + 1 + 3 - offset ) );
          offset += p->payloadSize;
          packets.push_back (p);
      }
      
      if (packets.size() > 0) { 
          packets[0]->PUSI = 1;
      }
      
      return packets;
  }
  
  unsigned section::calculate_section_length() {
    return section_length;
  }
  
  bool section::check_validity() {
      return _valid;
  }

  bool section::is_valid() {
    return _valid;
  }


}

}
