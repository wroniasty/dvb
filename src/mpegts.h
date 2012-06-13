#ifndef _DVB_MPEG_H_
#define _DVB_MPEG_H_ 1

#include <cmath>

#include <vector>
#include <map>
#include <set>

#include <boost/crc.hpp>

#include "Poco/NotificationCenter.h"
#include "Poco/Notification.h"
#include "Poco/Observer.h"
#include "Poco/NObserver.h"
#include "Poco/AutoPtr.h"
#include "Poco/SharedPtr.h"
#include "Poco/DateTime.h"

#include "bits/bits.h"
#include "bits/bits-stream.h"


#define MPEG_NULL_PID (0x1fff)
#define MPEG_PACKET_SIZE (188)

using namespace std;

namespace dvb {

  unsigned int bcd2i(unsigned int bcd);
  unsigned int i2bcd(unsigned int i);

  unsigned MJD (unsigned Y, unsigned M, unsigned D);
  Poco::DateTime fromMJD(unsigned MJD);
  
  Poco::DateTime read_mjd_datetime(bits::bitstream & source);
  Poco::Timespan read_bcd_time(bits::bitstream & source);
  void write_mjd_datetime(bits::bitstream & dest, Poco::DateTime dt);
  void write_bcd_time(bits::bitstream & dest, Poco::Timespan ts);

namespace util {
    
    class position {
        int _offset0;
        bits::bitstream & _stream;
    public:
        position (bits::bitstream & stream);
        int operator() ();
    };
};  
  
namespace mpeg {
  
  typedef boost::crc_optimal<32, 0x04C11DB7, 0xffffffff, 0x0, false, false> crc32_mpeg;
    
  class packet {
  public:
    unsigned int TEI;
    unsigned int PUSI;
    unsigned int TP;
    unsigned int PID;
    unsigned int scramblingControl;
    unsigned int adaptationFieldExists;
    unsigned int continuityCounter;
    unsigned int adaptationFieldLength;
    unsigned int discontinuityIndicator;
    unsigned int randomAccessIndicator;
    unsigned int elementaryStreamPriorityIndicator;
    unsigned int fPCR;
    unsigned int fOPCR;
    unsigned int splicingPointFlag;
    unsigned int transportPrivateData;
    unsigned int adaptationFieldExtension;
    unsigned int spliceCountdown;

    unsigned char PCR[6];
    unsigned char OPCR[6];

    unsigned char payload[188];

    int payloadSize;
    int null;
    int valid;

    packet();
    packet(unsigned char *buffer);
    packet(std::vector<char> &buffer);
    packet(bits::bitstream & stream);

    int read(unsigned char *buffer);
    int read(bits::bitstream & stream);
    int write(unsigned char *buffer, std::size_t max_size);
    int write(bits::bitstream & stream, std::size_t max_size);

    int max_payload_size();
    void copy_payload(const unsigned char * source, int size);
  };

  typedef Poco::SharedPtr<packet> packet_p;
  typedef std::vector<packet_p> packet_v;
  
  unsigned get_packet_pid ( unsigned char *buffer );
  unsigned get_packet_pid ( std::vector<unsigned char> buffer );

  class stream : public Poco::NotificationCenter {

    map<unsigned, Poco::SharedPtr<vector<unsigned char> > > partial_payload_units;
    map<unsigned, unsigned> continuity_counter;

    packet p;

    unsigned continuity_errors;

  public:

    class payload_unit_ready : public Poco::Notification {
    public:
      Poco::SharedPtr<vector<unsigned char> > buffer;
      unsigned PID;
      payload_unit_ready ( Poco::SharedPtr<vector<unsigned char> > payloadbuffer, unsigned PID );
    };

    unsigned operator<< (bits::bitstream & stream);
    unsigned operator<< (unsigned char * buffer);
    unsigned operator<< (std::vector<unsigned char> & buffer);
    unsigned operator<< (packet & p);
    
    void flush();

  };

} /* mpeg */

} /* dvb */

#endif
