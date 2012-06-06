#ifndef _DVB_SECTIONS_
#define _DVB_SECTIONS_ 1

#include "bits/bits-stream.h"
#include "mpegts.h"
#include "descriptors.h"

namespace dvb {

namespace si {

  class descriptor;

  typedef std::vector< Poco::SharedPtr < dvb::si::descriptor > > descriptors_v;

  class section {

  protected:
    virtual void read_header (bits::bitstream & source);
    virtual void read_contents (bits::bitstream & source);
    virtual void write_header (bits::bitstream & dest);
    virtual void write_contents (bits::bitstream & dest);
  public:
    unsigned table_id;
    unsigned section_syntax_indicator;
    unsigned section_length;

    section ();

    void read (bits::bitstream & source);
    void read (std::vector<unsigned char> & buffer);

    void write (bits::bitstream & destination);

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
