#include "Server.h"
#include "IPCProcs.h"
#include "MasterDB.h"

IPC_PROCEDURE_BINDING(OnWorldVerifyPassword, IPC_WORLD_REQVERIFYPASSWORD, IPC_DATA_WORLD_REQVERIFYPASSWORD) {
    IPC_DATA_AUTH_REQVERIFYPASSWORD* Request = PacketBufferInitExtended(Context->AuthSocket->PacketBuffer, IPC, AUTH_REQVERIFYPASSWORD);
	Request->ConnectionID = Packet->ConnectionID;
	Request->AccountID = Packet->AccountID;
	memcpy(Request->Credentials, Packet->Credentials, MAX_PASSWORD_LENGTH);
	return SocketSendAll(Context->AuthSocket, Request);
}

IPC_PROCEDURE_BINDING(OnAuthVerifyPassword, IPC_AUTH_ACKVERIFYPASSWORD, IPC_DATA_AUTH_ACKVERIFYPASSWORD) {
	IPC_DATA_WORLD_ACKVERIFYPASSWORD* Response = PacketBufferInitExtended(Context->WorldSocket->PacketBuffer, IPC, WORLD_ACKVERIFYPASSWORD);
	Response->ConnectionID = Packet->ConnectionID;
	Response->Success = Packet->Success;
	return SocketSendAll(Context->WorldSocket, Response);
}
