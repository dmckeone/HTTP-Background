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

/* STATIC METHODS (IMPLEMENTATION)
 * 
 * This file implements all static methods for the external.
 *
 * February 18, 2011 David McKeone (Created)
 */

#include "Static.he"

#include <extcomp.he>
#include "OmnisTools.he"
#include "Logging.he"

using namespace OmnisTools;

// Define static methods
const static qshort cStaticMethodLogTrace   = 20000,
                    cStaticMethodLogDebug   = 20001,
                    cStaticMethodLogInfo    = 20002,
                    cStaticMethodLogWarning = 20003,
                    cStaticMethodLogError   = 20004,
                    cStaticMethodLogFatal   = 20005;

// Parameters for Static Methods
// Columns are:
// 1) Name of Parameter (Resource #)
// 2) Return type (fft value)
// 3) Parameter flags of type EXTD_FLAG_xxxx
// 4) Extended flags.  Documentation states, "Must be 0"
ECOparam cStaticMethodsParamsTable[] = 
{
	// $logTrace
    5900, fftCharacter, 0, 0,
    // $logDebug
    5901, fftCharacter, 0, 0,
    // $logInfo
    5902, fftCharacter, 0, 0,
    // $logWarning
    5903, fftCharacter, 0, 0,
    // $logError
    5904, fftCharacter, 0, 0,
    // $logFatal
    5905, fftCharacter, 0, 0
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
ECOmethodEvent cStaticMethodsTable[] = 
{
	cStaticMethodLogTrace,   cStaticMethodLogTrace,   fftBoolean, 1, &cStaticMethodsParamsTable[0], 0, 0,
    cStaticMethodLogDebug,   cStaticMethodLogDebug,   fftBoolean, 1, &cStaticMethodsParamsTable[1], 0, 0,
    cStaticMethodLogInfo,    cStaticMethodLogInfo,    fftBoolean, 1, &cStaticMethodsParamsTable[2], 0, 0,
    cStaticMethodLogWarning, cStaticMethodLogWarning, fftBoolean, 1, &cStaticMethodsParamsTable[3], 0, 0,
    cStaticMethodLogError,   cStaticMethodLogError,   fftBoolean, 1, &cStaticMethodsParamsTable[4], 0, 0,
    cStaticMethodLogFatal,   cStaticMethodLogFatal,   fftBoolean, 1, &cStaticMethodsParamsTable[5], 0, 0
};

// List of methods in Simple
qlong returnStaticMethods(tThreadData* pThreadData)
{
	const qshort cStaticMethodCount = sizeof(cStaticMethodsTable) / sizeof(ECOmethodEvent);
	
	return ECOreturnMethods( gInstLib, pThreadData->mEci, &cStaticMethodsTable[0], cStaticMethodCount );
}

// Log trace message
void methodStaticLogTrace(tThreadData* pThreadData, qshort paramCount) {
	
    // Read message and post to log
    EXTfldval messageVal;
    bool success = false;
	if( getParamVar(pThreadData, 1, messageVal) == qtrue ) {
        LOG_TRACE << getStringFromEXTFldVal(messageVal);
        success = true;
    }
    
    // Return bool to caller
    EXTfldval retVal;    
    getEXTFldValFromBool(retVal, success);
    ECOaddParam(pThreadData->mEci, &retVal);
}

// Log debug message
void methodStaticLogDebug(tThreadData* pThreadData, qshort paramCount) {
	
    // Read message and post to log
    EXTfldval messageVal;
    bool success = false;
	if( getParamVar(pThreadData, 1, messageVal) == qtrue ) {
        LOG_DEBUG << getStringFromEXTFldVal(messageVal);
        success = true;
    }
    
    // Return bool to caller
    EXTfldval retVal;    
    getEXTFldValFromBool(retVal, success);
    ECOaddParam(pThreadData->mEci, &retVal);
}

// Log info message
void methodStaticLogInfo(tThreadData* pThreadData, qshort paramCount) {
    
    // Read message and post to log
    EXTfldval messageVal;
    bool success = false;
	if( getParamVar(pThreadData, 1, messageVal) == qtrue ) {
        LOG_INFO << getStringFromEXTFldVal(messageVal);
        
        success = true;
    }
    
    // Return bool to caller
    EXTfldval retVal;    
    getEXTFldValFromBool(retVal, success);
    ECOaddParam(pThreadData->mEci, &retVal);
}

// Log warning message
void methodStaticLogWarning(tThreadData* pThreadData, qshort paramCount) {
	
    // Read message and post to log
    EXTfldval messageVal;
    bool success = false;
	if( getParamVar(pThreadData, 1, messageVal) == qtrue ) {
        LOG_WARNING << getStringFromEXTFldVal(messageVal);
        success = true;
    }
    
    // Return bool to caller
    EXTfldval retVal;    
    getEXTFldValFromBool(retVal, success);
    ECOaddParam(pThreadData->mEci, &retVal);
}

// Log error message
void methodStaticLogError(tThreadData* pThreadData, qshort paramCount) {
	
    // Read message and post to log
    EXTfldval messageVal;
    bool success = false;
	if( getParamVar(pThreadData, 1, messageVal) == qtrue ) {
        LOG_ERROR << getStringFromEXTFldVal(messageVal);
        success = true;
    }
    
    // Return bool to caller
    EXTfldval retVal;    
    getEXTFldValFromBool(retVal, success);
    ECOaddParam(pThreadData->mEci, &retVal);
}

// Log fatal message
void methodStaticLogFatal(tThreadData* pThreadData, qshort paramCount) {
	
    // Read message and post to log
    EXTfldval messageVal;
    bool success = false;
	if( getParamVar(pThreadData, 1, messageVal) == qtrue ) {
        LOG_FATAL << getStringFromEXTFldVal(messageVal);
        success = true;
    }
    
    // Return bool to caller
    EXTfldval retVal;    
    getEXTFldValFromBool(retVal, success);
    ECOaddParam(pThreadData->mEci, &retVal);
}

// Static method dispatch
qlong staticMethodCall( OmnisTools::tThreadData* pThreadData ) {
	
	qshort funcId = (qshort)ECOgetId(pThreadData->mEci);
	qshort paramCount = ECOgetParamCount(pThreadData->mEci);
	
	switch( funcId )
	{
		case cStaticMethodLogTrace:
			pThreadData->mCurMethodName = "$logTrace";
			methodStaticLogTrace(pThreadData, paramCount);
			break;
        case cStaticMethodLogDebug:
			pThreadData->mCurMethodName = "$logDebug";
			methodStaticLogDebug(pThreadData, paramCount);
			break;
        case cStaticMethodLogInfo:
			pThreadData->mCurMethodName = "$logInfo";
			methodStaticLogInfo(pThreadData, paramCount);
			break;
        case cStaticMethodLogWarning:
			pThreadData->mCurMethodName = "$logWarning";
			methodStaticLogWarning(pThreadData, paramCount);
			break;
        case cStaticMethodLogError:
			pThreadData->mCurMethodName = "$logError";
			methodStaticLogError(pThreadData, paramCount);
			break;
        case cStaticMethodLogFatal:
			pThreadData->mCurMethodName = "$logFatal";
			methodStaticLogFatal(pThreadData, paramCount);
			break;
	}
	
	return 0L;
}

