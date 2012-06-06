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

#include <string.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#include <fstream>

#include "dvb.h"

using namespace std;
using namespace dvb;
using namespace dvb::mpeg;

class MpegTestSuite : public Test::Suite {
public:

  MpegTestSuite() {
    TEST_ADD(MpegTestSuite::packet_random_test);
    TEST_ADD(MpegTestSuite::stream_random_test);
    TEST_ADD(MpegTestSuite::stream_data_test);
  }

protected:

  virtual void setup() {
    srand(time(NULL));
  }

private:

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
    cout << "Payload unit received PID = " << puN->PID << " size:" << buffer->size() << endl;
    if (puN->PID == 0) {
        dvb::si::pat_section pat; pat.read(*buffer);
        cout << "PAT " << pat.table_id << " lgt:" << pat.section_length << " [";
        for ( dvb::si::pat_section::programs_v::iterator it = pat.programs.begin(); it != pat.programs.end(); it++ ) {
//            cout << (*it)->program_number << "<=>pid:" << (*it)->program_map_pid << " | ";
        }
        cout << "]" << endl;
    } else if (puN->PID == 0x12) {
        dvb::si::eit_section eit; eit.read (*buffer);
        cout << "EIT " << eit.table_id << " lgt:" << eit.section_length << " [";
        for (dvb::si::eit_section::events_v::iterator it = eit.events.begin(); it != eit.events.end(); it++) {
            cout << "event_id:" << (*it)->event_id << " " ;
            for (dvb::si::descriptors_v::iterator itd = (*it)->descriptors.begin(); itd != (*it)->descriptors.end(); itd++) {
                cout << " D:" << (int) (*itd)->tag << " {" << typeid( *(*itd)->data ).name() << "} ";
            }
        }
        cout << eit.events.size() << " events ";
        cout << "]" << endl;
    }
//    dvb::si::section section(*buffer);
  }

  void stream_random_test() {
    unsigned char sampleData_1[188], sampleData_2[188];
    mpeg::stream s;
    s.addObserver ( Poco::NObserver<MpegTestSuite, mpeg::stream::payload_unit_ready> ( *this, &MpegTestSuite::handle_payload_unit ) );
    for (int j=0;j<10000;j++) {
      make_random_buffer (sampleData_1, sizeof(sampleData_1));
      sampleData_1[0] = 0x47;
//      s << sampleData_1;
    }
    s.removeObserver ( Poco::NObserver<MpegTestSuite, mpeg::stream::payload_unit_ready> ( *this, &MpegTestSuite::handle_payload_unit ) );
  }

  void stream_data_test() {
    unsigned char sampleData_1[188], sampleData_2[188];
    ifstream data("data/sample1.ts", ios::in | ios::binary );
    if (!data) return;

    data.seekg(0, ios::end);
    ifstream::pos_type length = data.tellg();

    vector<unsigned char> buffer(length);
    data.seekg(0, ios::beg);

    data.read ((char *)&buffer[0], length);
    data.close();

    mpeg::stream s;
    s.addObserver ( Poco::NObserver<MpegTestSuite, mpeg::stream::payload_unit_ready> ( *this, &MpegTestSuite::handle_payload_unit ) );

    ifstream::pos_type offset = 0;

    unsigned allowed_pids[] = {0,0x10, 0x11, 0x12, 0x13, 0x14};
    std::set<unsigned> allowed(allowed_pids, allowed_pids + 5);

    while (length - offset >= 188) {
        if ( allowed.count(mpeg::get_packet_pid((unsigned char *) &buffer[offset])) == 1)
            s << (unsigned char *) &buffer[offset];
        offset += 188;
    }
    s.removeObserver ( Poco::NObserver<MpegTestSuite, mpeg::stream::payload_unit_ready> ( *this, &MpegTestSuite::handle_payload_unit ) );
  }

};

int main(int argc, char *argv[]) {
  MpegTestSuite test;
  Test::TextOutput output(Test::TextOutput::Verbose);
  return test.run(output);
}
