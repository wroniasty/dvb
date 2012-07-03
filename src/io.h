#ifndef __DVB__IO_H_
#define __DVB__IO_H_ 1

#include <Poco/DateTime.h>
#include <Poco/SharedPtr.h>
#include <Poco/Net/Socket.h>
#include <Poco/Net/DatagramSocket.h>

#include <map>
#include <fstream>

#include "mpegts.h"

using namespace std;
using namespace Poco::Net;
using Poco::SharedPtr;

namespace dvb {
    
namespace io {

    class raw_channel {
    public:
        virtual ~raw_channel();
        virtual int write (unsigned char *buffer, size_t length)=0;
    };

    class udp_ochannel : public raw_channel {
      SocketAddress _address;
      SharedPtr<DatagramSocket>  _socket;
      bool _halted; Poco::Timestamp _halted_when; long _halted_for;
    public:
        udp_ochannel (std::string host, unsigned port);
        virtual ~udp_ochannel();
        virtual int write ( unsigned char *buffer, size_t length);        
    };

    class file_ochannel : public raw_channel {
        ofstream _file;
    public:
        file_ochannel( string filename );
        virtual ~file_ochannel();
        virtual int write ( unsigned char *buffer, size_t length);     
    };
    
    typedef SharedPtr<raw_channel> raw_channel_p;
    typedef vector<raw_channel_p> raw_channel_v;

    raw_channel_p ochannel_from_uri ( string uri );
    
    class output {
        
        unsigned char null_packet_buffer[MPEG_PACKET_SIZE];
        unsigned long _counter;

    protected:
        map<unsigned, unsigned> _continuity_counters;
        void prepare (dvb::mpeg::packet_p p);
        
        raw_channel_v channels;
        
    public:
        
        output();

        int write ( unsigned char *buffer, size_t length);
        int write ( dvb::mpeg::packet_p p, long interval=0 );
        int write ( dvb::mpeg::packet_v pv, long interval=0 );
        int write_null (  long interval=0 );

        void add_channel(raw_channel_p channel);
        void add_channel(raw_channel * pchannel);
        
        int counter();
        void counter_reset();
        inline int channel_count() { return channels.size(); }
    };
        
}    
    
}


#endif
