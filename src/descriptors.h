#ifndef _DVB_DESC_H_
#define _DVB_DESC_H_ 1
#include "bits/bits-stream.h"
#include "mpegts.h"

namespace dvb {

namespace si {

  class descriptor_syntax;
  
  class descriptor {
    descriptor_syntax * new_syntax();       
  public:
    unsigned tag;
    unsigned length;
    
    Poco::SharedPtr <descriptor_syntax> data;

    descriptor ();
    virtual ~descriptor();

    template <class syntax> Poco::SharedPtr<syntax> set_data () {
      syntax * s = new syntax;
      tag = s->tag;
      data = Poco::SharedPtr <descriptor_syntax> (s);
      return data.cast<syntax>();
    }  

    template <class syntax> Poco::SharedPtr<syntax> get_data () {
      return data.cast<syntax>();
    }  
    
    void read (bits::bitstream & source);
    void read (std::vector<unsigned char> & buffer);

    unsigned get_length();
    
    void read_header (bits::bitstream & source);
    virtual void read_data (bits::bitstream & source);
  
    virtual void write (bits::bitstream & source);
    
  };
  std::ostream& operator<< (std::ostream& stream, const descriptor& d);
 
  class descriptor_syntax {
    public:
    const static unsigned char tag = 0x00;
    virtual ~descriptor_syntax();
    virtual void read(descriptor & d, bits::bitstream & source);
    virtual void write(descriptor & d, bits::bitstream & source);    
    virtual std::string to_string() const;
    virtual unsigned get_length(); 
  };


  typedef enum {
              QAM16 = 0x01,
              QAM32,
              QAM64,
              QAM128,
              QAM256
  } modulation_type;

  typedef enum {
              NO_OUTER_FEC = 0x01,
              RS_204_188
  } outer_fec_type;

  typedef enum {
               CONV_1_2=0x01,
               CONV_2_3,
               CONV_3_4,
               CONV_5_6,
               CONV_7_8,
               CONV_8_9,
               CONV_3_5,
               CONV_4_5,
               CONV_9_10
  } inner_fec_type;



}

}

#include "descriptor_short_event.h"
#include "descriptor_extended_event.h"
#include "descriptor_cable_delivery_system.h"
#include "descriptor_local_time_offset.h"

#endif
