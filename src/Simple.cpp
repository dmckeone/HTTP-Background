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

#include <boost/thread.hpp>
#include <boost/thread/mutex.hpp>

#include <iostream>

using namespace OmnisTools;

// Mutexes
boost::mutex read_mutex;

/**************************************************************************************************
 **                       CONSTRUCTORS / DESTRUCTORS                                             **
 **************************************************************************************************/

NVObjSimple::NVObjSimple(qobjinst objinst, tThreadData *pThreadData) : NVObjBase(objinst), myProperty(0)
{ }

NVObjSimple::~NVObjSimple()
{ }

/**************************************************************************************************
 **                              PROPERTY DECLERATION                                            **
 **************************************************************************************************/

// This is where the resource # of the methods is defined.  In this project it is also used as the Unique ID.
const static qshort cPropertyMyProperty = 3100;

/**************************************************************************************************
 **                               METHOD DECLERATION                                             **
 **************************************************************************************************/

// This is where the resource # of the methods is defined.  In this project is also used as the Unique ID.
const static qshort cMethodError    = 2100,
                    cMethodStart    = 2101,
					cMethodStop     = 2102,
					cMethodCheck    = 2103;

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
	4000, fftInteger  , 0, 0,
	4001, fftCharacter, 0, 0,
	4002, fftCharacter, 0, 0,
	4003, fftCharacter, 0, 0,
	4004, fftNumber,    0, 0
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
 **                              CUSTOM (YOUR) METHODS                                           **
 **************************************************************************************************/

// Start the thread
tResult NVObjSimple::methodStart( tThreadData* pThreadData, qshort pParamCount )
{
	stopThread();
	
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

// Thread

void doWork(qlong* curCount);

void NVObjSimple::startThread() {
	boost::thread thrd1(boost::bind(&doWork, &curCount));
}

void NVObjSimple::stopThread() {
}

void doWork(qlong* curCount) {
	for (*curCount = 0; *curCount < 1000000; ++(*curCount))
	{
		{
			boost::mutex::scoped_lock lock(read_mutex);
			std::cout << "Count: "<< *curCount << std::endl;
		}
	}
};
