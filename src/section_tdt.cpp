#include "sections.h"

namespace dvb {

namespace si {
  tdt_section::tdt_section() : section() {
      table_id = default_table_id;
      section_syntax_indicator = 0;
      _check_crc = false;
      has_crc = false;
  }
    
  void tdt_section::read_contents (bits::bitstream & source) {
    unsigned mjd;
    mjd = source.read <unsigned> (16);
    utc = dvb::fromMJD(mjd);
    unsigned h,m,s;
    h = dvb::bcd2i( source.read<unsigned> (8) );
    m = dvb::bcd2i( source.read<unsigned> (8) );
    s = dvb::bcd2i( source.read<unsigned> (8) );
    utc.assign( utc.year(), utc.month(), utc.day(), h,m,s);
  }

  void tdt_section::write_contents (bits::bitstream & dest) {
      unsigned mjd = dvb::MJD( utc.year(), utc.month(), utc.day() );
      dest.write (16, mjd );
      dest.write (8, dvb::i2bcd(utc.hour()) );
      dest.write (8, dvb::i2bcd(utc.minute()) );
      dest.write (8, dvb::i2bcd(utc.second()) );
  }

  unsigned tdt_section::calculate_section_length() {
      return 5;
  }


}

}
