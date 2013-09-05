/* Copyright (c) 2010 David McKeone
 * All rights reserved.
 * 
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 * notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 * notice, this list of conditions and the following disclaimer in the
 * documentation and/or other materials provided with the distribution.
 * 3. The name of the author may not be used to endorse or promote products
 * derived from this software without specific prior written permission.
 * 
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

/* SIMPLE OBJECT (IMPLEMENTATION)
 * 
 * This object has some simple functionality to show how to do basic method calls and property setting.
 *
 * March 30, 2010 David McKeone (Created)
 */

#include <extcomp.he>
#include "Simple.he"
#include "ThreadTimer.he"
#include "libpq-fe.h"

#include <boost/thread.hpp>
#include <boost/thread/mutex.hpp>
#include <boost/format.hpp>

using boost::format;

#include <iostream>

using namespace OmnisTools;

// Mutexes
boost::mutex read_mutex;
boost::mutex work_mutex;

/**************************************************************************************************
 **                       CONSTRUCTORS / DESTRUCTORS                                             **
 **************************************************************************************************/

NVObjSimple::NVObjSimple(qobjinst objinst, tThreadData *pThreadData) : NVObjBase(objinst), myProperty(0)
{ }

NVObjSimple::~NVObjSimple()
{ 
    stopThread();
}

/**************************************************************************************************
 **                              PROPERTY DECLERATION                                            **
 **************************************************************************************************/

// This is where the resource # of the methods is defined.  In this project it is also used as the Unique ID.
const static qshort cPropertyMyProperty = 2500;

/**************************************************************************************************
 **                               METHOD DECLERATION                                             **
 **************************************************************************************************/

// This is where the resource # of the methods is defined.  In this project is also used as the Unique ID.
const static qshort cMethodError    = 2000,
                    cMethodStart    = 2001,
					cMethodStop     = 2002,
					cMethodCheck    = 2003;

/**************************************************************************************************
 **                                 INSTANCE METHODS                                             **
 **************************************************************************************************/

// Call a method
qlong NVObjSimple::methodCall( tThreadData* pThreadData )
{
	tResult result = METHOD_OK;
	qshort funcId = (qshort)ECOgetId(pThreadData->mEci);
	qshort paramCount = ECOgetParamCount(pThreadData->mEci);

	switch( funcId )
	{
		case cMethodError:
			result = METHOD_OK; // Always return ok to prevent circular call to error.
			break;
		case cMethodStart:
			pThreadData->mCurMethodName = "$start";
			result = methodStart(pThreadData, paramCount);
			break;
		case cMethodStop:
			pThreadData->mCurMethodName = "$stop";
			result = methodStop(pThreadData, paramCount);
			break;
		case cMethodCheck:
			pThreadData->mCurMethodName = "$check";
			result = methodCheck(pThreadData, paramCount);
			break;
	}
	
	callErrorMethod(pThreadData, result);

	return 0L;
}

/**************************************************************************************************
 **                                PROPERTIES                                                    **
 **************************************************************************************************/

// Assignability of properties
qlong NVObjSimple::canAssignProperty( tThreadData* pThreadData, qlong propID ) {
	switch (propID) {
		case cPropertyMyProperty:
			return qtrue;
		default:
			return qfalse;
	}
}

// Method to retrieve a property of the object
qlong NVObjSimple::getProperty( tThreadData* pThreadData ) 
{
	EXTfldval fValReturn;

	qlong propID = ECOgetId( pThreadData->mEci );
	switch( propID ) {
		case cPropertyMyProperty:
			fValReturn.setLong(myProperty); // Put property into return value
			ECOaddParam(pThreadData->mEci, &fValReturn); // Return to caller
			break;	       
	}

	return 1L;
}

// Method to set a property of the object
qlong NVObjSimple::setProperty( tThreadData* pThreadData )
{
	// Retrieve value to set for property, always in first parameter
	EXTfldval fVal;
	if( getParamVar( pThreadData->mEci, 1, fVal) == qfalse ) 
		return qfalse;

	// Assign to the appropriate property
	qlong propID = ECOgetId( pThreadData->mEci );
	switch( propID ) {
		case cPropertyMyProperty:
			myProperty = fVal.getLong();
			break;
	}

	return 1L;
}

/**************************************************************************************************
 **                                        STATIC METHODS                                        **
 **************************************************************************************************/

/* METHODS */

// Table of parameter resources and types.
// Note that all parameters can be stored in this single table and the array offset can be  
// passed via the MethodsTable.
//
// Columns are:
// 1) Name of Parameter (Resource #)
// 2) Return type (fft value)
// 3) Parameter flags of type EXTD_FLAG_xxxx
// 4) Extended flags.  Documentation states, "Must be 0"
ECOparam cSimpleMethodsParamsTable[] = 
{
	2900, fftInteger  , 0, 0,
	2901, fftCharacter, 0, 0,
	2902, fftCharacter, 0, 0,
	2903, fftCharacter, 0, 0,
	2904, fftNumber,    0, 0
};

// Table of Methods available for Simple
// Columns are:
// 1) Unique ID 
// 2) Name of Method (Resource #)
// 3) Return Type 
// 4) # of Parameters
// 5) Array of Parameter Names (Taken from MethodsParamsTable.  Increments # of parameters past this pointer) 
// 6) Enum Start (Not sure what this does, 0 = disabled)
// 7) Enum Stop (Not sure what this does, 0 = disabled)
ECOmethodEvent cSimpleMethodsTable[] = 
{
	cMethodError,     cMethodError, fftNumber, 4, &cSimpleMethodsParamsTable[0], 0, 0,
	cMethodStart,     cMethodStart, fftNumber, 1, &cSimpleMethodsParamsTable[3], 0, 0,
	cMethodStop,      cMethodStop,  fftNone  , 0, 0                            , 0, 0,
	cMethodCheck,     cMethodCheck, fftNone  , 0, 0                            , 0, 0
};

// List of methods in Simple
qlong NVObjSimple::returnMethods(tThreadData* pThreadData)
{
	const qshort cMethodCount = sizeof(cSimpleMethodsTable) / sizeof(ECOmethodEvent);
	
	return ECOreturnMethods( gInstLib, pThreadData->mEci, &cSimpleMethodsTable[0], cMethodCount );
}

/* PROPERTIES */

// Table of properties available from Simple
// Columns are:
// 1) Unique ID 
// 2) Name of Property (Resource #)
// 3) Return Type 
// 4) Flags describing the property
// 5) Additional Flags describing the property
// 6) Enum Start (Not sure what this does, 0 = disabled)
// 7) Enum Stop (Not sure what this does, 0 = disabled)
ECOproperty cSimplePropertyTable[] = 
{
	cPropertyMyProperty, cPropertyMyProperty, fftInteger, EXTD_FLAG_PROPCUSTOM, 0, 0, 0 /* Shows under Custom category */
};

// List of properties in Simple
qlong NVObjSimple::returnProperties( tThreadData* pThreadData )
{
	const qshort propertyCount = sizeof(cSimplePropertyTable) / sizeof(ECOproperty);

	return ECOreturnProperties( gInstLib, pThreadData->mEci, &cSimplePropertyTable[0], propertyCount );
}

/**************************************************************************************************
 **                       THREAD TIMER NOTIFIER                                                  **
 **************************************************************************************************/

int NVObjSimple::notify() 
{    
    bool callOmnisMethod = false;
    
    {
        boost::mutex::scoped_lock lock(work_mutex);
        
        if(_threadRunning && _workStopped) {
            _threadRunning = false;
            callOmnisMethod = true;            
        }
    }
    
    EXTfldval retList;
    if(callOmnisMethod) {
        //EXTqlist* theList = new EXTqlist(_work.result());
        retList.setList(_work.result(), qtrue);
        //_work.setResult(0);
        //theList = 0;
        
        str31 methodName(initStr31("$done"));
        ECOdoMethod( this->getInstance(), &methodName, &retList, 1 );
        
        return ThreadTimer::kTimerStop;
    }
    
    return ThreadTimer::kTimerContinue;
}

/**************************************************************************************************
 **                              CUSTOM (YOUR) METHODS                                           **
 **************************************************************************************************/

// Start the thread
tResult NVObjSimple::methodStart( tThreadData* pThreadData, qshort pParamCount )
{
    // Stop any previous thread
    stopThread();
    
    EXTfldval dbVal;
    if(getParamVar(pThreadData, 1, dbVal) == qfalse) {
        pThreadData->mExtraErrorText = "1st parameter must be the name of the database to connect to";
        return ERR_METHOD_FAILED;
    }
    std::string dbName = getStringFromEXTFldVal(dbVal);
    
    EXTfldval usernameVal;
    if(getParamVar(pThreadData, 2, usernameVal) == qfalse) {
        pThreadData->mExtraErrorText = "2nd parameter must be the username to use";
        return ERR_METHOD_FAILED;
    }
    std::string username = getStringFromEXTFldVal(usernameVal);
    
    EXTfldval passwordVal;
    if(getParamVar(pThreadData, 3, passwordVal) == qfalse) {
        pThreadData->mExtraErrorText = "3rd parameter must be the password to use";
        return ERR_METHOD_FAILED;
    }
    std::string password = getStringFromEXTFldVal(passwordVal);    
    
    EXTfldval queryVal;
    if(getParamVar(pThreadData, 4, queryVal) == qfalse) {
        pThreadData->mExtraErrorText = "4th parameter must be the SQL to execute";
        return ERR_METHOD_FAILED;
    }
    _query = getStringFromEXTFldVal(queryVal);
    
    // Build connection info string
    _connInfo = str(format("dbname=%s\nuser=%s\npassword=%s") % dbName % username % password);
	
    // Start thread
	startThread();
	
	return METHOD_DONE_RETURN;
}

// Stop the thread
tResult NVObjSimple::methodStop( tThreadData* pThreadData, qshort pParamCount )
{ 
	stopThread();
	
	return METHOD_DONE_RETURN;
}

// Check the value of the thread
tResult NVObjSimple::methodCheck( tThreadData* pThreadData, qshort pParamCount )
{ 
	boost::mutex::scoped_lock lock(read_mutex);
	
	// Build return value
	EXTfldval valReturn;
	valReturn.setLong(curCount);
	
	// Return it to Omnis
	ECOaddParam(pThreadData->mEci, &valReturn);
	
	return METHOD_DONE_RETURN;
}

EXTqlist* doWork(NVObjSimple* theObj, qlong* curCount, std::string connInfo, std::string query, EXTqlist* list) 
{       
    PGconn* connection = PQconnectdb(connInfo.c_str());
    
    if( PQstatus(connection) == CONNECTION_OK ) {        
        PGresult* result = PQexec(connection, query.c_str());
        int status = PQresultStatus(result);
        if( (status == PGRES_TUPLES_OK || status == PGRES_COMMAND_OK) ) {
            // Fill list
            std::string value;
            int totalRows = PQntuples(result);
            int totalCols = PQnfields(result);
            
            // Add a column for each column in the result set
            str255 colName;
            for (int col = 0; col < totalCols; ++col) {
                colName = initStr255(PQfname(result, col));
                list->addCol(fftCharacter, dpFcharacter, 10000000, &colName);
            }
            
            // Fill the list
            EXTfldval colVal;
            for (int row = 0; row < totalRows; ++row) {
                list->insertRow();
                for (int col = 0; col < totalCols; ++col) {
                    value = PQgetvalue(result,row, col);
                    list->getColValRef(row+1,col+1,colVal,qtrue);
                    getEXTFldValFromString(colVal,value);
                }
            }
        }
         
        // Cleanup result
        PQclear(result);
    }
    
    // Cleanup connection
    PQfinish(connection);
    
    // Interruption syntax
    try {
        boost::this_thread::interruption_point();
    } catch (boost::thread_interrupted) {
        std::cout << "Interrupted" << std::endl;
    }
    
    // Mark work as done
    {
        boost::mutex::scoped_lock lock(work_mutex);
        
        theObj->_workStopped = true;
    }
    
    return list;
};

void NVObjSimple::startThread() 
{
    _threadRunning = true;
    _workStopped = false;
    
    ThreadTimer& timerInst = ThreadTimer::instance();
    timerInst.subscribe(this);
    
    // Functor Call
    EXTqlist* mylist = new EXTqlist(listVlen);  // NOTE: Lists MUST be allocated on main Omnis thread, but can be populated in other threads
    _work = WorkObj(this, &curCount, _connInfo, _query, mylist);
    currentThread = boost::thread(_work);
    
    // Static method Call
    //currentThread = boost::thread(boost::bind(&doWork, this, &curCount));
    
    // Interrupting a thread
    //currentThread.interrupt();   
}

void NVObjSimple::stopThread() {
    ThreadTimer& timerInst = ThreadTimer::instance();
    timerInst.unsubscribe(this);
    
    if(_threadRunning) {
        currentThread.detach();
    }
}
