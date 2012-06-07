#include <string.h>
#include <cassert>

#include "mpegts.h"

namespace dvb {

namespace mpeg {

  packet::packet() :
    TEI(0), PUSI(0), TP(0), PID(0),
    scramblingControl(0), adaptationFieldExists(1),
    continuityCounter(0), payloadSize(0), valid(1) {
  }

  packet::packet(std::vector<char> &buffer) {
    read( (unsigned char*) &buffer[0] );
  }

  packet::packet(unsigned char *buffer)  {
    read(buffer);
  }

  packet::packet(bits::bitstream & stream)  {
    read(stream);
  }

  int packet::read(unsigned char *buffer) {
    bits::bitstream stream((unsigned char *) buffer);
    return read(stream);
  }

  int packet::read(bits::bitstream & stream) {
    unsigned off0 = stream.position();
    valid =  stream.read<unsigned>(8) == 0x47;

    TEI = stream.read<unsigned>(1);
    PUSI = stream.read<unsigned>(1);
    TP = stream.read<unsigned>(1);

    PID = stream.read<unsigned>(13);
    null = (PID == MPEG_NULL_PID);

    scramblingControl = stream.read<unsigned> (2);
    adaptationFieldExists = stream.read<unsigned> (2);
    continuityCounter = stream.read<unsigned> (4);
    if (adaptationFieldExists>1) {
      adaptationFieldLength = stream.read<unsigned> (8);;
      discontinuityIndicator = stream.read<unsigned> (1);
      randomAccessIndicator = stream.read<unsigned> (1);
      elementaryStreamPriorityIndicator = stream.read<unsigned> (1);
      fPCR = stream.read<unsigned> (1);
      fOPCR = stream.read<unsigned> (1);
      splicingPointFlag = stream.read<unsigned> (1);
      transportPrivateData = stream.read<unsigned> (1);
      adaptationFieldExtension = stream.read<unsigned> (1);
      if (fPCR) stream.readstring(PCR, 6*8);
      if (fOPCR) stream.readstring(OPCR, 6*8);
      if (splicingPointFlag) spliceCountdown = stream.read<unsigned> (8);
    }

    memset(payload, 0, sizeof(payload));
    if (!null) {
      payloadSize = (MPEG_PACKET_SIZE*8 - stream.position()) / 8;
      stream.readstring(payload, payloadSize*8);
      valid &= stream.position() == MPEG_PACKET_SIZE*8;
    } else {
      payloadSize = 0;
    }
    return (stream.position() - off0);
  }

  int packet::write(unsigned char *buffer, std::size_t max_size) {
    bits::bitstream stream(buffer);
    return write (stream, max_size);
  }

  int packet::write(bits::bitstream & stream, std::size_t max_size) {
    stream.write(8, 0x47);
    stream.write(1, TEI);
    stream.write(1, PUSI);
    stream.write(1, TP);
    stream.write(13, PID);
    stream.write(2, scramblingControl);
    stream.write(2, adaptationFieldExists);
    stream.write(4, continuityCounter);
    if (adaptationFieldExists > 1) {
      stream.write(8, adaptationFieldLength);
      stream.write(1, discontinuityIndicator);
      stream.write(1, randomAccessIndicator);
      stream.write(1, elementaryStreamPriorityIndicator);
      stream.write(1, fPCR);
      stream.write(1, fOPCR);
      stream.write(1, splicingPointFlag);
      stream.write(1, transportPrivateData);
      stream.write(1, adaptationFieldExtension);
      if (fPCR) stream.writestring(6*8, PCR);
      if (fOPCR) stream.writestring(6*8, OPCR);
      if (splicingPointFlag) stream.write(8, spliceCountdown);
    }

    stream.writestring ( MPEG_PACKET_SIZE*8 - stream.position(), payload );
    return (stream.position() == MPEG_PACKET_SIZE*8);
  }

  unsigned get_packet_pid ( unsigned char *buffer ) { return bits::getbitbuffer<unsigned> (buffer, 11, 13); }
  unsigned get_packet_pid ( std::vector<unsigned char> buffer ) { return get_packet_pid ( (unsigned char *) &buffer[0] ); }


  stream::payload_unit_ready::payload_unit_ready ( Poco::SharedPtr<vector<unsigned char> > payloadbuffer, unsigned _PID ) :
    Poco::Notification(), buffer(payloadbuffer), PID(_PID) {
  }

  unsigned stream::operator<< (unsigned char * buffer) {
    bits::bitstream input(buffer);
    return (*this) << input;
  }

  unsigned stream::operator<< (std::vector<unsigned char> & buffer) {
      assert(buffer.size() >= MPEG_PACKET_SIZE);
      return (*this) << &buffer[0];
  }

  unsigned stream::operator<< (bits::bitstream & input) {
    p.read (input);
    if (!p.null && p.valid) {
      Poco::SharedPtr <vector<unsigned char> > buffer = partial_payload_units[p.PID];
      if (p.PUSI) {
        if (buffer) {
            postNotification ( new payload_unit_ready (buffer, p.PID) );
        }
        buffer = partial_payload_units[p.PID] = new vector<unsigned char>;
      }
      if (buffer) {
        (*buffer).insert ( (*buffer).end(), p.payload, p.payload+p.payloadSize );
      }
      if (p.continuityCounter != (continuity_counter[p.PID] + 1) % 16) continuity_errors++;
      continuity_counter[p.PID] = p.continuityCounter;
    }
  }


} /* mpeg */

} /* dvb */
