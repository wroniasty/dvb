#include <iostream>
#include <boost/foreach.hpp>
#include <boost/lambda/lambda.hpp>
#include <boost/lambda/algorithm.hpp>
#include <vector>
#include "epg.h"
#include "storage.h"

#include <soci/soci-config.h>
#include <soci/soci.h>
#include <soci/soci-backend.h>
#include <soci/postgresql/soci-postgresql.h>



namespace dvb {
    
namespace storage {    

   Poco::SharedPtr<soci::session> get_session (string uri) {
       Poco::URI u(uri);
       Poco::SharedPtr<soci::session> rval;
       string connect_string;
       string userInfo = u.getUserInfo();
       
       string::size_type p = userInfo.find ( ":", 0, 1 );

       if (p == string::npos)
         (connect_string += " user=") += u.getUserInfo();
       else {
         (connect_string += " user=") += userInfo.substr(0, p);           
         (connect_string += " password=") += userInfo.substr(p+1);           
       }
       
       if (u.getHost() != "local")
               (connect_string += " host=") += u.getHost();
       (connect_string += " dbname=") += u.getPath().substr(1);
       
       if (u.getScheme() == "postgresql") {
           rval.assign( new soci::session ( soci::postgresql, connect_string ));
       }
       
       return rval;
   }    
    

}  /* namespace epg */
    
}  /* namespace dvb */
