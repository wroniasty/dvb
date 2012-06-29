#include <cpptest.h>
#include <string.h>
#include <math.h>
#include <ctime>
#include <iconv.h>

#include <iostream>
#include <fstream>
#include <string>
#include <cassert>
#include <vector>
#include <map>

#include <boost/foreach.hpp>
#include <boost/tuple/tuple.hpp>
#include <boost/lexical_cast.hpp>

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
#include "Poco/Net/MulticastSocket.h"
#include "Poco/Net/NetException.h"

#include <soci/soci-config.h>
#include <soci/soci.h>
#include <soci/soci-backend.h>
#include <soci/postgresql/soci-postgresql.h>

#include "mpegts.h"
#include "sections.h"
#include "epg.h"        
         

#include <bits/bits.h>
#include <bits/bits-stream.h>

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

class MpegDEBUG: public Application
{
public:
  MpegDEBUG(): _helpRequested(false)
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
		      .callback(OptionCallback<MpegDEBUG>(this, &MpegDEBUG::handleHelp)));

    options.addOption(
		      Option("define", "D", "define a configuration property")
		      .required(false)
		      .repeatable(true)
		      .argument("name=value")
		      .callback(OptionCallback<MpegDEBUG>(this, &MpegDEBUG::handleDefine)));
				
    options.addOption(
		      Option("config-file", "f", "load configuration data from a file")
		      .required(false)
		      .repeatable(true)
		      .argument("file")
		      .callback(OptionCallback<MpegDEBUG>(this, &MpegDEBUG::handleConfig)));

    options.addOption ( Option( "target-id", "", "Predefined target id")
			.required(false)
                        .argument ( "ID" )
			.repeatable(false)
			.binding("destination.target-id"));

    options.addOption ( Option( "pat", "", "Send PAT section")
			.required(false) 
			.repeatable(false)
                        .argument ("0|1")
			.binding("mpeg.pat"));

    options.addOption ( Option( "time", "", "Send TOT/TDT sections")
			.required(false) 
			.repeatable(false)
                        .argument("0|1")
			.binding("mpeg.time"));

    options.addOption ( Option( "epg", "", "Send EPG sections")
			.required(false) 
			.repeatable(false)
                        .argument("0|1")
			.binding("mpeg.epg"));

    options.addOption ( Option( "tsid", "", "Transport stream ID")
			.required(false)
                        .argument ( "ID" )
			.repeatable(false)
			.binding("mpeg.tsid"));


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
      if (_helpRequested) return Application::EXIT_OK;
      
      dvb::si::eit_section eit;
      
           
      unsigned char buf[4096];
      bits::bitstream str (buf);
      
      eit.table_id = 0x4e;
      eit.original_network_id = 1;
      eit.section_number = 0;
      eit.last_section_number = 1;
      
      eit.service_id = 0xabcd;
      eit.transport_stream_id = 7;
      
      dvb::si::eit_section::event_p ev= dvb::si::eit_section::make_event(0x1FAC, 
              Poco::DateTime(2012,10,1,10), Poco::Timespan(0,1,0,0,0), 
              "pol", "NAME", "TEXT", "LONG", "iso-8859-2"              
      );
      ev->running_status = 0;
      eit.add_event(ev);
      
      eit.write(str);
      cout << eit.calculate_section_length() << endl;
      cout << bits::hexdump (buf, eit.calculate_section_length(), 20) << endl;
      
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


POCO_APP_MAIN(MpegDEBUG)



