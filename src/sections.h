#ifndef _DVB_SECTIONS_
#define _DVB_SECTIONS_ 1

#include <boost/crc.hpp>
#include "bits/bits-stream.h"
#include "mpegts.h"
#include "descriptors.h"

namespace dvb {

namespace si {

  class descriptor;

  typedef std::vector< Poco::SharedPtr < dvb::si::descriptor > > descriptors_v;

  unsigned peek_table_id (bits::bitstream & source);
  unsigned peek_table_id (std::vector<unsigned char> & buffer);
    
  
  typedef enum { 
      SECTION_OK=0,
      SECTION_ALIGNMENT_ERROR,
      SECTION_CRC_ERROR } section_status;

  class section {

  protected:
    virtual void read_header (bits::bitstream & source);
    virtual void read_contents (bits::bitstream & source);
    virtual void write_header (bits::bitstream & dest);
    virtual void write_contents (bits::bitstream & dest);
    virtual bool check_validity();
    bool _valid, _check_crc;
    int _read_section_offset, 
        _read_offset_0,
        _write_section_offset,
        _write_offset_0;
    
  public:
    unsigned table_id;
    unsigned section_syntax_indicator;
    unsigned section_length;
    unsigned crc32;
    
    section ();

    int read (bits::bitstream & source);
    int read (std::vector<unsigned char> & buffer);

    void write (bits::bitstream & destination);

    unsigned peek_section_length(bits::bitstream & source);
    
    unsigned get_section_length();
    bool is_valid();

  };

  class pat_section : public section {
     protected:

     virtual void read_contents (bits::bitstream & source);
     virtual void write_contents (bits::bitstream & dest);

     public:

     typedef struct { unsigned program_number, program_map_pid; } program;
     typedef std::vector<Poco::SharedPtr<program> > programs_v;

     programs_v programs;

     unsigned transport_stream_id;
     unsigned version_number;
     unsigned current_next_indicator;
     unsigned section_number;
     unsigned last_section_number;

  };

  class nit_section : public section {
     protected:

     virtual void read_contents (bits::bitstream & source);
     virtual void write_contents (bits::bitstream & dest);

     public:

     typedef struct {
        unsigned transport_stream_id;
        unsigned original_network_id;
        descriptors_v descriptors;
     } transport_stream;
     typedef std::vector<Poco::SharedPtr<transport_stream> > transport_stream_v;

     transport_stream_v transport_streams;

     unsigned network_id;
     unsigned version_number;
     unsigned current_next_indicator;
     unsigned section_number;
     unsigned last_section_number;
     
     descriptors_v network_descriptors;
     
  };

  class pmt_section : public section {
     protected:

     virtual void read_contents (bits::bitstream & source);
     virtual void write_contents (bits::bitstream & dest);
     public:

     typedef struct {
        unsigned stream_type;
        unsigned elementary_PID;
        unsigned info_length;
        descriptors_v info;
     } es_info;

     typedef std::vector<Poco::SharedPtr<es_info> > es_info_v;
     unsigned program_number;
     unsigned version_number;
     unsigned current_next_indicator;
     unsigned section_number;
     unsigned last_section_number;
     unsigned PCR_PID;
     unsigned program_info_length;
     descriptors_v program_info;
     es_info_v info;


  };

  class sdt_section : public section {
     protected:

     virtual void read_contents (bits::bitstream & source);
     virtual void write_contents (bits::bitstream & dest);

     public:

     typedef struct {
        unsigned service_id;
        unsigned EIT_schedule_flag;
        unsigned EIT_present_following_flag;
        unsigned running_status;
        unsigned free_CA_mode;
        descriptors_v descriptors;
     } service_info;
     typedef std::vector<Poco::SharedPtr<service_info> > service_info_v;


     unsigned transport_stream_id;
     unsigned version_number;
     unsigned current_next_indicator;
     unsigned section_number;
     unsigned last_section_number;
     unsigned original_network_id;

     service_info_v services;

  };

  class tdt_section : public section {
     protected:

     virtual void read_contents (bits::bitstream & source);
     virtual void write_contents (bits::bitstream & dest);

     public:
     char UTC_time[5];
  };

  class tot_section : public section {
     protected:

     virtual void read_contents (bits::bitstream & source);
     virtual void write_contents (bits::bitstream & dest);

     public:
     char UTC_time[5];
     descriptors_v descriptors;
  };

  class eit_section : public section {
     protected:

     virtual void read_contents (bits::bitstream & source);
     virtual void write_contents (bits::bitstream & dest);

     public:

     typedef struct {
         unsigned event_id;
         unsigned char start_time[5];
         unsigned char duration[3];
         unsigned running_status;
         unsigned free_CA_mode;
         unsigned descriptors_loop_length;
         descriptors_v descriptors;
     } event;
     typedef std::vector<Poco::SharedPtr<event> > events_v;

     events_v events;

     unsigned service_id;
     unsigned version_number;
     unsigned current_next_indicator;
     unsigned section_number;
     unsigned last_section_number;
     unsigned transport_stream_id;
     unsigned original_network_id;
     unsigned segment_last_section_number;
     unsigned last_table_id;

  };

}

}

#endif
