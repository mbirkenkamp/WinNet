#include "Server.h"

NET_SERVER_BEGIN_DATA_PACKAGE(Server)
NET_SERVER_DEFINE_PACKAGE(Test, Packages::PKG_TEST);
NET_SERVER_END_DATA_PACKAGE

void Server::Tick() {}
void Server::OnPeerConnect(NET_PEER peer) {}
void Server::OnPeerEstabilished(NET_PEER peer) {}
void Server::OnPeerDisconnect(NET_PEER peer) {}
void Server::OnPeerUpdate(NET_PEER peer) {}

NET_BEGIN_FNC_PKG(Server, Test)
LOG("Received Package from Client");
//const auto data = pkg.RawData("test");
//if (!data.valid()) return;

//NET_FILEMANAGER fmanager("s.exe", NET_FILE_READWRITE);
//fmanager.write(data.value(), data.size());

NET_FILEMANAGER fmanager("test.exe", NET_FILE_READ);

byte* data = nullptr;
size_t size = 0;
if (!fmanager.read(data, size)) return;

Package kk;
kk.AppendRawData("test", data, size);
DoSend(PEER, Packages::PKG_TEST, kk);
NET_END_FNC_PKG