#include <string.h>
#include <cassert>

#include "analyzer.h"


namespace dvb {
namespace mpeg {

  analyzer::analyzer() {
    PAT_PID = 0x00;
    CAT_PID = 0x01;
    NIT_PID = 0x10;
    SDT_PID = 0x11;
    EIT_PID = 0x12;
    _stream.addObserver ( Poco::NObserver<analyzer, mpeg::stream::payload_unit_ready> ( *this, &analyzer::handle_payload_unit ) );
  };

  analyzer::~analyzer() {
    _stream.removeObserver ( Poco::NObserver<analyzer, mpeg::stream::payload_unit_ready> ( *this, &analyzer::handle_payload_unit ) );
  };

  void analyzer::handle_payload_unit (const Poco::AutoPtr<mpeg::stream::payload_unit_ready> & puN) {
    Poco::SharedPtr < vector<unsigned char> > buffer(puN->buffer);

    /* TODO: move this to the <<operator so it makes sense */
    unsigned PID = dvb::mpeg::get_packet_pid ( *buffer );
    bool in_filter = (pid_filter.count(PID) == 1);

    if ( (in_filter && (filter_mode == FILTER_BLOCK)) || (!in_filter && (filter_mode == FILTER_ALLOW)) )
        return;

    if (puN->PID == PAT_PID) {
        dvb::si::pat_section pat; 
        if ((pat.read(*buffer) == dvb::si::SECTION_OK) && pat.is_valid()) {
            for ( dvb::si::pat_section::programs_v::iterator it = pat.programs.begin(); it != pat.programs.end(); it++ ) {
                pmt_pids[(*it)->program_map_pid] = (*it)->program_number;
                if (filter_mode == FILTER_ALLOW && collect_pmt) add_pid_to_filter((*it)->program_map_pid);
            }
        }
    } else if (puN->PID == EIT_PID) {
        dvb::si::eit_section eit; eit.read (*buffer);
    } else if (puN->PID == SDT_PID) {
    } else if ( collect_pmt && pmt_pids.count(puN->PID)  ) {
     //   dvb::si::pmt_section pmt; pmt.read (*buffer);
    }

  }

  void analyzer::add_pid_to_filter (unsigned PID) {
    pid_filter.insert (PID);
  }

  void analyzer::remove_pid_from_filter(unsigned PID) {
    pid_filter.erase (PID);
  }

  analyzer & analyzer::operator<< (bits::bitstream & stream) {
    _stream << stream; return *this;
  }

  analyzer & analyzer::operator<< (unsigned char * buffer) {
    _stream << buffer; return *this;
  }

} /* mpeg */

} /* dvb */
