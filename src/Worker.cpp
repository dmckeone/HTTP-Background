//
//  Worker.cpp
//  TMWorkQueue
//
//  Created by David McKeone on 11-11-01.
//  Copyright 2011 __MyCompanyName__. All rights reserved.
//

#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include "Worker.h"
#include "Queue.h"
#include "Logging.he"
#include "DBPool.h"
#include "OmnisTools.he"
#include "libpq-fe.h"

#include <boost/format.hpp>

using namespace OmnisTools;
using boost::format;

static const int SLEEP_MS = 100;  // Time to sleep when waiting for connection to finish on PostgreSQL server side
static const int WAIT_MS = 500;  // Time to sleep when nothing is done and waiting for notifications

Worker::Worker() : _complete(false), _running(false), _cancelled(false) 
{ }

Worker::Worker(const ParamMap& p, const boost::shared_ptr<WorkerDelegate>& d) : _params(p), _delegate(d), _complete(false), _running(false), _cancelled(false) 
{ }

Worker::Worker(const Worker& w)
{
    _workerName = w._workerName;
    _params = w._params;
    _result = w._result;
    
    _running = w._running;
    _complete = w._complete;
    _cancelled = w._cancelled;
    
    _queue = w._queue;
    _delegate = w._delegate;
} 

Worker::~Worker() {
    cancel();
    //_thread.join();  // Don't destruct objects until thread has finished
}

// Description of object used for logging
std::string Worker::desc() {
    if (!_workerName.empty()) {
        return str(format("Worker (%s)") % _workerName);
    } else {
        return "Worker";
    }  
}

// Override point for sub-classes to init objects that must run in the main thread
void Worker::init() {
    if (_delegate) {
        _delegate->init(_params);
    }
}

bool Worker::running() {
    // Get shared lock (multiple readers / one writer)
    boost::shared_lock<boost::shared_mutex> lock(_runMutex);
    return _running; 
}

void Worker::setRunning(bool r) {
    // Get unique lock
    boost::unique_lock<boost::shared_mutex> lock(_runMutex);
    _running = r; 
}

bool Worker::complete() {
    // Get shared lock (multiple readers / one writer)
    boost::shared_lock<boost::shared_mutex> lock(_completeMutex);
    
    return _complete; 
}

void Worker::setComplete(bool c) {
    // Get unique lock
    boost::unique_lock<boost::shared_mutex> lock(_completeMutex);
    _complete = c;
}

Worker::ParamMap Worker::result() 
{ 
    // Get shared lock (multiple readers / one writer)
    boost::shared_lock<boost::shared_mutex> lock(_resultMutex);
    
    return _result; 
}

void Worker::setResult(const Worker::ParamMap& pm) 
{ 
    boost::unique_lock<boost::shared_mutex> lock(_resultMutex);
    _result = pm; 
}

boost::shared_ptr<WorkerDelegate> Worker::delegate() {
    return _delegate;
}

// Cancel status
bool Worker::cancelled() {
    // Get shared lock (multiple readers / one writer)
    boost::shared_lock<boost::shared_mutex> lock(_cancelMutex);
    
    return _cancelled; 
}

// Cancel request
void Worker::cancel() { 
    boost::unique_lock<boost::shared_mutex>(_cancelMutex); // Locked so many callers can cancel at the same time
    
    _cancelled = true; 
    
    if(_delegate) {
        _delegate->cancel();
    }
    LOG_DEBUG << desc() << " requested cancel.";
    
    if( !_thread.interruption_requested() ) {
        _thread.interrupt();
    }  
}

// Reset the current worker objects list
void Worker::reset() {
}

// Run worker
void Worker::run() {
    if(_delegate) {
        setResult( _delegate->run(_params) );
    }
}

// Start-up to run item on a thread
void Worker::start() {
    
    if(running() == true) {
        // Only start if not already running
        return;
    }
    
    // Run thread
    _thread = boost::thread(WorkerThread(shared_from_this(), _delegate));
}

// Start-up to run item on a thread
void Worker::start(boost::shared_ptr<Queue> q) {
    
    if(running() == true) {
        // Only start if not already running
        return;
    }
    
    _queue = q;
    
    // Run thread
    _thread = boost::thread(WorkerThread(shared_from_this(), _delegate));
}

// Thread entry point
void Worker::WorkerThread::operator()() {
    
    // Get shared pointer
    boost::shared_ptr<Worker> ptr;
    Worker::ParamMap params;
    {
        try { 
            ptr = _worker.lock();
        } catch (const boost::bad_weak_ptr& e) {
            return;
        }
        
        // Lock work mutex
        boost::unique_lock<boost::mutex> lock(ptr->_workMutex);
        
        if(ptr->running()) {
            // Thread is already running
            return;
        }
        
        // Mark worker as running        
        ptr->setRunning(true);
        
        LOG_INFO << ptr->desc() << " started";
        
        // Copy Params
        params = ptr->_params;
        
        // Release shared pointer prior to starting logic (otherwise it holds a reference and prevents worker destruct)
        ptr.reset(); 
    }
    
    // Perform worker work
    Worker::ParamMap result = _delegate->run(params); 
    
    // Re-acquire shared pointer
    try { 
        ptr = _worker.lock();
    } catch (const boost::bad_weak_ptr& e) {
        // Worker out of scope
        LOG_INFO << "Worker out of scope at completion";
        return;
    }
    
    if (ptr) {
        ptr->setResult(result);
        
        if(ptr->cancelled()) {
            LOG_INFO << ptr->desc() << " canceled";
        } else {
            LOG_INFO << ptr->desc() << " complete";
        }
        
        ptr->setComplete(true);
        ptr->setRunning(false);
    }
}

void PostgreSQLDelegate::init(Worker::ParamMap& params) {
    // DEV NOTE: Lists can be populated in a background object, but must be allocated on the main thread.
}

void PostgreSQLDelegate::cancel() {
    cancelled = true;
}
 
// Run a PostgreSQL query and (optionally) fetch the result
Worker::ParamMap PostgreSQLDelegate::run(Worker::ParamMap& params) {
    // TODO: Start using   thread pool instead, rather then one-off connection
    
    Worker::ParamMap result;
    
    _listResult = boost::shared_ptr<EXTqlist>(new EXTqlist(listVlen));
    
    PostgreSQLConnection connection(0, params);
    connection.connect();
    
    // Fetch query command
    Worker::ParamMapIterator it = params.find("query");
    if( it == params.end()) {
        // No Query?  Nothing to do
        LOG_ERROR << "No query passed to postgresql worker";
        return result;
    }
    std::string command;
    try {
        command = boost::any_cast<std::string>(it->second);
    } catch (const boost::bad_any_cast& e) {
        LOG_ERROR << "Unable to convert query parameter to string. Error: " << e.what();
        return result;
    }
    
    // Determine if fetching the result or not
    bool fetchResult = false;
    it = params.find("fetchResult");
    if( it != params.end()) {
        try {
            fetchResult = boost::any_cast<bool>(it->second);
        } catch (const boost::bad_any_cast& e) {
            LOG_ERROR << "Unable to convert fetch result parameter to boolean. Error: " << e.what();
            return result;
        }
    }
    
    // Execute command
    connection.execute(command);
    
    // Wait for command to execute on server, this must be done otherwise the transaction will get rolled back.
    try {
        while (!connection.isFinished()) {
            connection.check();
            boost::this_thread::sleep(boost::posix_time::millisec(SLEEP_MS));  // Thread interruption point as well as sleep
        }
    } catch (const boost::thread_interrupted&) {
        cancel();
    }
    
    // Fetch result (if wanted)
    if(fetchResult && !cancelled) {          
        // Fetch results -- DEV NOTE: _listResult is assumed to be empty here
        PGresult* result = connection.result();
        while (result != NULL && !cancelled) {     
            // Allow cancellation
            try {
                boost::this_thread::interruption_point();
            } catch (const boost::thread_interrupted&) {
                cancel();
            }
            
            // Parse results that have been received
            int status = PQresultStatus(result);
            if( (status == PGRES_TUPLES_OK || status == PGRES_COMMAND_OK) ) {
                // Fill list
                std::string value;
                int totalRows = PQntuples(result);
                int totalCols = PQnfields(result);
                
                // Add a column for each column in the result set
                str255 colName;
                for (int col = 0; col < totalCols && !cancelled; ++col) {
                    colName = initStr255(PQfname(result, col));
                    _listResult->addCol(fftCharacter, dpFcharacter, 10000000, &colName);
                    
                    try {
                        boost::this_thread::interruption_point();
                    } catch (const boost::thread_interrupted&) {
                        cancel();
                    }
                }
                
                // Fill the list
                EXTfldval colVal;
                for (int row = 0; row < totalRows && !cancelled; ++row) {
                    _listResult->insertRow();
                    // Set each column value
                    for (int col = 0; col < totalCols; ++col) {
                        value = PQgetvalue(result,row,col);
                        
                        _listResult->getColValRef(row+1,col+1,colVal,qtrue);
                        getEXTFldValFromString(colVal,value);
                    }
                    
                    // Check for cancel on each row(in case there are a lot of rows)
                    try {
                        boost::this_thread::interruption_point();
                    } catch (const boost::thread_interrupted&) {
                        cancel();
                    }
                }
            }
            
            // Cleanup result (regardless of status this always needs to happen: http://www.postgresql.org/docs/9.1/static/libpq-async.html )
            PQclear(result);
            
            if(!cancelled) {
                // Get next result
                result = connection.result();
            }
        }
    }
    
    if(cancelled) {
        connection.cancel();  // Attempt to cancel connection
    }
    
    
    // Return list via parameters
    result["Result"] = _listResult;
    
    return result;
}

// Add a listening channel to the connection
void PostgreSQLNotifyDelegate::listen(std::string channel) {
    
    boost::mutex::scoped_lock lock(_listenMutex);
    _listens.push_back(channel);
}

// Stop listening to a channel on the connection
void PostgreSQLNotifyDelegate::unlisten(std::string channel) {
    
    boost::mutex::scoped_lock lock(_unlistenMutex);
    _unlistens.push_back(channel);
}

// Push a notification through the server
void PostgreSQLNotifyDelegate::notify(std::string channel, std::string payload) {
    
    boost::mutex::scoped_lock lock(_notificationMutex);
    _notifications.push_back(std::pair<std::string, std::string>(channel, payload));
}

// Pop a vector of all the currently queued LISTENs
std::vector<std::string> PostgreSQLNotifyDelegate::popListens() {
    
    boost::mutex::scoped_lock lock(_listenMutex);
    // Copy the vector and clear the original
    std::vector<std::string> ret(_listens);
    _listens.clear();
    
    return ret;
}

// Pop a vector of all the currently queued UNLISTENs
std::vector<std::string> PostgreSQLNotifyDelegate::popUnlistens() {
    
    boost::mutex::scoped_lock lock(_unlistenMutex);
    // Copy the vector and clear the original
    std::vector<std::string> ret(_unlistens);
    _unlistens.clear();
    
    return ret;
}

// Pop a vector of all the currently queued notifications
std::vector<std::pair<std::string, std::string> > PostgreSQLNotifyDelegate::popNotifications() {
    
    boost::mutex::scoped_lock lock(_notificationMutex);
    // Copy the vector and clear the original
    std::vector<std::pair<std::string, std::string> > ret(_notifications);
    _notifications.clear();
    
    return ret;
}

void PostgreSQLNotifyDelegate::receive(Worker::ParamMap notification) {
    
    boost::mutex::scoped_lock lock(_receivedMutex);
    _received.push_back(notification);
}

EXTqlist* PostgreSQLNotifyDelegate::popReceived() {
    
    boost::mutex::scoped_lock lock(_receivedMutex);
    if (_received.size() <= 0) {
        return NULL;
    }
    
    EXTqlist* _listResult = new EXTqlist(listVlen);
    
    // Add a column for each of the data items returned by the notification
    str255 colName;
    colName = initStr255("pid");
    _listResult->addCol(fftCharacter, dpFcharacter, 10000000, &colName);
    colName = initStr255("channel");
    _listResult->addCol(fftCharacter, dpFcharacter, 10000000, &colName);
    colName = initStr255("payload");
    _listResult->addCol(fftCharacter, dpFcharacter, 10000000, &colName);
    
    // Fill the list
    EXTfldval colVal;
    qshort row = 0;
    for(std::vector<Worker::ParamMap>::iterator it = _received.begin(); it != _received.end(); ++it) {
        _listResult->insertRow();
        
        // PID
        _listResult->getColValRef(row+1, 1, colVal, qtrue);
        getEXTFldValFromInt(colVal, boost::any_cast<int>((*it)["pid"]) );
        // Channel
        _listResult->getColValRef(row+1, 2, colVal, qtrue);
        getEXTFldValFromString(colVal, boost::any_cast<std::string>((*it)["channel"]) );
        // Payload
        _listResult->getColValRef(row+1, 3, colVal, qtrue);
        getEXTFldValFromString(colVal, boost::any_cast<std::string>((*it)["payload"]) );
        
        row++;
    }
    
    _received.clear();
    
    return _listResult;
}

bool PostgreSQLNotifyDelegate::hasReceived() {
    boost::mutex::scoped_lock lock(_receivedMutex);
    return _received.size() > 0;
}

void PostgreSQLNotifyDelegate::init(Worker::ParamMap& params) {
    // DEV NOTE: Lists can be populated in a background object, but must be allocated on the main thread.
}

void PostgreSQLNotifyDelegate::cancel() {
    cancelled = true;
}

// Run a PostgreSQL query and (optionally) fetch the result
Worker::ParamMap PostgreSQLNotifyDelegate::run(Worker::ParamMap& params) {
    // TODO: Start using   thread pool instead, rather then one-off connection
    Worker::ParamMap result;
    
    PostgreSQLConnection connection(0, params);
    bool status = connection.connect();
    if (!status) {
        cancel();
    }
    
    // Loop forever until cancelled
    while (!cancelled) {
        bool workDone = false;
        std::string command;
        
        // Listen to any channels
        // ------------------------------------------------
        if (cancelled) {
            break;
        }
        
        std::vector<std::string> listens = popListens();
        for(std::vector<std::string>::iterator it = listens.begin(); it != listens.end(); ++it) {
            workDone = true;
            command = str(format("LISTEN %s") % *it);
            LOG_INFO << command;
            connection.execute(command);
            // Wait for command to execute on server, this must be done otherwise the transaction will get rolled back.
            try {
                while (!connection.isFinished()) {
                    connection.check();
                    boost::this_thread::sleep(boost::posix_time::millisec(SLEEP_MS));  // Thread interruption point as well as sleep
                }
            } catch (const boost::thread_interrupted&) {
                cancel();
            }
            connection.discardResult();
        }
        listens.clear();
        
        // Unlisten from any channels
        // ------------------------------------------------
        if (cancelled) {
            break;
        }
        
        std::vector<std::string> unlistens = popUnlistens();
        for(std::vector<std::string>::iterator it = unlistens.begin(); it != unlistens.end(); ++it) {
            workDone = true;
            command = str(format("UNLISTEN %s") % *it);
            LOG_INFO << command;
            connection.execute(command);
            // Wait for command to execute on server, this must be done otherwise the transaction will get rolled back.
            try {
                while (!connection.isFinished()) {
                    connection.check();
                    boost::this_thread::sleep(boost::posix_time::millisec(SLEEP_MS));  // Thread interruption point as well as sleep
                }
            } catch (const boost::thread_interrupted&) {
                cancel();
            }
            connection.discardResult();
        }
        unlistens.clear();
        
        // Send any notifications
        // ------------------------------------------------
        if (cancelled) {
            break;
        }
        
        std::vector<std::pair<std::string, std::string> > notifications = popNotifications();
        for(std::vector<std::pair<std::string, std::string> >::iterator it = notifications.begin(); it != notifications.end(); ++it) {
            workDone = true;
            command = str(format("SELECT pg_notify('%s', '%s')") % it->first % it->second);
            LOG_INFO << command;
            connection.execute(command);
            // Wait for command to execute on server, this must be done otherwise the transaction will get rolled back.
            try {
                while (!connection.isFinished()) {
                    connection.check();
                    boost::this_thread::sleep(boost::posix_time::millisec(SLEEP_MS));  // Thread interruption point as well as sleep
                }
            } catch (const boost::thread_interrupted&) {
                cancel();
            }
            connection.discardResult();
        }
        notifications.clear();
        
        // Parse notifications that have been received
        // ------------------------------------------------
        if (cancelled) {
            break;
        }
        
        std::vector<Worker::ParamMap> received = connection.notifies();
        for(std::vector<Worker::ParamMap>::iterator it = received.begin(); it != received.end(); ++it) {
            workDone = true;
            receive(*it);
        }
        
        // Check if work was done in this cycle.  Sleep if nothing happened.
        // --------------------------------------------------------------------
        if (!cancelled && !workDone) {
            try {
                boost::this_thread::sleep(boost::posix_time::millisec(WAIT_MS));  // Thread interruption point as well as sleep
            } catch (const boost::thread_interrupted&) {
                cancel();
            }
        }
    }
    
    if(cancelled) {
        connection.cancel();  // Attempt to cancel connection
    }
    
    return result;
}