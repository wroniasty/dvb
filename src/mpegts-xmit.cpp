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
#include <boost/tuple/tuple.hpp>

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

    options.addOption ( Option( "tsid", "", "Transport stream ID")
			.required(true)
                        .argument ( "ID" )
			.repeatable(false)
			.binding("mpeg.tsid"));

    options.addOption ( Option( "bitrate", "", "Target stream bitrate")
                        .argument ( "bitrate/kbps" )
			.repeatable(false)
			.binding("mpeg.bitrate"));

    options.addOption ( Option( "service", "", "Add service to EIT/SDT")
			.required(false)
                        .repeatable(true)
                        .argument ( "Service_ID" )
			.repeatable(false)
			.binding("mpeg.tsid"));

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
      
      dvb::epg::service sv1;

      sv1.sid = 1000;
      sv1.name = "Service 1";
      sv1.reload_epg( sql, 17537, Poco::DateTime() );
      
//      sql.set_log_stream( &cout );
      time_t t2 = time(NULL);
      std::tm t3;  gmtime_r(&t2, &t3);

      soci::rowset<soci::row> pf_events = (sql.prepare << 
              "SELECT * FROM eit_collected_event WHERE service_description_id = 17537 "
           << " AND ((start_time <= :now AND end_time >= :now) OR start_time >= :now ) ORDER BY start_time LIMIT 2"
              , soci::use(t3, "now")
      );

      //      std::copy(rs.begin(), rs.end(), std::ostream_iterator<std::string>(std::cout, "\n"));
      for (soci::rowset<soci::row>::const_iterator it=pf_events.begin(); it!=pf_events.end(); ++it) {
          int event_id = (*it).get<int> ("event_id"),
              id = (*it).get<int> ("id");
                            
          soci::rowset<soci::row> event_desc = (sql.prepare <<
                  "SELECT * FROM eit_collected_event_description WHERE collected_event_id = :id "
               << "AND name IS NOT NULL"
                  , soci::use( id, "id" )
          );
          std::tm start_time = (*it).get<std::tm>("start_time");
          
          cout << event_id   
               << " " << asctime(&start_time)
               << endl;
          
          for (soci::rowset<soci::row>::const_iterator itd = event_desc.begin();
                         itd != event_desc.end(); ++itd) {                             
                cout << "  + " << (*itd).get<string> ( "name") << endl;
          }
      }
      
      return 1;
      
      unsigned TSID = config().getInt( "mpeg.tsid",  1);
      unsigned bitrate = config().getInt( "mpeg.bitrate",  512);
      Poco::Net::IPAddress dst_ip ( config().getString("destination.ip", "127.0.0.1") );
      int dst_port = config().getInt("destination.port", 12345);

      bool send_time = config().getBool( "mpeg.time", false );
      bool send_pat = config().getBool( "mpeg.pat", false );
      
      
      dvb::si::pat_section pat;
      dvb::si::tdt_section tdt;
      dvb::si::tot_section tot;
      dvb::si::eit_section_v pf;
      
      dvb::mpeg::packet_v p_pat, p_tot, p_tdt, p_eit;

      Poco::Timestamp now, 
              last_pat_transmit, 
              last_time_transmit, last_time_update, 
              last_eit_transmit,
              last_transfer_check;
      
      if (send_pat) {
          pat.section_number = 0;
          pat.last_section_number = 0;
          pat.transport_stream_id = TSID;
          pat.section_syntax_indicator = 1;
          pat.current_next_indicator = 1;
          pat.add_program(0, 0x10);  /* network */
          p_pat = pat.serialize_to_mpegts(0x00);
      }
      
      if (send_time) {
          tot.add_offset ( "POL", 0, 1, 0x0200, dvb::MJD(2012,10,01) * 0x1000000 + 0x033000, 0x0100);
          p_tdt = tdt.serialize_to_mpegts ( 0x14 );
          p_tot = tot.serialize_to_mpegts(0x14);
      }

      
      struct timespec sleeptime, remaining;
      
      Poco::Net::SocketAddress dst_addr ( dst_ip, dst_port);
      Poco::Net::DatagramSocket socket (       
          Poco::Net::SocketAddress  ( 
              Poco::Net::IPAddress(), 
              dst_port)
        );
      socket.connect( dst_addr );
      
      logger().information( std::string("Sending to ") + dst_addr.toString() );

      std::map<unsigned, unsigned> cc; unsigned cnt = 0; 
      unsigned char buffer[188], null_buffer[188];
      
      dvb::mpeg::packet nullPacket; nullPacket.PID = 0x1fff;
      nullPacket.write ( null_buffer, 188 );


#define SEND(V,I) { BOOST_FOREACH(dvb::mpeg::packet_p P, V) { \
                  P->continuityCounter = (cc[P->PID]++) % 16; \
                  P->write( buffer, sizeof(buffer) ); \
                  socket.sendBytes ( buffer, sizeof(buffer) ); \
                  sleeptime.tv_sec = 0; sleeptime.tv_nsec = I; nanosleep ( &sleeptime, &remaining); \
                  cnt++; \
        } }
      
      unsigned target_pps = bitrate*1024/8/188;
      long int sleep_time = 1e9 / target_pps;
      
      while (1) {
          now.update();

          if (send_pat && last_pat_transmit.isElapsed( 1e3 * 1000) ) {
              SEND(p_pat,sleep_time);
              cout << "PAT" << endl;
              last_pat_transmit.update();
          }          
          
          if (send_time && last_time_transmit.isElapsed( 1e3 * 250) ) {
              SEND(p_tdt,sleep_time);
              SEND(p_tot,sleep_time);
              cout << "TIME" << endl;
              last_time_transmit.update();
          }
          
          if (last_time_update.isElapsed( 1e6 )) {
              tdt.utc = Poco::DateTime();
              tot.utc = Poco::DateTime();
              p_tdt = tdt.serialize_to_mpegts ( 0x14 );
              p_tot = tot.serialize_to_mpegts ( 0x14 );
              cout << "TIME UPDATE" << endl;
              last_time_update.update();
          }
          
#define BITRATE_CHECK_INTERVAL 3
          if (last_transfer_check.isElapsed (BITRATE_CHECK_INTERVAL*1e6)) {
              unsigned pps = cnt / BITRATE_CHECK_INTERVAL;
              cout << "PPS " << pps << " " << target_pps << endl;
              
              float offset = (float)pps / (float)target_pps;
              cout << "OFFSET " << offset << endl;

              sleep_time = sleep_time * offset;
              cout << "ST " << sleep_time << endl;
              
              cnt = 0;
              last_transfer_check.update();
          }
          
          sleeptime.tv_nsec = sleep_time;
          sleeptime.tv_sec = 0;

          nanosleep ( &sleeptime, &remaining );
          socket.sendBytes( null_buffer, sizeof(null_buffer) );
          cnt++;
      }
      
      /*      
      pf = dvb::si::eit_prepare_present_following (
         0x1000, 1, 1, 1, 1,
           dvb::si::eit_section::make_event (
              1000, Poco::DateTime(), Poco::Timespan(0,0,60,0,0),
              "POL", "Present Event", "Some present event", "Long text of Present"
           ),
           dvb::si::eit_section::make_event (
              1001, Poco::DateTime() + Poco::Timespan(0,0,60,0,0), Poco::Timespan(0,0,60,0,0),
              "POL", "Following Event", "Some following event", "Long text of Following"
           )
      );
      
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
      
      BOOST_FOREACH ( dvb::si::eit_section_p eit, pf) {
          dvb::mpeg::packet_v pkt = eit->serialize_to_mpegts(0x12);
          packets.insert ( packets.end(), pkt.begin(), pkt.end() );
      }
      
      unsigned char buffer[188], null_buffer[188];
      
      dvb::mpeg::packet nullPacket; nullPacket.PID = 0x1fff;
      nullPacket.write ( null_buffer, 188 );
      
      
      std::map<unsigned, unsigned> cc; unsigned long cnt = 0; 

      sleeptime.tv_sec = 0;
      sleeptime.tv_nsec = 1e6;
      
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
      */
      
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



