#include <cpptest.h>
#include <string.h>
#include <math.h>

#include <iostream>
#include <fstream>
#include <string>
#include <cassert>
#include <vector>
#include <map>

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

#include "mpegts.h"
        
         
using Poco::Util::Application;
using Poco::Util::Option;
using Poco::Util::OptionSet;
using Poco::Util::HelpFormatter;
using Poco::Util::AbstractConfiguration;
using Poco::Util::OptionCallback;
using Poco::Net::IPAddress;
using Poco::SharedPtr;
using Poco::StringTokenizer;

using namespace dvb;

typedef std::vector<unsigned char> BinaryBuffer;
typedef struct { unsigned PID; SharedPtr<BinaryBuffer> data; unsigned size; } Section;
typedef std::vector< SharedPtr<Section> > SectionList;


SharedPtr< SectionList > readSectionFileData ( std::istream & i ) {
  SectionList * sections = new SectionList();

  while (!i.eof()) {
    std::string line;
    std::getline(i, line);
    Poco::StringTokenizer T (line, " ");
    if (T.count() == 2) {
      SharedPtr<Section> section ( new Section() );
      section->PID = Poco::NumberParser::parseUnsigned (T[0]);
      std::cout << section->PID << std::endl; 
      /*
      char buffer[T[1].length()];
      istringstream out(T[1]);
      Poco::Base64Decoder b64( out ); b64.read(buffer, sizeof(buffer));
      vector<char> section (buffer, buffer + sizeof(buffer));
      cout << PID << " " << section.size() << " " << T[1].length() <<  endl;
      targets[F[0]][PID].push_back (section);
      pidContinuity[F[0]][PID] = 0;
      */

    }
  } 

  return SharedPtr <SectionList> ( sections );
}


void readSectionFile ( const char * filename, std::map<std::string, SectionList > & storage ) {
  std::cout << "READING " << filename << std::endl;
  std::ifstream pf (filename, std::ios::in );
  std::string storageKey;

  std::getline(pf, storageKey);  
  StringTokenizer header(storageKey," ");
  storageKey = header[0] + ":" + header[1];

  std::cout << storageKey << std::endl;

  SharedPtr< SectionList > readSections = readSectionFileData (pf);
  storage[storageKey].insert ( storage[storageKey].end(), readSections->begin(), readSections->end() );

  pf.close();  
} 

         
class MpegXMIT: public Application
{
public:
  MpegXMIT(): _helpRequested(false)
  {       
  }
protected:	
  std::map<std::string, SectionList > sections_pf;
  std::map<std::string, SectionList > sections_sched;
  std::map<std::string, SectionList > sections_other;

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

    options.addOption ( Option( "pf", "p", "EIT present-following sections source file")
			.required(false)
			.repeatable(true)
			.argument("file")
			.callback(OptionCallback<MpegXMIT>(this, &MpegXMIT::handleSource)));

    options.addOption ( Option( "sched", "s", "EIT sched sections source file")
			.required(false)
			.repeatable(true)
			.argument("file")
			.callback(OptionCallback<MpegXMIT>(this, &MpegXMIT::handleSource)));

    options.addOption ( Option( "other", "o", "Other SI sections source file")
			.required(false)
			.repeatable(true)
			.argument("file")
			.callback(OptionCallback<MpegXMIT>(this, &MpegXMIT::handleSource)));

    options.addOption ( Option( "dst-address", "", "Destination IP address")
			.required(false)
			.repeatable(true)
			.argument("ip")
			.binding("destination.ip"));

    options.addOption ( Option( "dst-port", "", "Destination UDP port")
			.required(false)
			.repeatable(true)
			.argument("ip")
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
    if (name == "pf") 
      readSectionFile(value.c_str(), sections_pf);
    else if (name == "sched")
      readSectionFile(value.c_str(), sections_sched);
    else if (name == "other")
      readSectionFile(value.c_str(), sections_other);
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



/*
using namespace std;

#include <Poco/String.h>
#include <Poco/StringTokenizer.h>
#include <Poco/NumberParser.h>
#include <Poco/Base64Decoder.h>
#include <Poco/Net/IPAddress.h>

#include <sstream>
#include <map>

typedef map<unsigned, vector<vector<char> > > Target;
typedef map<string, Target > Targets;

int main(int argc, char *argv[]) {
  std::ifstream pf ("../data/target_3_sched.sec", std::ios::in );
  std::string line;

  Targets targets;
  
  std::getline(pf, line);
  Poco::StringTokenizer F(line," ");
  
  Poco::Net::IPAddress dstaddress = Poco::Net::IPAddress(F[0]);
  unsigned dstport = Poco::NumberParser::parseUnsigned(F[1]);
  
  //targets[F[0]] = map<unsigned, vector<string>> ();
    
  cout << dstaddress.toString() << ":" << dstport << endl << endl;

  map<string, map<unsigned, short>> pidContinuity;
							       
  while (!pf.eof()) {
    std::getline(pf, line);
    Poco::StringTokenizer T (line, " ");
    //    cout << " >> " << T.count() << endl;
    if (T.count() == 2) {
      unsigned PID = Poco::NumberParser::parseUnsigned (T[0]);
      char buffer[T[1].length()];
      istringstream out(T[1]);
      Poco::Base64Decoder b64( out ); b64.read(buffer, sizeof(buffer));
      vector<char> section (buffer, buffer + sizeof(buffer));
      cout << PID << " " << section.size() << " " << T[1].length() <<  endl;
      targets[F[0]][PID].push_back (section);
      pidContinuity[F[0]][PID] = 0;
    }
  } 


  for ( Targets::iterator tg = targets.begin(); tg != targets.end(); tg++) {
    string target = (*tg).first;
    for (Target::iterator it = (*tg).second.begin(); it != (*tg).second.end(); it++) {
      unsigned PID = (*it).first;
      cout << "  " << PID << " " << endl;
      for (int idx=0;idx < (*it).second.size(); idx++) {
	vector<char> data = (*it).second[idx];
	short CC = (++pidContinuity[target][PID]) %= 16;
	cout << "      " << idx << " "<<CC<<" packets: " << (data.size() / 188 + 1) << endl;
      }
    } 
  }
  
			    
  pf.close();

  
  std::ifstream sample ("../data/sample1.ts", std::ios::binary|std::ios::in);

  while (!sample.eof()) {
    unsigned char buffer[188];
    sample.read ( (char *)buffer, sizeof(buffer));
    packet P1(buffer);
    if (P1.null) continue;
    cout << "-- - --" << hex << P1.PID << " " << dec << P1.continuityCounter << endl;
    P1.write (buffer, 188);
    packet P2(buffer);

    assert ( P1.PID == P2.PID );
    assert ( P1.continuityCounter == P2.continuityCounter );

  }
  
  return 0;
}

*/
