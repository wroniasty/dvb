#ifndef _DVB_ANALYZER_H_
#define _DVB_ANALYZER_H_ 1

#include <vector>
#include <map>
#include <set>

#include "Poco/NotificationCenter.h"
#include "Poco/Notification.h"
#include "Poco/Observer.h"
#include "Poco/NObserver.h"
#include "Poco/AutoPtr.h"
#include "Poco/SharedPtr.h"

#include "bits/bits.h"
#include "bits/bits-stream.h"

#include "mpegts.h"
#include "sections.h"

#define MPEG_NULL_PID (0x1fff)
#define MPEG_PACKET_SIZE (188)

using namespace std;

namespace dvb {

namespace mpeg {

  class analyzer : public Poco::NotificationCenter {
     set<unsigned> pid_filter;
     map<unsigned, unsigned> pmt_pids;
     stream _stream;

     public:

     unsigned PAT_PID;// = 0x00;
     unsigned CAT_PID;// = 0x01;
     unsigned NIT_PID;//= 0x10;
     unsigned SDT_PID;// = 0x11;
     unsigned EIT_PID;// = 0x12;

     typedef enum { FILTER_ALLOW, FILTER_BLOCK } filter_mode_type;
     filter_mode_type filter_mode;
     bool collect_pmt;
     map<unsigned, Poco::SharedPtr<dvb::si::sdt_section::service_info> > services;
     map<unsigned, Poco::SharedPtr<dvb::si::pmt_section::es_info> > programs;


     analyzer();
     ~analyzer();

     void handle_payload_unit (const Poco::AutoPtr<mpeg::stream::payload_unit_ready> & puN);
     void add_pid_to_filter (unsigned PID);
     void remove_pid_from_filter(unsigned PID);

     analyzer & operator<< (bits::bitstream & stream);
     analyzer & operator<< (unsigned char * buffer);

  };

} /* mpeg */

} /* dvb */

#endif
