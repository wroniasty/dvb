#include <boost/foreach.hpp>
#include "section_tot.h"

namespace dvb {

namespace si {

   tot_section::tot_section() : section() {
      table_id = default_table_id;
      section_syntax_indicator = 0;
  }
    
  void tot_section::read_contents (bits::bitstream & source) {
    unsigned mjd;
    mjd = source.read <unsigned> (16);
    utc = dvb::fromMJD(mjd);
    unsigned h,m,s;
    h = dvb::bcd2i( source.read<unsigned> (8) );
    m = dvb::bcd2i( source.read<unsigned> (8) );
    s = dvb::bcd2i( source.read<unsigned> (8) );
    utc.assign( utc.year(), utc.month(), utc.day(), h,m,s);
    
    source.skip(4);
    unsigned descriptors_loop_length = source.read<unsigned> (12);
    
    dvb::util::position counter(source);
    descriptors.clear();
    while (counter() < descriptors_loop_length) {
       dvb::si::descriptor * d = new dvb::si::descriptor(); d->read(source);
       descriptors.push_back ( Poco::SharedPtr <dvb::si::descriptor> (d) );        
    }
    
    crc32 = source.read<unsigned> (32);
    
  }

  void tot_section::write_contents (bits::bitstream & dest) {
      unsigned mjd = dvb::MJD( utc.year(), utc.month(), utc.day() );
      dest.write (16, mjd );
      dest.write (8, dvb::i2bcd(utc.hour()) );
      dest.write (8, dvb::i2bcd(utc.minute()) );
      dest.write (8, dvb::i2bcd(utc.second()) );
      dest.write (4, 0xf);
      std::size_t length_position = dest.position();
      dest.skip(12);
      dvb::util::position counter(dest);
      BOOST_FOREACH (dvb::si::descriptor_p d, descriptors) {
          d->write (dest);
      }
      dest.write_at(length_position, 12, counter());
  }
  
  void tot_section::add_offset ( std::string code, unsigned region_id, 
        unsigned polarity, unsigned offset, unsigned long long time_of_change, 
        unsigned next_time_offset ) {
      dvb::si::descriptor_p d ( new dvb::si::descriptor );      
      Poco::SharedPtr<dvb::si::local_time_offset_descriptor>  
         lto ( d->set_data<dvb::si::local_time_offset_descriptor>() );            
      lto->add_offset(code, region_id, polarity, offset, time_of_change, next_time_offset);      
      descriptors.push_back(d);
  }
  
  unsigned tot_section::calculate_section_length() {
      unsigned l = 7 + 4;
      BOOST_FOREACH ( Poco::SharedPtr<dvb::si::descriptor> d, descriptors) {
          l += d->get_length();
      };
      return l;    
  }

}

}
