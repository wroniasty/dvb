#ifndef _DVB_EPG_
#define _DVB_EPG_ 1

#include <string>
#include <map>
#include <list>

#include "Poco/DateTime.h"
#include "Poco/SharedPtr.h"

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
        unsigned int start;
        /* duration in minutes */
        unsigned int duration;

        typedef Poco::SharedPtr<event_info_> event_info; 
        std::map<std::string, event_info > info;

        event();
        event_info get_info ( const std::string language );
        void delete_info ( const std::string language );
        
        bool operator== (const event & ev) const;
        bool operator== (const unsigned event_id) const;
        inline bool operator< ( const event &e) const {return start < e.start;}        
    };
    
    class service {
    public:
        typedef Poco::SharedPtr<event> event_p;
        typedef std::list<event_p> schedule_t;
    protected:
        
        void sort_schedule();
        void remove_past_events();        
        std::map<unsigned, event_p> event_by_id;
        schedule_t schedule;
    public:
        unsigned sid;
        std::string name;
                        
        service(unsigned _sid=0, std::string _name = "");

        event_p current_event ( unsigned now );
        std::list<event_p> get_schedule (unsigned t0, unsigned t1);
        event_p new_event (unsigned event_id, unsigned start=0, unsigned duration=0);
        event_p get_event (unsigned event_id);
        void remove_event (unsigned event_id);
    };

}

}


#endif
