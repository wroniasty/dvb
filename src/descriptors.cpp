#include <string.h>
#include <typeinfo>
#include <boost/lexical_cast.hpp>

#include "descriptors.h"

namespace dvb {

namespace si {

  descriptor::descriptor () : tag(0), length(0) {}
  descriptor::~descriptor () {}
  
  unsigned descriptor::get_length() {
      return length + 2;
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

  void short_event_descriptor::read(descriptor & d, bits::bitstream & source) {
      ISO_639_language_code = source.readstring( 24 );
      
      unsigned name_length = source.read<unsigned> ( 8 );
      event_name = source.readstring (name_length*8);
      unsigned text_length = source.read<unsigned> ( 8 );
      text = source.readstring (text_length*8);
  }
  
  void short_event_descriptor::write(descriptor & d, bits::bitstream & dest) {
      dest.writestring ( ISO_639_language_code, 24);
      dest.write (8, event_name.size()*8 );
      dest.writestring (event_name);
      dest.write (8, text.size()*8 );
      dest.writestring (text);      
  }
  
  std::string short_event_descriptor::to_string() const {
      return event_name + " " + text + " [" + ISO_639_language_code + "]";
  }

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
      dest.writestring (ISO_639_language_code, 24);
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
  
  std::string extended_event_descriptor::to_string() const {
      std::string s = boost::lexical_cast<std::string>( descriptor_number );
      s += "/" + boost::lexical_cast<std::string>( last_descriptor_number ) + " ";
      s += text + " [" + ISO_639_language_code + "] ITEMS:";
      s += boost::lexical_cast<std::string>(items.size());
      return s;
  }
  
  
}

}
