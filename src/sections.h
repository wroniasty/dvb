#ifndef _DVB_SECTIONS_
#define _DVB_SECTIONS_ 1

#include <boost/crc.hpp>
#include <boost/foreach.hpp>
#include "bits/bits-stream.h"
#include "mpegts.h"
#include "descriptors.h"

using namespace dvb::mpeg;

namespace dvb {

namespace si {

  class descriptor;

  typedef Poco::SharedPtr < dvb::si::descriptor > descriptor_p;
  typedef std::vector< descriptor_p > descriptors_v;
  typedef std::vector< descriptor_p > descriptor_v;
  
  unsigned peek_table_id (bits::bitstream & source);
  unsigned peek_table_id (std::vector<unsigned char> & buffer);
    
  
  typedef enum { 
      SECTION_OK=0,
      SECTION_ALIGNMENT_ERROR,
      SECTION_CRC_ERROR,
      SECTION_SIZE_INVALID,
      SECTION_INVALID } section_status;

  class section {

  protected:
    virtual void read_header (bits::bitstream & source);
    virtual void read_contents (bits::bitstream & source);
    virtual void write_header (bits::bitstream & dest);
    virtual void write_contents (bits::bitstream & dest);
    virtual bool check_validity();
    bool _valid, _check_crc;
    int _read_section_offset, 
//        _read_offset_0,
        _write_section_offset,
        _write_offset_0;
    
  public:
    unsigned table_id;
    unsigned section_syntax_indicator;
    unsigned section_length;
    unsigned crc32;
    unsigned max_length;
    bool has_crc;

    const static unsigned default_table_id = 0x0;
    
    section ();
    virtual ~section();
    
    int read (bits::bitstream & source);
    int read (std::vector<unsigned char> & buffer);

    void write (bits::bitstream & destination);

    unsigned peek_section_length(bits::bitstream & source);
    
    virtual unsigned calculate_section_length();
    bool is_valid();

    virtual dvb::mpeg::packet_v serialize_to_mpegts(unsigned PID);
    virtual void serialize_to_mpegts(unsigned PID, dvb::mpeg::packet_v & v);
    
  };
  
  typedef Poco::SharedPtr<section> section_p;
  typedef std::vector<section_p> section_v;

  template <class section_t>
     packet_v serialize_to_mpegts ( unsigned PID, std::vector< Poco::SharedPtr<section_t> > sections ) {
      packet_v v;
      BOOST_FOREACH (Poco::SharedPtr<section_t> section, sections) {
          section->serialize_to_mpegts(PID, v);
      }
      return v;       
  }
  
  template <class section_t>
     void serialize_to_mpegts ( unsigned PID, 
                        Poco::SharedPtr<section_t> section,
                        packet_v & v) {
      section->serialize_to_mpegts (PID, v);
  }

  template <class section_t>
     void serialize_to_mpegts ( unsigned PID, std::vector< Poco::SharedPtr<section_t> > sections,
          packet_v & v) {
      BOOST_FOREACH (Poco::SharedPtr<section_t> section, sections) {
          section->serialize_to_mpegts(PID, v);
      }
  }



}

}

#include "section_eit.h"
#include "section_nit.h"
#include "section_pat.h"
#include "section_pmt.h"
#include "section_sdt.h"
#include "section_tdt.h"
#include "section_tot.h"

#endif
