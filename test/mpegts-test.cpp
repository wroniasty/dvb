#include <iostream>
#include <string>
#include <vector>
#include <map>
#include <set>

#include <typeinfo>

#include <cpptest.h>

#include "Poco/NotificationCenter.h"
#include "Poco/Notification.h"
#include "Poco/Observer.h"
#include "Poco/NObserver.h"
#include "Poco/AutoPtr.h"
#include "Poco/SharedPtr.h"

#include <boost/foreach.hpp>

#include <string.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#include <fstream>

#include "dvb.h"
#include "epg.h"
#include "descriptors.h"
#include "analyzer.h"

#include "boost/format.hpp"

using namespace std;
using namespace dvb;
using namespace dvb::mpeg;
using namespace dvb::si;

class MpegTestSuite : public Test::Suite {
public:

  MpegTestSuite() {
    TEST_ADD(MpegTestSuite::packet_random_test);
    TEST_ADD(MpegTestSuite::stream_random_test);
    TEST_ADD(MpegTestSuite::stream_data_test);
    TEST_ADD(MpegTestSuite::analyzer_data_test);
    TEST_ADD(MpegTestSuite::sections_test);
    TEST_ADD(MpegTestSuite::descriptors_test);
    TEST_ADD(MpegTestSuite::epg_test);
    TEST_ADD(MpegTestSuite::eit_section_test);
    TEST_ADD(MpegTestSuite::pat_section_test);
    TEST_ADD(MpegTestSuite::tdt_section_test);
    TEST_ADD(MpegTestSuite::tot_section_test);
  }

protected:

  virtual void setup() {
    srand(time(NULL));
  }

public:

  void make_random_buffer (unsigned char *buffer, size_t size) {
    for (int b=0;b<size;b++) buffer[b] = rand() & 0xff;
  }

  void packet_random_test() {
    unsigned char sampleData_1[188], sampleData_2[188];
    mpeg::packet packet_1;

    for (int j=0;j<10000;j++) {
      make_random_buffer (sampleData_1, sizeof(sampleData_1));
      sampleData_1[0] = 0x47;

      TEST_ASSERT( packet_1.read (sampleData_1) );
      TEST_ASSERT( packet_1.write (sampleData_2, sizeof(sampleData_2)));

      if (!packet_1.null)
            TEST_ASSERT ( memcmp ( sampleData_1, sampleData_2, sizeof(sampleData_1)) == 0 );
    }

  }

  void handle_payload_unit (const Poco::AutoPtr<mpeg::stream::payload_unit_ready> & puN) {
    Poco::SharedPtr < vector<unsigned char> > buffer(puN->buffer);
    
    if (puN->PID == 0) {
        dvb::si::pat_section pat; //TEST_ASSERT ( pat.read(*buffer) == dvb::si::SECTION_OK );
        //for ( dvb::si::pat_section::programs_v::iterator it = pat.programs.begin(); it != pat.programs.end(); it++ ) {
        //}
    } else if (puN->PID == 0x12) {
        dvb::si::eit_section eit; eit.read (*buffer);
        //for (dvb::si::eit_section::events_v::iterator it = eit.events.begin(); it != eit.events.end(); it++) {
        //    for (dvb::si::descriptors_v::iterator itd = (*it)->descriptors.begin(); itd != (*it)->descriptors.end(); itd++) {
        //    }
        //}
    }
  }

  void stream_random_test() {
    unsigned char sampleData_1[256], sampleData_2[256];
    mpeg::stream s;
    s.addObserver ( Poco::NObserver<MpegTestSuite, mpeg::stream::payload_unit_ready> ( *this, &MpegTestSuite::handle_payload_unit ) );
    for (int j=0;j<10000;j++) {
      make_random_buffer (sampleData_1, sizeof(sampleData_1));
      sampleData_1[0] = 0x47;
      s << sampleData_1;
    }
    s.removeObserver ( Poco::NObserver<MpegTestSuite, mpeg::stream::payload_unit_ready> ( *this, &MpegTestSuite::handle_payload_unit ) );
  }

  void sections_test() {
      
  }
  
  void tot_section_test() {
      unsigned char buffer[1024];
      memset (buffer, 0, 1024);
      bits::bitstream stream(buffer);
      
      dvb::si::tot_section tot1, tot2;
      TEST_ASSERT ( tot1.table_id == 0x73 );
      
      tot1.add_offset ( "pol", 1 );      
      tot1.write ( stream );
      
      stream.rewind();
      
      TEST_ASSERT (tot2.read (stream) == dvb::si::SECTION_OK );

      TEST_ASSERT (tot1.descriptors.size() == 1);
      TEST_ASSERT (tot1.descriptors.size() == tot2.descriptors.size());
 
      
      TEST_ASSERT ( tot1.utc.year() == tot2.utc.year() );
      TEST_ASSERT ( tot1.utc.month() == tot2.utc.month() );
      TEST_ASSERT ( tot1.utc.day() == tot2.utc.day() );

      TEST_ASSERT ( tot1.utc.hour() == tot2.utc.hour() );
      TEST_ASSERT ( tot1.utc.minute() == tot2.utc.minute() );
      TEST_ASSERT ( tot1.utc.second() == tot2.utc.second() );
      
      
  }
  
  void tdt_section_test() {
      unsigned char buffer[1024];
      memset (buffer, 0, 1024);
      bits::bitstream stream(buffer);
      
      dvb::si::tdt_section tdt1, tdt2;
      
      Poco::DateTime now, now2;
      
      tdt1.utc = now;
      
      unsigned mjd = dvb::MJD( now.year(), now.month(), now.day());
      
      now2 = dvb::fromMJD( mjd );      
      TEST_ASSERT ( now.year() == now2.year() );
      TEST_ASSERT ( now.month() == now2.month() );
      TEST_ASSERT ( now.day() == now2.day() );
      
      now2 = dvb::fromMJD( 0xc079 );      
      TEST_ASSERT ( 1993 == now2.year() );
      TEST_ASSERT ( 10 == now2.month() );
      TEST_ASSERT ( 13 == now2.day() );

      TEST_ASSERT ( tdt1.table_id == 0x70 );
      TEST_ASSERT ( !tdt1.has_crc );

      tdt1.write (stream);
      stream.rewind();
      
      TEST_ASSERT (tdt2.read (stream) == dvb::si::SECTION_OK );
     
      TEST_ASSERT ( tdt1.table_id == tdt2.table_id );
      TEST_ASSERT ( tdt1.section_length == tdt2.section_length );
      
      TEST_ASSERT ( tdt1.utc.year() == tdt2.utc.year() );
      TEST_ASSERT ( tdt1.utc.month() == tdt2.utc.month() );
      TEST_ASSERT ( tdt1.utc.day() == tdt2.utc.day() );

      TEST_ASSERT ( tdt1.utc.hour() == tdt2.utc.hour() );
      TEST_ASSERT ( tdt1.utc.minute() == tdt2.utc.minute() );
      TEST_ASSERT ( tdt1.utc.second() == tdt2.utc.second() );
            
  }
  
  void pat_section_test() {
      unsigned char buffer[1024];
      memset (buffer, 0, 1024);
      bits::bitstream stream(buffer);
      
      dvb::si::pat_section pat1, pat2;

      pat1.add_program(0, 10);
      pat1.add_program(1000, 1234);
      pat1.add_program(1005, 389);

      TEST_ASSERT ( pat1.programs.size() == 3);
      
      pat1.write(stream);            
      stream.rewind();
      
      TEST_ASSERT_MSG (pat2.read(stream) == dvb::si::SECTION_OK, "CRC CHECK" );
      
      TEST_ASSERT ( pat1.table_id == pat2.table_id );
      TEST_ASSERT ( pat1.section_length == pat2.section_length );
      TEST_ASSERT ( pat1.programs.size() == pat2.programs.size() );

      for (int idx=0; idx < pat2.programs.size(); idx++) {
          TEST_ASSERT ( pat1.programs[idx]->program_map_pid == pat2.programs[idx]->program_map_pid );
          TEST_ASSERT ( pat1.programs[idx]->program_number == pat2.programs[idx]->program_number );
      }
      
      /* check if max-size restriction are enforced */
      for (int idx=0; idx < 300; idx++) pat2.add_program(idx+2000, idx+3000);
      
      TEST_ASSERT ( pat2.calculate_section_length() <= pat2.max_length - 3 );
      TEST_ASSERT ( pat2.add_program(1000, 1000) == 0 );
            
      dvb::mpeg::packet_v ps = pat1.serialize_to_mpegts(0x00);
      dvb::mpeg::analyzer a;      
      BOOST_FOREACH ( mpeg::packet_p p, ps) {
          a << *p;
      }
      a.flush();
      TEST_ASSERT ( a.pmt_pids.size() == pat1.programs.size() );

      a.pmt_pids.clear();
      ps = pat2.serialize_to_mpegts(0x00);
      BOOST_FOREACH ( mpeg::packet_p p, ps) {
          a << *p;
      }
      a.flush();
      TEST_ASSERT ( a.pmt_pids.size() == (pat2.programs.size() ) );
      
  }

  void eit_section_test() {
      unsigned char buffer[4096];
      memset (buffer, 0, 4096);
      bits::bitstream stream(buffer);
      
      dvb::si::eit_section eit1, eit2;
      dvb::si::eit_section::event_p ev;
      
      eit1.events.push_back(
        ev = dvb::si::eit_section::make_event(0x1000, Poco::DateTime(), 
              Poco::Timespan(0,0,120,0,0),
              "POL", "Event Title", "Short desc", "Lone long lone desc")
      );
      
      eit1.write(stream);      
      stream.rewind();
      
      TEST_ASSERT( eit2.read (stream) == dvb::si::SECTION_OK );

      TEST_ASSERT ( eit2.table_id == eit1.table_id );
      TEST_ASSERT ( eit2.events.size() == eit1.events.size() );
      
      dvb::si::eit_section_v pf;
      
      pf = dvb::si::eit_prepare_present_following (
         0x1000, 1, 1, 1, 1,
           dvb::si::eit_section::make_event (
              1000, Poco::DateTime(), Poco::Timespan(0,0,60,0,0),
              "POL", "Present Event", "Some present event", "Long text of Present"
           ),
           dvb::si::eit_section::make_event (
              1000, Poco::DateTime() + Poco::Timespan(0,0,60,0,0), Poco::Timespan(0,0,60,0,0),
              "POL", "Following Event", "Some following event", "Long text of Following"
           )
      );
      
     TEST_ASSERT( pf[0]->table_id == 0x4e );
     TEST_ASSERT( pf[0]->events.size() == 1 );

     TEST_ASSERT( pf[1]->table_id == 0x4e );
     TEST_ASSERT( pf[1]->events.size() == 1 );

 }
  
  void epg_test() {
      dvb::epg::service svc1(100, "TVP1");
      
      svc1.new_event(1000, 100, 60);
      svc1.new_event(1001, 160, 120);
      svc1.new_event(1002, 60, 40);
      
      dvb::epg::event_p e = svc1.get_event(1000);
      
      TEST_ASSERT ( e->id == 1000 && e->start == 100 );
      
      e = svc1.current_event( 80 );
      TEST_ASSERT ( !e.isNull() );
      
      if (e) {
        TEST_ASSERT (e->id == 1002);
      }
      
      std::list<dvb::epg::event_p> s = svc1.get_schedule(100, 200);
      e = s.front(); TEST_ASSERT ( e->id == 1000 );
      s.pop_front(); e = s.front(); TEST_ASSERT ( e->id == 1001 );

      dvb::epg::event::event_info info = e->get_info("pol");
      info->title = "Program title PL";
      info->text = "Program info PL";
      info->extended_text = "More info PL";      
      info->items["year"] = "2012";
      
      info = e->get_info ( "eng" );
      info->title = "Program title EN";
      info->text = "Program info EN";
      info->extended_text = "More info EN";      
      info->items["year"] = "2012";
      
      info = e->get_info ("pol");
      TEST_ASSERT ( info->title == "Program title PL");

      info = e->get_info ("eng");
      TEST_ASSERT ( info->title == "Program title EN");
      
  }

  void descriptors_test() {      
      unsigned char buffer[4096];
      bits::bitstream stream ( buffer );
      dvb::si::descriptor d1, d2;      
      
      /* events */
      
      Poco::SharedPtr<dvb::si::short_event_descriptor>  
         sd1 ( d1.set_data<dvb::si::short_event_descriptor>() );

      sd1->event_name = "EVENT1234567890";
      sd1->text = "TEXT1234567890";
      sd1->ISO_639_language_code = "pol";
      
      d1.write ( stream );      
      stream.rewind();
      d2.read (stream);
 
      TEST_ASSERT ( d2.tag == d1.tag );
      TEST_ASSERT ( d2.length == d1.length );
      
      Poco::SharedPtr<dvb::si::short_event_descriptor>  
         sd2 ( d2.get_data<dvb::si::short_event_descriptor>() );

      TEST_ASSERT ( sd1->event_name == sd2->event_name );
      TEST_ASSERT ( sd1->text == sd2->text );

      Poco::SharedPtr<extended_event_descriptor> 
         sd3 ( d1.set_data<extended_event_descriptor> () );
      
      sd3->add_item( "screenplay", "John Smith");
      sd3->add_item( "year", "1999" );
      
      sd3->ISO_639_language_code = "pol";
      sd3->text = "MOVIE123451234512345";
      sd3->descriptor_number = 0;
      sd3->last_descriptor_number = 10;
      
      stream.rewind();
      d1.write (stream);
      stream.rewind();
      d2.read (stream);

      
      Poco::SharedPtr<extended_event_descriptor> 
         sd4 ( d2.get_data<extended_event_descriptor> () );
      
      TEST_ASSERT ( d2.tag == d1.tag );
      TEST_ASSERT ( d2.length == d1.length );      
      TEST_ASSERT ( sd4->descriptor_number == sd3->descriptor_number );
      TEST_ASSERT ( sd4->last_descriptor_number == sd3->last_descriptor_number );
      TEST_ASSERT ( sd4->text == sd3->text );

      Poco::SharedPtr<cable_delivery_system_descriptor> 
         sd5 ( d1.set_data<cable_delivery_system_descriptor> () );
      
      sd5->frequency = 768000;
      sd5->FEC_inner = 0;
      sd5->FEC_outer = 0;
      sd5->modulation = QAM256;
      sd5->symbol_rate = 96000;

      stream.rewind();
      d1.write (stream);
      stream.rewind();
      d2.read (stream);
      
      Poco::SharedPtr<cable_delivery_system_descriptor> 
         sd6 ( d2.get_data<cable_delivery_system_descriptor> () );

      TEST_ASSERT ( d2.tag == d1.tag );
      TEST_ASSERT ( d2.length == d1.length );      
      
      TEST_ASSERT ( sd6->modulation == sd5->modulation );
      TEST_ASSERT ( sd6->FEC_inner == sd5->FEC_inner );
      TEST_ASSERT ( sd6->FEC_outer == sd5->FEC_outer );
      TEST_ASSERT ( sd6->frequency == sd5->frequency );
        
  }
  
  void stream_data_test() {
    ifstream data("data/sample1.ts", ios::in | ios::binary );
    TEST_ASSERT (data);

    data.seekg(0, ios::end);
    ifstream::pos_type length = data.tellg();
    data.seekg(0, ios::beg);

    vector<unsigned char> buffer(188);

    mpeg::stream s;
    s.addObserver ( Poco::NObserver<MpegTestSuite, mpeg::stream::payload_unit_ready> ( *this, &MpegTestSuite::handle_payload_unit ) );

    ifstream::pos_type offset = 0;

    unsigned allowed_pids[] = {0,0x10, 0x11, 0x12, 0x13, 0x14};
    std::set<unsigned> allowed(allowed_pids, allowed_pids + 5);

    while (length - offset >= 188) {
        data.read ((char *)&buffer[0], 188);
        if ( allowed.count(mpeg::get_packet_pid((unsigned char *) &buffer[0])) == 1)
            s << (unsigned char *) &buffer[0];
        offset += 188;
    }
    s.removeObserver ( Poco::NObserver<MpegTestSuite, mpeg::stream::payload_unit_ready> ( *this, &MpegTestSuite::handle_payload_unit ) );
    data.close();

  }

  void analyzer_data_test() {
    ifstream data("data/sample1.ts", ios::in | ios::binary );
    TEST_ASSERT (data);

    data.seekg(0, ios::end);
    ifstream::pos_type length = data.tellg();
    data.seekg(0, ios::beg);

    vector<unsigned char> buffer(188);

    mpeg::analyzer a;
    ifstream::pos_type offset = 0;
    a.collect_pmt = true;
    a.filter_mode = mpeg::analyzer::FILTER_ALLOW;

    a.add_pid_to_filter(0);
    a.add_pid_to_filter(0x10);
    a.add_pid_to_filter(0x12);
    
    while (length - offset >= 188) {
        data.read ((char *)&buffer[0], 188);
        a << (unsigned char *) &buffer[0];
        offset += 188;
    }
    
    data.close();
  }

};


int main(int argc, char *argv[]) {
  MpegTestSuite test;
  Test::TextOutput output(Test::TextOutput::Verbose);
  return test.run(output, false);
  try {
     test.pat_section_test();
  } catch (Poco::Exception & e) {
      cout << e.message() << endl;
     cout << e.displayText() << endl;   
  }
  return 1;
}
