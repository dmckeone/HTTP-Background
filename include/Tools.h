//
//  Tools.h
//  TMWorkQueue
//
//  Created by David McKeone on 11-11-22.
//  Copyright 2011 __MyCompanyName__. All rights reserved.
//

#ifndef TOOLS_H_
#define TOOLS_H_

#include <algorithm>
#include <boost/algorithm/string.hpp>

namespace Tools {
    struct CaseInsensitiveLess : std::binary_function<std::string, std::string, bool>
    {                
        bool operator() (const std::string & s1, const std::string & s2) const {
            return boost::lexicographical_compare(s1, s2, boost::is_iless());
        }
    };
}

#endif // TOOLS_H_