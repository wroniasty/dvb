#ifndef _DVB_STORAGE_
#define _DVB_STORAGE_ 1

#include <string>

#include "Poco/DateTime.h"
#include "Poco/SharedPtr.h"
#include "Poco/URI.h"

#include <soci/soci-config.h>
#include <soci/soci.h>
#include <soci/soci-backend.h>
#include <soci/postgresql/soci-postgresql.h>


namespace dvb {

    
namespace storage {

   Poco::SharedPtr<soci::session> get_session (string uri);
    
}

}


#endif
