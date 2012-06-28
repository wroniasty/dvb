#include <string.h>
#include <typeinfo>
#include <boost/lexical_cast.hpp>

#include "descriptors.h"

namespace dvb {

namespace si {

  descriptor::descriptor () : tag(0), length(0) {}
  descriptor::~descriptor () {}
  
  descriptor_syntax * descriptor::new_syntax () {
      descriptor_syntax * d;
      switch (tag) {
        case 0x4d:
            d = new short_event_descriptor;
            break;
        case 0x4e:
            d = new extended_event_descriptor;
            break;
        case 0x44:
            d = new cable_delivery_system_descriptor;
            break;
        case 0x58:
            d = new local_time_offset_descriptor;
            break;
        default:
            d = new descriptor_syntax;
    }      
    return d;
  }
  
   
  unsigned descriptor::get_length() {
      return data->get_length() + 2;
  }

  void descriptor::read (bits::bitstream & source) {
    read_header(source);
    read_data(source);
  }

  void descriptor::read (std::vector<unsigned char> & buffer) {
    bits::bitstream stream ( (unsigned char *) &buffer[0] );
    read ( stream );
  }


  void descriptor::read_header (bits::bitstream & source) {
    tag = source.read<unsigned> ( 8 );
    length = source.read<unsigned> ( 8 );
  }

  void descriptor::read_data (bits::bitstream & source) {
    descriptor_syntax * d = new_syntax();
    d->read (*this, source);
    data = Poco::SharedPtr<descriptor_syntax> (d);
  }

  void descriptor::write (bits::bitstream & dest) {
      dest.write (8, tag);
      length = data->get_length();
      dest.write (8, length);
      data->write( (*this), dest );
  }

  std::ostream & operator<<(std::ostream & o, const descriptor & d) {
      return o << std::hex << "0x" << (int)d.tag << std::dec << " (" << (int)d.length << ") " << d.data->to_string();
  }

  descriptor_syntax::~descriptor_syntax() {}
  
  void descriptor_syntax::read(descriptor & d, bits::bitstream & source) {
     source.skip (d.length*8);
  }

  void descriptor_syntax::write(descriptor & d, bits::bitstream & dest) {
     dest.skip (d.length*8);
  }
  
  std::string descriptor_syntax::to_string() const {
      return std::string(typeid(*this).name());
  }

  unsigned descriptor_syntax::get_length() {
      return 0;
  }

 

  
  
}

}
