#include "AuthDB.h"
#include "Context.h"
#include "ClientSocket.h"
#include "ClientProcedures.h"
#include "IPCProtocol.h"
#include "IPCProcedures.h"

#define C2S_COMMAND(__NAME__, __COMMAND__)                  \
Void SERVER_PROC_ ## __NAME__(                              \
    ServerRef Server,                                       \
    Void *ServerContext,                                    \
    SocketRef Socket,                                       \
    SocketConnectionRef Connection,                         \
    Void *ConnectionContext,                                \
    Void *Packet                                            \
) {                                                         \
    PROC_ ## __NAME__(                                      \
        Server,                                             \
        (ServerContextRef)ServerContext,                    \
        Socket,                                             \
        Connection,                                         \
        (ClientContextRef)ConnectionContext,                \
        (C2S_DATA_ ## __NAME__*)Packet                      \
    );                                                      \
}
#include "ClientCommands.h"

#define IPC_COMMAND_CALLBACK(__NAMESPACE__, __NAME__)               \
Void SERVER_IPC_ ## __NAMESPACE__ ## _PROC_ ## __NAME__(            \
    IPCSocketRef Socket,                                            \
    IPCSocketConnectionRef Connection,                              \
    IPCPacketRef Packet                                             \
) {                                                                 \
    ServerRef Server = (ServerRef)Socket->Userdata;                 \
    ServerContextRef Context = (ServerContextRef)Server->Userdata;  \
    SocketConnectionRef ClientConnection = NULL;                    \
    ClientContextRef Client = NULL;                                 \
                                                                    \
    if (Packet->TargetConnectionID > 0) {                           \
        ClientConnection = SocketGetConnection(                     \
            Context->ClientSocket,                                  \
            Packet->TargetConnectionID                              \
        );                                                          \
        if (!ClientConnection) return;                              \
        Client = (ClientContextRef)ClientConnection->Userdata;      \
    }                                                               \
                                                                    \
    IPC_ ## __NAMESPACE__ ## _PROC_ ## __NAME__(                    \
        Server,                                                     \
        Context,                                                    \
        Socket,                                                     \
        Connection,                                                 \
        ClientConnection,                                           \
        Client,                                                     \
        (IPC_ ## __NAMESPACE__ ## _DATA_ ## __NAME__ *)Packet       \
    );                                                              \
}

#define IPC_M2L_COMMAND(__NAME__) IPC_COMMAND_CALLBACK(M2L, __NAME__)
#include "IPCCommands.h"

#define IPC_W2L_COMMAND(__NAME__) IPC_COMMAND_CALLBACK(W2L, __NAME__)
#include "IPCCommands.h"

Void ServerOnUpdate(
    ServerRef Server,
    Void *ServerContext
) {
    ServerContextRef Context = (ServerContextRef)ServerContext;
    SocketConnectionIteratorRef Iterator = SocketGetConnectionIterator(Context->ClientSocket);
    while (Iterator) {
        SocketConnectionRef Connection = SocketConnectionIteratorFetch(Context->ClientSocket, Iterator);
        Iterator = SocketConnectionIteratorNext(Context->ClientSocket, Iterator);

        ClientContextRef Client = (ClientContextRef)Connection->Userdata;
        if (Client->Flags & CLIENT_FLAGS_CHECK_DISCONNECT_TIMER) {
            if (Client->DisconnectTimestamp < ServerGetTimestamp(Server)) {
                Client->Flags &= ~CLIENT_FLAGS_CHECK_DISCONNECT_TIMER;
                SocketDisconnect(Context->ClientSocket, Connection);
                continue;
            }
        }
    }

    Timestamp CurrentTimestamp = GetTimestampMs();
    if (Context->WorldListBroadcastTimestamp < CurrentTimestamp) {
        Context->WorldListBroadcastTimestamp = CurrentTimestamp + Context->Config.Login.WorldListBroadcastInterval;
        IPC_L2M_DATA_GET_WORLD_LIST* Request = IPCPacketBufferInit(Server->IPCSocket->PacketBuffer, L2M, GET_WORLD_LIST);
        Request->Header.Source = Server->IPCSocket->NodeID;
        Request->Header.Target.Group = 1;
        Request->Header.Target.Type = IPC_TYPE_MASTER;
        IPCSocketUnicast(Server->IPCSocket, Request);
    }
}

Int32 main(Int32 argc, CString* argv) {
    DiagnosticCreateLogFile("LoginSvr");

    Char Buffer[MAX_PATH] = { 0 };
    CString WorkingDirectory = PathGetCurrentDirectory(Buffer, MAX_PATH);
    CString ConfigFilePath = PathCombineNoAlloc(WorkingDirectory, "LoginSvr.ini");
    ServerConfig Config = ServerConfigLoad(ConfigFilePath);

    AllocatorRef Allocator = AllocatorGetSystemDefault();
    struct _ServerContext ServerContext = { 0 };
    ServerContext.Allocator = Allocator;
    ServerContext.Config = Config;
    ServerContext.ClientSocket = NULL;
    ServerContext.Database = NULL;
    ServerContext.WorldListBroadcastTimestamp = 0;
    ServerContext.WorldServerTable = IndexDictionaryCreate(Allocator, 256);

    IPCNodeID NodeID = kIPCNodeIDNull;
    NodeID.Group = 1;
    NodeID.Index = 0;
    NodeID.Type = IPC_TYPE_LOGIN;

    ServerRef Server = ServerCreate(
        Allocator,
        NodeID,
        Config.MasterSvr.Host,
        Config.MasterSvr.Port,
        Config.MasterSvr.Timeout,
        Config.NetLib.ReadBufferSize,
        Config.NetLib.WriteBufferSize,
        &ServerOnUpdate,
        &ServerContext
    );

    ServerContext.ClientSocket = ServerCreateSocket(
        Server,
        SOCKET_FLAGS_LISTENER | SOCKET_FLAGS_ENCRYPTED,
        NULL,
        Config.Login.Port,
        sizeof(struct _ClientContext),
        Config.NetLib.ProtocolIdentifier,
        Config.NetLib.ProtocolVersion,
        Config.NetLib.ProtocolExtension,
        Config.NetLib.ReadBufferSize,
        Config.NetLib.WriteBufferSize,
        Config.Login.MaxConnectionCount,
        &ClientSocketOnConnect,
        &ClientSocketOnDisconnect
    );

#define C2S_COMMAND(__NAME__, __COMMAND__) \
    ServerSocketRegisterPacketCallback(Server, ServerContext.ClientSocket, __COMMAND__, &SERVER_PROC_ ## __NAME__);
#include "ClientCommands.h"

#define IPC_M2L_COMMAND(__NAME__) \
    IPCSocketRegisterCommandCallback(Server->IPCSocket, IPC_M2L_ ## __NAME__, &SERVER_IPC_M2L_PROC_ ## __NAME__);
#include "IPCCommands.h"

#define IPC_W2L_COMMAND(__NAME__) \
    IPCSocketRegisterCommandCallback(Server->IPCSocket, IPC_W2L_ ## __NAME__, &SERVER_IPC_W2L_PROC_ ## __NAME__);
#include "IPCCommands.h"

    ServerContext.Database = DatabaseConnect(
        Config.AuthDB.Host,
        Config.AuthDB.Username,
        Config.AuthDB.Password,
        Config.AuthDB.Database,
        Config.AuthDB.Port,
        Config.AuthDB.AutoReconnect
    );
    if (!ServerContext.Database) FatalError("Database connection failed");

    AuthDBPrepareStatements(ServerContext.Database);
    ServerRun(Server);
    ServerDestroy(Server);
    DatabaseDisconnect(ServerContext.Database);
    DiagnosticTeardown();

    return EXIT_SUCCESS;
}