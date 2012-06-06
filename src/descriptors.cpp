#include <string.h>

#include "descriptors.h"

namespace dvb {

namespace si {

  descriptor::descriptor () : tag(0), length(0) {}

  void descriptor::read (bits::bitstream & source) {
    read_header(source);
    read_data(source);
  }

  void descriptor::read (std::vector<unsigned char> & buffer) {
    bits::bitstream stream ( (unsigned char *) &buffer[0] );
    read ( stream );
  }


  void descriptor::read_header (bits::bitstream & source) {
    tag = source.read<unsigned char> ( 8 );
    length = source.read<unsigned char> ( 8 );
  }

  void descriptor::read_data (bits::bitstream & source) {
    descriptor_syntax * d;
    switch (tag) {
        case 0x4d:
            d = new short_event_descriptor;
            break;
        case 0x4e:
            d = new extended_event_descriptor;
            break;
        default:
            d = new descriptor_syntax;
    }
    d->read (*this, source);
    data = Poco::SharedPtr<descriptor_syntax> (d);
  }

  void descriptor_syntax::read(descriptor & d, bits::bitstream & source) {
     source.skip (d.length*8);
  }

  void descriptor_syntax::write(descriptor & d, bits::bitstream & dest) {
     dest.skip (d.length*8);
  }

}

}
