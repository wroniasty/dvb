#include <iostream>
#include <boost/foreach.hpp>
#include <boost/lambda/lambda.hpp>
#include <boost/lambda/algorithm.hpp>
#include <vector>
#include "epg.h"

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
    
    service::event_p service::current_event(unsigned now) {
        event_p e;
        BOOST_FOREACH(event_p ev, schedule) {
            if (ev->start <= now && (ev->start+ev->duration*60) >= now) {
                e = ev; break;
            }
        };
        return e;
    }

    std::list<service::event_p> service::get_schedule(unsigned t0, unsigned t1) {
        std::list<event_p> sched;
        BOOST_FOREACH(event_p ev, schedule) {
            if (ev->start >= t0 && ev->start <= t1) {
                sched.push_back( ev );
            }
        };        
        return sched;        
    }
    
    service::event_p service::new_event(unsigned event_id, unsigned start, 
            unsigned duration) {
        remove_event (event_id);
        event_p e ( new event );
        e->id = event_id; e->start = start; e->duration = duration;
        event_by_id[event_id] = e;
        schedule.push_back(e);
        sort_schedule();
        return e;
    }
    
    service::event_p service::get_event (unsigned event_id) {
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

    bool event_time_compare (const service::event_p  e1, const service::event_p e2) { return (*e1) < (*e2); }
    
    void service::sort_schedule() {
        schedule.sort(event_time_compare);
    }
}  /* namespace epg */
    
}  /* namespace dvb */
