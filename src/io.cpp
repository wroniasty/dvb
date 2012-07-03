#include "io.h"

#include <boost/foreach.hpp>
#include <Poco/Net/MulticastSocket.h>
#include <Poco/Net/NetException.h>
#include <Poco/URI.h>

using Poco::URI;

namespace dvb {
    
namespace io {

    output::output() : _counter(0) {
        dvb::mpeg::packet null;
        null.PID = 0x1fff;
        null.write(null_packet_buffer, sizeof(null_packet_buffer));
    }

    void output::add_channel (raw_channel_p c) {
        channels.push_back(c);
    }

    void output::add_channel (raw_channel * c) {
        channels.push_back( raw_channel_p (c) );
    }
    
    int output::write(unsigned char* buffer, size_t length) {
        int _rval=0;
        BOOST_FOREACH (raw_channel_p c, channels) {
            _rval += c->write(buffer, length);  
        };
        return _rval;
    }
    
    int output::write(dvb::mpeg::packet_v pv,long interval) {
        int _rval=0;
        BOOST_FOREACH ( dvb::mpeg::packet_p p, pv) {
           _rval += write(p, interval);  
        };
        return _rval;
    }

    int output::write(dvb::mpeg::packet_p p, long interval) {
        struct timespec sleeptime, remaining;
        unsigned char buffer[MPEG_PACKET_SIZE];       
        int _rval;
        
        prepare(p);
        memset (buffer,0xff,sizeof(buffer));
        p->write( buffer, sizeof(buffer) );
        
        _rval = write ( buffer, sizeof(buffer) ); 
        
        if (interval > 0) dvb::nanosleep ( 0, interval );
                
        _counter++;
        
        return _rval;
    }
        
    void output::prepare(dvb::mpeg::packet_p p) {
        _continuity_counters[p->PID] = (++_continuity_counters[p->PID]%16);
        p->continuityCounter = _continuity_counters[p->PID];
    }
    
    int output::write_null(long interval) {
        _counter++;
        int _rval = write (null_packet_buffer, sizeof(null_packet_buffer));
        if (interval > 0) 
            dvb::nanosleep ( 0, interval );
        return _rval;
    }
    
    int output::counter() { return _counter; }
    void output::counter_reset() { _counter=0; }
    
    raw_channel::~raw_channel() {}
    
    udp_ochannel::udp_ochannel(std::string host, unsigned port) :
        raw_channel(), _halted(false),
        _address ( IPAddress(host), port )
    {
        SocketAddress local ( IPAddress(), port );
        if (_address.host().isMulticast()) {
            _socket.assign ( new MulticastSocket( local ) );
        } else {
            _socket.assign ( new DatagramSocket ( local ) );
        }        
        _socket->connect(_address);
    }
    
    udp_ochannel::~udp_ochannel() {
        _socket->close();
    }
    
    int udp_ochannel::write (unsigned char *buffer, std::size_t length) {
       try {
          if (_halted && _halted_when.isElapsed(_halted_for)) {
              _halted = false;               
          }
          return _halted ? 0 : _socket->sendBytes(buffer, length);
       } catch (const Poco::Net::NetException & e) {
           _halted = true; _halted_for = 1e6; _halted_when.update();
       }
    }

    file_ochannel::file_ochannel(std::string filename) :
        raw_channel()
    {
        _file.open(filename.c_str(), std::ios::out | std::ios::binary );
    }
    
    file_ochannel::~file_ochannel() {
        _file.close();
    }
    
    int file_ochannel::write (unsigned char *buffer, std::size_t length) {
       _file.write((const char*) buffer, length);
       return length;
    }
 
    raw_channel_p ochannel_from_uri ( string uri ) {
        URI u(uri);
        if (u.getScheme() == "udp") {          
          return raw_channel_p ( new udp_ochannel(u.getHost(), u.getPort()) );
        }
        else if (u.getScheme() == "file") 
            return raw_channel_p ( new file_ochannel (u.getPath()));
        return raw_channel_p();
    }

} /* io */    
    
} /* dvb */
