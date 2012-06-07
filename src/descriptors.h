#ifndef _DVB_DESC_H_
#define _DVB_DESC_H_ 1
#include "bits/bits-stream.h"
#include "mpegts.h"

namespace dvb {

namespace si {

  class descriptor_syntax;

  class descriptor {
  public:
    unsigned char tag;
    unsigned char length;
    
    Poco::SharedPtr <descriptor_syntax> data;

    descriptor ();
    virtual ~descriptor();

    void read (bits::bitstream & source);
    void read (std::vector<unsigned char> & buffer);

    unsigned get_length();
    
    void read_header (bits::bitstream & source);
    virtual void read_data (bits::bitstream & source);
  };
  std::ostream& operator<< (std::ostream& stream, const descriptor& d);
 
  class descriptor_syntax {
    public:
    virtual ~descriptor_syntax();
    virtual void read(descriptor & d, bits::bitstream & source);
    virtual void write(descriptor & d, bits::bitstream & source);    
    virtual std::string to_string() const;
    
  };

  class short_event_descriptor : public descriptor_syntax {
    public:
    std::string ISO_639_language_code;
    std::string event_name;
    std::string text;
    virtual void read(descriptor & d, bits::bitstream & source);
    virtual void write(descriptor & d, bits::bitstream & dst);
    virtual std::string to_string() const;
  };

  class extended_event_descriptor : public descriptor_syntax {
    public:

    typedef struct {
        std::string description;
        std::string item;
    } item;
    typedef std::vector< Poco::SharedPtr<item> > items_v;

    unsigned descriptor_number;
    unsigned last_descriptor_number;
    std::string ISO_639_language_code;
    items_v items;
    std::string text;

    virtual void read(descriptor & d, bits::bitstream & source);
    virtual void write(descriptor & d, bits::bitstream & dst);
    virtual std::string to_string() const;

    //virtual void read(descriptor & d, bits::bitstream & source);
  };

}

}

#endif
