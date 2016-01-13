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

#include "rasnetservercomm.hh"
#include "../mymalloc/mymalloc.h"
#include "../server/rasserver_entry.hh"
#include "../debug/debug-srv.hh"
#include "common/src/grpc/messages/error.pb.h"

using common::ErrorMessage;

RasnetServerComm::RasnetServerComm(::boost::shared_ptr<rasserver::ClientManager> clientManager)
{
    this->clientManager = clientManager;
}

RasnetServerComm::~RasnetServerComm(){}

grpc::Status RasnetServerComm::OpenServerDatabase(grpc::ServerContext *context, const rasnet::service::OpenServerDatabaseReq *request, rasnet::service::OpenServerDatabaseRepl *response)
{
    grpc::Status status = grpc::Status::OK;

    try
    {
        RasServerEntry& rasServerEntry = RasServerEntry::getInstance();
        rasServerEntry.compat_openDB(request->database_name().c_str());
    }
    catch (r_Ebase_dbms &edb)
    {
        LERROR << "Error: base DBMS reports: " << edb.what();
        status = RasnetServerComm::getRErrorStatus(edb);
    }
    catch (r_Error &ex)
    {
        LDEBUG << "request terminated: " << ex.what() << " exception kind=" << ex.get_kind() << ", errorno=" << ex.get_errorno();
        status = RasnetServerComm::getRErrorStatus(ex);
    }
    catch (std::exception &ex)
    {
        LERROR << "Error: request terminated with general exception: " << ex.what();
        status = RasnetServerComm::getSTLExceptionStatus(ex);
    }
    catch (...)
    {
        LERROR << "Error: request terminated with generic exception.";
        status = RasnetServerComm::getUnknownExceptionStatus();
    }

    return status;
}

grpc::Status RasnetServerComm::CloseServerDatabase(grpc::ServerContext *context, const rasnet::service::CloseServerDatabaseReq *request, rasnet::service::Void *response)
{
    grpc::Status status = grpc::Status::OK;

    try
    {
        RasServerEntry& rasServerEntry = RasServerEntry::getInstance();
        rasServerEntry.compat_closeDB();
    }
    catch (r_Ebase_dbms &edb)
    {
        LERROR << "Error: base DBMS reports: " << edb.what();
        status = RasnetServerComm::getRErrorStatus(edb);
    }
    catch (r_Error &ex)
    {
        LDEBUG << "request terminated: " << ex.what() << " exception kind=" << ex.get_kind() << ", errorno=" << ex.get_errorno();
        status = RasnetServerComm::getRErrorStatus(ex);
    }
    catch (std::exception &ex)
    {
        LERROR << "Error: request terminated with general exception: " << ex.what();
        status = RasnetServerComm::getSTLExceptionStatus(ex);
    }
    catch (...)
    {
        LERROR << "Error: request terminated with generic exception.";
        status = RasnetServerComm::getUnknownExceptionStatus();
    }

    return status;
}

grpc::Status RasnetServerComm::CreateDatabase(grpc::ServerContext *context, const rasnet::service::CreateDatabaseReq *request, rasnet::service::CreateDatabaseRepl *response)
{
    return grpc::Status::OK;
}

grpc::Status RasnetServerComm::DestroyDatabase(grpc::ServerContext *context, const rasnet::service::DestroyDatabaseReq *request, rasnet::service::DestroyDatabaseRepl *response)
{
    return grpc::Status::OK;
}

grpc::Status RasnetServerComm::BeginTransaction(grpc::ServerContext *context, const rasnet::service::BeginTransactionReq *request, rasnet::service::BeginTransactionRepl *response)
{
    grpc::Status status = grpc::Status::OK;

    try
    {
        RasServerEntry& rasServerEntry = RasServerEntry::getInstance();
        rasServerEntry.compat_beginTA(request->rw());
    }
    catch (r_Ebase_dbms &edb)
    {
        LERROR << "Error: base DBMS reports: " << edb.what();
        status = RasnetServerComm::getRErrorStatus(edb);
    }
    catch (r_Error &ex)
    {
        LDEBUG << "request terminated: " << ex.what() << " exception kind=" << ex.get_kind() << ", errorno=" << ex.get_errorno();
        status = RasnetServerComm::getRErrorStatus(ex);
    }
    catch (std::exception &ex)
    {
        LERROR << "Error: request terminated with general exception: " << ex.what();
        status = RasnetServerComm::getSTLExceptionStatus(ex);
    }
    catch (...)
    {
        LERROR << "Error: request terminated with generic exception.";
        status = RasnetServerComm::getUnknownExceptionStatus();
    }

    return status;
}

grpc::Status RasnetServerComm::CommitTransaction(grpc::ServerContext *context, const rasnet::service::CommitTransactionReq *request, rasnet::service::CommitTransactionRepl *response)
{
    grpc::Status status = grpc::Status::OK;

    try
    {
        RasServerEntry& rasServerEntry = RasServerEntry::getInstance();
        rasServerEntry.compat_commitTA();
    }
    catch (r_Ebase_dbms &edb)
    {
        LERROR << "Error: base DBMS reports: " << edb.what();
        status = RasnetServerComm::getRErrorStatus(edb);
    }
    catch (r_Error &ex)
    {
        LDEBUG << "request terminated: " << ex.what() << " exception kind=" << ex.get_kind() << ", errorno=" << ex.get_errorno();
        status = RasnetServerComm::getRErrorStatus(ex);
    }
    catch (std::exception &ex)
    {
        LERROR << "Error: request terminated with general exception: " << ex.what();
        status = RasnetServerComm::getSTLExceptionStatus(ex);
    }
    catch (...)
    {
        LERROR << "Error: request terminated with generic exception.";
        status = RasnetServerComm::getUnknownExceptionStatus();
    }

    return status;
}

grpc::Status RasnetServerComm::AbortTransaction(grpc::ServerContext *context, const rasnet::service::AbortTransactionReq *request, rasnet::service::AbortTransactionRepl *response)
{
    grpc::Status status = grpc::Status::OK;

    try
    {
        RasServerEntry& rasServerEntry = RasServerEntry::getInstance();
        rasServerEntry.compat_abortTA();
    }
    catch (r_Ebase_dbms &edb)
    {
        LERROR << "Error: base DBMS reports: " << edb.what();
        status = RasnetServerComm::getRErrorStatus(edb);
    }
    catch (r_Error &ex)
    {
        LDEBUG << "request terminated: " << ex.what() << " exception kind=" << ex.get_kind() << ", errorno=" << ex.get_errorno();
        status = RasnetServerComm::getRErrorStatus(ex);
    }
    catch (std::exception &ex)
    {
        LERROR << "Error: request terminated with general exception: " << ex.what();
        status = RasnetServerComm::getSTLExceptionStatus(ex);
    }
    catch (...)
    {
        LERROR << "Error: request terminated with generic exception.";
        status = RasnetServerComm::getUnknownExceptionStatus();
    }

    return status;
}

grpc::Status RasnetServerComm::IsTransactionOpen(grpc::ServerContext *context, const rasnet::service::IsTransactionOpenReq *request, rasnet::service::IsTransactionOpenRepl *response)
{
    grpc::Status status = grpc::Status::OK;

    try
    {
        RasServerEntry& rasserver = RasServerEntry::getInstance();
        bool isOpen = rasserver.compat_isOpenTA();
        response->set_isopen(isOpen);
    }
    catch (r_Ebase_dbms &edb)
    {
        LERROR << "Error: base DBMS reports: " << edb.what();
        status = RasnetServerComm::getRErrorStatus(edb);
    }
    catch (r_Error &ex)
    {
        LDEBUG << "request terminated: " << ex.what() << " exception kind=" << ex.get_kind() << ", errorno=" << ex.get_errorno();
        status = RasnetServerComm::getRErrorStatus(ex);
    }
    catch (std::exception &ex)
    {
        LERROR << "Error: request terminated with general exception: " << ex.what();
        status = RasnetServerComm::getSTLExceptionStatus(ex);
    }
    catch (...)
    {
        LERROR << "Error: request terminated with generic exception.";
        status = RasnetServerComm::getUnknownExceptionStatus();
    }

    return status;
}

grpc::Status RasnetServerComm::StartInsertMDD(grpc::ServerContext *context, const rasnet::service::StartInsertMDDReq *request, rasnet::service::StartInsertMDDRepl *response)
{
    grpc::Status status = grpc::Status::OK;

    try
    {
        RasServerEntry& rasserver = RasServerEntry::getInstance();

        const char* collName = request->collname().c_str();
        r_Minterval mddDomain(request->domain().c_str());
        int typeLength = request->type_length();
        const char* typeName = request->type_name().c_str();
        r_OId oid(request->oid().c_str());

        int statusCode = rasserver.compat_StartInsertPersMDD(collName, mddDomain, typeLength, typeName, oid);

        response->set_status(statusCode);
    }
    catch (r_Ebase_dbms &edb)
    {
        LERROR << "Error: base DBMS reports: " << edb.what();
        status = RasnetServerComm::getRErrorStatus(edb);
    }
    catch (r_Error &ex)
    {
        LDEBUG << "request terminated: " << ex.what() << " exception kind=" << ex.get_kind() << ", errorno=" << ex.get_errorno();
        status = RasnetServerComm::getRErrorStatus(ex);
    }
    catch (std::exception &ex)
    {
        LERROR << "Error: request terminated with general exception: " << ex.what();
        status = RasnetServerComm::getSTLExceptionStatus(ex);
    }
    catch (...)
    {
        LERROR << "Error: request terminated with generic exception.";
        status = RasnetServerComm::getUnknownExceptionStatus();
    }

    return status;
}

grpc::Status RasnetServerComm::StartInsertTransMDD(grpc::ServerContext *context, const rasnet::service::StartInsertTransMDDReq *request, rasnet::service::StartInsertTransMDDRepl *response)
{
    grpc::Status status = grpc::Status::OK;

    try
    {
        RasServerEntry& rasserver = RasServerEntry::getInstance();

        const char* domain = request->domain().c_str();
        int typeLength = request->type_length();
        const char* typeName = request->type_name().c_str();

        int statusCode = rasserver.compat_StartInsertTransMDD(domain, typeLength, typeName);
        response->set_status(statusCode);
    }
    catch (r_Ebase_dbms &edb)
    {
        LERROR << "Error: base DBMS reports: " << edb.what();
        status = RasnetServerComm::getRErrorStatus(edb);
    }
    catch (r_Error &ex)
    {
        LDEBUG << "request terminated: " << ex.what() << " exception kind=" << ex.get_kind() << ", errorno=" << ex.get_errorno();
        status = RasnetServerComm::getRErrorStatus(ex);
    }
    catch (std::exception &ex)
    {
        LERROR << "Error: request terminated with general exception: " << ex.what();
        status = RasnetServerComm::getSTLExceptionStatus(ex);
    }
    catch (...)
    {
        LERROR << "Error: request terminated with generic exception.";
        status = RasnetServerComm::getUnknownExceptionStatus();
    }

    return status;
}

grpc::Status RasnetServerComm::InsertTile(grpc::ServerContext *context, const rasnet::service::InsertTileReq *request, rasnet::service::InsertTileRepl *response)
{
    grpc::Status status = grpc::Status::OK;

    try
    {
        RasServerEntry& rasserver = RasServerEntry::getInstance();

        RPCMarray *rpcMarray = new RPCMarray();

        int persistent = request->persistent();

        rpcMarray->domain = strdup(request->domain().c_str());
        rpcMarray->cellTypeLength = request->type_length();
        rpcMarray->currentFormat = request->current_format();
        rpcMarray->storageFormat = request->storage_format();
        rpcMarray->data.confarray_len = request->data_length();
        rpcMarray->data.confarray_val = (char*) mymalloc(request->data_length());
        memcpy(rpcMarray->data.confarray_val, request->data().c_str(), request->data_length());

        int statusCode = rasserver.compat_InsertTile(persistent, rpcMarray);

        response->set_status(statusCode);

        delete rpcMarray;
    }
    catch (r_Ebase_dbms &edb)
    {
        LERROR << "Error: base DBMS reports: " << edb.what();
        status = RasnetServerComm::getRErrorStatus(edb);
    }
    catch (r_Error &ex)
    {
        LDEBUG << "request terminated: " << ex.what() << " exception kind=" << ex.get_kind() << ", errorno=" << ex.get_errorno();
        status = RasnetServerComm::getRErrorStatus(ex);
    }
    catch (std::exception &ex)
    {
        LERROR << "Error: request terminated with general exception: " << ex.what();
        status = RasnetServerComm::getSTLExceptionStatus(ex);
    }
    catch (...)
    {
        LERROR << "Error: request terminated with generic exception.";
        status = RasnetServerComm::getUnknownExceptionStatus();
    }

    return status;
}

grpc::Status RasnetServerComm::EndInsertMDD(grpc::ServerContext *context, const rasnet::service::EndInsertMDDReq *request, rasnet::service::EndInsertMDDRepl *response)
{
    grpc::Status status = grpc::Status::OK;

    try
    {
        RasServerEntry& rasserver = RasServerEntry::getInstance();
        int persistent = request->persistent();
        int statusCode = rasserver.compat_EndInsertMDD(persistent);

        response->set_status(statusCode);
    }
    catch (r_Ebase_dbms &edb)
    {
        LERROR << "Error: base DBMS reports: " << edb.what();
        status = RasnetServerComm::getRErrorStatus(edb);
    }
    catch (r_Error &ex)
    {
        LDEBUG << "request terminated: " << ex.what() << " exception kind=" << ex.get_kind() << ", errorno=" << ex.get_errorno();
        status = RasnetServerComm::getRErrorStatus(ex);
    }
    catch (std::exception &ex)
    {
        LERROR << "Error: request terminated with general exception: " << ex.what();
        status = RasnetServerComm::getSTLExceptionStatus(ex);
    }
    catch (...)
    {
        LERROR << "Error: request terminated with generic exception.";
        status = RasnetServerComm::getUnknownExceptionStatus();
    }

    return status;
}

grpc::Status RasnetServerComm::InsertCollection(grpc::ServerContext *context, const rasnet::service::InsertCollectionReq *request, rasnet::service::InsertCollectionRepl *response)
{
    grpc::Status status = grpc::Status::OK;

    try
    {
        RasServerEntry& rasserver = RasServerEntry::getInstance();

        const char* collName = request->collection_name().c_str();
        const char* typeName = request->type_name().c_str();
        r_OId oid(request->oid().c_str());

        int statusCode = rasserver.compat_InsertCollection(collName, typeName, oid);
        response->set_status(statusCode);
    }
    catch (r_Ebase_dbms &edb)
    {
        LERROR << "Error: base DBMS reports: " << edb.what();
        status = RasnetServerComm::getRErrorStatus(edb);
    }
    catch (r_Error &ex)
    {
        LDEBUG << "request terminated: " << ex.what() << " exception kind=" << ex.get_kind() << ", errorno=" << ex.get_errorno();
        status = RasnetServerComm::getRErrorStatus(ex);
    }
    catch (std::exception &ex)
    {
        LERROR << "Error: request terminated with general exception: " << ex.what();
        status = RasnetServerComm::getSTLExceptionStatus(ex);
    }
    catch (...)
    {
        LERROR << "Error: request terminated with generic exception.";
        status = RasnetServerComm::getUnknownExceptionStatus();
    }

    return status;
}

grpc::Status RasnetServerComm::DeleteCollectionByName(grpc::ServerContext *context, const rasnet::service::DeleteCollectionByNameReq *request, rasnet::service::DeleteCollectionByNameRepl *response)
{
    grpc::Status status = grpc::Status::OK;

    try
    {
        RasServerEntry& rasserver = RasServerEntry::getInstance();

        const char* collName = request->collection_name().c_str();
        int statusCode = rasserver.compat_DeleteCollByName(collName);

        response->set_status(statusCode);
    }
    catch (r_Ebase_dbms &edb)
    {
        LERROR << "Error: base DBMS reports: " << edb.what();
        status = RasnetServerComm::getRErrorStatus(edb);
    }
    catch (r_Error &ex)
    {
        LDEBUG << "request terminated: " << ex.what() << " exception kind=" << ex.get_kind() << ", errorno=" << ex.get_errorno();
        status = RasnetServerComm::getRErrorStatus(ex);
    }
    catch (std::exception &ex)
    {
        LERROR << "Error: request terminated with general exception: " << ex.what();
        status = RasnetServerComm::getSTLExceptionStatus(ex);
    }
    catch (...)
    {
        LERROR << "Error: request terminated with generic exception.";
        status = RasnetServerComm::getUnknownExceptionStatus();
    }

    return status;
}

grpc::Status RasnetServerComm::DeleteCollectionByOid(grpc::ServerContext *context, const rasnet::service::DeleteCollectionByOidReq *request, rasnet::service::DeleteCollectionByOidRepl *response)
{
    grpc::Status status = grpc::Status::OK;

    try
    {
        RasServerEntry& rasserver = RasServerEntry::getInstance();

        r_OId oid(request->oid().c_str());

        int statusCode = rasserver.compat_DeleteObjByOId(oid);
        response->set_status(statusCode);
    }
    catch (r_Ebase_dbms &edb)
    {
        LERROR << "Error: base DBMS reports: " << edb.what();
        status = RasnetServerComm::getRErrorStatus(edb);
    }
    catch (r_Error &ex)
    {
        LDEBUG << "request terminated: " << ex.what() << " exception kind=" << ex.get_kind() << ", errorno=" << ex.get_errorno();
        status = RasnetServerComm::getRErrorStatus(ex);
    }
    catch (std::exception &ex)
    {
        LERROR << "Error: request terminated with general exception: " << ex.what();
        status = RasnetServerComm::getSTLExceptionStatus(ex);
    }
    catch (...)
    {
        LERROR << "Error: request terminated with generic exception.";
        status = RasnetServerComm::getUnknownExceptionStatus();
    }

    return status;
}

grpc::Status RasnetServerComm::RemoveObjectFromCollection(grpc::ServerContext *context, const rasnet::service::RemoveObjectFromCollectionReq *request, rasnet::service::RemoveObjectFromCollectionRepl *response)
{
    grpc::Status status = grpc::Status::OK;

    try
    {
        RasServerEntry& rasserver = RasServerEntry::getInstance();

        const char* collName = request->collection_name().c_str();
        r_OId oid(request->oid().c_str());

        int statusCode = rasserver.compat_RemoveObjFromColl(collName, oid);
        response->set_status(statusCode);
    }
    catch (r_Ebase_dbms &edb)
    {
        LERROR << "Error: base DBMS reports: " << edb.what();
        status = RasnetServerComm::getRErrorStatus(edb);
    }
    catch (r_Error &ex)
    {
        LDEBUG << "request terminated: " << ex.what() << " exception kind=" << ex.get_kind() << ", errorno=" << ex.get_errorno();
        status = RasnetServerComm::getRErrorStatus(ex);
    }
    catch (std::exception &ex)
    {
        LERROR << "Error: request terminated with general exception: " << ex.what();
        status = RasnetServerComm::getSTLExceptionStatus(ex);
    }
    catch (...)
    {
        LERROR << "Error: request terminated with generic exception.";
        status = RasnetServerComm::getUnknownExceptionStatus();
    }

    return status;
}

grpc::Status RasnetServerComm::GetCollectionByNameOrOid(grpc::ServerContext *context, const rasnet::service::GetCollectionByNameOrOidReq *request, rasnet::service::GetCollectionByNameOrOidRepl *response)
{
    grpc::Status status = grpc::Status::OK;

    try
    {
        RasServerEntry& rasserver = RasServerEntry::getInstance();

        char* typeName      = NULL;
        char* typeStructure = NULL;
        char* collName      = NULL;
        r_OId oid;
        int statusCode = 0;

        if (request->is_name())
        {
            collName = strdup(request->collection_identifier().c_str());
            statusCode = rasserver.compat_GetCollectionByName(collName, typeName, typeStructure, oid);
        }
        else
        {
            oid = r_OId(request->collection_identifier().c_str());
            statusCode = rasserver.compat_GetCollectionByName(oid, typeName, typeStructure, collName);
        }

        response->set_status(statusCode);
        response->set_type_name(typeName);
        response->set_type_structure(typeStructure);
        response->set_oid(oid.get_string_representation());
        response->set_collection_name(collName);

        free(typeName);
        free(typeStructure);
        free(collName);
    }
    catch (r_Ebase_dbms &edb)
    {
        LERROR << "Error: base DBMS reports: " << edb.what();
        status = RasnetServerComm::getRErrorStatus(edb);
    }
    catch (r_Error &ex)
    {
        LDEBUG << "request terminated: " << ex.what() << " exception kind=" << ex.get_kind() << ", errorno=" << ex.get_errorno();
        status = RasnetServerComm::getRErrorStatus(ex);
    }
    catch (std::exception &ex)
    {
        LERROR << "Error: request terminated with general exception: " << ex.what();
        status = RasnetServerComm::getSTLExceptionStatus(ex);
    }
    catch (...)
    {
        LERROR << "Error: request terminated with generic exception.";
        status = RasnetServerComm::getUnknownExceptionStatus();
    }

    return status;
}

grpc::Status RasnetServerComm::GetCollOidsByNameOrOid(grpc::ServerContext *context, const rasnet::service::GetCollOidsByNameOrOidReq *request, rasnet::service::GetCollOidsByNameOrOidRepl *response)
{
    grpc::Status status = grpc::Status::OK;

    try
    {
        RasServerEntry& rasserver = RasServerEntry::getInstance();

        char* typeName      = NULL;
        char* typeStructure = NULL;
        char* collName      = NULL;
        r_OId oid;
        RPCOIdEntry* oidTable     = NULL;
        unsigned int oidTableSize = 0;
        int statusCode = 0;

        if (request->is_name())
        {
            collName = strdup(request->collection_identifier().c_str());
            statusCode = rasserver.compat_GetCollectionOidsByName(collName, typeName, typeStructure, oid, oidTable, oidTableSize);
        }
        else
        {
            oid = r_OId(request->collection_identifier().c_str());
            statusCode = rasserver.compat_GetCollectionOidsByOId(oid, typeName, typeStructure, oidTable, oidTableSize, collName);
        }

        response->set_status(statusCode);
        response->set_type_name(typeName != NULL ? typeName : "");
        response->set_type_structure(typeStructure != NULL ? typeStructure : "");
        response->set_collection_name(collName != NULL ? collName : "");

        if (oid.is_valid())
        {
            response->set_oids_string(oid.get_string_representation());
        }

        if (oidTable != NULL)
        {
            for (int i = 0; i < oidTableSize; ++i)
            {
                response->add_oid_set(oidTable[i].oid);
                free(oidTable[i].oid);
            }
        }

        free(typeName);
        free(typeStructure);
        free(collName);
        free(oidTable);
    }
    catch (r_Ebase_dbms &edb)
    {
        LERROR << "Error: base DBMS reports: " << edb.what();
        status = RasnetServerComm::getRErrorStatus(edb);
    }
    catch (r_Error &ex)
    {
        LDEBUG << "request terminated: " << ex.what() << " exception kind=" << ex.get_kind() << ", errorno=" << ex.get_errorno();
        status = RasnetServerComm::getRErrorStatus(ex);
    }
    catch (std::exception &ex)
    {
        LERROR << "Error: request terminated with general exception: " << ex.what();
        status = RasnetServerComm::getSTLExceptionStatus(ex);
    }
    catch (...)
    {
        LERROR << "Error: request terminated with generic exception.";
        status = RasnetServerComm::getUnknownExceptionStatus();
    }

    return status;
}

grpc::Status RasnetServerComm::GetNextMDD(grpc::ServerContext *context, const rasnet::service::GetNextMDDReq *request, rasnet::service::GetNextMDDRepl *response)
{
    grpc::Status status = grpc::Status::OK;

    try
    {
        RasServerEntry& rasserver = RasServerEntry::getInstance();
        r_Minterval  mddDomain;
        char*        typeName;
        char*        typeStructure;
        r_OId        oid;
        unsigned short  currentFormat;

        int statusCode = rasserver.compat_getNextMDD(mddDomain, typeName, typeStructure, oid, currentFormat);

        response->set_status(statusCode);
        response->set_domain(mddDomain.get_string_representation());
        response->set_type_name(typeName);
        response->set_type_structure(typeStructure);
        response->set_oid(oid.get_string_representation() ? oid.get_string_representation() : "");
        response->set_current_format(currentFormat);

        free(typeName);
        free(typeStructure);

    }
    catch (r_Ebase_dbms &edb)
    {
        LERROR << "Error: base DBMS reports: " << edb.what();
        status = RasnetServerComm::getRErrorStatus(edb);
    }
    catch (r_Error &ex)
    {
        LDEBUG << "request terminated: " << ex.what() << " exception kind=" << ex.get_kind() << ", errorno=" << ex.get_errorno();
        status = RasnetServerComm::getRErrorStatus(ex);
    }
    catch (std::exception &ex)
    {
        LERROR << "Error: request terminated with general exception: " << ex.what();
        status = RasnetServerComm::getSTLExceptionStatus(ex);
    }
    catch (...)
    {
        LERROR << "Error: request terminated with generic exception.";
        status = RasnetServerComm::getUnknownExceptionStatus();
    }

    return status;
}

grpc::Status RasnetServerComm::GetNextTile(grpc::ServerContext *context, const rasnet::service::GetNextTileReq *request, rasnet::service::GetNextTileRepl *response)
{
    grpc::Status status = grpc::Status::OK;

    try
    {
        RasServerEntry& rasserver = RasServerEntry::getInstance();

        RPCMarray *tempRpcMarray;

        int statusCode = rasserver.compat_getNextTile(&tempRpcMarray);

        response->set_status(statusCode);

        if (tempRpcMarray != NULL)
        {
            response->set_domain(tempRpcMarray->domain);
            response->set_cell_type_length(tempRpcMarray->cellTypeLength);
            response->set_current_format(tempRpcMarray->currentFormat);
            response->set_storage_format(tempRpcMarray->storageFormat);
            response->set_data_length(tempRpcMarray->data.confarray_len);
            response->set_data(tempRpcMarray->data.confarray_val, tempRpcMarray->data.confarray_len);

            free(tempRpcMarray->domain);
            free(tempRpcMarray);
        }
    }
    catch (r_Ebase_dbms &edb)
    {
        LERROR << "Error: base DBMS reports: " << edb.what();
        status = RasnetServerComm::getRErrorStatus(edb);
    }
    catch (r_Error &ex)
    {
        LDEBUG << "request terminated: " << ex.what() << " exception kind=" << ex.get_kind() << ", errorno=" << ex.get_errorno();
        status = RasnetServerComm::getRErrorStatus(ex);
    }
    catch (std::exception &ex)
    {
        LERROR << "Error: request terminated with general exception: " << ex.what();
        status = RasnetServerComm::getSTLExceptionStatus(ex);
    }
    catch (...)
    {
        LERROR << "Error: request terminated with generic exception.";
        status = RasnetServerComm::getUnknownExceptionStatus();
    }

    return status;
}

grpc::Status RasnetServerComm::EndTransfer(grpc::ServerContext *context, const rasnet::service::EndTransferReq *request, rasnet::service::EndTransferRepl *response)
{
    grpc::Status status = grpc::Status::OK;

    try
    {
        RasServerEntry& rasServerEntry = RasServerEntry::getInstance();
        int statusCode = rasServerEntry.compat_endTransfer();

        response->set_status(statusCode);
    }
    catch (r_Ebase_dbms &edb)
    {
        LERROR << "Error: base DBMS reports: " << edb.what();
        status = RasnetServerComm::getRErrorStatus(edb);
    }
    catch (r_Error &ex)
    {
        LDEBUG << "request terminated: " << ex.what() << " exception kind=" << ex.get_kind() << ", errorno=" << ex.get_errorno();
        status = RasnetServerComm::getRErrorStatus(ex);
    }
    catch (std::exception &ex)
    {
        LERROR << "Error: request terminated with general exception: " << ex.what();
        status = RasnetServerComm::getSTLExceptionStatus(ex);
    }
    catch (...)
    {
        LERROR << "Error: request terminated with generic exception.";
        status = RasnetServerComm::getUnknownExceptionStatus();
    }

    return status;
}

grpc::Status RasnetServerComm::InitUpdate(grpc::ServerContext *context, const rasnet::service::InitUpdateReq *request, rasnet::service::InitUpdateRepl *response)
{
    grpc::Status status = grpc::Status::OK;

    try
    {
        RasServerEntry& rasserver = RasServerEntry::getInstance();

        int statusCode = rasserver.compat_InitUpdate();
        response->set_status(statusCode);
    }
    catch (r_Ebase_dbms &edb)
    {
        LERROR << "Error: base DBMS reports: " << edb.what();
        status = RasnetServerComm::getRErrorStatus(edb);
    }
    catch (r_Error &ex)
    {
        LDEBUG << "request terminated: " << ex.what() << " exception kind=" << ex.get_kind() << ", errorno=" << ex.get_errorno();
        status = RasnetServerComm::getRErrorStatus(ex);
    }
    catch (std::exception &ex)
    {
        LERROR << "Error: request terminated with general exception: " << ex.what();
        status = RasnetServerComm::getSTLExceptionStatus(ex);
    }
    catch (...)
    {
        LERROR << "Error: request terminated with generic exception.";
        status = RasnetServerComm::getUnknownExceptionStatus();
    }

    return status;
}


#define INITPTR(a) a = 0
#define SECUREPTR(a) if(a == 0) a = strdup("")
#define FREEPTR(a) free(a)

grpc::Status RasnetServerComm::ExecuteQuery(grpc::ServerContext *context, const rasnet::service::ExecuteQueryReq *request, rasnet::service::ExecuteQueryRepl *response)
{
    grpc::Status status = grpc::Status::OK;

    try
    {
        const char* query = request->query().c_str();
        RasServerEntry& rasserver = RasServerEntry::getInstance();

        ExecuteQueryRes queryResult;
        INITPTR(queryResult.token);
        INITPTR(queryResult.typeName);
        INITPTR(queryResult.typeStructure);

        int statusCode = rasserver.compat_executeQueryRpc(query, queryResult);

        SECUREPTR(queryResult.token);
        SECUREPTR(queryResult.typeName);
        SECUREPTR(queryResult.typeStructure);

        response->set_status(statusCode);
        response->set_err_no(queryResult.errorNo);
        response->set_line_no(queryResult.lineNo);
        response->set_col_no(queryResult.columnNo);
        response->set_token(queryResult.token);
        response->set_type_name(queryResult.typeName);
        response->set_type_structure(queryResult.typeStructure);

        FREEPTR(queryResult.token);
        FREEPTR(queryResult.typeName);
        FREEPTR(queryResult.typeStructure);
    }
    catch (r_Ebase_dbms &edb)
    {
        LERROR << "Error: base DBMS reports: " << edb.what();
        status = RasnetServerComm::getRErrorStatus(edb);
    }
    catch (r_Error &ex)
    {
        LDEBUG << "request terminated: " << ex.what() << " exception kind=" << ex.get_kind() << ", errorno=" << ex.get_errorno();
        status = RasnetServerComm::getRErrorStatus(ex);
    }
    catch (std::exception &ex)
    {
        LERROR << "Error: request terminated with general exception: " << ex.what();
        status = RasnetServerComm::getSTLExceptionStatus(ex);
    }
    catch (...)
    {
        LERROR << "Error: request terminated with generic exception.";
        status = RasnetServerComm::getUnknownExceptionStatus();
    }

    return status;
}

grpc::Status RasnetServerComm::ExecuteHttpQuery(grpc::ServerContext *context, const rasnet::service::ExecuteHttpQueryReq *request, rasnet::service::ExecuteHttpQueryRepl *response)
{
    grpc::Status status = grpc::Status::OK;

    try
    {
        RasServerEntry& rasserver = RasServerEntry::getInstance();
        char *resultBuffer = 0;
        int resultLen = rasserver.compat_executeQueryHttp(request->data().c_str(), request->data_length(), resultBuffer);

        response->set_data(resultBuffer, resultLen);
        delete[] resultBuffer;
    }
    catch (r_Ebase_dbms &edb)
    {
        LERROR << "Error: base DBMS reports: " << edb.what();
        status = RasnetServerComm::getRErrorStatus(edb);
    }
    catch (r_Error &ex)
    {
        LDEBUG << "request terminated: " << ex.what() << " exception kind=" << ex.get_kind() << ", errorno=" << ex.get_errorno();
        status = RasnetServerComm::getRErrorStatus(ex);
    }
    catch (std::exception &ex)
    {
        LERROR << "Error: request terminated with general exception: " << ex.what();
        status = RasnetServerComm::getSTLExceptionStatus(ex);
    }
    catch (...)
    {
        LERROR << "Error: request terminated with generic exception.";
        status = RasnetServerComm::getUnknownExceptionStatus();
    }

    return status;
}

grpc::Status RasnetServerComm::GetNextElement(grpc::ServerContext *context, const rasnet::service::GetNextElementReq *request, rasnet::service::GetNextElementRepl *response)
{
    grpc::Status status = grpc::Status::OK;

    try
    {
        RasServerEntry& rasServerEntry = RasServerEntry::getInstance();

        char* buffer = NULL;
        unsigned int bufferSize;

        int statusCode = rasServerEntry.compat_getNextElement(buffer, bufferSize);
        response->set_data(buffer, bufferSize);
        response->set_data_length(bufferSize);
        response->set_status(statusCode);
        free(buffer);
    }
    catch (r_Ebase_dbms &edb)
    {
        LERROR << "Error: base DBMS reports: " << edb.what();
        status = RasnetServerComm::getRErrorStatus(edb);
    }
    catch (r_Error &ex)
    {
        LDEBUG << "request terminated: " << ex.what() << " exception kind=" << ex.get_kind() << ", errorno=" << ex.get_errorno();
        status = RasnetServerComm::getRErrorStatus(ex);
    }
    catch (std::exception &ex)
    {
        LERROR << "Error: request terminated with general exception: " << ex.what();
        status = RasnetServerComm::getSTLExceptionStatus(ex);
    }
    catch (...)
    {
        LERROR << "Error: request terminated with generic exception.";
        status = RasnetServerComm::getUnknownExceptionStatus();
    }

    return status;
}

grpc::Status RasnetServerComm::ExecuteUpdateQuery(grpc::ServerContext *context, const rasnet::service::ExecuteUpdateQueryReq *request, rasnet::service::ExecuteUpdateQueryRepl *response)
{
    grpc::Status status = grpc::Status::OK;

    try
    {
        RasServerEntry& rasserver = RasServerEntry::getInstance();
        const char* query = request->query().c_str();

        ExecuteUpdateRes returnStructure;
        returnStructure.token = NULL;

        int statusCode = rasserver.compat_ExecuteUpdateQuery(query, returnStructure);

        const char *token = returnStructure.token != NULL ? returnStructure.token : "";

        response->set_status(statusCode);
        response->set_errono(returnStructure.errorNo);
        response->set_lineno(returnStructure.lineNo);
        response->set_colno(returnStructure.columnNo);
        response->set_token(token);

        free(returnStructure.token);
    }
    catch (r_Ebase_dbms &edb)
    {
        LERROR << "Error: base DBMS reports: " << edb.what();
        status = RasnetServerComm::getRErrorStatus(edb);
    }
    catch (r_Error &ex)
    {
        LDEBUG << "request terminated: " << ex.what() << " exception kind=" << ex.get_kind() << ", errorno=" << ex.get_errorno();
        status = RasnetServerComm::getRErrorStatus(ex);
    }
    catch (std::exception &ex)
    {
        LERROR << "Error: request terminated with general exception: " << ex.what();
        status = RasnetServerComm::getSTLExceptionStatus(ex);
    }
    catch (...)
    {
        LERROR << "Error: request terminated with generic exception.";
        status = RasnetServerComm::getUnknownExceptionStatus();
    }

    return status;
}

grpc::Status RasnetServerComm::ExecuteInsertQuery(grpc::ServerContext *context, const rasnet::service::ExecuteInsertQueryReq *request, rasnet::service::ExecuteInsertQueryRepl *response)
{
    grpc::Status status = grpc::Status::OK;

    try
    {
        RasServerEntry& rasserver = RasServerEntry::getInstance();
        const char* query = request->query().c_str();

        ExecuteQueryRes queryResult;
        INITPTR(queryResult.token);
        INITPTR(queryResult.typeName);
        INITPTR(queryResult.typeStructure);

        int statusCode = rasserver.compat_ExecuteInsertQuery(query, queryResult);
        SECUREPTR(queryResult.token);
        SECUREPTR(queryResult.typeName);
        SECUREPTR(queryResult.typeStructure);

        response->set_status(statusCode);
        response->set_errono(queryResult.errorNo);
        response->set_lineno(queryResult.lineNo);
        response->set_colno(queryResult.columnNo);
        response->set_token(queryResult.token);
        response->set_type_name(queryResult.typeName);
        response->set_type_structure(queryResult.typeStructure);

        FREEPTR(queryResult.token);
        FREEPTR(queryResult.typeName);
        FREEPTR(queryResult.typeStructure);
    }
    catch (r_Ebase_dbms &edb)
    {
        LERROR << "Error: base DBMS reports: " << edb.what();
        status = RasnetServerComm::getRErrorStatus(edb);
    }
    catch (r_Error &ex)
    {
        LDEBUG << "request terminated: " << ex.what() << " exception kind=" << ex.get_kind() << ", errorno=" << ex.get_errorno();
        status = RasnetServerComm::getRErrorStatus(ex);
    }
    catch (std::exception &ex)
    {
        LERROR << "Error: request terminated with general exception: " << ex.what();
        status = RasnetServerComm::getSTLExceptionStatus(ex);
    }
    catch (...)
    {
        LERROR << "Error: request terminated with generic exception.";
        status = RasnetServerComm::getUnknownExceptionStatus();
    }

    return status;
}

grpc::Status RasnetServerComm::GetNewOid(grpc::ServerContext *context, const rasnet::service::GetNewOidReq *request, rasnet::service::GetNewOidRepl *response)
{
    grpc::Status status = grpc::Status::OK;

    try
    {
        RasServerEntry& rasServerEntry = RasServerEntry::getInstance();
        int objectType = request->object_type();
        r_OId oid = rasServerEntry.compat_getNewOId((unsigned short)objectType);

        response->set_oid(oid.get_string_representation());
    }
    catch (r_Ebase_dbms &edb)
    {
        LERROR << "Error: base DBMS reports: " << edb.what();
        status = RasnetServerComm::getRErrorStatus(edb);
    }
    catch (r_Error &ex)
    {
        LDEBUG << "request terminated: " << ex.what() << " exception kind=" << ex.get_kind() << ", errorno=" << ex.get_errorno();
        status = RasnetServerComm::getRErrorStatus(ex);
    }
    catch (std::exception &ex)
    {
        LERROR << "Error: request terminated with general exception: " << ex.what();
        status = RasnetServerComm::getSTLExceptionStatus(ex);
    }
    catch (...)
    {
        LERROR << "Error: request terminated with generic exception.";
        status = RasnetServerComm::getUnknownExceptionStatus();
    }

    return status;
}

grpc::Status RasnetServerComm::GetObjectType(grpc::ServerContext *context, const rasnet::service::GetObjectTypeReq *request, rasnet::service::GetObjectTypeRepl *response)
{
    grpc::Status status = grpc::Status::OK;

    try
    {
        RasServerEntry& rasserver = RasServerEntry::getInstance();

        r_OId oid(request->oid().c_str());
        unsigned short objectType;

        int statusCode = rasserver.compat_GetObjectType(oid, objectType);

        response->set_status(statusCode);
        response->set_object_type(objectType);
    }
    catch (r_Ebase_dbms &edb)
    {
        LERROR << "Error: base DBMS reports: " << edb.what();
        status = RasnetServerComm::getRErrorStatus(edb);
    }
    catch (r_Error &ex)
    {
        LDEBUG << "request terminated: " << ex.what() << " exception kind=" << ex.get_kind() << ", errorno=" << ex.get_errorno();
        status = RasnetServerComm::getRErrorStatus(ex);
    }
    catch (std::exception &ex)
    {
        LERROR << "Error: request terminated with general exception: " << ex.what();
        status = RasnetServerComm::getSTLExceptionStatus(ex);
    }
    catch (...)
    {
        LERROR << "Error: request terminated with generic exception.";
        status = RasnetServerComm::getUnknownExceptionStatus();
    }

    return status;
}

grpc::Status RasnetServerComm::GetTypeStructure(grpc::ServerContext *context, const rasnet::service::GetTypeStructureReq *request, rasnet::service::GetTypeStructureRepl *response)
{
    grpc::Status status = grpc::Status::OK;

    try
    {
        RasServerEntry& rasserver = RasServerEntry::getInstance();

        const char* typeName = request->type_name().c_str();
        int typeType = request->type_type();
        char* typeStructure = NULL;

        int statusCode = rasserver.compat_GetTypeStructure(typeName, typeType, typeStructure);

        response->set_status(statusCode);
        response->set_type_structure(typeStructure);

        free(typeStructure);
    }
    catch (r_Ebase_dbms &edb)
    {
        LERROR << "Error: base DBMS reports: " << edb.what();
        status = RasnetServerComm::getRErrorStatus(edb);
    }
    catch (r_Error &ex)
    {
        LDEBUG << "request terminated: " << ex.what() << " exception kind=" << ex.get_kind() << ", errorno=" << ex.get_errorno();
        status = RasnetServerComm::getRErrorStatus(ex);
    }
    catch (std::exception &ex)
    {
        LERROR << "Error: request terminated with general exception: " << ex.what();
        status = RasnetServerComm::getSTLExceptionStatus(ex);
    }
    catch (...)
    {
        LERROR << "Error: request terminated with generic exception.";
        status = RasnetServerComm::getUnknownExceptionStatus();
    }

    return status;
}

grpc::Status RasnetServerComm::SetFormat(grpc::ServerContext *context, const rasnet::service::SetFormatReq *request, rasnet::service::SetFormatRepl *response)
{
    grpc::Status status = grpc::Status::OK;

    try
    {
        RasServerEntry& rasserver = RasServerEntry::getInstance();

        int whichFormat = request->transfer_format();
        int format = request->format();
        const char* params = request->format_params().c_str();

        int statusCode = 0;

        if(whichFormat == 1)
        {
            statusCode = rasserver.compat_SetTransferFormat(format, params);
        }
        else
        {
            statusCode = rasserver.compat_SetStorageFormat(format, params);
        }

        response->set_status(statusCode);
    }
    catch (r_Ebase_dbms &edb)
    {
        LERROR << "Error: base DBMS reports: " << edb.what();
        status = RasnetServerComm::getRErrorStatus(edb);
    }
    catch (r_Error &ex)
    {
        LDEBUG << "request terminated: " << ex.what() << " exception kind=" << ex.get_kind() << ", errorno=" << ex.get_errorno();
        status = RasnetServerComm::getRErrorStatus(ex);
    }
    catch (std::exception &ex)
    {
        LERROR << "Error: request terminated with general exception: " << ex.what();
        status = RasnetServerComm::getSTLExceptionStatus(ex);
    }
    catch (...)
    {
        LERROR << "Error: request terminated with generic exception.";
        status = RasnetServerComm::getUnknownExceptionStatus();
    }

    return status;
}

grpc::Status RasnetServerComm::KeepAlive(grpc::ServerContext *context, const rasnet::service::KeepAliveRequest *request, rasnet::service::Void *response)
{
    grpc::Status status = grpc::Status::OK;

    try
    {
        this->clientManager->resetLiveliness(request->client_uuid());
    }
    catch (std::exception &ex)
    {
        LERROR << "Error: request terminated with general exception: " << ex.what();
        status = RasnetServerComm::getSTLExceptionStatus(ex);
    }
    catch (...)
    {
        LERROR << "Error: request terminated with generic exception.";
        status = RasnetServerComm::getUnknownExceptionStatus();
    }

    return status;
}

grpc::Status RasnetServerComm::getRErrorStatus(r_Error &err)
{
    ErrorMessage message;
    message.set_error_no(err.get_errorno());
    message.set_kind(err.get_kind());
    message.set_error_text(err.what());
    message.set_type(ErrorMessage::RERROR);

    grpc::Status status(grpc::StatusCode::UNKNOWN, message.SerializeAsString());

    return status;
}

grpc::Status RasnetServerComm::getSTLExceptionStatus(std::exception &ex)
{
    ErrorMessage message;
    message.set_error_text(ex.what());
    message.set_type(ErrorMessage::STL);

    grpc::Status status(grpc::StatusCode::UNKNOWN, message.SerializeAsString());

    return status;
}

grpc::Status RasnetServerComm::getUnknownExceptionStatus()
{
    ErrorMessage message;
    message.set_type(ErrorMessage::UNKNOWN);

    grpc::Status status(grpc::StatusCode::UNKNOWN, message.SerializeAsString());

    return status;

}
