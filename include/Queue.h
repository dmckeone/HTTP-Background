//
//  Queue.h
//  TMWorkQueue
//
//  Created by David McKeone on 11-11-01.
//  Copyright 2011 __MyCompanyName__. All rights reserved.
//

#ifndef QUEUE_H_
#define QUEUE_H_

#include "Worker.h"
#include "DBPool.h"

#include <string>
#include <vector>
#include <map>
#include <boost/enable_shared_from_this.hpp>
#include <boost/make_shared.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/thread/mutex.hpp>

class Queue : public boost::enable_shared_from_this<Queue> {
public:
    typedef std::vector<boost::shared_ptr<Worker> > WorkQueue;
    typedef std::vector<boost::shared_ptr<Worker> >::iterator WorkQueueIterator;
    
    Queue() {}
    ~Queue() {}
       
    void add(boost::shared_ptr<Worker>);
    
    void setSize(int size) { _size = size; }
    int size() { return _size; }
    
    void workerDone(boost::weak_ptr<Worker>);
    
    WorkQueue& completed() { return _completed; }
    void clearCompleted() { _completed.clear(); }
    
    void setQueueName(std::string qn) { queueName = qn; }
    virtual std::string desc();
    
private:
    std::map<std::string, boost::shared_ptr<DBPool> > pools;
public:
    // Create a pool for the given pool type T.  T must be inherited from DBPool
    template<class T>
    boost::shared_ptr<T> addPool(std::string name, const ConnInfoMap& ci) {
        pools[name] = boost::make_shared<T>(ci, shared_from_this());
        return pools[name];
    }
protected:
    std::string queueName;
private:
    void checkQueue();
    
    int _size;
    WorkQueue _queue;
    WorkQueue _completed;
           boost::mutex _queueMutex;
    boost::mutex _finishedMutex;
};


#endif // QUEUE_H_