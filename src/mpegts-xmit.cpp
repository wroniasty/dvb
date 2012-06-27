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

    options.addOption ( Option( "bitrate", "", "Target stream bitrate")
                        .argument ( "bitrate/kbps" )
			.repeatable(false)
			.binding("mpeg.bitrate"));

    options.addOption ( Option( "dst-address", "", "Destination IP address")
			.required(true)
			.argument("ip")
			.binding("destination.ip"));

    options.addOption ( Option( "dst-port", "", "Destination UDP port")
			.required(true)
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
      if (_helpRequested) return Application::EXIT_OK;

      soci::session sql(soci::postgresql, "dbname=mpegts user=mpegts"); 

            
      unsigned target_id = config().getInt( "destination.target-id",  0);
      unsigned TSID = config().getInt( "mpeg.tsid",  0);
      unsigned bitrate = config().getInt( "mpeg.bitrate",  512);
      Poco::Net::IPAddress dst_ip ( config().getString("destination.ip", "127.0.0.1") );
      int dst_port = config().getInt("destination.port", 12345);

      bool send_time = config().getBool( "mpeg.time", false );
      bool send_pat = config().getBool( "mpeg.pat", false );
      bool send_epg = config().getBool( "mpeg.epg", false );

      dvb::epg::target target;      
            
      
      dvb::si::pat_section pat;
      dvb::si::tdt_section tdt;
      dvb::si::tot_section tot;
      dvb::si::eit_section_v eit_pf, eit_sched;
      
      dvb::mpeg::packet_v p_pat, p_tot, p_tdt, p_eit_pf;

      Poco::Timestamp now, 
              last_pat_transmit, 
              last_time_transmit, last_time_update, 
              last_eit_transmit,
              last_transfer_check;

      if (send_epg) {
        logger().information( "Sending EPG.");
        target.init (sql, target_id);
        TSID = target.tsid;
        
        BOOST_FOREACH ( dvb::epg::service_p svc, target.services) {
            dvb::epg::schedule_v pf = svc->present_following_event(now);
            logger().information( string(" + ") + svc->name );
	    if (pf[0]) {
	      logger().information ( pf[0]->info["pol"]->title );
	      logger().information ( pf[0]->info["pol"]->text );
	    }
	    if (pf[1]) {
	      logger().information ( pf[1]->info["pol"]->title );
	      logger().information ( pf[1]->info["pol"]->text );
	    }
	    logger().information ( string("TSID: ")  + boost::lexical_cast<string> ( target.tsid ) );
            eit_pf = 
            dvb::si::eit_prepare_present_following (
               svc->sid, 20, 1, target.tsid, target.origid,
               ( pf[0] ?
                 dvb::si::eit_section::make_event (
                   pf[0]->id, pf[0]->start, pf[0]->duration,
                   "pol", pf[0]->info["pol"]->title, pf[0]->info["pol"]->text, "Long text of Present"
               ) : 0 ),
               ( pf[1] ? 
                 dvb::si::eit_section::make_event (
                   pf[1]->id, pf[1]->start, pf[1]->duration,
                   "pol", pf[1]->info["pol"]->title, pf[1]->info["pol"]->text, "Long text of Present"
               ) : 0)
            );

            dvb::si::serialize_to_mpegts<dvb::si::eit_section> (0x12, eit_pf, p_eit_pf);       
        }
      }
      
      if (send_pat) {
          logger().information( "Sending PAT.");
          pat.section_number = 0;
          pat.last_section_number = 0;
          pat.transport_stream_id = TSID; 
          pat.section_syntax_indicator = 1;
          pat.current_next_indicator = 1;
          pat.add_program(0, 0x10);  /* network */
          p_pat = pat.serialize_to_mpegts(0x00);
      }
      
      if (send_time) {
          logger().information( "Sending TDT/TOT.");
          logger().information( " + time offset: +02:00");
          tot.add_offset ( "POL", 0, 1, 0x0200, dvb::MJD(2013,10,01) * 0x1000000 + 0x033000, 0x0100);
      }

      
      struct timespec sleeptime, remaining;
      
      Poco::Net::SocketAddress dst_addr ( dst_ip, dst_port);
      Poco::SharedPtr<Poco::Net::DatagramSocket>  socket;
      
      if (dst_ip.isMulticast()) {
         socket.assign( new Poco::Net::MulticastSocket (       
          Poco::Net::SocketAddress  ( 
              Poco::Net::IPAddress(), 
              dst_port)
         ) );
         
      } else {
         socket.assign  ( new Poco::Net::DatagramSocket (       
          Poco::Net::SocketAddress  ( 
              Poco::Net::IPAddress(), 
              dst_port)
         ) );
     }

      socket->connect( dst_addr );
      
      logger().information( std::string("Sending to ") + dst_addr.toString() );

      std::map<unsigned, unsigned> cc; unsigned cnt = 0; 
      unsigned char buffer[188], null_buffer[188];
      
      dvb::mpeg::packet nullPacket; nullPacket.PID = 0x1fff;
      nullPacket.write ( null_buffer, 188 );


#define SEND(V,I) { BOOST_FOREACH(dvb::mpeg::packet_p P, V) { \
                  P->continuityCounter = (cc[P->PID]++) % 16; \
                  memset (buffer,0xff,sizeof(buffer)); \
                  P->write( buffer, sizeof(buffer) ); \
                  socket->sendBytes ( buffer, sizeof(buffer) ); \
                  sleeptime.tv_sec = 0; sleeptime.tv_nsec = I; nanosleep ( &sleeptime, &remaining); \
                  cnt++; \
        } }
      
      unsigned target_pps = bitrate*1024/8/188;
      long int sleep_time = 1e9 / target_pps;
      
     
    while (1) {
        now.update();

        try { 

        if (send_pat && last_pat_transmit.isElapsed( 1e3 * 1000) ) {
            SEND(p_pat,sleep_time);
            last_pat_transmit.update();
        }          

        if (send_time && last_time_transmit.isElapsed( 1e3 * 250) ) {
            SEND(p_tdt,sleep_time);
            SEND(p_tot,sleep_time);
            last_time_transmit.update();
        }

        if (send_epg && last_eit_transmit.isElapsed( 1e3 * 100) ) {
            SEND(p_eit_pf, sleep_time);
            last_eit_transmit.update();
        }

        if (last_time_update.isElapsed( 1e6 )) {
            tdt.utc = Poco::DateTime();
            tot.utc = Poco::DateTime();
            p_tdt = tdt.serialize_to_mpegts ( 0x14 );
            p_tot = tot.serialize_to_mpegts ( 0x14 );
            last_time_update.update();
        }

#define BITRATE_CHECK_INTERVAL 3

        if (last_transfer_check.isElapsed (BITRATE_CHECK_INTERVAL*1e6)) {
            unsigned pps = cnt / (last_transfer_check.elapsed() / 1e6);
            float offset = (float)pps / (float)target_pps;
            sleep_time = sleep_time * offset;
            cnt = 0;
            last_transfer_check.update();
        }

        sleeptime.tv_nsec = sleep_time;
        sleeptime.tv_sec = 0;

        nanosleep ( &sleeptime, &remaining );          
        socket->sendBytes( null_buffer, sizeof(null_buffer) );
        cnt++;

      } catch ( const Poco::Net::ConnectionRefusedException & e) {
           // wait for 1 second
          sleeptime.tv_sec = 1;
          nanosleep (&sleeptime, &remaining);
          last_transfer_check.update();
          cnt = 0;          
      } catch (const std::exception & e) {
          cout << "ERROR:"  << e.what() << endl;
          cout << typeid(e).name() << endl;
          break;
      }
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



