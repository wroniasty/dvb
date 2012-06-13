#include <string.h>
#include <typeinfo>
#include <boost/lexical_cast.hpp>

#include "descriptors.h"

namespace dvb {

namespace si {

  void cable_delivery_system_descriptor::read(descriptor & d, bits::bitstream & source) {
      frequency = dvb::bcd2i(source.read<unsigned> (32));
      source.skip (12);
      FEC_outer = source.read<unsigned> (4);
      modulation = source.read<unsigned> (8);
      symbol_rate = dvb::bcd2i(source.read<unsigned> (28));
      FEC_inner = source.read<unsigned> (4);
  }
  
  void cable_delivery_system_descriptor::write(descriptor & d, bits::bitstream & dest) {
        dest.write (32, dvb::i2bcd(frequency));
        dest.write (12, 0xfff);
        dest.write (4, FEC_outer);
        dest.write (8, modulation);
        dest.write (28, dvb::i2bcd(symbol_rate));
        dest.write (4, FEC_inner);
  }

  unsigned cable_delivery_system_descriptor::get_length() {
      return 11;
  }
  
  std::string cable_delivery_system_descriptor::to_string() const {
      std::string s ("cable delivery ");
      s += boost::lexical_cast<std::string>( frequency ) + " MHz";
      s += " modulation: " + boost::lexical_cast<std::string> (modulation);
      s += " symbol rate: " + boost::lexical_cast<std::string> (symbol_rate);
      return s;
  }
  
  
}

}
