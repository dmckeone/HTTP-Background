//
//  DBPool.h
//  TMWorkQueue
//
//  Created by David McKeone on 11-11-17.
//  Copyright 2011 __MyCompanyName__. All rights reserved.
//

#ifndef DB_POOL_H
#define DB_POOL_H

#include "Tools.h"
#include "Worker.h"

#include <boost/any.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/thread.hpp>
#include <boost/thread/mutex.hpp>

#include <string>
#include <map>
#include <vector>
#include "libpq-fe.h"

class Queue;

// Connection parameters.  (Host, User, Pass, etc..)
typedef std::map<std::string, boost::any, Tools::CaseInsensitiveLess> ConnInfoMap;
typedef std::map<std::string, boost::any, Tools::CaseInsensitiveLess>::iterator ConnInfoMapIterator;

// Base class for a database connection
class DBConnection {
public:
    DBConnection(int id, const ConnInfoMap& ci) : _poolID(id), _connInfo(ci) { }
    
    // Connect  (Note: Disconnect should be handled automatically by scope)
    virtual bool connect() { return false; }
    
    // Execute a query
    virtual bool execute(std::string command) { return false; }
    // Cancel the query
    virtual void cancel() { }
    
    // Check the connection and update any information
    void check();
    virtual void checkStatus() {}
    
    // NOTE: Results are purposely not handled here since the optimal way to parse a result 
    //       should occur in an associated worker.
    
    // Connection status
    virtual bool isReady() { return false; }
    virtual bool isWaiting() { return false; }
    virtual bool isFinished() { return false; }
private:
    ConnInfoMap _connInfo;
protected: 
    ConnInfoMap& connectionInfo() { return _connInfo; }
    int poolID() { return _poolID; }
private:
    int _poolID;
    
    boost::mutex checkMutex;
};

// Base class for a database pool
class DBPool {
public:    
    DBPool(const ConnInfoMap& ci, boost::weak_ptr<Queue> qp) : _connInfo(ci), _queuePtr(qp) { init(); }
private:
    ConnInfoMap _connInfo;
protected:  
    // Create connections in pool
    virtual void init() {}
    ConnInfoMap& connectionInfo() { return _connInfo; }
private:        
    std::vector<DBConnection> _pool;
    
    boost::weak_ptr<Queue> _queuePtr;
};

// PostgreSQL connection
class PostgreSQLConnection : public DBConnection {
public:
    PostgreSQLConnection(int id, const ConnInfoMap& ci);
    virtual ~PostgreSQLConnection();
    
    // Connect (Note: Disconnect is handled by shared_ptr destructor)
    virtual bool connect();
    
    // Execute a query
    virtual bool execute(std::string command);
    
    // Fetch a notification
    virtual std::vector<Worker::ParamMap> notifies();
    
    // Cancel a query
    virtual void cancel();
    
    // Discard a result
    virtual void discardResult();
    
    // Update the status of the connection
    virtual void checkStatus();
    
    // Result
    PGresult* result();
    
    // Connection
    PGconn* raw() { return _connection.get(); }
    
    // Connection status
    virtual bool isReady();
    virtual bool isWaiting();
    virtual bool isFinished();
private:
    boost::shared_ptr<PGconn> _connection;
    std::string _connString;
    std::string _lastError;
    
    bool _queryRunning;
    bool _queryReady;
    
    // Stored Notifications
    void checkNotifications();
    std::vector<Worker::ParamMap> _notifications;
};

// PostgreSQL connection pool
class PostgreSQLPool : DBPool {
public:
    PostgreSQLPool(const ConnInfoMap& ci, boost::weak_ptr<Queue> qp) : DBPool(ci, qp) {}
    virtual ~PostgreSQLPool() {}
protected:
    virtual void init();
private:
};

#endif