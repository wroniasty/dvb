#include <cpptest.h>
#include <string.h>
#include <math.h>
#include <ctime>
#include <iconv.h>

#include <iostream>
#include <sstream>
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
#include "Poco/AutoPtr.h"
#include "Poco/String.h"
#include "Poco/StringTokenizer.h"
#include "Poco/NumberParser.h"
#include "Poco/Base64Decoder.h"
#include "Poco/Net/IPAddress.h"
#include "Poco/Net/DatagramSocket.h"
#include "Poco/Net/MulticastSocket.h"
#include "Poco/Net/NetException.h"
#include "Poco/URI.h"
#include "Poco/LogStream.h"
#include "Poco/FormattingChannel.h"
#include "Poco/PatternFormatter.h"
#include "Poco/FileChannel.h"
#include "Poco/SignalHandler.h"

#include <bits/bits.h>
#include <bits/bits-stream.h>
#include <Poco/DateTimeFormatter.h>

#include "mpegts.h"
#include "sections.h"
#include "epg.h"
#include "io.h"        
#include "storage.h"


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
using Poco::URI;

using namespace dvb;

class MpegXMIT : public Application {
public:

    MpegXMIT() : _helpRequested(false) {

    }
protected:

    dvb::epg::target target;

    dvb::si::pat_section pat;
    dvb::si::tdt_section tdt;
    dvb::si::tot_section tot;
    dvb::si::eit_section_v eit_pf, eit_sched;
    dvb::mpeg::packet_v p_pat, p_tot, p_tdt, p_eit_pf, p_eit_sched;
    dvb::epg::schedule_v pf, sched;

    unsigned target_id, TSID, bitrate, si_version;

    bool send_time, send_pat, send_epg, 
       send_other_sched, send_other_epg;
 
    unsigned pat_interval, time_interval, epg_interval,
       epg_update_interval, target_update_interval, 
       epg_schedule_days, epg_schedule_interval,
       epg_other_interval, epg_other_update_interval,
       epg_other_schedule_days,
       epg_other_schedule_interval,
       other_target_update_interval;

    std::string default_language, diag_stream, database_uri;

    Poco::Timestamp now,
    last_pat_transmit,
    last_time_transmit, last_time_update,
    last_eit_transmit, last_eit_sched_transmit,
    last_transfer_check,
    last_epg_update, last_target_update;

    void initialize(Application& self) {
        loadConfiguration(); // load default configuration files, if present
        Application::initialize(self);
        // add your own initialization code here

        target_id = config().getInt("destination.target-id", 0);
        TSID = config().getInt("mpeg.tsid", 0);
        bitrate = config().getInt("mpeg.bitrate", 512);
        si_version = config().getInt("mpeg.version", 1);

        send_time = config().hasOption("mpeg.time");
        send_pat = config().hasOption("mpeg.pat");
        send_epg = config().hasOption("mpeg.epg");

        send_other_epg = config().hasOption("mpeg.other-epg");
        send_other_sched = config().hasOption("mpeg.other-sched");

        pat_interval = config().getInt("mpeg.pat-interval", 1000);
        time_interval = config().getInt("mpeg.time-interval", 900);
        epg_interval = config().getInt("mpeg.epg-interval", 500);
        epg_update_interval = config().getInt("mpeg.epg-update-interval", 60);
        epg_schedule_days = config().getInt("mpeg.epg-sched-days", 8);
        epg_schedule_interval = config().getInt("mpeg.epg-schedule-interval", 5000);
        target_update_interval = config().getInt("mpeg.target-update-interval", 10 * 60);

        /* other TS - not used yet */
        epg_other_interval = config().getInt("mpeg.epg-interval", 900);
        epg_other_update_interval = config().getInt("mpeg.epg-update-interval", 60);
        epg_other_schedule_days = config().getInt("mpeg.epg-sched-days", 2);
        epg_other_schedule_interval = config().getInt("mpeg.epg-schedule-interval", 10000);
        other_target_update_interval = config().getInt("mpeg.target-update-interval", 25 * 60);
        /***************************/
        
        default_language = config().getString("mpeg.epg-language", "pol");
        diag_stream = config().getString("diag.stream", "/dev/stdout");

        database_uri = config().getString("mpeg.database", "postgresql://mpegts@local/mpegts");

        Poco::AutoPtr<Poco::FileChannel> fc(new Poco::FileChannel(diag_stream));
        Poco::AutoPtr<Poco::PatternFormatter> pFmt(new Poco::PatternFormatter);
        pFmt->setProperty("pattern", "%Y-%m-%d %H:%M:%S mpegts-xmit: %t");
        Poco::AutoPtr<Poco::FormattingChannel> fmtC(new Poco::FormattingChannel(pFmt, fc));
        logger().setChannel(fmtC);

    }

    void uninitialize() {
        // add your own uninitialization code here
        Application::uninitialize();
    }

    void reinitialize(Application& self) {
        Application::reinitialize(self);
        // add your own reinitialization code here
    }

    void defineOptions(OptionSet& options) {
        Application::defineOptions(options);

        options.addOption(
                Option("help", "h", "display help information on command line arguments")
                .required(false)
                .repeatable(false)
                .callback(OptionCallback<MpegXMIT > (this, &MpegXMIT::handleHelp)));

        options.addOption(
                Option("define", "D", "define a configuration property")
                .required(false)
                .repeatable(true)
                .argument("name=value")
                .callback(OptionCallback<MpegXMIT > (this, &MpegXMIT::handleDefine)));

        options.addOption(
                Option("config-file", "f", "load configuration data from a file")
                .required(false)
                .repeatable(true)
                .argument("file")
                .callback(OptionCallback<MpegXMIT > (this, &MpegXMIT::handleConfig)));

        options.addOption(Option("target-id", "", "SI target record ID")
                .required(false)
                .argument("ID")
                .repeatable(false)
                .binding("destination.target-id"));

        options.addOption(Option("pat", "p", "Send PAT section")
                .required(false)
                .repeatable(false)
                .binding("mpeg.pat"));

        options.addOption(Option("pat-interval", "", "Interval between PAT transmits (ms)")
                .argument("ms")
                .repeatable(false)
                .binding("mpeg.pat-interval"));

        options.addOption(Option("time", "t", "Send TOT/TDT sections")
                .required(false)
                .repeatable(false)
                .binding("mpeg.time"));

        options.addOption(Option("time-interval", "", "Interval between TOT/TDT transmits (ms)")
                .argument("ms")
                .repeatable(false)
                .binding("mpeg.time-interval"));

        options.addOption(Option("epg", "e", "Send EPG sections")
                .required(false)
                .repeatable(false)
                .binding("mpeg.epg"));

        options.addOption(Option("epg-interval", "", "Interval between EIT transmits (ms)")
                .argument("ms")
                .repeatable(false)
                .binding("mpeg.epg-interval"));

        options.addOption(Option("epg-sched-interval", "", "Interval between EIT schedule transmits (ms)")
                .argument("ms")
                .repeatable(false)
                .binding("mpeg.epg-schedule-interval"));

        options.addOption(Option("epg-update-interval", "", "Interval between EPG content updates")
                .argument("seconds")
                .repeatable(false)
                .binding("mpeg.epg-update-interval"));

        options.addOption(Option("epg-sched-days", "", "Generate EPG schedule for next n days")
                .argument("n")
                .repeatable(false)
                .binding("mpeg.epg-sched-days"));

        options.addOption(Option("epg-language", "", "Default EPG language")
                .argument("code")
                .repeatable(false)
                .binding("mpeg.epg-language"));

        options.addOption(Option("target-update-interval", "", "Interval between EPG content reloads from DB")
                .argument("seconds")
                .repeatable(false)
                .binding("mpeg.target-update-interval"));

        options.addOption(Option("tsid", "", "Transport stream ID")
                .required(false)
                .argument("ID")
                .repeatable(false)
                .binding("mpeg.tsid"));

        options.addOption(Option("si-version", "", "SI tables version_number")
                .required(false)
                .repeatable(false)
                .argument("0-31")
                .binding("mpeg.version"));

        options.addOption(Option("output", "", "diagnostic messages log file (stdout)")
                .required(false)
                .repeatable(false)
                .argument("filename")
                .binding("diag.stream"));

        options.addOption(Option("database", "", "database URI")
                .required(false)
                .repeatable(false)
                .argument("uri")
                .binding("mpeg.database"));

        options.addOption(Option("bitrate", "", "Target stream bitrate")
                .argument("bitrate/kbps")
                .repeatable(false)
                .binding("mpeg.bitrate"));

        options.addOption(Option("dump-config", "", "Show all configuration options")
                .repeatable(false)
                .binding("diag.config"));


    }

    void handleHelp(const std::string& name, const std::string& value) {
        _helpRequested = true;
        displayHelp();
        stopOptionsProcessing();

    }

    void handleDefine(const std::string& name, const std::string& value) {
        defineProperty(value);
    }

    void handleConfig(const std::string& name, const std::string& value) {
        loadConfiguration(value);
    }

    void handleSource(const std::string& name, const std::string& value) {
        logger().information(name + " " + value);
    }

    void displayHelp() {
        HelpFormatter helpFormatter(options());
        helpFormatter.setCommand(commandName());
        helpFormatter.setUsage("[OPTIONS] destination_uri ...");
        helpFormatter.setHeader("MPEG transport stream transmitter.");
        helpFormatter.format(std::cout);
    }

    void defineProperty(const std::string& def) {
        std::string name;
        std::string value;
        std::string::size_type pos = def.find('=');
        if (pos != std::string::npos) {
            name.assign(def, 0, pos);
            value.assign(def, pos + 1, def.length() - pos);
        } else name = def;
        config().setString(name, value);
    }

    void reload_epg() {
        string msg;
        stringstream sout(msg);
        Poco::SharedPtr<soci::session> sql = dvb::storage::get_session(database_uri);

        target.init(*sql, target_id);

        sql->close();

        sout << "EPG services: ";

        BOOST_FOREACH(dvb::epg::service_p svc, target.services) {
            sout << svc->name << "(" << svc->sid << ") ";
        }
        logger().information(sout.str());

        last_target_update.update();
        update_pf_epg_content();
    }

    void update_pf_epg_content() {
        static unsigned i = 0;
        TSID = target.tsid;
        logger().information("Refreshing EIT P/F");

        p_eit_pf.clear();
        si_version = si_version % 31 + 1;

        BOOST_FOREACH(dvb::epg::service_p svc, target.services) {
            string msg = svc->name + " P/F ";
            pf = svc->present_following_event(now);
            std::string lang = default_language;
            if (pf[0]) {
                if (!pf[0]->info[lang]) {
                    if (pf[0]->info.size() == 0) continue;
                    lang = (*pf[0]->info.begin()).first;
                }
            }
            bool has0 = pf[0] && pf[0]->info[lang];
            bool has1 = pf[1] && pf[1]->info[lang];

            if (has0) {
                msg += Poco::DateTimeFormatter::format(pf[0]->start, "PRESENT:%H:%M \"") + pf[0]->info[lang]->title + "\" ";
            }
            if (has1) {
                msg += Poco::DateTimeFormatter::format(pf[1]->start, "FOLLOWING:%H:%M \"") + pf[1]->info[lang]->title + "\" ";
            }

            eit_pf =
                    dvb::si::eit_prepare_present_following(
                    svc->sid, si_version, 1, svc->tsid, target.origid,
                    (has0 ?
                    dvb::si::eit_section::make_event(
                    pf[0]->id, pf[0]->start, pf[0]->duration,
                    lang, pf[0]->info[lang]->title, pf[0]->info[lang]->text,
                    pf[0]->info[lang]->extended_text
                    ) : 0),
                    (has1 ?
                    dvb::si::eit_section::make_event(
                    pf[1]->id, pf[1]->start, pf[1]->duration,
                    lang, pf[1]->info[lang]->title, pf[1]->info[lang]->text,
                    pf[1]->info[lang]->extended_text
                    ) : 0)
                    );
            dvb::si::serialize_to_mpegts<dvb::si::eit_section > (0x12, eit_pf, p_eit_pf);
            msg += " (" + boost::lexical_cast<string> (p_eit_pf.size()) + ")";
            logger().information(msg);

            /* UPDATE SCHEDULE version_number !!! */
            //dvb::si::eit_section::event_v sched; /* = svc->get_schedule(now, now); */            
            //eit_sched = dvb::si::eit_prepare_schedule(svc->sid, si_version, 1, target.tsid, 1, 
            //        sched);
            //dvb::si::serialize_to_mpegts<dvb::si::eit_section> (0x12, eit_sched, p_eit_sched);                   

        }
        last_epg_update.update();
	
        if (++i % 20 == 1) {
	    update_sched_epg_content();
            i = 1;
        } else {
            p_eit_sched.clear();
	    
            BOOST_FOREACH(dvb::si::eit_section_p s, eit_sched) {
                s->version_number = si_version;
            }
            dvb::si::serialize_to_mpegts<dvb::si::eit_section > (0x12, eit_sched, p_eit_sched);
            logger().information ( string("Updated ") + boost::lexical_cast<string>(eit_sched.size()) + " sched sections." );
	    
        }
	
    }

    void update_sched_epg_content() {
        TSID = target.tsid;
        p_eit_sched.clear();
        logger().information("Refreshing EPG schedule.");
        eit_sched.clear();
        
        BOOST_FOREACH(dvb::epg::service_p svc, target.services) {
            string msg = svc->name + " ";
            dvb::si::eit_section_v svc_sched =
                dvb::si::eit_prepare_schedule(svc, Poco::DateTime(), epg_schedule_days, si_version, 1, svc->tsid, 1);
            msg += boost::lexical_cast<string > (svc_sched.size()) + " sections. ";
            logger().information(msg);
            eit_sched.insert ( eit_sched.end(), svc_sched.begin(), svc_sched.end() );
        };
        dvb::si::serialize_to_mpegts<dvb::si::eit_section > (0x12, eit_sched, p_eit_sched);
        last_epg_update.update();
    }

    int main(const std::vector<std::string>& args) {

        if (args.size() == 0) {
            displayHelp();
            _helpRequested = true;
        }
        if (_helpRequested) return Application::EXIT_OK;

        if (config().hasOption("diag.config")) printProperties("");
        //sout.open(diag_stream.c_str(), std::ios::out);

        dvb::io::output output;

        BOOST_FOREACH(string uri, args) {
            try {
                dvb::io::raw_channel_p c = dvb::io::ochannel_from_uri(uri);
                if (c) output.add_channel(c);
                else {
                    logger().error(string("Unable to initialize destination: ") + uri);
                }
            } catch (const Poco::Exception & e) {
                logger().error(e.displayText() + string(" - while initializing destination: ") + uri);
            }
        };

        if (output.channel_count() == 0) {
            logger().error("No valid destinations found.");
            return Application::EXIT_CONFIG;
        }


        if (send_pat) {
            logger().information("Sending PAT");
            pat.section_number = 0;
            pat.last_section_number = 0;
            pat.transport_stream_id = TSID;
            pat.section_syntax_indicator = 1;
            pat.current_next_indicator = 1;
            pat.add_program(0, 0x10); /* network */
            p_pat = pat.serialize_to_mpegts(0x00);
        }

        if (send_time) {
	  /*
European Summer Time begins (clocks go forward) at 01:00 UTC on

    31 March 2013
    30 March 2014
    29 March 2015
    27 March 2016
    26 March 2017

Formula used to calculate the beginning of European Summer Time:

Sunday (31 − ((((5 × y) ÷ 4) + 4) mod 7)) March at 01:00 GMT

(valid until 2099[4]), where y is the year, and for the nonnegative a, a mod b is the remainder of division after truncating both operands to an integer.

European Summer Time ends (clocks go backward) at 01:00 GMT on

    27 October 2013
    26 October 2014
    25 October 2015
    30 October 2016
    29 October 2017

    Formula used to calculate the end of European Summer Time:

    Sunday (31 − ((((5 × y) ÷ 4) + 1) mod 7)) October at 01:00 GMT
	   */
            logger().information("Sending TDT/TOT");
            logger().information("Time offset is +01:00");
            tot.add_offset("POL", 0, 0, 0x0100, (unsigned long long) dvb::MJD(2014, 3, 30) * 0x1000000 + 0x010000, 0x0100);
        }

        if (send_epg) {
            logger().information("Sending EPG");
            reload_epg();
        }

        unsigned target_pps = bitrate * 1024 / 8 / 188;
        unsigned long long sleep_time = 1e9 / target_pps;
	unsigned long long min_sleep_time = sleep_time / 3;
        
        logger().information("Streaming...");

	//	dvb::nanosleep (1, 0);
	last_pat_transmit.update();
	last_time_transmit.update();
	last_eit_transmit.update();
	last_transfer_check.update();

        try {
            
            while (1) {
                now.update();

                if (send_pat && last_pat_transmit.isElapsed(1e3 * pat_interval)) {
                    output.write(p_pat, sleep_time);
                    last_pat_transmit.update();
                }

                if (send_time && last_time_transmit.isElapsed(1e3 * time_interval)) {
                    output.write(p_tdt, sleep_time);
                    output.write(p_tot, sleep_time);
                    last_time_transmit.update();
                }

                if (send_epg) {
                    if (last_eit_transmit.isElapsed(1e3 * epg_interval)) {
                       output.write(p_eit_pf, sleep_time);
                       logger().debug(boost::lexical_cast<string> ( p_eit_pf.size() ) + " PF pkts." );
                       last_eit_transmit.update();
                    }
                    if (last_eit_sched_transmit.isElapsed(1e3 * epg_schedule_interval)) {
                       output.write(p_eit_sched, sleep_time);
                       logger().debug(boost::lexical_cast<string> ( p_eit_sched.size() ) + " SCHED pkts." );
                       last_eit_sched_transmit.update();
                    }
                }

                if (last_time_update.isElapsed(1e6)) {
                    tdt.utc = Poco::DateTime();
                    tot.utc = Poco::DateTime();
                    p_tdt = tdt.serialize_to_mpegts(0x14);
                    p_tot = tot.serialize_to_mpegts(0x14);
                    last_time_update.update();
                }
#define BITRATE_CHECK_INTERVAL 3

                if (last_transfer_check.isElapsed(BITRATE_CHECK_INTERVAL * 1e6)) {
                    unsigned pps = output.counter() / (last_transfer_check.elapsed() / 1e6);
                    float offset = ((float) pps / (float) target_pps);
                    sleep_time = sleep_time * offset;
		    if (sleep_time < min_sleep_time) sleep_time = min_sleep_time;
                    logger().information(boost::lexical_cast<string> ( pps ) + " pps " + boost::lexical_cast<string> ( sleep_time ) );
                    output.counter_reset();
                    last_transfer_check.update();
                }

                if (last_epg_update.isElapsed(1e6 * epg_update_interval)) {
                    update_pf_epg_content();
                }

                if (last_target_update.isElapsed(1e6 * target_update_interval)) {
                    reload_epg();
                }

                output.write_null(sleep_time);



            } /* while(1) */

        } catch (const Poco::Exception & e) {
            logger().log(e);
        } catch (const std::exception & e) {
            logger().error(string(e.what()) + " :: " + typeid (e).name());
        }
        
        logger().information("Exiting.");

        return Application::EXIT_OK;
    }

    void printProperties(const std::string& base) {
        AbstractConfiguration::Keys keys;
        config().keys(base, keys);
        if (keys.empty()) {
            if (config().hasProperty(base)) {
                std::string msg;
                msg.append(base);
                msg.append(" = ");
                msg.append(config().getString(base));
                logger().information(msg);
            }
        } else {
            for (AbstractConfiguration::Keys::const_iterator it = keys.begin(); it != keys.end(); ++it) {
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



