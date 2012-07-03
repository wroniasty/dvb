#include <Poco/DateTimeFormatter.h>
#include <boost/foreach.hpp>
#include <streambuf>
#include "sections.h"

namespace dvb {

namespace si {
   eit_section::event::event()
    : event_id(0), 
           running_status(0), free_CA_mode(0),
           descriptors_loop_length(0)           
   {}
   
   eit_section::event::event(unsigned _event_id, Poco::DateTime _start_time, 
         Poco::Timespan _duration, std::string language,
         std::string title, std::string short_text, std::string long_text)
    : event_id (_event_id), start_time(_start_time), duration(_duration),
      running_status(0), free_CA_mode(0), descriptors_loop_length(0) 
   {

      dvb::si::descriptor_p d_short ( new dvb::si::descriptor );
      Poco::SharedPtr<dvb::si::short_event_descriptor> d_short_data =
              d_short->set_data<dvb::si::short_event_descriptor> ();
      
      d_short_data->ISO_639_language_code = language;
      d_short_data->event_name =  title;
      d_short_data->text = short_text;
      descriptors.push_back(d_short);

      std::size_t offset = 0;
      descriptors_v ext_desc_v;
      unsigned descriptor_number=0;

      while (offset < long_text.size()) {
          dvb::si::descriptor_p d_ext ( new dvb::si::descriptor );
          Poco::SharedPtr<dvb::si::extended_event_descriptor> d_ext_data =
              d_ext->set_data<dvb::si::extended_event_descriptor> ();
          d_ext_data->descriptor_number = descriptor_number++;
          d_ext_data->last_descriptor_number = 0;
          d_ext_data->ISO_639_language_code = language;
          d_ext_data->text = long_text.substr(offset, std::min(long_text.size(), offset+200) ) ;
          ext_desc_v.push_back(d_ext);          
          offset += 200;
      }
      BOOST_FOREACH (descriptor_p d, ext_desc_v) {
          Poco::SharedPtr<dvb::si::extended_event_descriptor> d_ext_data =
              d->get_data<dvb::si::extended_event_descriptor> ();
          d_ext_data->last_descriptor_number = descriptor_number-1;
          descriptors.push_back(d);
      }
       
   }
   
   

  void eit_section::event::read(bits::bitstream& source) {
     event_id = source.read<unsigned> (16);
     start_time = dvb::read_mjd_datetime(source);
     duration = dvb::read_bcd_time(source);

     running_status = source.read<unsigned> (3);
     free_CA_mode = source.read<unsigned> (1);
     descriptors_loop_length = source.read<unsigned> (12);

     dvb::util::position counter(source); 
     std::size_t descriptors_start = source.position();
     descriptors.clear();
     while (counter() < descriptors_loop_length) {
       dvb::si::descriptor * d = new dvb::si::descriptor(); d->read(source);
       descriptors.push_back ( Poco::SharedPtr <dvb::si::descriptor> (d) );
     }
     
  }

  void eit_section::event::write(bits::bitstream& dest) {      
      dest.write(16, event_id);
      dvb::write_mjd_datetime(dest, start_time);
      dvb::write_bcd_time(dest, duration);
      dest.write(3, running_status);
      dest.write(1, free_CA_mode);
      dest.skip(12);
      std::size_t descriptors_start = dest.position();
      dvb::util::position counter(dest); 
      BOOST_FOREACH ( Poco::SharedPtr<dvb::si::descriptor> d, descriptors) {
          d->write( dest );
      };
      descriptors_loop_length = counter();
      dest.write_at(descriptors_start - 12, 12, descriptors_loop_length);
  }

  unsigned eit_section::event::calculate_length() {
      unsigned l = 12;
      BOOST_FOREACH ( Poco::SharedPtr<dvb::si::descriptor> d, descriptors) {
          l += d->get_length();
      };
      return l;      
  }

  eit_section::eit_section() : section(),        
        service_id(1), version_number(1), current_next_indicator(1),
        section_number(0), last_section_number(0),
        transport_stream_id(1), original_network_id(1),
        segment_last_section_number(0), last_table_id(0x4e)
     {
      table_id = 0x4e;
      max_length = 4096; 
  }

  
  void eit_section::read_contents (bits::bitstream & source) {
    dvb::util::position counter(source);  
    service_id = source.read<unsigned> (16);
    source.skip(2);
    version_number = source.read<unsigned> (5);
    current_next_indicator = source.read<unsigned> (1);
    section_number = source.read<unsigned> (8);
    last_section_number = source.read<unsigned> (8);
    transport_stream_id = source.read<unsigned> (16);
    original_network_id = source.read<unsigned> (16);
    segment_last_section_number = source.read<unsigned> (8);
    last_table_id = source.read<unsigned> (8);

    events.clear();
    while ( counter() < (section_length - 4) ) {
        event_p e ( new event );
        e->read(source);
        events.push_back ( e );
    }
    _valid = ( counter() == section_length - 4 );
    crc32 = source.read<unsigned> (32); /* CRC32 */

  }

  void eit_section::write_contents (bits::bitstream & dest) {
      dvb::util::position counter(dest);
      dest.write(16, service_id);
      dest.write (2, 0xffU);
      dest.write (5, version_number);
      dest.write (1, current_next_indicator);
      dest.write (8, section_number);
      dest.write (8, last_section_number);
      dest.write (16, transport_stream_id);
      dest.write (16, original_network_id);
      dest.write (8, segment_last_section_number);
      dest.write (8, last_table_id);
      
      BOOST_FOREACH( Poco::SharedPtr<event> e, events) {
         e->write( dest );  
      };
      assert ( counter() == calculate_section_length() - 4 );
  }

  int eit_section::add_event(event_p e) {      
      if (calculate_section_length() + e->calculate_length() > max_length - 2) {
          return 0;
      } else {
          events.push_back(e);
      }
      return 1;
  }
  
  int eit_section::add_event(unsigned event_id, Poco::DateTime start_time, 
          Poco::Timespan duration, std::string language, 
          std::string title, std::string short_text, std::string long_text, 
          std::string charset) {
      event_p e = make_event ( event_id, start_time, duration, language,
              title, short_text, long_text, charset);
      return add_event (e);
  } 
  
  eit_section::event_p eit_section::make_event ( 
        unsigned event_id, Poco::DateTime start_time, 
        Poco::Timespan duration, std::string language,
        std::string title, std::string short_text, std::string long_text        
        )
  {
      return make_event (event_id, start_time, duration, language, title, short_text, long_text, "iso-8859-2");
  }

  eit_section::event_p eit_section::make_event ( 
        unsigned event_id, Poco::DateTime start_time, 
        Poco::Timespan duration, std::string language,
        std::string title, std::string short_text, std::string long_text,
        std::string charset
        )
  {      

      event_p e ( new event (event_id, start_time, duration, language, 
              dvb::string_encoding::encode(charset, title), 
              dvb::string_encoding::encode(charset, short_text), 
              dvb::string_encoding::encode(charset, long_text) )
      );
      
      return e;
  }

  unsigned eit_section::calculate_section_length() {
      unsigned l = 15;
      BOOST_FOREACH ( Poco::SharedPtr<event> e, events) {
          l += e->calculate_length();
      };
      return l;      
  }
  
  eit_section_v eit_prepare_present_following ( 
          unsigned service_id, 
          unsigned version_number, 
          unsigned current_next,
          unsigned transport_stream_id,
          unsigned original_network_id,
          eit_section::event_p present_event,
          eit_section::event_p following_event          
          ) 
   {
      eit_section_v pf;
      
      eit_section_p present ( new eit_section );      
      present->table_id = 0x4e;
      present->last_table_id = 0x4e;
      present->service_id = service_id;
      present->version_number = version_number;
      present->current_next_indicator = current_next;
      present->section_syntax_indicator = 1;
      present->section_number = 0;
      present->last_section_number = 1;
      present->original_network_id = original_network_id;
      present->segment_last_section_number = 0;
      present->transport_stream_id = transport_stream_id;
      if (present_event)
         present->add_event(present_event);

      eit_section_p following ( new eit_section );      
      following->table_id = 0x4e;
      following->last_table_id = 0x4e;
      following->service_id = service_id;
      following->version_number = version_number;
      following->current_next_indicator = current_next;
      following->section_syntax_indicator = 1;
      following->section_number = 1;
      following->last_section_number = 1;      
      following->original_network_id = original_network_id;
      following->transport_stream_id = transport_stream_id;
      following->segment_last_section_number = 0;
      
      if (following_event)
        following->add_event ( following_event );

      pf.push_back (present);
      pf.push_back (following);
      
      return pf;      
  }

  bool compare_event_p ( eit_section::event_p e1, eit_section::event_p e2 ) {
      return e1->start_time < e2->start_time;
  }
  
  eit_section_v eit_prepare_schedule ( 
          dvb::epg::service_p service,
          Poco::DateTime now,
          int days_ahead, 
          unsigned version_number, 
          unsigned current_next,
          unsigned transport_stream_id,
          unsigned original_network_id
          ) 
   {
      eit_section_v sched;      
      int last_table_id = std::min(0x50 + days_ahead/2, 0x5f);
      
      for (int subtable_id=0x50; subtable_id < last_table_id; subtable_id++) {
          eit_section_v subtable_sched = 
                eit_prepare_schedule_subtable (service, now, subtable_id, 
                     version_number, current_next, transport_stream_id, 
                     original_network_id);
          sched.insert ( sched.end(), subtable_sched.begin(), subtable_sched.end());
      }
      
      BOOST_FOREACH (dvb::si::eit_section_p s, sched) {
          s->last_table_id = last_table_id;
      };
      return sched;
   }

  /*
   * An EIT schedule subtable (table_id 0x50 - 0x5f current, 0x60 - 0x6f - other) 
   * is divided into 16 segments (numbered from 0 to 15) each segment carrying
   * schedule information for a period of 3 hours. A segment is made up of up to
   * 8 EIT sections. E.g.:
   * 
   * table_id 0x50 (period today, today+1)
   * segment #0:  (today 00:00 - today 03:00)
   *    sections  (section_number = 0..7) with events
   * segment #1   (today 03:00 - today 06:00)
   *    sections  (section_number = 8..15) with events
   * .....
   * segment #15  (tomorrow 21:00 - tomorrow 24:00)
   *    sections  (section_number = 248..255) with events
   * 
   * Following tables (0x51, 0x52, etc) carry schedule information for following
   * days (0x51 is today+2 & today+3, etc...)
   */
   eit_section_v eit_prepare_schedule_subtable (
          dvb::epg::service_p service,
          Poco::DateTime now,
          unsigned subtable_id,
          unsigned version_number, 
          unsigned current_next,
          unsigned transport_stream_id,
          unsigned original_network_id
   ) {
      eit_section_v sched;      
      unsigned last_section_number=0;
      
      Poco::DateTime t0(now.year(), now.month(), now.day()), t1(t0);
      Poco::Timespan segment_time_offset(0,3,0,0,0);
      
      //Calculate and modify the starting date:
      int offset = (subtable_id >= 0x60 ) ? (subtable_id - 0x60) : (subtable_id - 0x50);      
      t0 += Poco::Timespan(offset*2,0,0,0,0);
      
      for (int segment_number=0; segment_number<32; segment_number++) {
          t1 = t0 + segment_time_offset;
          
          //fetch events
          dvb::epg::schedule_t segment_events = service->get_schedule(t0, t1);

          eit_section_v segment_sections;
          eit_section_p current_section;

          current_section.assign ( new eit_section );
                  
          current_section->service_id = service->sid;
          current_section->version_number = version_number;
          current_section->table_id = subtable_id;
          current_section->transport_stream_id = transport_stream_id;
          current_section->original_network_id = original_network_id;
          current_section->current_next_indicator = current_next;
                  
          segment_sections.push_back( current_section );
          
          /*
          cout << service->name << " table_id:" << std::hex << subtable_id << std::dec
                  << " segment: " << segment_number 
                  << " " << Poco::DateTimeFormatter::format(t0, "%d-%m-%Y  %H:%M")
                  << "-" << Poco::DateTimeFormatter::format(t1, "%H:%M")
                  << endl;
          */
          
          BOOST_FOREACH (dvb::epg::event_p e, segment_events) {
              //skip events without information data
              if (!e->has_info()) continue;
              //TODO: language?
              dvb::epg::event::event_info info = e->get_info();
              dvb::si::eit_section::event_p eit_e = eit_section::make_event(e->id, e->start, e->duration,           
                      info->language, info->title, info->text, info->extended_text );
              //(note the difference between si::event_p and epg::event_p)

              /*
              cout << " - " << info->language << " " << Poco::DateTimeFormatter::format(e->start, "%H:%M")
                   << " " << (e->has_info() ? e->get_info()->title : "No info")
                   << endl;
               */
                             
              if ((current_section->calculate_section_length() + eit_e->calculate_length() > 4092)) {
                  if (segment_sections.size() >= 8) break;
                  current_section.assign ( new eit_section );
                  
                  current_section->service_id = service->sid;
                  current_section->version_number = version_number;
                  current_section->table_id = subtable_id;
                  current_section->transport_stream_id = transport_stream_id;
                  current_section->original_network_id = original_network_id;
                  current_section->current_next_indicator = current_next;
                  
                  segment_sections.push_back( current_section );
              }
              //no need to check section length overflow - it's checked above
              current_section->add_event(eit_e);
          };
          
          
          unsigned section_number=segment_number*8;
          unsigned segment_last_section_number = section_number + segment_sections.size() - 1;

          // segment_last_section_number is set to the highest section_number 
          // in the segment.
          // if no events fall into a segment it shall be transmitted empty 
          // (section_last_segment_number == 8*segment_number)
          BOOST_FOREACH (dvb::si::eit_section_p s, segment_sections) {
              s->section_number = section_number++;
              s->segment_last_section_number = segment_last_section_number;
              last_section_number = std::max(last_section_number, s->section_number);
          }
          
          sched.insert (sched.end(), segment_sections.begin(), segment_sections.end());
          
          t0 = t1;
      }
      
      BOOST_FOREACH (dvb::si::eit_section_p s, sched) {
          s->last_section_number = last_section_number;
      };
      return sched;      
   }


}

}
