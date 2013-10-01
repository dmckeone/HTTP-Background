//
//  CppNetlibDelegate.h
//  HTTPlib
//
//  Created by Mac Build on 9/23/13.
//  Copyright 2013 __MyCompanyName__. All rights reserved.
//

#ifndef CPPNETLIBWORKERDELEGATE_H_
#define CPPNETLIBWORKERDELEGATE_H_

#include "Worker.h"
#include "OmnisTools.he"

#undef nil  // WORKAROUND: nil is defined in a header and it conflicts with some Boost libraries
#define BOOST_NETWORK_ENABLE_HTTPS 
#include <boost/network/protocol/http/client.hpp>

class CppNetlibDelegate : public WorkerDelegate {
public:
    virtual void init(OmnisTools::ParamMap&);
    virtual OmnisTools::ParamMap run(OmnisTools::ParamMap&);
    virtual void cancel();
    
private:
    boost::shared_ptr<EXTqlist> _listResult;
    boost::shared_ptr<EXTqlist> _headerResult;
	void buildHeaderList(boost::network::http::client::response);
};

#endif
