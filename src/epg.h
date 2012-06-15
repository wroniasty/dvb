#ifndef _DVB_EPG_
#define _DVB_EPG_ 1

#include <string>
#include <map>
#include <list>

#include "Poco/DateTime.h"
#include "Poco/SharedPtr.h"

#include <soci/soci-config.h>
#include <soci/soci.h>
#include <soci/soci-backend.h>
#include <soci/postgresql/soci-postgresql.h>

#include "mpegts.h"

namespace dvb {

    
namespace epg {

    typedef struct {
        std::string title;
        std::string text;
        std::string extended_text;
        std::map<std::string, std::string> items;
    } event_info_;
        
    class event {
    public:        
        unsigned id;
        
        /* start time as a UTC timestamp*/
        Poco::DateTime start;
        /* duration in minutes */
        Poco::Timespan duration;

        typedef Poco::SharedPtr<event_info_> event_info; 
        std::map<std::string, event_info > info;

        event();
        event_info get_info ( const std::string language );
        void delete_info ( const std::string language );
        
        bool operator== (const event & ev) const;
        bool operator== (const unsigned event_id) const;
        inline bool operator< ( const event &e) const {return start < e.start;}        
    };
    
    typedef Poco::SharedPtr<event> event_p;
    typedef std::list<event_p> schedule_t;
    typedef std::vector<event_p> schedule_v;
    
    class service {
    public:
    protected:
        
        void sort_schedule();
        void remove_past_events();        
        std::map<unsigned, event_p> event_by_id;
        schedule_t schedule;
    public:
        unsigned id;
        unsigned sid;
        std::string name;
                     
        service(unsigned _sid=0, std::string _name = "");

        schedule_v present_following_event ( Poco::DateTime now );
        schedule_t get_schedule (Poco::DateTime t0, Poco::DateTime t1);

        event_p new_event (unsigned event_id, Poco::DateTime start=0, Poco::Timespan duration=0);
        event_p get_event (unsigned event_id);
        void remove_event (unsigned event_id);
        
        int reload_epg(soci::session & sql, int service_description_id, Poco::DateTime now);

    };

    typedef Poco::SharedPtr<service> service_p;
    typedef std::list<service_p> service_l;
    typedef std::vector<service_p> service_v;
    
    class target {
    public:
        unsigned id;
        unsigned tsid, origid;
        service_l services;
        
        void init(soci::session & sql, int target_id);
    };
}

}


#endif
