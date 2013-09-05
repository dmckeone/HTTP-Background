//
//  NVObjQueue.cpp
//  PostgreSQLBackground
//
//  Created by David McKeone on 11-10-21.
//  Copyright 2011 __MyCompanyName__. All rights reserved.
//

#include <extcomp.he>
#include "NVObjQueue.he"
#include "ThreadTimer.he"

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
NVObjQueue::NVObjQueue(qobjinst objinst, tThreadData* pThreadData) : NVObjBase(objinst) {
    
}

// Destructor
NVObjQueue::~NVObjQueue() {
    
}

/**************************************************************************************************
 **                              PROPERTY DECLERATION                                            **
 **************************************************************************************************/

// This is where the resource # of the methods is defined.  In this project it is also used as the Unique ID.
const static qshort cPropertyMyProperty = 3500;

/**************************************************************************************************
 **                               METHOD DECLERATION                                             **
 **************************************************************************************************/

// This is where the resource # of the methods is defined.  In this project is also used as the Unique ID.
const static qshort cMethodError      = 3000,
                    cMethodInitialize = 3001;;

/**************************************************************************************************
 **                                 INSTANCE METHODS                                             **
 **************************************************************************************************/

// Call a method
qlong NVObjQueue::methodCall( tThreadData* pThreadData )
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
	}
	
	callErrorMethod(pThreadData, result);
    
	return 0L;
}

/**************************************************************************************************
 **                                PROPERTIES                                                    **
 **************************************************************************************************/

// Assignability of properties
qlong NVObjQueue::canAssignProperty( tThreadData* pThreadData, qlong propID ) {
	switch (propID) {
		case cPropertyMyProperty:
			return qtrue;
		default:
			return qfalse;
	}
}

// Method to retrieve a property of the object
qlong NVObjQueue::getProperty( tThreadData* pThreadData ) 
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
qlong NVObjQueue::setProperty( tThreadData* pThreadData )
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
ECOparam cQueueMethodsParamsTable[] = 
{
	3900, fftInteger  , 0, 0,
	3901, fftCharacter, 0, 0,
	3902, fftCharacter, 0, 0,
	3903, fftCharacter, 0, 0
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
ECOmethodEvent cQueueMethodsTable[] = 
{
	cMethodError,      cMethodError,      fftNumber,  4, &cQueueMethodsParamsTable[0], 0, 0,
	cMethodInitialize, cMethodInitialize, fftBoolean, 0,                            0, 0, 0
};

// List of methods in Simple
qlong NVObjQueue::returnMethods(tThreadData* pThreadData)
{
	const qshort cMethodCount = sizeof(cQueueMethodsTable) / sizeof(ECOmethodEvent);
	
	return ECOreturnMethods( gInstLib, pThreadData->mEci, &cQueueMethodsTable[0], cMethodCount );
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
ECOproperty cQueuePropertyTable[] = 
{
	cPropertyMyProperty, cPropertyMyProperty, fftInteger, EXTD_FLAG_PROPCUSTOM, 0, 0, 0 /* Shows under Custom category */
};

// List of properties in Simple
qlong NVObjQueue::returnProperties( tThreadData* pThreadData )
{
	const qshort propertyCount = sizeof(cQueuePropertyTable) / sizeof(ECOproperty);
    
	return ECOreturnProperties( gInstLib, pThreadData->mEci, &cQueuePropertyTable[0], propertyCount );
}

/**************************************************************************************************
 **                       THREAD TIMER NOTIFIER                                                  **
 **************************************************************************************************/

int NVObjQueue::notify() 
{        
    return ThreadTimer::kTimerContinue;
}

/**************************************************************************************************
 **                              CUSTOM (YOUR) METHODS                                           **
 **************************************************************************************************/

// Start the thread
tResult NVObjQueue::methodInitialize( tThreadData* pThreadData, qshort pParamCount )
{
    // Create new queue object
    _queue = boost::make_shared<Queue>();
	
    EXTfldval retVal;
    getEXTFldValFromBool(retVal,true);
    ECOaddParam(pThreadData->mEci, &retVal);
    
	return METHOD_DONE_RETURN;
}