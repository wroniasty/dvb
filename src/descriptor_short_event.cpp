#include <string.h>
#include <typeinfo>
#include <boost/lexical_cast.hpp>

#include "descriptors.h"

namespace dvb {

namespace si {

  void short_event_descriptor::read(descriptor & d, bits::bitstream & source) {
      ISO_639_language_code = source.readstring( 24 );
      
      unsigned name_length = source.read<unsigned> ( 8 );
      event_name = source.readstring (name_length*8);
      unsigned text_length = source.read<unsigned> ( 8 );
      text = source.readstring (text_length*8);
  }
  
  void short_event_descriptor::write(descriptor & d, bits::bitstream & dest) {
    if (event_name.length() > 128) event_name = event_name.substr(0, 128);
    if (text.length() > 128) text = text.substr(0, 128);

      dest.writestring ( ISO_639_language_code, 3);
      dest.write (8, event_name.size() );
      dest.writestring (event_name);
      dest.write (8, text.size() );
      dest.writestring (text);      
  }

  unsigned short_event_descriptor::get_length() {
      return 5 + event_name.size() + text.size();
  }
  
  std::string short_event_descriptor::to_string() const {
      return event_name + " " + text + " [" + ISO_639_language_code + "]";
  }
  
  
}

}
