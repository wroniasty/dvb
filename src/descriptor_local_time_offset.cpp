#include <string.h>
#include <typeinfo>
#include <boost/lexical_cast.hpp>
#include <boost/foreach.hpp>

#include "descriptor_local_time_offset.h"

namespace dvb {

namespace si {

  void local_time_offset_descriptor::read(descriptor & d, bits::bitstream & source) {
      dvb::util::position counter(source);
      while (counter() < d.length) {
          std::string code = source.readstring(24);
          offset_p o = add_offset ( code  );
          o->region_id = source.read<unsigned> (6);
          source.skip(1);
          o->local_time_offset_polarity = source.read<unsigned> (1);
          o->local_time_offset = source.read<unsigned> (16);
          o->time_of_change = source.read<unsigned long long> (40);
          o->next_time_offset = source.read<unsigned> (16);
      }
  }
  
  local_time_offset_descriptor::offset_p local_time_offset_descriptor::add_offset ( 
          std::string code, 
          unsigned region_id, 
          unsigned polarity, unsigned offset, unsigned long long time_of_change, 
          unsigned next_time_offset ) {
      offset_p o ( new _offset_t );
      o->code = code;
      o->region_id = region_id;
      o->local_time_offset_polarity = polarity;
      o->local_time_offset = offset;
      o->next_time_offset = next_time_offset;
      o->time_of_change = time_of_change;
      offsets.push_back(o);
      return o;
  }
  
  void local_time_offset_descriptor::write(descriptor & d, bits::bitstream & dest) {
      BOOST_FOREACH ( offset_p o, offsets ) {
          dest.writestring ( o->code, 3);
          dest.write (6, o->region_id);
          dest.write (1, 1);
          dest.write (1, o->local_time_offset_polarity);
          dest.write (16,o->local_time_offset);
          dest.write (40, o->time_of_change);
          dest.write (16, o->next_time_offset);
      }
  }

  unsigned local_time_offset_descriptor::get_length() {
      return offsets.size() * 13;
  }
  
  std::string local_time_offset_descriptor::to_string() const {
      std::string s ("local_time_offset");
      return s;
  }
  
  
}

}
