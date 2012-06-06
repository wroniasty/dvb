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

    void read (bits::bitstream & source);
    void read (std::vector<unsigned char> & buffer);

    void read_header (bits::bitstream & source);
    virtual void read_data (bits::bitstream & source);
  };

  class descriptor_syntax {
    public:
    virtual void read(descriptor & d, bits::bitstream & source);
    virtual void write(descriptor & d, bits::bitstream & source);
  };

  class short_event_descriptor : public descriptor_syntax {
    public:
    char ISO_639_language_code[3];
    std::string event_name;
    std::string text;
    //virtual void read(descriptor & d, bits::bitstream & source);
  };

  class extended_event_descriptor : public descriptor_syntax {
    public:

    typedef struct {
        std::string description;
        std::string text;
    } item;
    typedef std::vector< Poco::SharedPtr<item> > items_v;

    unsigned descriptor_number;
    unsigned last_descriptor_number;
    char ISO_639_language_code[3];
    items_v items;
    std::string text;

    //virtual void read(descriptor & d, bits::bitstream & source);
  };

}

}

#endif
