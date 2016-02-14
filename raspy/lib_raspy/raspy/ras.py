import error_message_pb2 as error
import client_rassrvr_service_pb2 as client
import rasmgr_client_service_pb2 as rasmgr
import hashlib
# import numpy as np
# from scipy import sparse

from grpc.beta import implementations

_TIMEOUT_SECONDS = 30


def make_connect_req(username, password):
    con_req = rasmgr.ConnectReq(userName=username, passwordHash=password)
    if not con_req:
        raise Exception("Can't create Connect request")
    return con_req


def make_disconnect_req(cuiid, cid):
    discon_req = rasmgr.DisconnectReq(clientUUID=cuiid, clientID=cid)
    if not discon_req:
        raise Exception("Can't create Disconnect request")
    return discon_req


def make_keep_alive_req(cuiid, cid):
    keep_alive_req = rasmgr.KeepAliveReq(clientUUID=cuiid, clientID=cid)
    if not keep_alive_req:
        raise Exception("Can't create KeepAlive request")
    return keep_alive_req


def make_open_db_req(cuiid, cid, dbname):
    open_db_req = rasmgr.OpenDbReq(clientUUID=cuiid, clientID=cid, databaseName=dbname)
    if not open_db_req:
        raise Exception("Can't create OpenDb request")
    return open_db_req


def make_close_db_req(cuuid, cid, dbsid):
    close_db_req = rasmgr.CloseDbReq(clientUUID=cuuid, clientID=cid, dbSessionId=dbsid)
    if not close_db_req:
        raise Exception("Can't create CloseDb request")
    return close_db_req


def make_begin_transaction_req(cid, rw):
    begin_transaction_req = client.BeginTransactionReq(client_id=cid, rw=rw)
    if not begin_transaction_req:
        raise Exception("Can't create BeginTransaction request")
    return begin_transaction_req


def make_commit_transaction_req(cid):
    commit_transaction_req = client.CommitTransactionReq(client_id=cid)
    if not commit_transaction_req:
        raise Exception("Can't create CommitTransaction request")
    return commit_transaction_req


def make_abort_transaction_req(cid):
    abort_transaction_req = client.AbortTransactionReq(client_id=cid)
    if not abort_transaction_req:
        raise Exception("Can't create AbortTransaction request")
    return abort_transaction_req


def make_execute_query_req(cid, query):
    execute_query_req = client.ExecuteQueryReq(client_id=cid, query=query)
    if not execute_query_req:
        raise Exception("Can't create ExecuteQuery request")
    return execute_query_req


def make_execute_http_query_req(cid, data, data_length):
    execute_http_query_req = client.ExecuteHttpQueryReq(client_id=cid, data=data, data_length=data_length)
    if not execute_http_query_req:
        raise Exception("Can't create ExecuteHttpQuery request")
    return execute_http_query_req


def make_get_collection_req(cid, colid):
    get_collection_req = client.GetCollectionByNameOrOidReq(client_id=cid, collection_identifier=colid, is_name=True)
    if not get_collection_req:
        raise Exception("Can't create GetCollectionByNameOrOid request")
    return get_collection_req

def make_get_next_mdd_req(cid):
    get_next_mdd_req = client.GetNextMDD(client_id=cid)
    if not get_next_mdd_req:
        raise Exception("Can't create GetNextMDD request")
    return get_next_mdd_req

def make_get_next_tile_req(cid):
    get_next_tile_req = client.GetNextTile(client_id=cid)
    if not get_next_tile_req:
        raise Exception("Can't create GetNextTile")
    return get_next_tile_req

def rasmgr_connect(stub, username, password):
    connection = stub.Connect(make_connect_req(username,password), _TIMEOUT_SECONDS)
    if not connection:
        raise Exception("Remote function 'Connect' did not return anything")
    return connection


def rasmgr_disconnect(stub, cuiid, cid):
    return stub.Disconnect(make_disconnect_req(cuiid, cid), _TIMEOUT_SECONDS)


def rasmgr_keep_alive(stub, cuiid, cid):
    return stub.KeepAliveReq(make_keep_alive_req(cuiid, cid), _TIMEOUT_SECONDS)


def rasmgr_open_db(stub, cuiid, cid, dbname):
    resp = stub.OpenDb(make_open_db_req(cuiid, cid, dbname), _TIMEOUT_SECONDS)
    if not resp:
        raise Exception("Remote function 'OpenDb' did not return anything")
    return resp


def rasmgr_close_db(stub, cuuid, cid, dbsid):
    return stub.CloseDb(make_close_db_req(cuuid, cid, dbsid), _TIMEOUT_SECONDS)


def client_begin_transaction(stub, cid, rw):
    return stub.BeginTransaction(make_begin_transaction_req(cid, rw), _TIMEOUT_SECONDS)


def client_commit_transaction(stub, cid):
    return stub.CommitTransaction(make_commit_transaction_req(cid), _TIMEOUT_SECONDS)


def client_abort_transaction(stub, cid):
    return stub.AbortTransaction(make_abort_transaction_req(cid), _TIMEOUT_SECONDS)


def client_execute_query(stub, cid, query):
    resp = stub.ExecuteQuery(make_execute_query_req(cid, query), _TIMEOUT_SECONDS)
    if not resp:
        raise Exception("Remote function 'ExecuteQuery' did not return anything")
    return resp


def client_execute_http_query(stub, cid, data, data_length):
    resp = stub.ExecuteHttpQuery(make_execute_http_query_req(cid, data, data_length), _TIMEOUT_SECONDS)
    if not resp:
        raise Exception("Remote function 'ExecuteHttpQuery' did not return anything")
    return resp


def client_get_collection_by_name(stub, cid, name):
    resp = stub.GetCollectionByNameOrOid(make_get_collection_req(cid, name), _TIMEOUT_SECONDS)
    if not resp:
        raise Exception("Remote function 'GetCollectionByNameOrOid' did not return anything")
    return resp

def client_get_next_mdd(stub, cid):
    resp = stub.GetNextMDD(cid)
    if not resp:
        raise Exception("Remote function 'GetNextMDD' did not return anything")
    return resp

def client_get_next_tile(stub, cid):
    resp = stub.GetNextTile(cid)
    if not resp:
        raise Exception("Remote function 'GetNextTile' did not return anything")
    return resp

class Connection:
    def __init__(self, hostname="0.0.0.0", port=7001, username="rasguest", password="rasguest"):
        """
        Class to represent the connection from the python client to the rasdaman server
        :param str hostname: the hostname of the rasdaman server
        :param int port: the port on which rasdaman listens on
        """
        self.hostname = hostname
        self.port = port
        self.username = username
        self.passwordHash = hashlib.md5()
        self.passwordHash.update(password)
        self.passwordHash = self.passwordHash.hexdigest()
        self.channel = implementations.insecure_channel(hostname, port)
        self.stub = rasmgr.beta_create_RasMgrClientService_stub(self.channel)
        self.session = rasmgr_connect(self.stub, self.username, self.passwordHash)
        # rasmgr_keep_alive(self.stub, self.username, self.passwordHash)

    def disconnect(self):
        rasmgr_disconnect(self.stub, self.session.clientUUID, self.session.clientID)

    def connect(self):
        self.session = rasmgr_connect(self.stub, self.username, self.passwordHash)
        # rasmgr_keep_alive(self.stub, self.username, self.passwordHash)

    def database(self, name):
        """
        Returns a database object initialized with this connection
        :rtype: Database
        :return: a new database object
        """
        database = Database(self, name)
        return database


class Database:
    def __init__(self, connection, name):
        """
        Class to represent a database stored inside a rasdaman server
        :param Connection connection: the connection to the rasdaman server
        :param str name: the name of the database
        """
        self.connection = connection
        self.name = name
        self.db = rasmgr_open_db(self.connection.stub, self.connection.session.clientUUID, self.connection.session.clientID, self.name)
        self.stub = client.beta_create_ClientRassrvrService_stub(self.connection.channel)

    def open(self):
        self.db = rasmgr_open_db(self.connection.stub, self.connection.session.clientUUID, self.connection.session.clientID, self.name)

    def close(self):
        rasmgr_close_db(self.connection.stub, self.connection.session.clientUUID, self.connection.session.clientID, self.db.dbSessionId)

    def transaction(self, rw=False):
        """
        Returns a new transaction object for this database
        :rtype: Transaction
        :return: a new transaction object
        """
        transaction = Transaction(self, rw)
        return transaction

    def collections(self):
        """
        Returns all the collections for this database
        :rtype: list[Collection]
        """
        transaction = self.transaction()
        query = transaction.query("select r from RAS_COLLECTIONNAMES as r")
        result = query.execute()
        collection = [client_get_collection_by_name(self.stub, self.connection.channel.clientID, name) for name in result]
        return collection


class Collection:
    def __init__(self, transaction, name=None):
        """
        Constructor for the class
        :param Transaction transaction: the transaction for which the collections should be returned
        """
        self.transaction = transaction
        if name:
            self.data = client_get_collection_by_name(self.transaction.database.stub, self.transaction.database.connection.session.clientID, name)


    def name(self):
        """
        Returns the name of the collection
        :rtype: str
        """
        pass

    def arrays(self):
        """
        Return all the arrays in this collection
        :rtype: Array
        """
        pass

    def insert(self, array):
        """
        Inserts an array in the collection
        :param Array array: the array to be inserted
        """
        pass

    def update(self, array):
        """
        Updates the array in the collection
        :param Array array: the array to be updated in the collection
        """


class Transaction:
    def __init__(self, database, rw=False):
        """
        Class to represent a transaction on the selected database
        :param Database database: the database to which the transaction is bound to
        """
        self.database = database
        self.rw = rw
        self.begin_transaction()

    def begin_transaction(self):
        client_begin_transaction(self.database.stub, self.database.connection.channel.clientID, self.rw)

    def commit_transaction(self):
        client_commit_transaction(self.database.stub, self.database.connection.channel.clientID)

    def abort_transaction(self):
        client_abort_transaction(self.database.stub, self.database.connection.channel.clientID)

    def query(self, query_str):
        """
        Returns a new query object initialized with this transaction and the query string
        :param str query_str: the query to be executed
        :rtype: Query
        :return: a new query object
        """
        query = Query(self, query_str)
        return query

    def get_collection(self, name):
        """
        Returns a Collection object using this transaction
        :param name: name of the collection
        :return:
        """
        collection = Collection(self, name)
        if not collection:
            raise Exception("Can't get or create Collection from name")
        return collection


class Query:
    def __init__(self, transaction, query_str):
        """
        Class to represent a rasql query that can be executed in a certain transaction
        :param transaction: the transaction to which the query is bound to
        :param query_str: the query as a string
        """
        self.transaction = transaction
        self.query_str = query_str

    def execute(self, buffer_size=1048576):
        """
        Executes the query and returns back a result
        :return: the resulting array returned by the query
        :rtype: Array
        """
        result = client_execute_query(self.transaction.database.stub, self.transaction.database.connection.channel.clientID, self.query_str)
        if result.status == 0 or result.status == 1:
            pass
        elif result.status == 4 or result.status == 5:
            raise Exception("Error executing query: err_no = " + str(result.err_no) + ", line_no = " + str(result.line_no) + ", col_no = " + str(result.col_no) + ", token = " + result.token)
        mddstatus = 0
        res = []
        while mddstatus == 0:
            array = []
            metadata = []
            mddresp = client_get_next_mdd(self.transaction.database.stub, self.transaction.database.connection.session.clientID)
            mddstatus = mddresp.status
            if mddstatus == 2:
                raise Exception("getMDDCollection - no transfer or empty collection")
            tilestatus = 2
            while tilestatus == 2 or tilestatus == 3:
                tileresp = client_get_next_tile(self.transaction.database.stub, self.transaction.database.connection.session.clientID)
                tilestatus = tileresp.status
                if tilestatus == 4:
                    raise Exception("rpcGetNextTile - no tile to transfer or empty collection")
                array.append((tileresp.data, tileresp.data_length))
            if tilestatus == 0:
                break
            res.append(array)
        return Array(values=res, metadata=result)


class BandType:
    """
    Enum containing possible band types in rasdaman
    """
    INVALID = 0,
    CHAR = 1,
    USHORT = 2,
    SHORT = 3,
    ULONG = 4,
    LONG = 5,
    FLOAT = 6,
    DOUBLE = 7


class SpatialDomain:
    def __init__(self, *interval_parameters):
        """
        Class to represent a spatial domain in rasdaman
        :param list[tuple] interval_parameters: a list of intervals represented as tuples
        """
        pass


class ArrayMetadata:
    def __init__(self, spatial_domain, band_types):
        """
        Class to represent the metadata associated to an array
        :param SpatialDomain spatial_domain: the spatial domain in which the array is represented
        :param dict[str, BandType] band_types: a dictionary containing the name of each band and its type
        """
        self.spatial_domain = spatial_domain
        self.band_types = band_types


class Array:
    def __init__(self, metadata=None, values=None):
        """
        Class to represent an array produced by a rasdaman query
        :param ArrayMetadata metadata: the metadata of the array
        :param list[list[int | float]] values: the values of the array stored band-interleaved (we store a list of
        values for each band)
        """
        self.metadata = metadata
        self.values = values

    def __getitem__(self, item):
        """
        Operator overloading for [], it will slice the array on the first dimension available
        :param slice | int item: the slicing index
        :return: an array with one less dimensions
        :rtype: Array
        """
        pass

    def subset(self, spatial_domain):
        """
        Subsets the array based on the given spatial domain
        :param SpatialDomain spatial_domain: the spatial domain to restrict to
        :rtype: Array
        """
        pass

    def get(self):
        """
        Returns the array contents as a one dimensional list containing a tuple of each band value for the array cell
        :rtype: list[tuple(int | float)]
        """
        pass

    def point(self, *dimension_indices):
        """
        Returns the point at the given position
        :param list[int] dimension_indices: the indices on each of the existing axis. If one of the dimension index is
        not given, an exception should be thrown
        :rtype: int | float
        """

    def toArray(self, type="numpy"):
        """
        Returns the serialized array as a numpy, scipy, or pandas data structure
        :param type: valid option - "numpy", "scipy", "pandas"
        :return:
        """
        if type == "numpy":
            return np.frombuffer(self.values)
        elif type == "scipy":
            return sparse.csr_matrix(np.frombuffer(self.values))
        elif type == "pandas":
            raise NotImplementedError("No Support for Pandas yet")
        else:
            raise NotImplementedError("Invalid type: only valid types are 'numpy' (default), 'scipy', and 'pandas'")