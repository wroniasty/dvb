#include <string.h>
#include <typeinfo>
#include <boost/lexical_cast.hpp>

#include "descriptors.h"

namespace dvb {

namespace si {

  void extended_event_descriptor::read(descriptor & d, bits::bitstream & source) {
      descriptor_number = source.read<unsigned> (4);
      last_descriptor_number = source.read<unsigned> (4);
      ISO_639_language_code = source.readstring( 24 );
      
      unsigned length_of_items = source.read<unsigned> ( 8 );
      items.clear();
      while (length_of_items > 0) {
          Poco::SharedPtr<item> i (new item);
          unsigned item_description_length = source.read<unsigned> (8);
          i->description = source.readstring ( item_description_length * 8 );
          unsigned item_length = source.read<unsigned> (8);
          i->item = source.readstring ( item_length * 8);
          length_of_items -= (item_description_length + item_length + 2);
          items.push_back( i );
      }
      unsigned text_length = source.read<unsigned> (8);
      text = source.readstring (text_length*8);
  }
  
  void extended_event_descriptor::write(descriptor & d, bits::bitstream & dest) {
      dest.write (4, descriptor_number);
      dest.write (4, last_descriptor_number);
      dest.writestring (ISO_639_language_code, 3);
      unsigned length_of_items_offset = dest.position();
      dest.skip(8);
      unsigned length_of_items = 0;
      for (items_v::iterator it=items.begin(); it != items.end(); it++) {
          Poco::SharedPtr<item> i = (*it);
          dest.write (8, i->description.size());
          dest.writestring ( i->description );
          dest.write (8, i->item.size());
          dest.writestring (i->item);
          length_of_items += 2 + i->description.size() + i->item.size();
      }
      dest.write_at (length_of_items_offset, 8, length_of_items);
      dest.write (8, text.size());
      dest.writestring (text);
  }

  unsigned extended_event_descriptor::get_length() {
      unsigned length = 5;
      for (items_v::iterator it=items.begin(); it != items.end(); it++) {
          length += 2 + (*it)->description.size() + (*it)->item.size();
      }
      length += 1 + text.size();
      return length;
  }

  void extended_event_descriptor::add_item ( std::string description, std::string item_text) {
      Poco::SharedPtr<item> i ( new item );
      i->description = description;
      i->item = item_text;
      items.push_back( i );
  }

  std::string extended_event_descriptor::to_string() const {
      std::string s = boost::lexical_cast<std::string>( descriptor_number );
      s += "/" + boost::lexical_cast<std::string>( last_descriptor_number ) + " ";
      s += text + " [" + ISO_639_language_code + "] ITEMS:";
      s += boost::lexical_cast<std::string>(items.size());
      return s;
  }
  
  
}

}
