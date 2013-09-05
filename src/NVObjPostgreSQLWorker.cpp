//
//  NVObjPostgreSQLWorker.cpp
//  PostgreSQLBackground
//
//  Created by David McKeone on 11-10-21.
//  Copyright 2011 __MyCompanyName__. All rights reserved.
//

#include <extcomp.he>
#include "NVObjPostgreSQLWorker.he"
#include "ThreadTimer.he"
#include "Logging.he"

#include <boost/make_shared.hpp>
#include <boost/thread.hpp>
#include <boost/thread/mutex.hpp>
#include <boost/format.hpp>

using boost::format;
using namespace OmnisTools;

/**************************************************************************************************
 **                       CONSTRUCTORS / DESTRUCTORS                                             **
 **************************************************************************************************/

// Constructor
NVObjPostgreSQLWorker::NVObjPostgreSQLWorker(qobjinst objinst, tThreadData* pThreadData) : NVObjBase(objinst) {

}

// Destructor
NVObjPostgreSQLWorker::~NVObjPostgreSQLWorker() {    
    // Unsubscribe this instance from the timer
    ThreadTimer& timerInst = ThreadTimer::instance();
    timerInst.unsubscribe(this);
}

/**************************************************************************************************
 **                              PROPERTY DECLERATION                                            **
 **************************************************************************************************/

// This is where the resource # of the methods is defined.  In this project it is also used as the Unique ID.
const static qshort cPropertyMyProperty = 4500;

/**************************************************************************************************
 **                               METHOD DECLERATION                                             **
 **************************************************************************************************/

// This is where the resource # of the methods is defined.  In this project is also used as the Unique ID.
const static qshort cMethodError      = 4000,
                    cMethodInitialize = 4001,
                    cMethodRun        = 4002,
                    cMethodStart      = 4003,
                    cMethodCancel     = 4004;

/**************************************************************************************************
 **                                 INSTANCE METHODS                                             **
 **************************************************************************************************/

// Call a method
qlong NVObjPostgreSQLWorker::methodCall( tThreadData* pThreadData )
{
	tResult result = METHOD_OK;
	qshort funcId = (qshort)ECOgetId(pThreadData->mEci);
	qshort paramCount = ECOgetParamCount(pThreadData->mEci);
    
	switch( funcId )
	{
		case cMethodError:
			result = METHOD_OK; // Always return ok to prevent circular call to error.
			break;
		case cMethodInitialize:
			pThreadData->mCurMethodName = "$initialize";
			result = methodInitialize(pThreadData, paramCount);
			break;
        case cMethodRun:
            pThreadData->mCurMethodName = "$run";
            result = methodRun(pThreadData, paramCount);
            break;
        case cMethodStart:
            pThreadData->mCurMethodName = "$start";
            result = methodStart(pThreadData, paramCount);
            break;
        case cMethodCancel:
            pThreadData->mCurMethodName = "$cancel";
            result = methodCancel(pThreadData, paramCount);
            break;
	}
	
	callErrorMethod(pThreadData, result);
    
	return 0L;
}

/**************************************************************************************************
 **                                PROPERTIES                                                    **
 **************************************************************************************************/

// Assignability of properties
qlong NVObjPostgreSQLWorker::canAssignProperty( tThreadData* pThreadData, qlong propID ) {
	switch (propID) {
		case cPropertyMyProperty:
			return qtrue;
		default:
			return qfalse;
	}
}

// Method to retrieve a property of the object
qlong NVObjPostgreSQLWorker::getProperty( tThreadData* pThreadData ) 
{
	EXTfldval fValReturn;
    
	qlong propID = ECOgetId( pThreadData->mEci );
	switch( propID ) {
		case cPropertyMyProperty:
			//fValReturn.setLong(myProperty); // Put property into return value
			ECOaddParam(pThreadData->mEci, &fValReturn); // Return to caller
			break;	       
	}
    
	return 1L;
}

// Method to set a property of the object
qlong NVObjPostgreSQLWorker::setProperty( tThreadData* pThreadData )
{
	// Retrieve value to set for property, always in first parameter
	EXTfldval fVal;
	if( getParamVar( pThreadData->mEci, 1, fVal) == qfalse ) 
		return qfalse;
    
	// Assign to the appropriate property
	qlong propID = ECOgetId( pThreadData->mEci );
	switch( propID ) {
		case cPropertyMyProperty:
			//myProperty = fVal.getLong();
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
ECOparam cPGWorkerMethodsParamsTable[] = 
{
	4900, fftInteger  , 0, 0,
	4901, fftCharacter, 0, 0,
	4902, fftCharacter, 0, 0,
	4903, fftCharacter, 0, 0
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
ECOmethodEvent cPGWorkerMethodsTable[] = 
{
	cMethodError,      cMethodError,      fftNumber,  4, &cPGWorkerMethodsParamsTable[0], 0, 0,
	cMethodInitialize, cMethodInitialize, fftBoolean, 0,                               0, 0, 0,
    cMethodRun,        cMethodRun,        fftNone,    0,                               0, 0, 0,
    cMethodStart,      cMethodStart,      fftNone,    0,                               0, 0, 0,
    cMethodCancel,     cMethodCancel,     fftNone,    0,                               0, 0, 0
};

// List of methods in Simple
qlong NVObjPostgreSQLWorker::returnMethods(tThreadData* pThreadData)
{
	const qshort cMethodCount = sizeof(cPGWorkerMethodsTable) / sizeof(ECOmethodEvent);
	
	return ECOreturnMethods( gInstLib, pThreadData->mEci, &cPGWorkerMethodsTable[0], cMethodCount );
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
ECOproperty cPGWorkerPropertyTable[] = 
{
	cPropertyMyProperty, cPropertyMyProperty, fftInteger, EXTD_FLAG_PROPCUSTOM, 0, 0, 0 /* Shows under Custom category */
};

// List of properties in Simple
qlong NVObjPostgreSQLWorker::returnProperties( tThreadData* pThreadData )
{
	const qshort propertyCount = sizeof(cPGWorkerPropertyTable) / sizeof(ECOproperty);
    
	return ECOreturnProperties( gInstLib, pThreadData->mEci, &cPGWorkerPropertyTable[0], propertyCount );
}

/**************************************************************************************************
 **                              PARAM CONVERSION                                                **
 **************************************************************************************************/

bool NVObjPostgreSQLWorker::getParamsFromRow(tThreadData* pThreadData, EXTfldval& row, Worker::ParamMap& params) {
    
    if(getType(row).valType != fftRow && getType(row).valType != fftList) {
        return false;
    }
    
    // Convert row into more appropriate variables for parameters
    EXTfldval colVal, colTitleVal;
    EXTqlist rowData;
    row.getList(&rowData, qfalse);
    for( qshort col = 1; col <= rowData.colCnt(); ++col) {
        str255 colName;
        rowData.getCol(col, qfalse, colName);
        colTitleVal.setChar(colName);
        
        rowData.getColValRef(1, col, colVal, qfalse);
        
        // Get column definition type
        ffttype fft;
        qshort fdp;
        rowData.getColType( col, fft, fdp );
        
        // Assign map based on definition
        switch (fft) {
            case fftCharacter:
                params[getStringFromEXTFldVal(colTitleVal)] = getStringFromEXTFldVal(colVal);
                break;
            case fftInteger:
                params[getStringFromEXTFldVal(colTitleVal)] = getIntFromEXTFldVal(colVal);
                break;
            case fftNumber:
                params[getStringFromEXTFldVal(colTitleVal)] = getDoubleFromEXTFldVal(colVal);
                break;
            case fftBoolean:
                params[getStringFromEXTFldVal(colTitleVal)] = getBoolFromEXTFldVal(colVal);
                break;
            default:
                LOG_DEBUG << "Unknown column type when converting parameters.";
                break;
        }
    }    
    
    return true;
}

bool NVObjPostgreSQLWorker::getRowFromParams(EXTfldval& row, Worker::ParamMap& params) {
    
    Worker::ParamMapIterator it;
    str255 colName;
    EXTfldval colVal;
    EXTqlist* retList = new EXTqlist(listVlen); // Return row
    
    // Add all output columns
    colName = initStr255("Result");
    retList->addCol(fftRow, dpDefault, 0, &colName);
    
    retList->insertRow();
    
    // Look for output data
    it = params.find("Result");
    if( it != params.end()) {
        retList->getColValRef(1,1,colVal,qtrue);
        
        try {
            boost::shared_ptr<EXTqlist> ptr = boost::any_cast<boost::shared_ptr<EXTqlist> >(it->second);
            colVal.setList(ptr.get(), qtrue); 
        } catch( const boost::bad_any_cast& e ) {
            LOG_ERROR << "Unable to cast return value from PostgreSQL worker.";
        }
    }
    
    row.setList(retList,qtrue);
    
    return true;
}

/**************************************************************************************************
 **                       THREAD TIMER NOTIFIER                                                  **
 **************************************************************************************************/

int NVObjPostgreSQLWorker::notify() 
{        
    if(_worker->complete()) {
        // Worker completed.  Call back into Omnis
        EXTfldval retVal;
        Worker::ParamMap pm = _worker->result();
        getRowFromParams(retVal, pm);
        
        str31 methodName(initStr31("$completed"));
        ECOdoMethod( this->getInstance(), &methodName, &retVal, 1 );
        
        return ThreadTimer::kTimerStop;
    } else if (_worker->cancelled()) {
        str31 methodName(initStr31("$canceled"));
        ECOdoMethod( this->getInstance(), &methodName, 0, 0 );
        
        return ThreadTimer::kTimerStop;
    }
    
    return ThreadTimer::kTimerContinue;
}

/**************************************************************************************************
 **                              CUSTOM (YOUR) METHODS                                           **
 **************************************************************************************************/

// Initialize the worker
tResult NVObjPostgreSQLWorker::methodInitialize( tThreadData* pThreadData, qshort pParamCount )
{    
    EXTfldval rowVal;
    if (getParamVar(pThreadData,1,rowVal) == qfalse) {
        pThreadData->mExtraErrorText = "1st parameter must be a row of parameters";
        return ERR_METHOD_FAILED;
    }
    
    // Convert row into parameters
    Worker::ParamMap params;
    if( getParamsFromRow(pThreadData, rowVal, params) == false) {
        pThreadData->mExtraErrorText = "1st parameter must be a row of parameters";
        return ERR_METHOD_FAILED;
    }
    
    // Create new worker object
	boost::shared_ptr<PostgreSQLDelegate> bgDelegate = boost::make_shared<PostgreSQLDelegate>();
    _worker = boost::make_shared<Worker>(params, bgDelegate);
    
    // Call all worker initialization code while on main thread
    _worker->init();
    
    EXTfldval retVal;
    getEXTFldValFromBool(retVal,true);
    ECOaddParam(pThreadData->mEci, &retVal);
	
	return METHOD_DONE_RETURN;
}

tResult NVObjPostgreSQLWorker::methodRun( tThreadData* pThreadData, qshort pParamCount )
{
    if( _worker == boost::shared_ptr<Worker>() ) {
        return ERR_METHOD_FAILED;
    }
    
    _worker->run();  // Run worker function object
    
    // Manually call notify since this is single-threaded
    notify();
    
	return METHOD_DONE_RETURN;
}

tResult NVObjPostgreSQLWorker::methodStart( tThreadData* pThreadData, qshort pParamCount )
{
    if( _worker == boost::shared_ptr<Worker>() ) {
        return ERR_METHOD_FAILED;
    }
    
    // Initiate timer to watch for finished events
    ThreadTimer& timerInst = ThreadTimer::instance();
    timerInst.subscribe(this);
    
    _worker->start();  // Run background thread
    
	return METHOD_DONE_RETURN;
}

tResult NVObjPostgreSQLWorker::methodCancel( tThreadData* pThreadData, qshort pParamCount )
{
    if( _worker == boost::shared_ptr<Worker>() ) {
        return ERR_METHOD_FAILED;
    }
    
    _worker->cancel();  // Attempt to cancel worker
    
	return METHOD_DONE_RETURN;
}