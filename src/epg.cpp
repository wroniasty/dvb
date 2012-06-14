#include <iostream>
#include <boost/foreach.hpp>
#include <boost/lambda/lambda.hpp>
#include <boost/lambda/algorithm.hpp>
#include <vector>
#include "epg.h"

using namespace std;
using namespace boost::lambda;
//using namespace boost::mpl::placeholders;

namespace dvb {
    
namespace epg {
    
    event::event () : id(0), start(0), duration(0) {}
    
    event::event_info event::get_info(const std::string language) {
        if (info.count(language) == 0) {
            info[language] = event_info ( new event_info_ );
        }
        return info[language];
    }
    
    void event::delete_info(const std::string language) {
        info.erase(language);
    }
    
    bool event::operator ==(const event& ev) const {
        return id == ev.id;
    }
    
    bool event::operator ==(const unsigned event_id) const {
        return id == event_id;
    }
    
    service::service(unsigned _sid, std::string _name) : sid(_sid), name(_name) {}
    
    event_p service::current_event(Poco::DateTime now) {
        event_p e;
        BOOST_FOREACH(event_p ev, schedule) {
            if (ev->start <= now && (ev->start+ev->duration) >= now) {
                e = ev; break;
            }
        };
        return e;
    }

    std::list<event_p> service::get_schedule(Poco::DateTime t0, Poco::DateTime t1) {
        std::list<event_p> sched;
        BOOST_FOREACH(event_p ev, schedule) {
            if (ev->start >= t0 && ev->start <= t1) {
                sched.push_back( ev );
            }
        };        
        return sched;        
    }
    
    event_p service::new_event(unsigned event_id, Poco::DateTime start, 
            Poco::Timespan duration) {
        remove_event (event_id);
        event_p e ( new event );
        e->id = event_id; e->start = start; e->duration = duration;
        event_by_id[event_id] = e;
        schedule.push_back(e);
        sort_schedule();
        return e;
    }
    
    event_p service::get_event (unsigned event_id) {
        event_p e;
        if (event_by_id.count(event_id) > 0) e = event_by_id[event_id];
        return e;
    }

    void service::remove_event(unsigned event_id) {
        event_p ep = event_by_id[event_id];
        event_by_id.erase (event_id);
        schedule.remove(ep); 
        sort_schedule();
    }

    int service::reload_epg(soci::session& sql, int service_description_id, 
            Poco::DateTime now) {
        
        time_t tnow = now.utcTime();
        std::tm tm_now;  gmtime_r(&tnow, &tm_now);

        soci::rowset<soci::row> events = (sql.prepare << 
              "SELECT * " 
           << "FROM eit_collected_event WHERE service_description_id = :id "
           << "AND end_time >= :now ORDER BY start_time "
              , soci::use(tm_now, "now"), soci::use(service_description_id, "id")
        );
      
        schedule.clear();
        event_by_id.clear();
      
        for (soci::rowset<soci::row>::const_iterator it=events.begin(); it!=events.end(); ++it) {
          int event_id = (*it).get<int> ("event_id");
          std::tm start_time = (*it).get<std::tm> ("start_time"), 
                  end_time = (*it).get<std::tm> ("end_time");
          event_p e = new_event ( event_id, Poco::DateTime(), Poco::Timespan(0,0,60,0,0) );
          
          e->start.assign( start_time.tm_year+1900, start_time.tm_mon+1, start_time.tm_mday,
                  start_time.tm_hour, start_time.tm_min);
          Poco::DateTime end_dt( end_time.tm_year+1900, end_time.tm_mon+1, end_time.tm_mday,
                  end_time.tm_hour, end_time.tm_min);
          e->duration = end_dt - e->start;
          schedule.push_back(e);
          event_by_id[event_id] = e;
        }

        soci::rowset<soci::row> descriptions = (sql.prepare << 
              "SELECT e.event_id, d.* " 
           << "FROM eit_collected_event e INNER JOIN eit_collected_event_description d "
           << "ON e.id = d.collected_event_id "
           << "WHERE e.service_description_id = :id "
           << "AND e.end_time >= :now ORDER BY e.start_time "
              , soci::use(tm_now, "now"), soci::use(service_description_id, "id")
        );

        for (soci::rowset<soci::row>::const_iterator it=descriptions.begin(); 
                it!=descriptions.end(); ++it) {
          int event_id = (*it).get<int> ("event_id");
          event_p e = get_event (event_id);
          if (!e) continue;
          try {
            string name = (*it).get<string> ("name"), 
                   text = (*it).get<string> ("text"),
                   lang = (*it).get<string> ("language_code"); 
            if (lang.size() != 3) continue;
            event::event_info info = e->get_info(lang);
            info->title = name;
            info->text = text;
          } catch ( const soci::soci_error & e) {
              continue;
          }
        }

    }

//    dvb::si::
    
    bool event_time_compare (const event_p  e1, const event_p e2) { return (*e1) < (*e2); }
    
    void service::sort_schedule() {
        schedule.sort(event_time_compare);
    }
}  /* namespace epg */
    
}  /* namespace dvb */
