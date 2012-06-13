#include <string.h>
#include <cassert>
#include <boost/foreach.hpp>
#include <boost/tuple/tuple.hpp>
#include "mpegts.h"

namespace dvb {

    
    
  unsigned int bcd2i(unsigned int bcd) {
    unsigned int decimalMultiplier = 1;
    unsigned int digit = 0;
    unsigned int i = 0;
    while (bcd > 0) {
        digit = bcd & 0xF;
        i += digit * decimalMultiplier;
        decimalMultiplier *= 10;
        bcd >>= 4;
    }
    return i;
  }

  unsigned int i2bcd(unsigned int i) {
    unsigned int binaryShift = 0;
    unsigned int digit = 0;
    unsigned int bcd = 0;
    while (i > 0) {
        digit = i % 10;
        bcd += (digit << binaryShift);
        binaryShift += 4;
        i /= 10;
    }
    return bcd;
  }
  
  unsigned MJD (unsigned Y, unsigned M, unsigned D) {
      unsigned L = 0; Y -= 1900;
      if (M == 1 || M == 2) L = 1;
      return 14956 + D + 
              (unsigned) ( (float) (Y-L) * 365.25f ) + 
              (unsigned) ( (float) (M+1+L*12) * 30.6001f );
  }

  Poco::DateTime fromMJD(unsigned MJD) {
      unsigned Y,M,D,Y_, M_, K=0;
      Y_ = (MJD - 15078.2f) / 365.25f;
      M_ = ((MJD - 14956.1f) - (unsigned) ( Y_ * 365.25f )) / 30.6001f;
      D = MJD - 14956 - (unsigned) (Y_ * 365.25f) - (unsigned) (M_ * 30.6001f);
      if (M_ == 14 || M_ == 15) K = 1;
      Y = Y_ + K;
      M = M_ - 1 + K *12;      
      return Poco::DateTime (Y+1900,M,D);
  }
  
  Poco::DateTime read_mjd_datetime(bits::bitstream & source) {
    unsigned mjd;
    mjd = source.read <unsigned> (16);
    Poco::DateTime dt = dvb::fromMJD(mjd);
    unsigned h,m,s;
    h = dvb::bcd2i( source.read<unsigned> (8) );
    m = dvb::bcd2i( source.read<unsigned> (8) );
    s = dvb::bcd2i( source.read<unsigned> (8) );
    dt.assign( dt.year(), dt.month(), dt.day(), h,m,s);      
    return dt;
  }

  Poco::Timespan read_bcd_time(bits::bitstream & source) {
    unsigned h,m,s;
    h = dvb::bcd2i( source.read<unsigned> (8) );
    m = dvb::bcd2i( source.read<unsigned> (8) );
    s = dvb::bcd2i( source.read<unsigned> (8) );      
    return Poco::Timespan (0,h,m,s,0);
  }  
  
  void write_mjd_datetime(bits::bitstream & dest, Poco::DateTime dt) {
      unsigned mjd = dvb::MJD( dt.year(), dt.month(), dt.day() );
      dest.write (16, mjd );
      dest.write (8, dvb::i2bcd(dt.hour()) );
      dest.write (8, dvb::i2bcd(dt.minute()) );
      dest.write (8, dvb::i2bcd(dt.second()) );      
  }
  
  void write_bcd_time(bits::bitstream & dest, Poco::Timespan ts) {
      dest.write (8, dvb::i2bcd(ts.hours()) );
      dest.write (8, dvb::i2bcd(ts.minutes()) );
      dest.write (8, dvb::i2bcd(ts.seconds()) );      
  }

  
  namespace util {
    position::position(bits::bitstream& stream) : _stream(stream) {
        _offset0 = _stream.position();
    }  
    
    int position::operator ()() {
        return (_stream.position() - _offset0) / 8 + (_stream.aligned() ? 0 : 1);
    }
}  
  
namespace mpeg {

  packet::packet() :
    TEI(0), PUSI(0), TP(0), PID(0),
    scramblingControl(0), adaptationFieldExists(1),
    continuityCounter(0), payloadSize(0), valid(1), null(0) {
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
  
  int packet::max_payload_size() {
      if (adaptationFieldExists > 1) return 188 - 4 - 1 - adaptationFieldLength;
      else return 188 - 4;
  }

  void packet::copy_payload(const unsigned char * source, int size) {
      memcpy ( payload, source, size );
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
 
  unsigned stream::operator<< (packet & p) {
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
    
  unsigned stream::operator<< (bits::bitstream & input) {
    p.read (input);
    (*this) << p;
  }
  
  void stream::flush() {  
      unsigned PID; Poco::SharedPtr<vector<unsigned char> > buffer;      
      BOOST_FOREACH (boost::tie(PID, buffer), partial_payload_units) {
          postNotification ( new payload_unit_ready (buffer, PID) );
      }
  }


} /* mpeg */

} /* dvb */
