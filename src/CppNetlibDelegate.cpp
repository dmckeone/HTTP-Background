//
//  CppNetlibDelegate.cpp
//  HTTPlib
//
//  Created by Mac Build on 9/23/13.
//  Copyright 2013 __MyCompanyName__. All rights reserved.
//

#include "CppNetlibDelegate.h"

#include <vector>
#include <string>

#include <boost/algorithm/string.hpp>

using namespace OmnisTools;

void CppNetlibDelegate::init(OmnisTools::ParamMap&)
{
    // DEV NOTE: Lists can be populated in a background object, but must be allocated on the main thread.
    _listResult = boost::shared_ptr<EXTqlist>(new EXTqlist(listVlen));
	_headerResult = boost::shared_ptr<EXTqlist>(new EXTqlist(listVlen));
}

void CppNetlibDelegate::cancel()
{
}

void CppNetlibDelegate::buildHeaderList(boost::network::http::client::response response_)
{
	using namespace boost::network;
	typedef headers_range<http::client::response>::type response_headers;
	typedef boost::range_iterator<response_headers>::type iterator;
    
	std::string name;
	std::string value;
    str255 colName;
	EXTfldval colVal;
    int col = 0;
    
	response_headers headers_ = http::headers(response_);
	for (iterator it = headers_.begin(); it != headers_.end(); ++it) 
	{
		name = it->first;
		colName = initStr255(name.c_str());
        _headerResult->addCol(fftCharacter, dpFcharacter, 10000000, &colName);
	}
    
	_headerResult->insertRow();
	for (iterator it = headers_.begin(); it != headers_.end(); ++it) 
	{
		value = it->second;
	    _headerResult->getColValRef(1,col+1,colVal,qtrue);
        getEXTFldValFromString(colVal,value);
		col++;
	}
}

OmnisTools::ParamMap CppNetlibDelegate::run(OmnisTools::ParamMap& params) 
{
    // TODO: Consider using streaming body to indicate download success
    //       http://cpp-netlib.org/0.10.1/reference/http_client.html
    
    using namespace boost::network;
    
    OmnisTools::ParamMap result;
    
	str255 colName;
	EXTfldval colVal;
    
	// Read all parameters
    int status;
    std::string method = "GET";
    std::string url;
    std::vector<OmnisTools::ParamMap> headers;
    std::string requestBody;
    std::string requestBodyType;
    
    for (OmnisTools::ParamMap::iterator it = params.begin(); it != params.end(); ++it) {
        try {
            if (boost::iequals(it->first, "url")) {
                url = boost::any_cast<std::string>(it->second);
            } 
            else if (boost::iequals(it->first, "method")) {
                method = boost::any_cast<std::string>(it->second);
            }
            else if (boost::iequals(it->first, "headers")) {
                headers = boost::any_cast<std::vector<OmnisTools::ParamMap> > (it->second);
            }
            else if (boost::iequals(it->first, "body_type")) {
                requestBodyType = boost::any_cast<std::string>(it->second);
            }
            else if (boost::iequals(it->first, "body")) {
                requestBody = boost::any_cast<std::string>(it->second);
            }
        } catch (const boost::bad_any_cast& e ) {
            LOG_ERROR << "Unable to cast parameter";
        }
    }
    
    // Validate parameters
    if (url.empty()) {
        LOG_ERROR << "URL is empty";
        return result;
    }
    
	try {
        http::client::options options;
        options.follow_redirects(true)
               .cache_resolved(true);
        
		http::client::request request_(url);
        
        std::string headKey, headValue;
        for (std::vector<OmnisTools::ParamMap>::iterator head = headers.begin(); head != headers.end(); ++head) {
            // Extract valid keys
            for (OmnisTools::ParamMap::iterator item = head->begin(); item != head->end(); ++item) {
                if (boost::iequals(item->first, "key")) {
                    headKey = boost::any_cast<std::string>(item->second);
                } else if (boost::iequals(item->first, "value")) {
                    headValue = boost::any_cast<std::string>(item->second);
                }
            }
            request_ << header(headKey, headValue);
        }

		http::client client_(options);
        http::client::response response_;
        if (boost::iequals(method, "GET")
            || boost::iequals(method, "PUT")
            || boost::iequals(method, "POST")
            || boost::iequals(method, "DELETE")) {
            
            // GET, POST PUT, and DELETE -- Body Available
            http::client::response response_;
            if (boost::iequals(method, "GET")) 
                response_ = client_.get(request_);
            else if (boost::iequals(method, "POST"))
                if (requestBodyType.empty())
                    response_ = client_.post(request_, requestBody);
                else
                    response_ = client_.post(request_, requestBody, requestBodyType);
            else if (boost::iequals(method, "PUT")) 
                if (requestBodyType.empty())
                    response_ = client_.put(request_, requestBody);
                else
                    response_ = client_.put(request_, requestBody, requestBodyType);
            else if (boost::iequals(method, "DELETE")) 
                response_ = client_.delete_(request_);
            
            status = http::status(response_);
            std::string body_ = http::body(response_);
            
            buildHeaderList(response_);
            colName = initStr255("status");
            _listResult->addCol(fftInteger, 0, 1, &colName);
            
            colName = initStr255("headers");
            _listResult->addCol(fftList, dpFcharacter, 1, &colName);
            
            colName = initStr255("body");
            _listResult->addCol(fftCharacter, dpFcharacter, 10000000, &colName);
            
            _listResult->insertRow();
            
            //add status
            _listResult->getColValRef(1,1,colVal,qtrue);
            colVal.setLong(status);
            
            //add headers
            _listResult->getColValRef(1,2,colVal,qtrue);
            boost::shared_ptr<EXTqlist> ptr = boost::any_cast<boost::shared_ptr<EXTqlist> > (_headerResult);
            colVal.setList(ptr.get(), qtrue);
            
            //add body
            _listResult->getColValRef(1,3,colVal,qtrue);
            getEXTFldValFromString(colVal,body_);
            
            // Return list via parameters
            result["Result"] = _listResult;
        } else if (boost::iequals(method, "HEAD")) {
            // HEAD -- No Body Required
            http::client::response response_ = client_.head(request_);
            
            status = http::status(response_);
            
            buildHeaderList(response_);
            colName = initStr255("status");
            _listResult->addCol(fftList, dpFcharacter, 1, &colName);
            
            colName = initStr255("headers");
            _listResult->addCol(fftList, dpFcharacter, 1, &colName);
            
            _listResult->insertRow();
            
            //add status
            _listResult->getColValRef(1,1,colVal,qtrue);
            colVal.setLong(status);
            
            //add headers
            _listResult->getColValRef(1,2,colVal,qtrue);
            boost::shared_ptr<EXTqlist> ptr = boost::any_cast<boost::shared_ptr<EXTqlist> > (_headerResult);
            colVal.setList(ptr.get(), qtrue);
            
            // Return list via parameters
            result["Result"] = _listResult;
        } else {
            return result;
        }
        
        result["Method"] = method;
        result["URL"] = url;
        
	} catch (std::exception &e) {
	}
    
	return result;
}