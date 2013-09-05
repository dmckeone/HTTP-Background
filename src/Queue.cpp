//
//  Queue.cpp
//  PostgreSQLBackground
//
//  Created by David McKeone on 11-11-01.
//  Copyright 2011 __MyCompanyName__. All rights reserved.
//

#include "Queue.h"
#include "Logging.he"

#include <boost/format.hpp>

using boost::format;
using boost::shared_ptr;

// Description of object used for logging
std::string Queue::desc() {
    if (!queueName.empty()) {
        return str(format("Queue (%s)") % queueName);
    } else {
        return "Queue";
    }  
}

void Queue::add(boost::shared_ptr<Worker> w) { 
    _queue.push_back(w); 
    
    LOG_INFO << "Adding " << w->desc() << " to " << desc();
    
    // Init object prior to performing threading (for objects that need allocation on main thread)
    w->init();
    
    checkQueue(); 
}

// Check queue for new items
void Queue::checkQueue() {
    
    // The queue will be checked frequently, so if we can't get the lock, we will try again later
    if( _queueMutex.try_lock() == false ) {
        return;
    }
    
    int runSize = 0;
    WorkQueueIterator it = _queue.begin();
    shared_ptr<Worker> worker;
    while( it != _queue.end() && runSize < _size ) {
        
        worker = (*it);
        
        if( worker->complete() == true) {
            // Item is complete, pop it off
            _queue.erase(it);
        } else {
            // Start Workers until the queue size is hit
            if( worker->running() == false) {
                LOG_INFO << "Starting " << worker->desc() << " in " << desc();
                worker->start(shared_from_this());
            }
            
            ++runSize;
        }
        ++it;
    }
}

// Mark worker as completed and move it into completed vector
void Queue::workerDone(boost::weak_ptr<Worker> worker) {
    
    {
        boost::mutex::scoped_lock lock(_finishedMutex);
        
        // Convert weak ptr back to shared_ptr
        boost::shared_ptr<Worker> completedWorker;
        try {
            completedWorker = worker.lock();
        } catch (boost::bad_weak_ptr bwp) {
            LOG_ERROR << "Unable to get reference to completed worker in " << desc();
        }
        
        if(completedWorker != boost::shared_ptr<Worker>()) {
            LOG_INFO << completedWorker->desc() << " marked as completed in " << desc();
            _completed.push_back(completedWorker); 
        }
    }
    
    // When worker completes check the queue
    checkQueue();
}