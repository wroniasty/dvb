#include <cpptest.h>
#include <string.h>
#include <math.h>
#include <ctime>

#include <iostream>
#include <fstream>
#include <string>
#include <cassert>
#include <vector>
#include <map>

#include <boost/foreach.hpp>

#include "Poco/Util/Application.h"
#include "Poco/Util/Option.h"
#include "Poco/Util/OptionSet.h"
#include "Poco/Util/HelpFormatter.h"
#include "Poco/Util/AbstractConfiguration.h"
#include "Poco/SharedPtr.h"
#include "Poco/String.h"
#include "Poco/StringTokenizer.h"
#include "Poco/NumberParser.h"
#include "Poco/Base64Decoder.h"
#include "Poco/Net/IPAddress.h"
#include "Poco/Net/DatagramSocket.h"

#include "mpegts.h"
#include "sections.h"
        
         
using Poco::Util::Application;
using Poco::Util::Option;
using Poco::Util::OptionSet;
using Poco::Util::HelpFormatter;
using Poco::Util::AbstractConfiguration;
using Poco::Util::OptionCallback;
using Poco::Net::IPAddress;
using Poco::Net::DatagramSocket;
using Poco::SharedPtr;
using Poco::StringTokenizer;

using namespace dvb;

class MpegXMIT: public Application
{
public:
  MpegXMIT(): _helpRequested(false)
  {       
  }
protected:	

    void initialize(Application& self)
  {
    loadConfiguration(); // load default configuration files, if present
    Application::initialize(self);
    // add your own initialization code here
  }
	
  void uninitialize()
  {
    // add your own uninitialization code here
    Application::uninitialize();
  }
	
  void reinitialize(Application& self)
  {
    Application::reinitialize(self);
    // add your own reinitialization code here
  }
	
  void defineOptions(OptionSet& options)
  {
    Application::defineOptions(options);

    options.addOption(
		      Option("help", "h", "display help information on command line arguments")
		      .required(false)
		      .repeatable(false)
		      .callback(OptionCallback<MpegXMIT>(this, &MpegXMIT::handleHelp)));

    options.addOption(
		      Option("define", "D", "define a configuration property")
		      .required(false)
		      .repeatable(true)
		      .argument("name=value")
		      .callback(OptionCallback<MpegXMIT>(this, &MpegXMIT::handleDefine)));
				
    options.addOption(
		      Option("config-file", "f", "load configuration data from a file")
		      .required(false)
		      .repeatable(true)
		      .argument("file")
		      .callback(OptionCallback<MpegXMIT>(this, &MpegXMIT::handleConfig)));


    options.addOption ( Option( "dst-address", "d", "Destination IP address")
			.required(false)
			.repeatable(true)
			.argument("ip")
			.binding("destination.ip"));

    options.addOption ( Option( "dst-port", "p", "Destination UDP port")
			.required(false)
			.repeatable(true)
			.argument("udp port")
			.binding("destination.port"));

  }
	
  void handleHelp(const std::string& name, const std::string& value)
  {
    _helpRequested = true;
    displayHelp();
    stopOptionsProcessing();
  }
	
  void handleDefine(const std::string& name, const std::string& value)
  {
    defineProperty(value);
  }
	
  void handleConfig(const std::string& name, const std::string& value)
  {
    loadConfiguration(value);
  }

  void handleSource(const std::string& name, const std::string& value)
  {
    logger().information( name + " " + value );
  }
		
  void displayHelp()
  {
    HelpFormatter helpFormatter(options());
    helpFormatter.setCommand(commandName());
    helpFormatter.setUsage("OPTIONS");
    helpFormatter.setHeader("MPEG transport stream transmitter.");
    helpFormatter.format(std::cout);
  }
	
  void defineProperty(const std::string& def)
  {
    std::string name;
    std::string value;
    std::string::size_type pos = def.find('=');
    if (pos != std::string::npos)
      {
	name.assign(def, 0, pos);
	value.assign(def, pos + 1, def.length() - pos);
      }
    else name = def;
    config().setString(name, value);
  }

  int main(const std::vector<std::string>& args)
  {
      dvb::si::pat_section pat;
      dvb::si::tdt_section tdt;
      dvb::si::tot_section tot;
      
      pat.section_number = 0;
      pat.last_section_number = 0;
      pat.transport_stream_id = 1;
      pat.section_syntax_indicator = 1;
      pat.current_next_indicator = 1;

      pat.add_program(0, 0x10);

      pat.add_program(1000, 0xf14);
      pat.add_program(1001, 0xf16);
      pat.add_program(1002, 0xf17);

      
      tdt.table_id = 0x70;
      
      tot.add_offset("POL", 0, 1, 0x0200, 0xc013010100, 0x0230);
      
      
      struct timespec sleeptime, remaining;
      
      Poco::Net::IPAddress dst_ip ( config().getString("destination.ip", "10.2.100.127") );
      int dst_port = config().getInt("destination.port", 10001);
      Poco::Net::SocketAddress dst_addr ( dst_ip, dst_port);
      Poco::Net::DatagramSocket socket (       
          Poco::Net::SocketAddress  ( 
              Poco::Net::IPAddress(), 
              dst_port)
        );
      socket.connect( dst_addr );
      
      logger().information( std::string("Sending to ") + dst_addr.toString() );
      
      dvb::mpeg::packet_v packets, 
              p_pat = pat.serialize_to_mpegts(0x00),
              p_tdt = tdt.serialize_to_mpegts(0x14),
              p_tot = tot.serialize_to_mpegts(0x14);

      packets.insert (packets.end(), p_pat.begin(), p_pat.end() );
      packets.insert (packets.end(), p_tdt.begin(), p_tdt.end() );
      packets.insert (packets.end(), p_tot.begin(), p_tot.end() );
      
      unsigned char buffer[188], null_buffer[188];
      
      dvb::mpeg::packet nullPacket; nullPacket.PID = 0x1fff;
      nullPacket.write ( null_buffer, 188 );
      
      
      std::map<unsigned, unsigned> cc; unsigned long cnt = 0; 

      sleeptime.tv_sec = 0;
      sleeptime.tv_nsec = 1e5;
      
      cout << packets.size() << endl;
      
      for (int i = 0; i < 20000000; i++) {
        if (++cnt % 25 == 0) {
          BOOST_FOREACH ( dvb::mpeg::packet_p p, packets ) {
                  p->continuityCounter = (cc[p->PID]++) % 16;
                  p->write( buffer, sizeof(buffer) );
                  socket.sendBytes ( buffer, sizeof(buffer) );
                  nanosleep ( &sleeptime, &remaining); 
          }
        } else {
                  socket.sendBytes( null_buffer, sizeof(null_buffer) );
        }
        nanosleep ( &sleeptime, &remaining); 
      }
      
      
      return Application::EXIT_OK;
  }
	
  void printProperties(const std::string& base)
  {
    AbstractConfiguration::Keys keys;
    config().keys(base, keys);
    if (keys.empty())
      {
	if (config().hasProperty(base))
	  {
	    std::string msg;
	    msg.append(base);
	    msg.append(" = ");
	    msg.append(config().getString(base));
	    logger().information(msg);
	  }
      }
    else
      {
	for (AbstractConfiguration::Keys::const_iterator it = keys.begin(); it != keys.end(); ++it)
	  {
	    std::string fullKey = base;
	    if (!fullKey.empty()) fullKey += '.';
	    fullKey.append(*it);
	    printProperties(fullKey);
	  }
      }
  }
	
private:
  bool _helpRequested;
};


POCO_APP_MAIN(MpegXMIT)



