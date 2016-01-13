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

#include <cstring>
#include <fstream>

#include <grpc++/grpc++.h>
#include <grpc++/security/credentials.h>

#include "../rasodmg/transaction.hh"
#include "../rasodmg/database.hh"
#include "../rasodmg/iterator.hh"
#include "../rasodmg/set.hh"
#include "../rasodmg/ref.hh"
#include "../rasodmg/storagelayout.hh"
#include "../rasodmg/tiling.hh"

#include "../raslib/minterval.hh"
#include "../raslib/rminit.hh"
#include "../raslib/primitivetype.hh"
#include "../raslib/complextype.hh"
#include "../raslib/structuretype.hh"
#include "../raslib/primitive.hh"
#include "../raslib/complex.hh"
#include "../raslib/structure.hh"
#include "../raslib/parseparams.hh"
#include "../mymalloc/mymalloc.h"

#include "../debug/debug.hh"

#include "../common/src/crypto/crypto.hh"
#include "../common/src/uuid/uuid.hh"
#include "../common/src/grpc/grpcutils.hh"
#include "../common/src/logging/easylogging++.hh"

#include "../common/src/grpc/messages/error_message.pb.h"

#include "../include/globals.hh"
#include "rasnetclientcomm.hh"


using boost::scoped_ptr;
using boost::shared_ptr;
using boost::shared_mutex;
using boost::unique_lock;
using boost::thread;

using common::UUID;
using common::GrpcUtils;

using grpc::Channel;
using grpc::ClientContext;
using grpc::Status;

using rasnet::service::OpenServerDatabaseReq;
using rasnet::service::OpenServerDatabaseRepl;
using rasnet::service::CloseServerDatabaseReq;
using rasnet::service::AbortTransactionRepl;
using rasnet::service::ClientIdentity;
using rasnet::service::AbortTransactionReq;
using rasnet::service::BeginTransactionRepl;
using rasnet::service::BeginTransactionReq;
using rasnet::service::CloseDbReq;
using rasnet::service::CloseDbReq;
using rasnet::service::ConnectReq;
using rasnet::service::ConnectRepl;
using rasnet::service::OpenDbReq;
using rasnet::service::OpenDbRepl;
using rasnet::service::DisconnectReq;
using rasnet::service::Void;
using rasnet::service::CommitTransactionRepl;
using rasnet::service::CommitTransactionReq;
using rasnet::service::DeleteCollectionByNameRepl;
using rasnet::service::DeleteCollectionByNameReq;
using rasnet::service::DeleteCollectionByOidRepl;
using rasnet::service::DeleteCollectionByOidReq;
using rasnet::service::EndInsertMDDRepl;
using rasnet::service::EndInsertMDDReq;
using rasnet::service::EndTransferRepl;
using rasnet::service::EndTransferReq;
using rasnet::service::ExecuteQueryRepl;
using rasnet::service::ExecuteQueryReq;
using rasnet::service::ExecuteUpdateQueryRepl;
using rasnet::service::ExecuteUpdateQueryReq;
using rasnet::service::ExecuteInsertQueryReq;
using rasnet::service::ExecuteInsertQueryRepl;
using rasnet::service::GetCollOidsByNameOrOidRepl;
using rasnet::service::GetCollOidsByNameOrOidReq;
using rasnet::service::GetCollectionByNameOrOidRepl;
using rasnet::service::GetCollectionByNameOrOidReq;
using rasnet::service::GetNewOidRepl;
using rasnet::service::GetNewOidReq;
using rasnet::service::GetNextElementRepl;
using rasnet::service::GetNextElementReq;
using rasnet::service::GetNextMDDRepl;
using rasnet::service::GetNextMDDReq;
using rasnet::service::GetNextTileRepl;
using rasnet::service::GetNextTileReq;
using rasnet::service::GetObjectTypeRepl;
using rasnet::service::GetObjectTypeReq;
using rasnet::service::GetTypeStructureRepl;
using rasnet::service::GetTypeStructureReq;
using rasnet::service::InitUpdateRepl;
using rasnet::service::InitUpdateReq;
using rasnet::service::InsertCollectionRepl;
using rasnet::service::InsertCollectionReq;
using rasnet::service::InsertTileRepl;
using rasnet::service::InsertTileReq;
using rasnet::service::RemoveObjectFromCollectionRepl;
using rasnet::service::RemoveObjectFromCollectionReq;
using rasnet::service::SetFormatRepl;
using rasnet::service::SetFormatReq;
using rasnet::service::StartInsertMDDRepl;
using rasnet::service::StartInsertMDDReq;
using rasnet::service::StartInsertTransMDDRepl;
using rasnet::service::StartInsertTransMDDReq;
using rasnet::service::ClientIdentity;
using rasnet::service::KeepAliveReq;
using rasnet::service::KeepAliveRequest;
using common::ErrorMessage;

using std::string;

RasnetClientComm::RasnetClientComm(string rasmgrHost, int rasmgrPort):
  transferFormatParams(NULL),
  storageFormatParams(NULL)
{
    this->clientId = -1;

    clientParams = new r_Parse_Params();

    this->rasmgrHost = GrpcUtils::convertAddressToString(rasmgrHost, rasmgrPort);

    this->initializedRasMgrService = false;
    this->initializedRasServerService = false;
}

RasnetClientComm::~RasnetClientComm() throw()
{
    this->stopRasMgrKeepAlive();
    this->stopRasServerKeepAlive();

    closeRasmgrService();
    closeRasserverService();
}

int RasNetClientComm::RasNetClientCommDestroy()
{
    this->~RasnetClientComm::RasNetClientCommDestroy();
    return 0;
}

int RasnetClientComm::connectClient(string userName, string passwordHash)
{
    ConnectReq connectReq;
    ConnectRepl connectRepl;

    connectReq.set_username(userName);
    connectReq.set_passwordhash(passwordHash);

    grpc::ClientContext context;
    grpc::Status status = this->getRasMgrService()->Connect(&context, connectReq, &connectRepl);

    if (!status.ok())
    {
        throw r_Error(r_Error::r_Error_AccesDenied, INCORRECT_USER_PASSWORD);
    }

    this->clientId = connectRepl.clientid();
    this->clientUUID = connectRepl.clientuuid();

    // Send keep alive messages to rasmgr until openDB is called
    this->keepAliveTimeout = connectRepl.keepalivetimeout();
    this->startRasMgrKeepAlive();

    return 0;
}

int RasnetClientComm::disconnectClient()
{
    DisconnectReq disconnectReq;
    Void disconnectRepl;

    disconnectReq.set_clientuuid(this->clientUUID);

    grpc::ClientContext context;
    grpc::Status status = this->getRasMgrService()->Disconnect(&context, disconnectReq, &disconnectRepl);
    this->closeRasmgrService();

    if (!status.ok())
    {
        handleError(status.error_message());
    }

    return 0;
}

int RasnetClientComm::openDB(const char *database)
{
    int retval = 0;

    OpenDbReq openDatabaseReq;
    OpenDbRepl openDatabaseRepl;

    openDatabaseReq.set_clientid(this->clientId);
    openDatabaseReq.set_clientuuid(this->clientUUID.c_str());
    openDatabaseReq.set_databasename(database);

    grpc::ClientContext openDbContext;
    grpc::Status openDbStatus = this->getRasMgrService()->OpenDb(&openDbContext, openDatabaseReq, &openDatabaseRepl);

    //stop sending keep alive messages to rasmgr
    this->stopRasMgrKeepAlive();

    if (!openDbStatus.ok())
    {
        string errorText = openDbStatus.error_message();
        handleError(errorText);
    }

    this->rasServerHost = openDatabaseRepl.serverhostname();
    this->rasServerPort = openDatabaseRepl.port();
    this->sessionId = openDatabaseRepl.dbsessionid();

    OpenServerDatabaseReq openServerDatabaseReq;
    OpenServerDatabaseRepl openServerDatabaseRepl;

    openServerDatabaseReq.set_client_id(this->clientId);
    openServerDatabaseReq.set_database_name(database);

    grpc::ClientContext openServerDbContext;
    grpc::Status openServerDbStatus = this->getRasServerService()->OpenServerDatabase(&openServerDbContext, openServerDatabaseReq, &openServerDatabaseRepl);

    if (!openServerDbStatus.ok())
    {
        handleError(openServerDbStatus.error_message());
    }

    // Send keep alive messages to rasserver until openDB is called
    this->startRasServerKeepAlive();

    return retval;
}

int RasnetClientComm::closeDB()
{
    int retval = 0;

    CloseServerDatabaseReq closeServerDatabaseReq;
    CloseDbReq closeDbReq;
    Void closeDatabaseRepl;

    closeServerDatabaseReq.set_client_id(this->clientId);
    closeDbReq.set_clientid(this->clientId);
    closeDbReq.set_clientuuid(this->clientUUID);
    closeDbReq.set_dbsessionid(this->sessionId);

    grpc::ClientContext closeServerDbContext;
    grpc::Status closeServerDbStatus = this->getRasServerService()->CloseServerDatabase(&closeServerDbContext, closeServerDatabaseReq, &closeDatabaseRepl);
    if(!closeServerDbStatus.ok())
    {
        handleError(closeServerDbStatus.error_message());
    }

    grpc::ClientContext closeDbContext;
    grpc::Status closeDbStatus = this->getRasMgrService()->CloseDb(&closeDbContext, closeDbReq, &closeDatabaseRepl);
    if(!closeDbStatus.ok())
    {
        handleError(closeDbStatus.error_message());
    }

    this->stopRasServerKeepAlive();

    this->disconnectClient();

    this->closeRasserverService();

    return retval;
}

int RasnetClientComm::createDB(const char *name) throw (r_Error)
{
    r_Error* error = r_Error::getAnyError("Not implemented exception;");
    r_Error tmp = *error;
    delete error;
    throw tmp;
}

int RasnetClientComm::destroyDB(const char *name) throw (r_Error)
{
    r_Error* error = r_Error::getAnyError("Not implemented exception;");
    r_Error tmp = *error;
    delete error;
    throw tmp;
}

int RasnetClientComm::openTA(unsigned short readOnly) throw (r_Error)
{
    int retval = 1;

    BeginTransactionReq beginTransactionReq;
    BeginTransactionRepl beginTransactionRepl;

    beginTransactionReq.set_rw(readOnly == 0);
    beginTransactionReq.set_client_id(this->clientId);

    grpc::ClientContext context;
    grpc::Status beginTransationStatus = this->getRasServerService()->BeginTransaction(&context, beginTransactionReq, &beginTransactionRepl);
    if (!beginTransationStatus.ok())
    {
        const char* errorText = beginTransationStatus.error_message().c_str();
        throw r_Ebase_dbms(r_Error::r_Error_TransactionOpen, errorText);
    }

    return retval;
}

int RasnetClientComm::commitTA() throw (r_Error)
{
    int retval = 1;

    CommitTransactionReq commitTransactionReq;
    CommitTransactionRepl commitTransactionRepl;

    commitTransactionReq.set_client_id(this->clientId);

    grpc::ClientContext context;
    grpc::Status commitStatus = this->getRasServerService()->CommitTransaction(&context, commitTransactionReq, &commitTransactionRepl);
    if (!commitStatus.ok())
    {
        handleError(commitStatus.error_message());
    }

    return retval;
}

int RasnetClientComm::abortTA()
{
    AbortTransactionReq abortTransactionReq;
    AbortTransactionRepl AbortTransactionRepl;

    abortTransactionReq.set_client_id(this->clientId);

    grpc::ClientContext context;
    grpc::Status abortTransactionStatus = this->getRasServerService()->AbortTransaction(&context, abortTransactionReq, &AbortTransactionRepl);
    if (!abortTransactionStatus.ok())
    {
        handleError(abortTransactionStatus.error_message());
    }
}

void RasnetClientComm::insertMDD(const char *collName, r_GMarray *mar) throw (r_Error)
{
    checkForRwTransaction();

    r_Minterval     spatdom;
    r_Bytes         marBytes;
    RPCMarray*      rpcMarray;
    r_Bytes         tileSize = 0;

    // get the spatial domain of the r_GMarray
    spatdom = mar->spatial_domain();

    // determine the amount of data to be transferred
    marBytes = mar->get_array_size();

    const r_Base_Type* baseType = mar->get_base_type_schema();

    // if the MDD is too large for being transfered as one block, it has to be
    // divided in tiles
    const r_Tiling* til = mar->get_storage_layout()->get_tiling();
    r_Tiling_Scheme scheme = til->get_tiling_scheme();
    if (scheme == r_NoTiling)
        tileSize = RMInit::RMInit::clientTileSize;
    else
        //allowed because the only subclass of tiling without size is no tiling
        tileSize = ((const r_Size_Tiling*)til)->get_tile_size();


    // initiate composition of MDD at server side
    int status = executeStartInsertPersMDD(collName, mar);  //rpcStatusPtr = rpcstartinsertpersmdd_1( params, binding_h );

    switch( status )
    {
    case 0:
        break; // OK
    case 2:
        throw r_Error( r_Error::r_Error_DatabaseClassUndefined );
        break;
    case 3:
        throw r_Error( r_Error::r_Error_CollectionElementTypeMismatch );
        break;
    case 4:
        throw r_Error( r_Error::r_Error_TypeInvalid );
        break;
    default:
        throw r_Error( r_Error::r_Error_TransferFailed );
        break;
    }

    r_Set< r_GMarray* >* bagOfTiles;

    bagOfTiles = mar->get_storage_layout()->decomposeMDD( mar );

    LTRACE << "decomposing into " << bagOfTiles->cardinality() << " tiles";

    r_Iterator< r_GMarray* > iter = bagOfTiles->create_iterator();
    r_GMarray *origTile;

    for(iter.reset(); iter.not_done(); iter.advance() )
    {
        origTile = *iter;

        LTRACE << "inserting Tile with domain " << origTile->spatial_domain() << ", " << origTile->spatial_domain().cell_count() * origTile->get_type_length() << " bytes";

        getMarRpcRepresentation( origTile, rpcMarray, mar->get_storage_layout()->get_storage_format(), baseType );

        status = executeInsertTile(true, rpcMarray);

        // free rpcMarray structure (rpcMarray->data.confarray_val is freed somewhere else)
        freeMarRpcRepresentation( origTile, rpcMarray );

        // delete current tile (including data block)
        delete origTile;

        if( status > 0 )
        {
            throw r_Error( r_Error::r_Error_TransferFailed );
        }
    }

    executeEndInsertMDD(true); //rpcendinsertmdd_1( params3, binding_h );

    // delete transient data
    bagOfTiles->remove_all();
    delete bagOfTiles;
}

r_Ref_Any RasnetClientComm::getMDDByOId(const r_OId &oid) throw (r_Error)
{
    LERROR << "Internal error: RasnetClientComm::getMDDByOId() not implemented, returning empty r_Ref_Any().";
    return r_Ref_Any();
}

void RasnetClientComm::insertColl(const char *collName, const char *typeName, const r_OId &oid) throw (r_Error)
{
    checkForRwTransaction();

    InsertCollectionReq insertCollectionReq;
    InsertCollectionRepl  insertCollectionRepl;

    insertCollectionReq.set_client_id(this->clientId);
    insertCollectionReq.set_collection_name(collName);
    insertCollectionReq.set_type_name(typeName);
    insertCollectionReq.set_oid(oid.get_string_representation());

    grpc::ClientContext context;
    grpc::Status insertStatus = this->getRasServerService()->InsertCollection(&context, insertCollectionReq, &insertCollectionRepl);
    if (!insertStatus .ok())
    {
        handleError(insertStatus.error_message());
    }

    int status = insertCollectionRepl.status();

    handleStatusCode(status, "insertColl");
}

void RasnetClientComm::deleteCollByName(const char *collName) throw (r_Error)
{
    checkForRwTransaction();

    DeleteCollectionByNameReq deleteCollectionByNameReq;
    DeleteCollectionByNameRepl deleteCollectionByNameRepl;

    deleteCollectionByNameReq.set_client_id(this->clientId);
    deleteCollectionByNameReq.set_collection_name(collName);

    grpc::ClientContext context;
    grpc::Status deleteCollectionStatus = this->getRasServerService()->DeleteCollectionByName(&context, deleteCollectionByNameReq, &deleteCollectionByNameRepl);
    if (!deleteCollectionStatus.ok())
    {
        handleError(deleteCollectionStatus.error_message());
    }

    handleStatusCode(deleteCollectionByNameRepl.status(), "deleteCollByName");
}

void RasnetClientComm::deleteObjByOId(const r_OId &oid) throw (r_Error)
{
    checkForRwTransaction();

    DeleteCollectionByOidReq deleteCollectionByOidReq;
    DeleteCollectionByOidRepl deleteCollectionByOidRepl;

    deleteCollectionByOidReq.set_client_id(this->clientId);
    deleteCollectionByOidReq.set_oid(oid.get_string_representation());

    grpc::ClientContext context;
    grpc::Status deleteCollectionStatus = this->getRasServerService()->DeleteCollectionByOid(&context, deleteCollectionByOidReq, &deleteCollectionByOidRepl);
    if (!deleteCollectionStatus.ok())
    {
        handleError(deleteCollectionStatus.error_message());
    }

    handleStatusCode(deleteCollectionByOidRepl.status(), "deleteCollByName");
}

void RasnetClientComm::removeObjFromColl(const char *name, const r_OId &oid) throw (r_Error)
{
    checkForRwTransaction();

    RemoveObjectFromCollectionReq removeObjectFromCollectionReq;
    RemoveObjectFromCollectionRepl removeObjectFromCollectionRepl;

    removeObjectFromCollectionReq.set_client_id(this->clientId);
    removeObjectFromCollectionReq.set_collection_name(name);
    removeObjectFromCollectionReq.set_oid(oid.get_string_representation());

    grpc::ClientContext context;
    grpc::Status removeObjectStatus = this->getRasServerService()->RemoveObjectFromCollection(&context, removeObjectFromCollectionReq, &removeObjectFromCollectionRepl);
    if (!removeObjectStatus.ok())
    {
        handleError(removeObjectStatus.error_message());
    }

    int status = removeObjectFromCollectionRepl.status();
    handleStatusCode(status, "removeObjFromColl");
}

r_Ref_Any RasnetClientComm::getCollByName(const char *name) throw (r_Error)
{
    r_Ref_Any result = executeGetCollByNameOrOId ( name, r_OId() );
    return result;
}

r_Ref_Any RasnetClientComm::getCollByOId(const r_OId &oid) throw (r_Error)
{
    r_Ref_Any result = executeGetCollByNameOrOId ( NULL, oid );

    return result;
}

r_Ref_Any RasnetClientComm::getCollOIdsByName(const char *name) throw (r_Error)
{
    r_Ref_Any result = executeGetCollOIdsByNameOrOId ( name, r_OId() );

    return result;
}

r_Ref_Any RasnetClientComm::getCollOIdsByOId(const r_OId &oid) throw (r_Error)
{
    r_Ref_Any result = executeGetCollOIdsByNameOrOId ( NULL, oid );

    return result;
}

void RasnetClientComm::executeQuery(const r_OQL_Query &query, r_Set<r_Ref_Any> &result) throw (r_Error)
{
    sendMDDConstants(query);
    int status = executeExecuteQuery( query.get_query(), result );

    switch(status)
    {
    case 0:
        getMDDCollection( result, 1 );
        break; // 1== isQuery
    case 1:
        getElementCollection( result );
        break;
        //case 2:  nothing
    default:
        LERROR << "Internal error: RasnetClientComm::executeQuery(): illegal status value " << status;
    }

}

void RasnetClientComm::executeQuery(const r_OQL_Query &query) throw (r_Error)
{
    checkForRwTransaction();

    sendMDDConstants(query);

    executeExecuteUpdateQuery(query.get_query());
}

void RasnetClientComm::executeQuery(const r_OQL_Query &query, r_Set<r_Ref_Any> &result, int dummy) throw (r_Error)
{
    checkForRwTransaction();

    sendMDDConstants(query);

    int status = executeExecuteUpdateQuery(query.get_query(), result);

    LDEBUG <<"executeUpdateQuery (retrieval) returns " << status;

    switch(status)
    {
    case 0:
        getMDDCollection( result, 1 );
        break; // 1== isQuery
    case 1:
        getElementCollection( result );
        break;
        // case 2:  nothing, should not be error?
    default:
        LERROR << "Internal error: RasnetClientComm::executeQuery(): illegal status value " << status;
    }

}

r_OId RasnetClientComm::getNewOId(unsigned short objType) throw (r_Error)
{
    GetNewOidReq getNewOidReq;
    GetNewOidRepl getNewOidRepl;

    getNewOidReq.set_client_id(this->clientId);
    getNewOidReq.set_object_type(objType);

    grpc::ClientContext context;
    grpc::Status getOidStatus = this->getRasServerService()->GetNewOid(&context, getNewOidReq, &getNewOidRepl);
    if (!getOidStatus.ok())
    {
        handleError(getOidStatus.error_message());
    }

    r_OId oid(getNewOidRepl.oid().c_str());
    return oid;
}

unsigned short RasnetClientComm::getObjectType(const r_OId &oid) throw (r_Error)
{
    GetObjectTypeReq getObjectTypeReq;
    GetObjectTypeRepl getObjectTypeRepl;

    getObjectTypeReq.set_client_id(this->clientId);
    getObjectTypeReq.set_oid(oid.get_string_representation());

    grpc::ClientContext context;
    grpc::Status getObjectTypeStatus = this->getRasServerService()->GetObjectType(&context, getObjectTypeReq, &getObjectTypeRepl);
    if (!getObjectTypeStatus.ok())
    {
        handleError(getObjectTypeStatus.error_message());
    }

    int status = getObjectTypeRepl.status();
    handleStatusCode(status, "getObjectType");

    unsigned short objectType = getObjectTypeRepl.object_type();
    return objectType;
}

char* RasnetClientComm::getTypeStructure(const char *typeName, r_Type_Type typeType) throw (r_Error)
{
    GetTypeStructureReq getTypeStructureReq;
    GetTypeStructureRepl getTypeStructureRepl;

    getTypeStructureReq.set_client_id(this->clientId);
    getTypeStructureReq.set_type_name(typeName);
    getTypeStructureReq.set_type_type(typeType);

    grpc::ClientContext context;
    grpc::Status getTypesStructuresStatus = this
                                            ->getRasServerService()
                                            ->GetTypeStructure(&context, getTypeStructureReq, &getTypeStructureRepl);
    if (!getTypesStructuresStatus.ok())
    {
        handleError(getTypesStructuresStatus.error_message());
    }

    int status = getTypeStructureRepl.status();
    handleStatusCode(status, "getTypeStructure");

    char* typeStructure = strdup(getTypeStructureRepl.type_structure().c_str());
    return typeStructure;
}

int RasnetClientComm::setTransferFormat(r_Data_Format format, const char *formatParams)
{
    storageFormat = format;

    if (storageFormatParams != NULL)
    {
        free(storageFormatParams);
        storageFormatParams = NULL;
    }
    if (formatParams != NULL)
    {
        storageFormatParams = (char*)mymalloc(strlen(formatParams) + 1);
        strcpy(storageFormatParams, formatParams);
        // extract ``compserver'' if present
        clientParams->process(storageFormatParams);
    }

    int result = executeSetFormat( false, format, formatParams);

    return result;
}

int RasnetClientComm::setStorageFormat(r_Data_Format format, const char *formatParams)
{
    transferFormat = format;

    if (transferFormatParams != NULL)
    {
        free(transferFormatParams);
        transferFormatParams = NULL;
    }
    if (formatParams != NULL)
    {
        transferFormatParams = (char*)mymalloc(strlen(formatParams)+1);
        strcpy(transferFormatParams, formatParams);
        // extract ``exactformat'' if present
        clientParams->process(transferFormatParams);
    }

    int result = executeSetFormat( true, format, formatParams);
    return result;
}

boost::shared_ptr<rasnet::service::ClientRassrvrService::Stub> RasnetClientComm::getRasServerService()
{
    boost::unique_lock<boost::shared_mutex> lock(this->rasServerServiceMtx);
    if (!this->initializedRasServerService)
    {
        try
        {
            std::string rasServerAddress = GrpcUtils::convertAddressToString(rasServerHost, rasServerPort);
            std::shared_ptr<grpc::Channel> channel( grpc::CreateChannel(rasServerAddress, grpc::InsecureCredentials()));
            this->rasserverService.reset(new ::rasnet::service::ClientRassrvrService::Stub(channel));

            this->initializedRasServerService = true;
        }
        catch(std::exception& ex)
        {
            LERROR<<ex.what();
            handleError(ex.what());
        }
    }

    return this->rasserverService;
}

void RasnetClientComm::closeRasserverService()
{
    boost::unique_lock<shared_mutex> lock(this->rasServerServiceMtx);
    if (this->initializedRasServerService)
    {
        this->initializedRasServerService = false;
        this->rasserverService.reset();
    }
}

boost::shared_ptr<rasnet::service::RasMgrClientService::Stub> RasnetClientComm::getRasMgrService()
{
    boost::unique_lock<shared_mutex> lock(this->rasMgrServiceMtx);
    if (!this->initializedRasMgrService)
    {
        try
        {
            std::shared_ptr<Channel> channel( grpc::CreateChannel(rasmgrHost, grpc::InsecureCredentials()));
            this->rasmgrService.reset(new ::rasnet::service::RasMgrClientService::Stub(channel));

            this->initializedRasMgrService = true;
        }
        catch(std::exception& ex)
        {
            LERROR<<ex.what();
            handleError(ex.what());
        }
    }

    return this->rasmgrService;
}

void RasnetClientComm::closeRasmgrService()
{
    boost::unique_lock<boost::shared_mutex> lock(this->rasMgrServiceMtx);
    if (this->initializedRasMgrService)
    {
        this->initializedRasMgrService = false;
        this->rasmgrService.reset();
    }
}


int RasnetClientComm::executeStartInsertPersMDD(const char *collName, r_GMarray *mar)
{
    StartInsertMDDReq startInsertMDDReq;
    StartInsertMDDRepl startInsertMDDRepl;

    startInsertMDDReq.set_client_id(this->clientId);
    startInsertMDDReq.set_collname(collName);
    startInsertMDDReq.set_domain(mar->spatial_domain().get_string_representation());
    startInsertMDDReq.set_type_length(mar->get_type_length());
    startInsertMDDReq.set_type_name(mar->get_type_name());
    startInsertMDDReq.set_oid(mar->get_oid().get_string_representation());

    grpc::ClientContext context;
    grpc::Status startInsertStatus = this->getRasServerService()->StartInsertMDD(&context, startInsertMDDReq, &startInsertMDDRepl);
    if (!startInsertStatus.ok())
    {
        handleError(startInsertStatus.error_message());
    }

    return startInsertMDDRepl.status();
}

int RasnetClientComm::executeInsertTile(bool persistent, RPCMarray *tile)
{
    InsertTileReq insertTileReq;
    InsertTileRepl insertTileRepl;

    insertTileReq.set_client_id(this->clientId);
    insertTileReq.set_persistent(persistent);
    insertTileReq.set_domain(tile->domain);
    insertTileReq.set_type_length(tile->cellTypeLength);
    insertTileReq.set_current_format(tile->currentFormat);
    insertTileReq.set_storage_format(tile->storageFormat);
    insertTileReq.set_data(tile->data.confarray_val, tile->data.confarray_len);
    insertTileReq.set_data_length(tile->data.confarray_len);

    grpc::ClientContext context;
    grpc::Status insertTileStatus = this->getRasServerService()->InsertTile(&context, insertTileReq, &insertTileRepl);
    if (!insertTileStatus.ok())
    {
        handleError(insertTileStatus.error_message());
    }

    return insertTileRepl.status();
}

void RasnetClientComm::executeEndInsertMDD(bool persistent)
{
    EndInsertMDDReq endInsertMDDReq;
    EndInsertMDDRepl endInsertMDDRepl;

    endInsertMDDReq.set_client_id(this->clientId);
    endInsertMDDReq.set_persistent(persistent);

    grpc::ClientContext context;
    grpc::Status endInsertMDDStatus = this->getRasServerService()->EndInsertMDD(&context, endInsertMDDReq, &endInsertMDDRepl);
    if (!endInsertMDDStatus.ok())
    {
        handleError(endInsertMDDStatus.error_message());
    }

    handleStatusCode(endInsertMDDRepl.status(), "executeEndInsertMDD");
}

void RasnetClientComm::getMDDCollection(r_Set<r_Ref_Any> &mddColl, unsigned int isQuery) throw (r_Error)
{
    unsigned short tileStatus=0;
    unsigned short mddStatus = 0;

    while( mddStatus == 0 ) // repeat until all MDDs are transferred
    {
        r_Ref<r_GMarray> mddResult;

        // Get spatial domain of next MDD
        GetMDDRes* thisResult = executeGetNextMDD();

        mddStatus = thisResult->status;

        if( mddStatus == 2 )
        {
            LFATAL << "Error: getMDDCollection(...) - no transfer collection or empty transfer collection";
            throw r_Error( r_Error::r_Error_TransferFailed );
        }

        tileStatus = getMDDCore(mddResult, thisResult, isQuery);

        // finally, insert the r_Marray into the set

        mddColl.insert_element( mddResult, 1 );

        free(thisResult->domain);
        free(thisResult->typeName);
        free(thisResult->typeStructure);
        free(thisResult->oid);
        delete   thisResult;

        if( tileStatus == 0 ) // if this is true, we're done with this collection
            break;

    } // end while( mddStatus == 0 )

    executeEndTransfer();
}

int RasnetClientComm::executeEndTransfer()
{
    EndTransferReq endTransferReq;
    EndTransferRepl endTransferRepl;

    endTransferReq.set_client_id(this->clientId);

    grpc::ClientContext context;
    grpc::Status endTransferStatus = this->getRasServerService()->EndTransfer(&context, endTransferReq, &endTransferRepl);
    if (!endTransferStatus.ok())
    {
        handleError(endTransferStatus.error_message());
    }

    return endTransferRepl.status();
}

GetMDDRes* RasnetClientComm::executeGetNextMDD()
{
    GetNextMDDReq getNextMDDReq;
    GetNextMDDRepl getNextMDDRepl;

    getNextMDDReq.set_client_id(this->clientId);

    grpc::ClientContext context;
    grpc::Status getNextMDD = this->getRasServerService()->GetNextMDD(&context, getNextMDDReq, &getNextMDDRepl);
    if (!getNextMDD.ok())
    {
        handleError(getNextMDD.error_message());
    }

    GetMDDRes* result = new GetMDDRes();
    result->status = getNextMDDRepl.status();
    result->domain = strdup(getNextMDDRepl.domain().c_str());
    result->typeName = strdup(getNextMDDRepl.type_name().c_str());
    result->typeStructure = strdup(getNextMDDRepl.type_structure().c_str());
    result->oid = strdup(getNextMDDRepl.oid().c_str());
    result->currentFormat = getNextMDDRepl.current_format();

    return result;
}

unsigned short RasnetClientComm::getMDDCore(r_Ref<r_GMarray> &mdd, GetMDDRes *thisResult, unsigned int isQuery) throw (r_Error)
{
    //  create r_Minterval and oid
    r_Minterval mddDomain( thisResult->domain );
    r_OId       rOId     ( thisResult->oid );
    r_GMarray  *marray;

    if( isQuery )
        marray = new( r_Database::actual_database, r_Object::transient, rOId ) r_GMarray();
    else
        marray = new( r_Database::actual_database, r_Object::read     , rOId ) r_GMarray();

    marray->set_spatial_domain( mddDomain );
    marray->set_type_by_name  ( thisResult->typeName );
    marray->set_type_structure( thisResult->typeStructure );

    r_Data_Format currentFormat = (r_Data_Format)(thisResult->currentFormat);
    //    currentFormat = r_Array;
    marray->set_current_format( currentFormat );

    r_Data_Format decompFormat;

    const r_Base_Type *baseType = marray->get_base_type_schema();

    // Variables needed for tile transfer
    GetTileRes* tileRes=0;
    unsigned short  mddDim = mddDomain.dimension();  // we assume that each tile has the same dimensionality as the MDD
    r_Minterval     tileDomain;
    r_GMarray*      tile;  // for temporary tile
    char*           memCopy;
    unsigned long   memCopyLen;
    int             tileCntr = 0;
    unsigned short  tileStatus   = 0;

    tileStatus = 2; // call rpcgetnexttile_1 at least once

    while( tileStatus == 2 || tileStatus == 3 )  // while( for all tiles of the current MDD )
    {
        tileRes = executeGetNextTile();

        tileStatus = tileRes->status;

        if( tileStatus == 4 )
        {
            freeGetTileRes(tileRes);
            LFATAL << "Error: rpcGetNextTile(...) - no tile to transfer or empty transfer collection";
            throw r_Error( r_Error::r_Error_TransferFailed );
        }

        // take cellTypeLength for current MDD of the first tile
        if( tileCntr == 0 )
            marray->set_type_length( tileRes->marray->cellTypeLength );

        tileDomain = r_Minterval( tileRes->marray->domain );
        memCopyLen = tileDomain.cell_count() * marray->get_type_length(); // cell type length of the tile must be the same
        if (memCopyLen < tileRes->marray->data.confarray_len)
            memCopyLen = tileRes->marray->data.confarray_len;   // may happen when compression expands
        memCopy    = new char[ memCopyLen ];

        // create temporary tile
        tile = new r_GMarray();
        tile->set_spatial_domain( tileDomain );
        tile->set_array( memCopy );
        tile->set_array_size( memCopyLen );
        tile->set_type_length( tileRes->marray->cellTypeLength );
        tileCntr++;

        // Variables needed for block transfer of a tile
        unsigned long  blockOffset = 0;
        unsigned short subStatus  = 3;
        currentFormat = (r_Data_Format)(tileRes->marray->currentFormat);

        switch( tileStatus )
        {
        case 3: // at least one block of the tile is left

            // Tile arrives in several blocks -> put them together
            concatArrayData(tileRes->marray->data.confarray_val, tileRes->marray->data.confarray_len, memCopy, memCopyLen, blockOffset);
            freeGetTileRes(tileRes);

            tileRes = executeGetNextTile();//rpcgetnexttile_1( &clientID, binding_h );

            subStatus = tileRes->status;

            if( subStatus == 4 )
            {
                freeGetTileRes(tileRes);
                throw r_Error( r_Error::r_Error_TransferFailed );
            }

            concatArrayData(tileRes->marray->data.confarray_val, tileRes->marray->data.confarray_len, memCopy, memCopyLen, blockOffset);
            freeGetTileRes(tileRes);

            tileStatus = subStatus;
            break;

        default: // tileStatus = 0,3 last block of the current tile

            // Tile arrives as one block.
            concatArrayData(tileRes->marray->data.confarray_val, tileRes->marray->data.confarray_len, memCopy, memCopyLen, blockOffset);
            freeGetTileRes(tileRes);

            break;
        }

        char* marrayData = NULL;
        // Now the tile is transferred completely, insert it into current MDD
        if( tileStatus < 2 && tileCntr == 1 && (tile->spatial_domain() == marray->spatial_domain()))
        {
            // MDD consists of just one tile that is the same size of the mdd

            // simply take the data memory of the tile
            marray->set_array( tile->get_array() );
            marray->set_array_size( tile->get_array_size() );
            tile->set_array( 0 );
        }
        else
        {
            // MDD consists of more than one tile or the tile does not cover the whole domain

            r_Bytes size = mddDomain.cell_count() * marray->get_type_length();

            if( tileCntr == 1 )
            {
                // allocate memory for the MDD
                marrayData = new char[ size ];
                memset(marrayData, 0, size);

                marray->set_array( marrayData );
            }
            else
                marrayData = marray->get_array();


            // copy tile data into MDD data space (optimized, relying on the internal representation of an MDD )
            char*         mddBlockPtr;
            char*         tileBlockPtr = tile->get_array();
            unsigned long blockCells   = tileDomain[tileDomain.dimension()-1].high()-tileDomain[tileDomain.dimension()-1].low()+1;
            unsigned long blockSize    = blockCells * marray->get_type_length();
            unsigned long blockNo      = tileDomain.cell_count() / blockCells;

            for( unsigned long blockCtr = 0; blockCtr < blockNo; blockCtr++ )
            {
                mddBlockPtr = marrayData + marray->get_type_length()*mddDomain.cell_offset( tileDomain.cell_point( blockCtr * blockCells ) );
                memcpy( (void*)mddBlockPtr, (void*)tileBlockPtr, (size_t)blockSize );
                tileBlockPtr += blockSize;
            }

            // former non-optimized version
            // for( i=0; i<tileDomain->cell_count(); i++ )
            //   (*marray)[tileDomain->cell_point( i )] = (*tile)[tileDomain->cell_point( i )];

            marray->set_array_size( size );
        }

        // delete temporary tile
        delete tile;

    }  // end while( MDD is not transferred completely )


    mdd = r_Ref<r_GMarray>( marray->get_oid(), marray );

    return tileStatus;
}

GetTileRes* RasnetClientComm::executeGetNextTile()
{
    GetNextTileReq getNextTileReq;
    GetNextTileRepl getNextTileRepl;

    getNextTileReq.set_client_id(this->clientId);

    grpc::ClientContext context;
    grpc::Status getNextTileStatus = this->getRasServerService()->GetNextTile(&context, getNextTileReq, &getNextTileRepl);
    if (!getNextTileStatus.ok())
    {
        handleError(getNextTileStatus.error_message());
    }

    GetTileRes* result = new GetTileRes();
    result->marray = new RPCMarray();

    result->status = getNextTileRepl.status();
    result->marray->domain = strdup(getNextTileRepl.domain().c_str());
    result->marray->cellTypeLength = getNextTileRepl.cell_type_length();
    result->marray->currentFormat = getNextTileRepl.current_format();
    result->marray->storageFormat = getNextTileRepl.storage_format();

    int length = getNextTileRepl.data_length();
    result->marray->data.confarray_len = length;
    result->marray->data.confarray_val = (char*) mymalloc(length);
    memcpy(result->marray->data.confarray_val, getNextTileRepl.data().c_str(), length);

    return result;
}

void RasnetClientComm::getMarRpcRepresentation(const r_GMarray *mar, RPCMarray *&rpcMarray, r_Data_Format initStorageFormat, const r_Base_Type *baseType)
{
    // allocate memory for the RPCMarray data structure and assign its fields
    rpcMarray                 = (RPCMarray*)mymalloc( sizeof(RPCMarray) );
    rpcMarray->domain         = mar->spatial_domain().get_string_representation();
    rpcMarray->cellTypeLength = mar->get_type_length();
    rpcMarray->currentFormat = initStorageFormat;
    rpcMarray->data.confarray_len = mar->get_array_size();
    rpcMarray->data.confarray_val = (char*)(mar->get_array());
    rpcMarray->storageFormat = initStorageFormat;
}


void RasnetClientComm::freeMarRpcRepresentation(const r_GMarray *mar, RPCMarray *rpcMarray)
{
    if (rpcMarray->data.confarray_val != ((r_GMarray*)mar)->get_array())
    {
        delete[] rpcMarray->data.confarray_val;
    }
    free( rpcMarray->domain );
    free( rpcMarray );
}

int RasnetClientComm::concatArrayData( const char *source, unsigned long srcSize, char *&dest, unsigned long &destSize, unsigned long &destLevel )
{
    if (destLevel + srcSize > destSize)
    {
        // need to extend dest
        unsigned long newSize = destLevel + srcSize;
        char *newArray;

        // allocate a little extra if we have to extend
        newSize = newSize + newSize / 16;

        //    LTRACE << "RasnetClientComm::concatArrayData(): need to extend from " << destSize << " to " << newSize;

        if ((newArray = new char[newSize]) == NULL)
        {
            return -1;
        }

        memcpy(newArray, dest, destLevel);
        delete [] dest;
        dest = newArray;
        destSize = newSize;
    }

    memcpy(dest + destLevel, source, srcSize);
    destLevel += srcSize;

    return 0;
}

void RasnetClientComm::freeGetTileRes(GetTileRes *ptr)
{
    if(ptr->marray->domain)
        free(ptr->marray->domain);
    if(ptr->marray->data.confarray_val)
        free(ptr->marray->data.confarray_val);
    delete ptr->marray;
    delete ptr;
}

r_Ref_Any RasnetClientComm::executeGetCollByNameOrOId(const char *collName, const r_OId &oid) throw( r_Error )
{
    GetCollectionByNameOrOidReq getCollectionByNameOrOidReq;
    GetCollectionByNameOrOidRepl getCollectionByNameOrOidRepl;


    getCollectionByNameOrOidReq.set_client_id(this->clientId);

    if (collName != NULL)
    {
        getCollectionByNameOrOidReq.set_collection_identifier(collName);
        getCollectionByNameOrOidReq.set_is_name(true);
    }
    else
    {
        getCollectionByNameOrOidReq.set_collection_identifier(oid.get_string_representation());
        getCollectionByNameOrOidReq.set_is_name(false);
    }

    grpc::ClientContext context;
    grpc::Status rasServerStatus = this->getRasServerService()->GetCollectionByNameOrOid(&context, getCollectionByNameOrOidReq, &getCollectionByNameOrOidRepl);
    if (!rasServerStatus.ok())
    {
        handleError(rasServerStatus.error_message());
    }

    int status = getCollectionByNameOrOidRepl.status();
    handleStatusCode(status, "getCollByName");

    r_OId rOId(getCollectionByNameOrOidRepl.oid().c_str());
    r_Set< r_Ref_Any >* set  = new ( r_Database::actual_database, r_Object::read, rOId )  r_Set< r_Ref_Any >;

    set->set_type_by_name(getCollectionByNameOrOidRepl.type_name().c_str());
    set->set_type_structure(getCollectionByNameOrOidRepl.type_structure().c_str());
    set->set_object_name(getCollectionByNameOrOidRepl.collection_name().c_str());

    if( status == 0 )
        getMDDCollection( *set, 0 );
    //  else rpcStatus == 1 -> Result collection is empty and nothing has to be got.

    r_Ref_Any result = r_Ref_Any( set->get_oid(), set );
    return result;
}

r_Ref_Any RasnetClientComm::executeGetCollOIdsByNameOrOId(const char *collName, const r_OId &oid) throw( r_Error )
{
    GetCollOidsByNameOrOidReq getCollOidsByNameOrOidReq;
    GetCollOidsByNameOrOidRepl getCollOidsByNameOrOidRepl;

    getCollOidsByNameOrOidReq.set_client_id(this->clientId);

    if (collName != NULL)
    {
        getCollOidsByNameOrOidReq.set_collection_identifier(collName);
        getCollOidsByNameOrOidReq.set_is_name(true);
    }
    else
    {
        getCollOidsByNameOrOidReq.set_collection_identifier(oid.get_string_representation());
        getCollOidsByNameOrOidReq.set_is_name(false);
    }

    grpc::ClientContext context;
    grpc::Status getCollOidsStatus = this->getRasServerService()->GetCollOidsByNameOrOid(&context, getCollOidsByNameOrOidReq, &getCollOidsByNameOrOidRepl);
    if (!getCollOidsStatus.ok())
    {
        handleError(getCollOidsStatus.error_message());
    }

    int status = getCollOidsByNameOrOidRepl.status();

    if (status != 0 && status != 1)
    {
        handleStatusCode(status, "executeGetCollOIdsByNameOrOId");
    }

    const char* typeName = getCollOidsByNameOrOidRepl.type_name().c_str();
    const char* typeStructure = getCollOidsByNameOrOidRepl.type_structure().c_str();
    const char* oidString = getCollOidsByNameOrOidRepl.oids_string().c_str();
    const char* collectionName = getCollOidsByNameOrOidRepl.collection_name().c_str();

    r_OId rOId(oidString);
    r_Set< r_Ref<r_GMarray> >* set = new ( r_Database::actual_database, r_Object::read, rOId )  r_Set< r_Ref< r_GMarray > >;

    set->set_type_by_name  ( typeName );
    set->set_type_structure( typeStructure );
    set->set_object_name   ( collName );

    for (int i = 0; i < getCollOidsByNameOrOidRepl.oid_set_size(); ++i)
    {
        r_OId roid(getCollOidsByNameOrOidRepl.oid_set(i).c_str());
        set->insert_element(r_Ref<r_GMarray>(roid), 1);
    }

    r_Ref_Any result = r_Ref_Any( set->get_oid(), set );
    return result;
}

void RasnetClientComm::sendMDDConstants( const r_OQL_Query& query ) throw( r_Error )
{
    unsigned short status;

    if( query.get_constants() )
    {
        r_Set< r_GMarray* >* mddConstants = (r_Set< r_GMarray* >*)query.get_constants();

        // in fact executeInitUpdate prepares server structures for MDD transfer
        if(executeInitUpdate() != 0)
        {
            throw r_Error( r_Error::r_Error_TransferFailed );
        }

        r_Iterator<r_GMarray*> iter = mddConstants->create_iterator();

        for( iter.reset(); iter.not_done(); iter++ )
        {
            r_GMarray* mdd = *iter;

            const r_Base_Type* baseType = mdd->get_base_type_schema();

            if( mdd )
            {
                status = executeStartInsertTransMDD(mdd);
                switch( status )
                {
                case 0:
                    break; // OK
                case 2:
                    throw r_Error( r_Error::r_Error_DatabaseClassUndefined );
                    break;
                case 3:
                    throw r_Error( r_Error::r_Error_TypeInvalid );
                    break;
                default:
                    throw r_Error( r_Error::r_Error_TransferFailed );
                    break;
                }


                r_Set< r_GMarray* >* bagOfTiles = NULL;

                if (mdd->get_array())
                {
                    bagOfTiles = mdd->get_storage_layout()->decomposeMDD( mdd );
                }
                else
                {
                    bagOfTiles = mdd->get_tiled_array();
                }

                r_Iterator< r_GMarray* > iter2 = bagOfTiles->create_iterator();

                for(iter2.reset(); iter2.not_done(); iter2.advance())
                {
                    RPCMarray* rpcMarray;

                    r_GMarray *origTile = *iter2;

                    getMarRpcRepresentation( origTile, rpcMarray, mdd->get_storage_layout()->get_storage_format(), baseType );

                    status = executeInsertTile(false, rpcMarray);

                    // free rpcMarray structure (rpcMarray->data.confarray_val is freed somewhere else)
                    freeMarRpcRepresentation( origTile, rpcMarray );

                    // delete current tile (including data block)
                    delete origTile;
                    origTile = NULL;

                    if( status > 0 )
                    {
                        throw r_Error( r_Error::r_Error_TransferFailed );
                    }
                }

                bagOfTiles->remove_all();
                delete bagOfTiles;
                bagOfTiles = NULL;

                executeEndInsertMDD(false);
            }
        }
    }
}

int RasnetClientComm::executeStartInsertTransMDD(r_GMarray *mdd)
{
    StartInsertTransMDDReq startInsertTransMDDReq;
    StartInsertTransMDDRepl startInsertTransMDDRepl;

    startInsertTransMDDReq.set_client_id(this->clientId);
    startInsertTransMDDReq.set_domain(mdd->spatial_domain().get_string_representation());
    startInsertTransMDDReq.set_type_length(mdd->get_type_length());
    startInsertTransMDDReq.set_type_name(mdd->get_type_name());

    grpc::ClientContext context;
    grpc::Status startInsertTransMDDStatus = this->getRasServerService()->StartInsertTransMDD(&context, startInsertTransMDDReq, &startInsertTransMDDRepl);
    if (!startInsertTransMDDStatus.ok())
    {
        handleError(startInsertTransMDDStatus.error_message());
    }

    return startInsertTransMDDRepl.status();
}

int RasnetClientComm::executeInitUpdate()
{
    InitUpdateReq initUpdateReq;
    InitUpdateRepl initUpdateRepl;

    initUpdateReq.set_client_id(this->clientId);
    grpc::ClientContext context;
    grpc::Status initUpdataStatus = this->getRasServerService()->InitUpdate(&context, initUpdateReq, &initUpdateRepl);
    if (!initUpdataStatus.ok())
    {
        handleError(initUpdataStatus.error_message());
    }

    return initUpdateRepl.status();
}


int RasnetClientComm::executeExecuteQuery(const char *query, r_Set<r_Ref_Any> &result) throw( r_Error )
{
    ExecuteQueryReq executeQueryReq;
    ExecuteQueryRepl executeQueryRepl;

    executeQueryReq.set_client_id(this->clientId);
    executeQueryReq.set_query(query);

    grpc::ClientContext context;
    grpc::Status executeQueryStatus = this->getRasServerService()->ExecuteQuery(&context, executeQueryReq, &executeQueryRepl);
    if (!executeQueryStatus.ok())
    {
        handleError(executeQueryStatus.error_message());
    }

    int status = executeQueryRepl.status();
    int errNo = executeQueryRepl.err_no();
    int lineNo = executeQueryRepl.line_no();
    int colNo = executeQueryRepl.col_no();
    const char* token = executeQueryRepl.token().c_str();
    const char* typeName = executeQueryRepl.type_name().c_str();
    const char* typeStructure = executeQueryRepl.type_structure().c_str();

    if(status == 0 || status == 1)
    {
        result.set_type_by_name( typeName );
        result.set_type_structure( typeStructure );
    }

    if( status == 4 || status == 5 )
    {
        r_Equery_execution_failed err( errNo, lineNo, colNo, token );
        throw err;
    }

    return status;
}

GetElementRes* RasnetClientComm::executeGetNextElement()
{
    GetNextElementReq getNextElementReq;
    GetNextElementRepl getNextElementRepl;

    getNextElementReq.set_client_id(this->clientId);

    grpc::ClientContext context;
    grpc::Status getNextElementStatus = this->getRasServerService()->GetNextElement(&context, getNextElementReq, &getNextElementRepl);
    if (!getNextElementStatus.ok())
    {
        handleError(getNextElementStatus.error_message());
    }

    GetElementRes* result = new GetElementRes();

    result->data.confarray_len = getNextElementRepl.data_length();
    result->data.confarray_val = new char[getNextElementRepl.data_length()];
    memcpy(result->data.confarray_val, getNextElementRepl.data().c_str(), getNextElementRepl.data_length());
    result->status = getNextElementRepl.status();

    return result;
}

void RasnetClientComm::getElementCollection( r_Set< r_Ref_Any >& resultColl ) throw(r_Error)
{
    unsigned short rpcStatus = 0;

    LDEBUG << "got set of type " << resultColl.get_type_structure();

    while( rpcStatus == 0 ) // repeat until all elements are transferred
    {
        GetElementRes* thisResult = executeGetNextElement();

        rpcStatus = thisResult->status;

        if( rpcStatus == 2 )
        {
            throw r_Error( r_Error::r_Error_TransferFailed );
        }
        // create new collection element, use type of collection resultColl
        r_Ref_Any     element;
        const r_Type* elementType = resultColl.get_element_type_schema();

        switch( elementType->type_id() )
        {
        case r_Type::BOOL:
        case r_Type::CHAR:
        case r_Type::OCTET:
        case r_Type::SHORT:
        case r_Type::USHORT:
        case r_Type::LONG:
        case r_Type::ULONG:
        case r_Type::FLOAT:
        case r_Type::DOUBLE:
            element = new r_Primitive( thisResult->data.confarray_val, (r_Primitive_Type*) elementType );
            r_Transaction::actual_transaction->add_object_list( r_Transaction::SCALAR, (void*) element );
            break;

        case r_Type::COMPLEXTYPE1:
        case r_Type::COMPLEXTYPE2:
            element = new r_Complex(thisResult->data.confarray_val, (r_Complex_Type *)elementType);
            r_Transaction::actual_transaction->add_object_list(r_Transaction::SCALAR, (void *)element);
            break;

        case r_Type::STRUCTURETYPE:
            element = new r_Structure( thisResult->data.confarray_val, (r_Structure_Type*) elementType );
            r_Transaction::actual_transaction->add_object_list( r_Transaction::SCALAR, (void*) element );
            break;

        case r_Type::POINTTYPE:
        {
            char* stringRep = new char[thisResult->data.confarray_len+1];
            strncpy( stringRep, thisResult->data.confarray_val, thisResult->data.confarray_len );
            stringRep[thisResult->data.confarray_len] = '\0';

            r_Point* typedElement = new r_Point( stringRep );
            element               = typedElement;
            r_Transaction::actual_transaction->add_object_list( r_Transaction::POINT, (void*) typedElement );
            delete [] stringRep;
        }
        break;

        case r_Type::SINTERVALTYPE:
        {
            char* stringRep = new char[thisResult->data.confarray_len+1];
            strncpy( stringRep, thisResult->data.confarray_val, thisResult->data.confarray_len );
            stringRep[thisResult->data.confarray_len] = '\0';

            r_Sinterval* typedElement = new r_Sinterval( stringRep );
            element                   = typedElement;
            r_Transaction::actual_transaction->add_object_list( r_Transaction::SINTERVAL, (void*) typedElement );
            delete [] stringRep;
        }
        break;

        case r_Type::MINTERVALTYPE:
        {
            char* stringRep = new char[thisResult->data.confarray_len+1];
            strncpy( stringRep, thisResult->data.confarray_val, thisResult->data.confarray_len );
            stringRep[thisResult->data.confarray_len] = '\0';

            r_Minterval* typedElement = new r_Minterval( stringRep );
            element                   = typedElement;
            r_Transaction::actual_transaction->add_object_list( r_Transaction::MINTERVAL, (void*) typedElement );
            delete [] stringRep;
        }
        break;

        case r_Type::OIDTYPE:
        {
            char* stringRep = new char[thisResult->data.confarray_len+1];
            strncpy( stringRep, thisResult->data.confarray_val, thisResult->data.confarray_len );
            stringRep[thisResult->data.confarray_len] = '\0';

            r_OId* typedElement = new r_OId( stringRep );
            element             = typedElement;
            r_Transaction::actual_transaction->add_object_list( r_Transaction::OID, (void*) typedElement );
            delete [] stringRep;
        }
        break;
        default:
            break;
        }

        LDEBUG << "got an element";

        // insert element into result set
        resultColl.insert_element( element, 1 );

        delete[] thisResult->data.confarray_val;
        delete   thisResult;
    }

    executeEndTransfer();
}

int RasnetClientComm::executeExecuteUpdateQuery(const char *query) throw( r_Error )
{
    ExecuteUpdateQueryReq executeUpdateQueryReq;
    ExecuteUpdateQueryRepl executeUpdateQueryRepl;

    executeUpdateQueryReq.set_client_id(this->clientId);
    executeUpdateQueryReq.set_query(query);

    grpc::ClientContext context;
    grpc::Status executeUpdateQueryStatus = this->getRasServerService()->ExecuteUpdateQuery(&context, executeUpdateQueryReq, &executeUpdateQueryRepl);
    if (!executeUpdateQueryStatus.ok())
    {
        handleError(executeUpdateQueryStatus.error_message());
    }

    int status = executeUpdateQueryRepl.status();
    int errNo = executeUpdateQueryRepl.errono();
    int lineNo = executeUpdateQueryRepl.lineno();
    int colNo = executeUpdateQueryRepl.colno();

    string token = executeUpdateQueryRepl.token();

    if( status == 2 || status == 3 )
    {
        throw r_Equery_execution_failed( errNo, lineNo, colNo, token.c_str() );
    }

    if( status == 1 )
    {
        throw r_Error( r_Error::r_Error_ClientUnknown );
    }

    if( status > 3 )
    {
        throw r_Error( r_Error::r_Error_TransferFailed );
    }

    return status;
}

int  RasnetClientComm::executeExecuteUpdateQuery(const char *query, r_Set< r_Ref_Any >& result) throw(r_Error)
{
    ExecuteInsertQueryReq executeInsertQueryReq;
    ExecuteInsertQueryRepl executeInsertQueryRepl;

    executeInsertQueryReq.set_client_id(this->clientId);
    executeInsertQueryReq.set_query(query);

    grpc::ClientContext context;
    grpc::Status executeInsertStatus = this->getRasServerService()->ExecuteInsertQuery(&context, executeInsertQueryReq, &executeInsertQueryRepl);
    if (!executeInsertStatus.ok())
    {
        handleError(executeInsertStatus.error_message());
    }

    int status = executeInsertQueryRepl.status();
    int errNo = executeInsertQueryRepl.errono();
    int lineNo = executeInsertQueryRepl.lineno();
    int colNo = executeInsertQueryRepl.colno();
    string token = executeInsertQueryRepl.token();
    const char* typeName = executeInsertQueryRepl.type_name().c_str();
    const char* typeStructure = executeInsertQueryRepl.type_structure().c_str();

    if(status == 0 || status == 1 || status == 2)
    {
        result.set_type_by_name( typeName );
        result.set_type_structure( typeStructure );
    }

    // status == 2 - empty result

    if( status == 4 || status == 5 )
    {
        throw r_Equery_execution_failed( errNo, lineNo, colNo, token.c_str() );
    }

    return status;
}

int RasnetClientComm::executeSetFormat(bool lTransferFormat, r_Data_Format format, const char *formatParams)
{
    SetFormatReq setFormatReq;
    SetFormatRepl setFormatRepl;

    setFormatReq.set_client_id(this->clientId);
    setFormatReq.set_transfer_format((lTransferFormat ? 1 : 0));
    setFormatReq.set_format(format);
    setFormatReq.set_format_params(formatParams);

    grpc::ClientContext context;
    grpc::Status setFormatStatus = this->getRasServerService()->SetFormat(&context, setFormatReq, &setFormatRepl);
    if (!setFormatStatus.ok())
    {
        handleError(setFormatStatus.error_message());
    }

    return setFormatRepl.status();
}

void RasnetClientComm::checkForRwTransaction() throw( r_Error )
{
    r_Transaction *trans = r_Transaction::actual_transaction;
    if(  trans == 0 || trans->get_mode() == r_Transaction::read_only )
    {
        LDEBUG << "RasnetClientComm::checkForRwTransaction(): throwing exception from failed TA rw check.";
        throw r_Error( r_Error::r_Error_TransactionReadOnly );
    }
}

void RasnetClientComm::handleError(string error)
{
    ErrorMessage message;

    if(message.ParseFromString(error))
    {
        if (message.type() == ErrorMessage::RERROR)
        {
            r_Error rError(static_cast<r_Error::kind>(message.kind()), message.error_no());
            LDEBUG << "rasnet protocol throwning exception: " << message.error_text();
            throw rError;
        }
        else
        {
            std::runtime_error ex(message.error_text());
            throw ex;
        }
    }
    else
    {
        throw r_EGeneral(error);
    }
}

void RasnetClientComm::handleStatusCode(int status, string method) throw( r_Error )
{
    switch( status )
    {
    case 0:
        break;
    case 1:
        LDEBUG << "RasnetClientComm::" << method << ": error: status = " << status;
        throw r_Error( r_Error::r_Error_ClientUnknown );
        break;
    case 2:
        LDEBUG << "RasnetClientComm::" << method << ": error: status = " << status;
        throw r_Error( r_Error::r_Error_ObjectUnknown );
        break;
    case 3:
        throw r_Error( r_Error::r_Error_ClientUnknown );
        break;
    default:
        LDEBUG << "RasnetClientComm::" << method << ": error: status = " << status;
        throw r_Error( r_Error::r_Error_General );
        break;
    }
}

bool RasnetClientComm::effectivTypeIsRNP() throw()
{
    r_Error* error = r_Error::getAnyError("Not implemented exception;");
    r_Error tmp = *error;
    delete error;
    throw tmp;
}

long unsigned int RasnetClientComm::getClientID() const
{
    return this->clientId;
}

void RasnetClientComm::triggerAliveSignal()
{
    r_Error* error = r_Error::getAnyError("Not implemented exception;");
    r_Error tmp = *error;
    delete error;
    throw tmp;
}

void RasnetClientComm::sendAliveSignal()
{
    r_Error* error = r_Error::getAnyError("Not implemented exception;");
    r_Error tmp = *error;
    delete error;
    throw tmp;
}

const char* RasnetClientComm::getExtendedErrorInfo() throw (r_Error)
{
    r_Error* error = r_Error::getAnyError("Not implemented exception;");
    r_Error tmp = *error;
    delete error;
    throw tmp;
}

void RasnetClientComm::setUserIdentification(const char *userName, const char *plainTextPassword)
{

    connectClient(string(userName), common::Crypto::messageDigest(string(plainTextPassword), DEFAULT_DIGEST));
}

void RasnetClientComm::setMaxRetry(unsigned int newMaxRetry)
{
    r_Error* error = r_Error::getAnyError("Not implemented exception;");
    r_Error tmp = *error;
    delete error;
    throw tmp;
}

unsigned int RasnetClientComm::getMaxRetry()
{
    r_Error* error = r_Error::getAnyError("Not implemented exception;");
    r_Error tmp = *error;
    delete error;
    throw tmp;
}

void RasnetClientComm::setTimeoutInterval(int seconds)
{
    r_Error* error = r_Error::getAnyError("Not implemented exception;");
    r_Error tmp = *error;
    delete error;
    throw tmp;
}

int RasnetClientComm::getTimeoutInterval()
{
    r_Error* error = r_Error::getAnyError("Not implemented exception;");
    r_Error tmp = *error;
    delete error;
    throw tmp;

}

/* START: KEEP ALIVE */

/* RASMGR */
void RasnetClientComm::startRasMgrKeepAlive()
{
    boost::lock_guard<boost::mutex> lock(this->rasmgrKeepAliveMutex);

    //TODO-GM
    this->isRasmgrKeepAliveRunning = true;
    this->rasMgrKeepAliveManagementThread.reset(new thread(&RasnetClientComm::clientRasMgrKeepAliveRunner, this));

}

void RasnetClientComm::stopRasMgrKeepAlive()
{
    try
    {
        {
            boost::unique_lock<boost::mutex> lock(this->rasmgrKeepAliveMutex);
            this->isRasmgrKeepAliveRunning = false;
        }

        if(!rasMgrKeepAliveManagementThread)
        {
            LDEBUG<<"Thread that sends messages from client to rasmgr is not running.";
        }
        else
        {

            this->isRasmgrKeepAliveRunningCondition.notify_one();

            if (this->rasMgrKeepAliveManagementThread->joinable())
            {
                LDEBUG<<"Joining rasmgr keep alive management thread.";
                this->rasMgrKeepAliveManagementThread->join();
                LDEBUG<<"Joined rasmgr keep alive management thread.";
            }
            else
            {
                LDEBUG<<"Interrupting rasmgr keep alive management thread.";
                this->rasMgrKeepAliveManagementThread->interrupt();
                LDEBUG<<"Interrupted rasmgr keep alive management thread.";
            }

        }
    }
    catch (std::exception& ex)
    {
        LERROR<<ex.what();
    }
    catch (...)
    {
        LERROR<<"Stoping rasmgr keep alive has failed";
    }
}

void RasnetClientComm::clientRasMgrKeepAliveRunner()
{
    boost::posix_time::time_duration timeToSleepFor = boost::posix_time::milliseconds(this->keepAliveTimeout);

    boost::unique_lock<boost::mutex> threadLock(this->rasmgrKeepAliveMutex);
    while (this->isRasmgrKeepAliveRunning)
    {
        try
        {
            // Wait on the condition variable to be notified from the
            // destructor when it is time to stop the worker thread
            if(!this->isRasmgrKeepAliveRunningCondition.timed_wait(threadLock, timeToSleepFor))
            {
                KeepAliveReq keepAliveReq;
                Void keepAliveRepl;

                keepAliveReq.set_clientuuid(this->clientUUID);

                grpc::ClientContext context;
                grpc::Status keepAliveStatus = this->getRasMgrService()->KeepAlive(&context, keepAliveReq, &keepAliveRepl);
                if (!keepAliveStatus.ok())
                {
                    LERROR<<"Failed to send keep alive message to rasmgr:"<<keepAliveStatus.error_message();
                    LDEBUG<<"Stopping client-rasmgr keep alive thread.";
                    this->isRasmgrKeepAliveRunning = false;
                }
            }
        }
        catch (std::exception& ex)
        {
            LERROR<<"Rasmgr Keep Alive thread has failed";
            LERROR<<ex.what();
        }
        catch (...)
        {
            LERROR<<"Rasmgr Keep Alive thread failed for unknown reason.";
        }
    }
}

/* RASSERVER */
void RasnetClientComm::startRasServerKeepAlive()
{
    boost::lock_guard<boost::mutex> lock(this->rasserverKeepAliveMutex);

    this->isRasserverKeepAliveRunning = true;
    this->rasServerKeepAliveManagementThread.reset(
        new thread(&RasnetClientComm::clientRasServerKeepAliveRunner, this));
}

void RasnetClientComm::stopRasServerKeepAlive()
{
    try
    {
        {
            boost::unique_lock<boost::mutex> lock(this->rasserverKeepAliveMutex);
            // Change the condition and notify the variable
            this->isRasserverKeepAliveRunning = false;
        }

        if(!rasServerKeepAliveManagementThread)
        {
            LDEBUG<<"Thread that sends messages from client to rasserver is not running.";
        }
        else
        {
            this->isRasserverKeepAliveRunningCondition.notify_one();

            if (this->rasServerKeepAliveManagementThread->joinable())
            {
                LDEBUG<<"Joining rasserver keep alive management thread.";
                this->rasServerKeepAliveManagementThread->join();
                LDEBUG<<"Joined rasserver keep alive management thread.";
            }
            else
            {
                LDEBUG<<"Interrupting rasserver keep alive management thread.";
                this->rasServerKeepAliveManagementThread->interrupt();
                LDEBUG<<"Interrupted rasserver keep alive management thread.";
            }

        }
    }
    catch (std::exception& ex)
    {
        LERROR<<ex.what();
    }
    catch (...)
    {
        LERROR<<"Stoping rasmgr keep alive has failed";
    }
}

void RasnetClientComm::clientRasServerKeepAliveRunner()
{
    boost::posix_time::time_duration timeToSleepFor = boost::posix_time::milliseconds(this->keepAliveTimeout);

    boost::unique_lock<boost::mutex> threadLock(this->rasserverKeepAliveMutex);
    while (this->isRasserverKeepAliveRunning)
    {
        try
        {
            // Wait on the condition variable to be notified from the
            // destructor when it is time to stop the worker thread
            if(!this->isRasserverKeepAliveRunningCondition.timed_wait(threadLock, timeToSleepFor))
            {
                ::rasnet::service::KeepAliveRequest keepAliveReq;
                Void keepAliveRepl;

                keepAliveReq.set_client_uuid(this->clientUUID);
                keepAliveReq.set_session_id(this->sessionId);

                grpc::ClientContext context;
                grpc::Status keepAliveStatus = this->getRasServerService()->KeepAlive(&context, keepAliveReq, &keepAliveRepl);
                if (!keepAliveStatus.ok())
                {
                    LERROR<<"Failed to send keep alive message to rasserver:"<<keepAliveStatus.error_message();
                    LDEBUG<<"Stopping client-rasserver keep alive thread.";
                    this->isRasserverKeepAliveRunning = false;
                }

            }
        }
        catch (std::exception& ex)
        {
            LERROR<<"RasServer Keep Alive thread has failed";
            LERROR<<ex.what();
        }
        catch (...)
        {
            LERROR<<"RasServer Keep Alive thread failed for unknown reason.";
        }
    }

}
/* END: KEEP ALIVE */
