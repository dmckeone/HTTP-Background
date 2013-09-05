//
//  DBPool.cpp
//  PostgreSQLBackground
//
//  Created by David McKeone on 11-11-17.
//  Copyright 2011 __MyCompanyName__. All rights reserved.
//

#include "Logging.he"
#include "DBPool.h"

#include <exception>

#include <boost/make_shared.hpp>
#include <boost/algorithm/string.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/format.hpp>
#include <boost/foreach.hpp>

using boost::format;

// Check is designed to be called frequently and is therefore not queued.  
// If the mutex cannot be locked then the check returns quickly.
void DBConnection::check() {
    
    if (checkMutex.try_lock()) {
        // Update the status of the object
        try {
            checkStatus();
        } catch (...) {
            // Don't throw exceptions
        }
        
        checkMutex.unlock();
    }
}

std::string translateStatus(boost::shared_ptr<PGconn> connection) 
{
    switch(PQstatus(connection.get()))
    {
        case CONNECTION_STARTED:
            return "Connecting";
        case CONNECTION_MADE:
            return "Connection made";
        case CONNECTION_OK:
            return "Connection ok";
        case CONNECTION_BAD:
            return "Connection bad";
        case CONNECTION_AWAITING_RESPONSE:
            return "Connection awaiting response";
        case CONNECTION_AUTH_OK:
            return "Connection auth ok";
        case CONNECTION_SETENV:
            return "Connection set environment";
        case CONNECTION_SSL_STARTUP:
            return "Connection ssl startup";
        case CONNECTION_NEEDED:
            return "Connection needed";
    }
}

// Initialize all passed in connection parameters
PostgreSQLConnection::PostgreSQLConnection(int id, const ConnInfoMap& ci) : DBConnection(id, ci) { 
    
    using boost::any_cast;
    using boost::lexical_cast;
    
    _queryRunning = false;
    _queryReady = false;
    
    ConnInfoMap& info = connectionInfo();
    _connString = "";
    
    for( ConnInfoMap::iterator it = info.begin(); it != info.end(); ++it) {
        try {
            if(boost::iequals(it->first,"host")) {
                _connString = str(format("%shost='%s'\n") % _connString % any_cast<std::string>(it->second));
            } else if (boost::iequals(it->first,"hostaddr")) {
                _connString = str(format("%shostaddr='%s'\n") % _connString % any_cast<std::string>(it->second));    
            } else if (boost::iequals(it->first,"port")) {
                _connString = str(format("%sport='%s'\n") % _connString % lexical_cast<std::string>(any_cast<int>(it->second)));  
            } else if (boost::iequals(it->first,"dbname")) {
                _connString = str(format("%sdbname='%s'\n") % _connString % any_cast<std::string>(it->second)); 
            } else if (boost::iequals(it->first,"user")) {
                _connString = str(format("%suser='%s'\n") % _connString % any_cast<std::string>(it->second));
            } else if (boost::iequals(it->first,"password")) {
                _connString = str(format("%spassword='%s'\n") % _connString % any_cast<std::string>(it->second));
            }
        } catch(const boost::bad_any_cast& e) {
            LOG_ERROR << str(format("Failed to cast parameter \"%s\". Error: %s") % it->first % e.what());
        }
    }
}

PostgreSQLConnection::~PostgreSQLConnection() {
    
}

bool PostgreSQLConnection::connect() {
    LOG_INFO << "Starting database pool connection";
    
    _queryRunning = false;
    _queryReady = false;
    
    // Create connection
    _connection = boost::shared_ptr<PGconn>(PQconnectdb(_connString.c_str()), PQfinish);
    if(_connection == boost::shared_ptr<PGconn>()) {
        LOG_ERROR << "Failed create PostgreSQL connection object.";
        return false;
    }
    
    // Verify connection
    bool status = (PQstatus(_connection.get()) == CONNECTION_OK);
    if(!status) {
        LOG_ERROR << str(format("Failed to connect pool connection to database.  Status: %s ") % translateStatus(_connection));
        
        // Check for error message
        if (PQerrorMessage(_connection.get()) != 0) {
            _lastError = PQerrorMessage(_connection.get());
            LOG_ERROR << str(format("Failed to connect pool connection to database.  Error: %s ") % _lastError);
        }
    }
    
    return status;
}

// Execute a query
bool PostgreSQLConnection::execute(std::string command) {
    if( !isReady() ) {
        LOG_ERROR << "Cannot execute query since connection is not ready.";
        return false;
    }
    
    int result = PQsendQuery(_connection.get(), command.c_str());
    if (result != 1) {
        // Failure case
        LOG_ERROR << str(format("Query failed to execute.  Query: %s") % command);
        _lastError = PQerrorMessage(_connection.get()); 
        LOG_ERROR << str(format("Query failed to execute.  Error: %s") % _lastError);
        return false;
    }
    
    // Success case
    _queryReady = false;
    _queryRunning = true;
    
    return true;
}

// Check for notifications and add them to the local notification store.  This should be done
// after every call to PQconsumeInput() and PQgetResult()
void PostgreSQLConnection::checkNotifications() {
    pgNotify* notification = PQnotifies(_connection.get());
    while (notification) {
        LOG_INFO << "Received: " << notification->relname << " -- " << notification->extra;
        
        Worker::ParamMap item;
        item["pid"] = boost::any_cast<int>(notification->be_pid);                           /* process ID of notifying server process */
        item["channel"] = boost::any_cast<std::string>(std::string(notification->relname)); /* notification channel name */
        item["payload"] = boost::any_cast<std::string>(std::string(notification->extra));   /* notification payload string */
        
        _notifications.push_back(item);
        
        // Cleanup notification (regardless of state this always needs to happen: http://www.postgresql.org/docs/9.2/static/libpq-notify.html)
        PQfreemem(notification);
        
        // Look for next notification
        notification = PQnotifies(_connection.get());
    }

}

// Look for notifications explicitly
std::vector<Worker::ParamMap> PostgreSQLConnection::notifies() {
    
    // Check the connection for any notifications
    PQconsumeInput(_connection.get());
    
    // Check for any notifications
    checkNotifications();
    
    // Translate notification
    std::vector<Worker::ParamMap> result(_notifications);
    _notifications.clear();
    
    return result;
}

// Discard the result of a query
void PostgreSQLConnection::discardResult() {
    PGresult* discard = result();
    if (discard) {
        PQclear(discard);
    }
}

// Check connection state
void PostgreSQLConnection::checkStatus() {    
    // Check connection input if waiting on data
    if( _queryRunning && !_queryReady ) {
        if( PQconsumeInput(_connection.get()) != 1 ) {
            checkNotifications();
            
            _lastError = PQerrorMessage(_connection.get()); 
            LOG_ERROR << str(format("Failed to consume query data.  Error: %s") % _lastError); 
        }
        if( PQisBusy(_connection.get()) != 1 ) {
            // All query data is now available
            _queryReady = true;
        }
    }
}

// Cancel
void PostgreSQLConnection::cancel() {
    if(_queryRunning && !_queryReady) {
        // Scoped pointer
        boost::shared_ptr<PGcancel> cancel = boost::shared_ptr<PGcancel>(PQgetCancel(_connection.get()), PQfreeCancel);
        
        // Dispatch request
        int errSize = 256;  // Recommended by: http://www.postgresql.org/docs/9.1/interactive/libpq-cancel.html
        char error[errSize];
        if( PQcancel(cancel.get(), error, errSize) != 1 ) {
            LOG_ERROR << str(format("Failed to dispatch cancel request.  Error %s") % std::string(error));
        }
        
        // Check query again (cancel request should have updated it) 
        check();
    }
}

// Result
PGresult* PostgreSQLConnection::result() {
    PGresult* ret = PQgetResult(_connection.get());
    checkNotifications();
    return ret;
}

// Connection status
bool PostgreSQLConnection::isReady() {
    return _connection != boost::shared_ptr<PGconn>();
}

bool PostgreSQLConnection::isWaiting() {
    if (PQisBusy(_connection.get()) == 1) {
        return true;
    }
        
    return false;
}

bool PostgreSQLConnection::isFinished() {
    return _queryRunning && _queryReady;
}
