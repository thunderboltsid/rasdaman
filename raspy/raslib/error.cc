/*
* This file is part of rasdaman community.
*
* Rasdaman community is free software: you can redistribute it and/or modify
* it under the terms of the GNU General Public License as published by
* the Free Software Foundation, either version 3 of the License, or
* (at your option) any later version.
*
* Rasdaman community is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
* GNU General Public License for more details.
*
* You should have received a copy of the GNU General Public License
* along with rasdaman community.  If not, see <http://www.gnu.org/licenses/>.
*
* Copyright 2003, 2004, 2005, 2006, 2007, 2008, 2009 Peter Baumann /
rasdaman GmbH.
*
* For more information please see <http://www.rasdaman.org>
* or contact Peter Baumann via <baumann@rasdaman.com>.
*/
/***
 * SOURCE: error.cc
 *
 * MODULE: raslib
 * CLASS:   r_Error
 *
 * COMMENTS:
 * - in general, string should be used instead of dynamic char* mgmnt
 * - r_Error specializations must not use int literals
 * - freeTextTable() does not free strings -> mem leak
*/

using namespace std;

using namespace std;

#include "config.h"
#include "mymalloc/mymalloc.h"

#include "error.hh"
#include "debug.hh"

#include <string.h>
#include <sstream>

#include <fstream>
#include <string>
#include <utility>
#include <list>
#include <stdlib.h>
#include <stdio.h>

#include "../common/src/logging/easylogging++.hh"


using std::endl;
using std::ends;
using std::ios;
using std::ostringstream;

r_Error::errorInfo* r_Error::textList = NULL;
/// has error text file been loaded, i.e., is table filled?
bool r_Error::errorTextsLoaded = false;
std::list<std::pair<std::pair<int,char>, char*> > *r_Error::errorTexts;

r_Error::r_Error()
    :   theKind(r_Error_General),
        errorText(0),
        errorNo(0)
{
    resetErrorText();
}

r_Error::r_Error(const r_Error& err)
    :   theKind(err.theKind),
        errorText(0),
        errorNo(err.errorNo)

{
    if (err.errorText)
    {
        errorText = new char[ strlen(err.errorText)+1 ];
        strcpy(errorText, err.errorText);
    }
}

r_Error::r_Error(kind the_kind, unsigned int newErrorNo)
    :   theKind(the_kind),
        errorText(0),
        errorNo(newErrorNo)
{
    resetErrorText();
}

r_Error::~r_Error() throw()
{
    if (errorText)
        delete[] errorText;
}



const char*
r_Error::what() const throw ()
{
    return static_cast<const char*>(errorText);
}



const r_Error&
r_Error::operator=(const r_Error& obj)
{
    if (this != &obj)
    {
        if (errorText)
        {
            delete[] errorText;
            errorText = NULL;
        }

        theKind  = obj.theKind;
        errorNo  = obj.errorNo;

        if (obj.errorText)
        {
            errorText = new char[ strlen(obj.errorText)+1 ];
            strcpy(errorText, obj.errorText);
        }
    }

    return *this;
}

char*
r_Error::serialiseError()
{
    // default implementation for errors not of kind r_Error_SerialisableException
    char* retVal = NULL;
    char buf[80]; // should be enough for two numbers

    sprintf(buf, "%d\t%d", theKind, errorNo);
    retVal = static_cast<char*>(mymalloc(strlen(buf) + 1));
    strcpy(retVal, buf);
    return retVal;
}


r_Error*
r_Error::getAnyError(char* serErr)
{
    r_Error* retVal = NULL;
    char* currChar = NULL;
    kind newTheKind;
    unsigned int newErrNum = 0;
    // first read the attributes of r_Error to find out which subclass it is
    // (stored in errNum).
    newTheKind = static_cast<r_Error::kind>(strtol(serErr, &currChar, 10));
    newErrNum = strtol(currChar+1, &currChar, 10);

    if (newTheKind != r_Error_SerialisableException)
    {
        retVal = new r_Error(newTheKind, newErrNum);
    }
    else
    {
        // add new serialisable errors to this case!
        switch (newErrNum)
        {
        case 206:
            retVal = new r_Ebase_dbms(newTheKind, newErrNum, currChar+1);
            break;
        default: break;
        }
    }
    return retVal;
}

//
// --- error text file table maintenance ---------------------------------
//

void r_Error::initTextTable()
{
    char errorFileName[FILENAME_MAX];   // error file path/name
    std::ifstream errortexts;
    string line;                // current input line read from file
    char errKind;
    int errNo;
    unsigned int numOfEntries = 0;
    int result;             // sscanf() result

    if (r_Error::errorTexts == NULL)
        r_Error::errorTexts = new list<pair<pair<int,char>,char*> >();

    // determine file path + name
    int filenameLength = snprintf( errorFileName, FILENAME_MAX, "%s/%s", SHARE_DATA_DIR, ERRORTEXT_FILE );
    if (filenameLength < FILENAME_MAX )
    {
        //LINFO << "Using error text file: " << errorFileName;
    }
    else
    {
        errorFileName[FILENAME_MAX-1] = '\0';   // force-terminate string before printing
        cerr << "Warning: error text file path longer than allowed by OS, likely file will not be found: " << errorFileName << endl;
    }

    // errortexts.open(errorFileName, ios::nocreate | ios::in);
    errortexts.open(errorFileName, ios::in);

    // In case the file 'errtxts' can't be found
#ifdef __VISUALC__
    if (!errortexts.is_open())
#else
    if (!errortexts)
#endif
    {
        r_Error::textList = NULL;
        LINFO << "No error texts file found at: " << errorFileName;
    }
    else    // File errtxts found
    {
        // read entries from file
        numOfEntries = 0;   // just for displaying, currently not used otherwise
        while ( ! errortexts.eof() )
        {
            getline( errortexts, line );
            char *errText = static_cast<char*>(mymalloc( line.length() + 1 ));
            if (errText == NULL)
            {
                LFATAL << "Fatal error: cannot allocate error text table line #" << numOfEntries;
                throw r_Error( r_Error::r_Error_MemoryAllocation );
            }
            // general line format is (aside of comments and empty lines): ddd^f^text...
            result = sscanf( line.c_str(), "%d^%c^%[^\n]s\n", &errNo, &errKind, errText );
            if (result == 3)    // consider only lines that match given structure
            {
                r_Error::errorTexts->push_back( pair<pair<int,char>,char*> (pair<int,char>( errNo, errKind ), errText) );
                numOfEntries++;
            }
            else
            {
                free(errText);
            }
        }
        // LFATAL << "number of error texts loaded: " << numOfEntries;
        errorTextsLoaded = true;
    }
    errortexts.close();
}



void r_Error::freeTextTable()
{
    if (!r_Error::errorTexts)
        return;

    list<pair<pair<int,char>, char*> >::iterator iter = r_Error::errorTexts->begin();
    list<pair<pair<int,char>, char*> >::iterator end = r_Error::errorTexts->end();

    if (errorTextsLoaded)       // have we initialized the table previously?
    {
        // yes -> free each string
        while ( iter != end  )
        {
            if (iter->second)
                free( iter->second );
            iter->second = NULL;
            ++iter;
        }
        r_Error::errorTexts->clear();   // now clear list itself
        delete r_Error::errorTexts;
    }
}



r_Error::r_Error(unsigned int errorno)
    :   theKind(r_Error_General),
        errorText(NULL),
        errorNo(errorno)
{
    resetErrorText();
}



void
r_Error::setErrorTextOnKind()
{
    char buffer[256];

    switch (theKind)
    {
    case r_Error_General :
        strcpy(buffer, " ODMG General");
        break;

    case r_Error_DatabaseClassMismatch :
        strcpy(buffer, "Database Class Mismatch");
        break;

    case r_Error_DatabaseClassUndefined :
        strcpy(buffer, "Database Class Undefined");
        break;

    case r_Error_DatabaseClosed :
        strcpy(buffer, "Database Closed");
        break;

    case r_Error_DatabaseOpen :
        strcpy(buffer, "Database Open");
        break;

    case r_Error_DateInvalid :
        strcpy(buffer, "Date Invalid");
        break;

    case r_Error_IteratorExhausted :
        strcpy(buffer, "Iterator Exhausted");
        break;

    case r_Error_NameNotUnique :
        strcpy(buffer, "Name Not Unique");
        break;

    case r_Error_QueryParameterCountInvalid :
        strcpy(buffer, "Query Parameter Count Invalid");
        break;

    case r_Error_QueryParameterTypeInvalid :
        strcpy(buffer, "Query Parameter Type Invalid");
        break;

    case r_Error_RefInvalid :
        strcpy(buffer, "Ref Invalid");
        break;

    case r_Error_RefNull :
        strcpy(buffer, "Ref Null");
        break;

    case r_Error_TimeInvalid :
        strcpy(buffer, "Time Invalid");
        break;

    case r_Error_TimestampInvalid :
        strcpy(buffer, "Timestamp Invalid");
        break;

    case r_Error_TransactionOpen :
        strcpy(buffer, "Transaction Open");
        break;

    case r_Error_TransactionNotOpen :
        strcpy(buffer, "Transaction Not Open");
        break;

    case r_Error_TypeInvalid :
        strcpy(buffer, "Type Invalid");
        break;

    case r_Error_DatabaseUnknown :
        strcpy(buffer, "Database Unknown");
        break;

    case r_Error_TransferFailed :
        strcpy(buffer, "Transfer Failed");
        break;

    case r_Error_HostInvalid :
        strcpy(buffer, "Host Invalid");
        break;

    case r_Error_ServerInvalid :
        strcpy(buffer, "Server Invalid");
        break;

    case r_Error_ClientUnknown :
        strcpy(buffer, "Client Unknown");
        break;

    case r_Error_ObjectUnknown :
        strcpy(buffer, "Object Unknown");
        break;

    case r_Error_ObjectInvalid :
        strcpy(buffer, "Object Invalid");
        break;

    case r_Error_QueryExecutionFailed :
        strcpy(buffer, "Query Execution Failed");
        break;

    case r_Error_BaseDBMSFailed :
        strcpy(buffer, "Base DBMS Failed");
        break;

    case r_Error_CollectionElementTypeMismatch :
        strcpy(buffer, "Collection Element Type Mismatch");
        break;

    case r_Error_CreatingOIdFailed :
        strcpy(buffer, "Creation of OID failed");
        break;

    case r_Error_TransactionReadOnly :
        strcpy(buffer, "Transaction is read only");
        break;

    case r_Error_LimitsMismatch :
        strcpy(buffer, "Limits reported to an object mismatch");
        break;

    case r_Error_NameInvalid :
        strcpy(buffer, "Name Invalid");
        break;

    case r_Error_FeatureNotSupported :
        strcpy(buffer, "Feature is not supported");
        break;

    case r_Error_AccesDenied:
        strcpy(buffer, "Access denied");
        break;

    case r_Error_MemoryAllocation:
        strcpy(buffer, "Memory allocation failed");
        break;

    case r_Error_InvalidOptimizationLevel:
        strcpy(buffer, "Illegal value for optimization level");
        break;

    case r_Error_Conversion:
        strcpy(buffer, "Format conversion failed");
        break;

    default:
        strcpy(buffer, "not specified");
        break;
    }

    if (errorText)
        delete[] errorText;

    char preText[] = "Exception: ";

    errorText = new char[strlen(preText) + strlen(buffer) + 1];
    strcpy(errorText, preText);
    strcat(errorText, buffer);
}



void
r_Error::setErrorTextOnNumber()
{
    char *result = NULL;    // ptr to error text constructed

    // delete old error text, if any
    if (errorText)
    {
        delete[] errorText;
        errorText = 0;
    }

    // has list been built earlier?
    if (errorTextsLoaded)
    {
        // yes, we have a list -> search
        // search through list to find entry
        list<pair<pair<int,char>, char*> >::iterator iter = r_Error::errorTexts->begin();
        list<pair<pair<int,char>, char*> >::iterator end = r_Error::errorTexts->end();
        bool found = false;
        while ( iter != end  && ! found )
        {
            if (iter->first.first == static_cast<int>(errorNo))
                found = true;
            else
                iter++;
        }

        if (found)
            result = iter->second;
        else {
	    char str[] = "(no explanation text available for this error code.)";
	    result = str;
	}
    }
    else {   // no, there is no spoon -err- list
        char str[] = "(no explanation text available - cannot find/load file with standard error messages.)";
        result = str;
    }

    errorText = new char[strlen(result) + 1];
    if (errorText == NULL)
    {
        LFATAL << "Error: cannot allocate error text.";
        throw r_Error( r_Error::r_Error_MemoryAllocation );
    }
    else
        strcpy(errorText, result);

    return;
}



void
r_Error::setTextParameter(const char* parameterName, int value)
{
    // convert long value to string
    ostringstream valueStream;
    valueStream << value;

    setTextParameter(parameterName, valueStream.str().c_str());
}



void
r_Error::setTextParameter(const char* parameterName, const char* value)
{
    if (errorText)
    {
        // locate the next matching parameter in the query string
        char* paramStart = strstr(errorText, parameterName);

        if (paramStart)
        {
            // allocate a new query string and fill it
            char* paramEnd = NULL;
            int  paramLength = 0;

            paramLength = strlen(parameterName);
            paramEnd    = paramStart + paramLength;

            *paramStart = '\0';

            ostringstream queryStream;
            queryStream << errorText << value << paramEnd;
            delete[] errorText;
            errorText = strdup(queryStream.str().c_str());

        }
    }
}



void
r_Error::resetErrorText()
{
    // If no error number is specified use the error kind for the text.
    if (errorNo)
        setErrorTextOnNumber();
    else
        setErrorTextOnKind();
}



r_Eno_interval::r_Eno_interval()
    : r_Error(201)
{
    LDEBUG << "r_Error::resetErrorText() - code 201";
    resetErrorText();
}

r_EGeneral::r_EGeneral(const std::string& errorText2)
{
    this->errorText = strdup(errorText2.c_str());
}


r_Eindex_violation::r_Eindex_violation(r_Range dlow, r_Range dhigh, r_Range dindex)
    : r_Error(202), low(dlow), high(dhigh), index(dindex)
{
    LDEBUG << "r_Error::r_Eindex_violation() - code 202";
    resetErrorText();
}



void
r_Eindex_violation::resetErrorText()
{
    setErrorTextOnNumber();

    setTextParameter("$low",     low    );
    setTextParameter("$high",   high    );
    setTextParameter("$index", index);
}



r_Edim_mismatch::r_Edim_mismatch(r_Dimension pdim1, r_Dimension pdim2)
    : r_Error(203), dim1(pdim1), dim2(pdim2)
{
    LDEBUG << "r_Error::r_Edim_mismatch() - code 203; dim1=" << pdim1 << ", dim2=" << pdim2;
    resetErrorText();
}



void
r_Edim_mismatch::resetErrorText()
{
    setErrorTextOnNumber();

    setTextParameter("$dim1", static_cast<int>(dim1));
    setTextParameter("$dim2", static_cast<int>(dim2));
}



r_Einit_overflow::r_Einit_overflow()
    : r_Error(204)
{
    LDEBUG << "r_Error::r_Einit_overflow() - code 204";
    resetErrorText();
}



r_Eno_cell::r_Eno_cell()
    : r_Error(205)
{
    LDEBUG << "r_Error::r_Eno_cell() - code 205";
    resetErrorText();
}



r_Equery_execution_failed::r_Equery_execution_failed(unsigned int errorno, unsigned int lineno, unsigned int columnno, const char* initToken)
    : r_Error(errorno),
      lineNo(lineno),
      columnNo(columnno)
{
    LDEBUG << "r_Error::r_Equery_execution_failed() - errorno=" << errorno;

    token = new char[strlen(initToken)+1];
    strcpy(token, initToken);

    resetErrorText();
}



r_Equery_execution_failed::r_Equery_execution_failed(const r_Equery_execution_failed &err)
    : r_Error(err),
      lineNo(0),
      columnNo(0)
{
    LDEBUG << "r_Error::r_Equery_execution_failed()";

    lineNo   = err.lineNo;
    columnNo = err.columnNo;

    token = new char[strlen(err.token)+1];
    strcpy(token, err.token);
}



r_Equery_execution_failed::~r_Equery_execution_failed() throw()
{
    if (token)
        delete[] token;
}



void
r_Equery_execution_failed::resetErrorText()
{
    setErrorTextOnNumber();

    setTextParameter("$errorNo",  static_cast<int>(errorNo));
    setTextParameter("$lineNo",   static_cast<int>(lineNo));
    setTextParameter("$columnNo", static_cast<int>(columnNo));
    setTextParameter("$token",    token);
}



r_Elimits_mismatch::r_Elimits_mismatch(r_Range lim1, r_Range lim2)
    : r_Error(r_Error_LimitsMismatch), i1(lim1), i2(lim2)
{
    LDEBUG << "r_Error::r_Elimits_mismatch() - lim1=" << lim1 << ", lim2=" << lim2;
    resetErrorText();
}



void
r_Elimits_mismatch::resetErrorText()
{
    setErrorTextOnNumber();

    setTextParameter("$dim1", i1);
    setTextParameter("$dim2", i2);
}

/* ------------------------------------------------------------------
     class r_Ebase_dbms
     ------------------------------------------------------------------ */

r_Ebase_dbms::r_Ebase_dbms(const long& newDbmsErrNum, const char* newDbmsErrTxt)
    : r_Error(r_Error_SerialisableException, 206),
      dbmsErrNum(newDbmsErrNum),
      whatTxt(0)
{
    LDEBUG << "r_Error::r_Ebase_dbms() code 206 - " << newDbmsErrTxt;
    baseDBMS = strdup("Error in base DBMS, error number: ");
    dbmsErrTxt = strdup(newDbmsErrTxt);
    buildWhat();
}

r_Ebase_dbms::r_Ebase_dbms(const r_Ebase_dbms& obj)
    : r_Error(obj),
      dbmsErrNum(0),
      whatTxt(0)
{
    LDEBUG << "r_Error::r_Ebase_dbms()";

    dbmsErrNum = obj.dbmsErrNum;
    if (obj.baseDBMS)
    {
        baseDBMS = static_cast<char*>(mymalloc(strlen(obj.baseDBMS) + 1));
        strcpy(baseDBMS, obj.baseDBMS);
    }
    else
    {
        baseDBMS = 0;
    }
    if (obj.dbmsErrTxt)
    {
        dbmsErrTxt = static_cast<char*>(mymalloc(strlen(obj.dbmsErrTxt) + 1));
        strcpy(dbmsErrTxt, obj.dbmsErrTxt);
    }
    else
    {
        dbmsErrTxt = 0;
    }
    buildWhat();
}

// r_Ebase_dbms: constructor receiving kind, errno, and descriptive string
// NB: the string must have the format "<kind>\t<errno>\t<text>"
//     (currently the only location where this is used is getAnyError() above)
r_Ebase_dbms::r_Ebase_dbms(kind newTheKind, unsigned long newErrNum, const char* myStr)
    : r_Error(newTheKind, newErrNum), whatTxt(0)
{
    LDEBUG << "r_Error::r_Ebase_dbms() - kind=" << newTheKind;

    // as the const char* cannot be passed to strtol() - this was a bug anyway -
    // we copy the string. Efficiency is not a concern here. -- PB 2005-jan-14
    // const char* currChar = myStr;
    char* tmpBuf = NULL;                // temp copy of myStr
    char* currChar = NULL;              // ptr iterating over tmpBuf
    tmpBuf = strdup( myStr );
    currChar = tmpBuf;              // initialize char ptr to begin of buffer

    // read the attributes of r_Ebase_dbms from the string already partially parsed by
    // r_Error::getAnyError(char* serErr)
    dbmsErrNum = strtol(currChar, &currChar, 10);
    // of course \t is part of the string, so we trick a little
    char* tmpPtr = strchr((static_cast<char*>(currChar))+1, '\t');
    if (tmpPtr==NULL)               // no trailing information found?
    {
        baseDBMS = strdup( "unknown" );
        dbmsErrTxt = strdup( "unknown" );
    }
    else                        // yes -> analyse it
    {
        *tmpPtr = '\0';             // terminate item for strdup()
        baseDBMS = strdup(currChar+1);      // extract item (db name) from string
        *tmpPtr = '\t';             // re-substitute EOS with tab
        currChar = strchr(currChar+1, '\t');    // search for tab as item delimiter
        dbmsErrTxt = strdup(currChar+1);    // extract final item which is error text
    }
    buildWhat();

    free( tmpBuf );                 // allocated in strdup() -- PB 2005-jan-14
}

r_Ebase_dbms::~r_Ebase_dbms() throw()
{
    if (whatTxt)
        free(whatTxt);
    if (dbmsErrTxt)
        free(dbmsErrTxt);
    if (baseDBMS)
        free(baseDBMS);
}

const r_Ebase_dbms&
r_Ebase_dbms::operator=(const r_Ebase_dbms& obj)
{
    if (this != &obj)
    {
        dbmsErrNum = obj.dbmsErrNum;

        if (obj.baseDBMS)
        {
            if (baseDBMS) free(baseDBMS);
            baseDBMS = static_cast<char*>(mymalloc(strlen(obj.baseDBMS) + 1));
            strcpy(baseDBMS, obj.baseDBMS);
        }
        else
        {
            baseDBMS = 0;
        }
        if (obj.dbmsErrTxt)
        {
            if (dbmsErrTxt) free(dbmsErrTxt);
            dbmsErrTxt = static_cast<char*>(mymalloc(strlen(obj.dbmsErrTxt) + 1));
            strcpy(dbmsErrTxt, obj.dbmsErrTxt);
        }
        else
        {
            dbmsErrTxt = 0;
        }
        buildWhat();
    }
    return *this;
}

void
r_Ebase_dbms::buildWhat()
{
    // Ok, we have to build the error message for this class. The problem is that ressource
    // allocation is involved here!
    if(whatTxt)
        free(whatTxt);
    // assumes 10 digits for errNum
    whatTxt = static_cast<char*>(mymalloc(strlen(baseDBMS) + strlen(dbmsErrTxt) + 12));
    strcpy(whatTxt, baseDBMS);
    sprintf(whatTxt + strlen(whatTxt), "%ld\n", dbmsErrNum);
    strcat(whatTxt, dbmsErrTxt);
}

const char*
r_Ebase_dbms::what() const throw()
{
    return whatTxt;
}

char*
r_Ebase_dbms::serialiseError()
{
    char* tmpRes = r_Error::serialiseError();
    char buf[40];
    char* retVal;

    sprintf(buf, "\t%ld\t", dbmsErrNum);
    retVal = static_cast<char*>(mymalloc(strlen(tmpRes) + strlen(buf) + strlen(baseDBMS) + strlen(dbmsErrTxt) + 2));
    strcpy(retVal, tmpRes);
    strcat(retVal, buf);
    strcat(retVal, baseDBMS);
    strcat(retVal, "\t");
    strcat(retVal, dbmsErrTxt);

    free(tmpRes);
    return retVal;
}

r_Eno_permission::r_Eno_permission()
    :r_Error(r_Error_AccesDenied,NO_PERMISSION_FOR_OPERATION)
{
    LDEBUG << "r_Error::r_Eno_permission()";
    resetErrorText();
}


r_Ememory_allocation::r_Ememory_allocation(): r_Error(r_Error_MemoryAllocation, 66) // 66 is: mem alloc failed
{
    LDEBUG << "r_Error::r_Ememory_allocation() - code 66";
    resetErrorText();
}

r_Ecapability_refused::r_Ecapability_refused()
    :r_Error(r_Error_AccesDenied,CAPABILITY_REFUSED)
{
    LDEBUG << "r_Error::r_Ecapability_refused()";
    resetErrorText();
}

