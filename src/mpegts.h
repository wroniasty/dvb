#ifndef _DVB_MPEG_H_
#define _DVB_MPEG_H_ 1

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

#include "sections.h"

#define MPEG_NULL_PID (0x1fff)
#define MPEG_PACKET_SIZE (188)

using namespace std;

namespace dvb {

namespace mpeg {


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


  };

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

    stream & operator<< (bits::bitstream & stream);
    stream & operator<< (unsigned char * buffer);

  };


} /* mpeg */

} /* dvb */

#endif
