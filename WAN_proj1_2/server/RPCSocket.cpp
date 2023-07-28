


#include "RPCSocket.h"

#ifdef WIN32
#include <winsock.h>

typedef int socklen_t;
typedef char raw_type;
#else
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <netinet/in.h>
#include <cstring>

typedef void raw_type;
#endif

#include <iostream>
#include <cstdlib>
#include <errno.h>

using namespace std;

#ifdef WIN32
static bool initialized = false;
#endif

// ==============================================================SRPCSocketException Code

SRPCSocketException::SRPCSocketException(const string &msg,
		bool isSysMsg) throw () :
		exceptionInfo(msg) {
	if (isSysMsg) {
		exceptionInfo.append(": ");
		exceptionInfo.append(strerror(errno));
	}
}

SRPCSocketException::~SRPCSocketException() throw () {
	
}

const char *SRPCSocketException::info() const throw () {
	return exceptionInfo.c_str();
}



static void fillAddr(const string &address, unsigned short port,
		sockaddr_in &addr) {

	memset(&addr, 0, sizeof(addr));
	addr.sin_family = AF_INET;

	hostent *host;
	if ((host = gethostbyname(address.c_str())) == NULL) {
		throw SRPCSocketException("Failed to resolve name.");
	}
	addr.sin_addr.s_addr = *((unsigned long *) host->h_addr_list[0]);

	addr.sin_port = htons(port);     // Assign port in network byte order
}

// Socket Code

Socket::Socket(int type, int protocol) throw (SRPCSocketException) {
#ifdef WIN32
	if (!initialized) {
		WORD wVersionRequested;
		WSADATA wsaData;

		wVersionRequested = MAKEWORD(2, 0);              // Request WinSock v2.0
		if (WSAStartup(wVersionRequested, &wsaData) != 0) { // Load WinSock DLL
			throw SocketException("Unable to load WinSock DLL");
		}
		initialized = true;
	}
#endif

	// Make a new socket
	if ((sockDesc = socket(PF_INET, type, protocol)) < 0) {
		throw SRPCSocketException("Failed to initiate socket.", true);
	}
}

Socket::Socket(int sockDesc) {
	this->sockDesc = sockDesc;
}

Socket::~Socket() {
#ifdef WIN32
	::closesocket(sockDesc);
#else
	::close(sockDesc);
#endif
	sockDesc = -1;
}

string Socket::getLocalAddress() throw (SRPCSocketException) {

	sockaddr_in addr;
	unsigned int addr_len = sizeof(addr);

	if (getsockname(sockDesc, (sockaddr *) &addr, (socklen_t *) &addr_len)
			< 0) {
		throw SRPCSocketException("Failed to get local address.",
				true);
	}
	return inet_ntoa(addr.sin_addr);
}

unsigned short Socket::getLocalPort() throw (SRPCSocketException) {
	sockaddr_in addr;
	unsigned int addr_len = sizeof(addr);

	if (getsockname(sockDesc, (sockaddr *) &addr, (socklen_t *) &addr_len)
			< 0) {
		throw SRPCSocketException("Failed to get local port.",
				true);
	}
	return ntohs(addr.sin_port);
}

void Socket::setLocalPort(unsigned short localPort) throw (SRPCSocketException) {
	sockaddr_in localAddr;

	memset(&localAddr, 0, sizeof(localAddr));
	localAddr.sin_family = AF_INET;
	localAddr.sin_addr.s_addr = htonl(INADDR_ANY);
	localAddr.sin_port = htons(localPort);

	if (bind(sockDesc, (sockaddr *) &localAddr, sizeof(sockaddr_in)) < 0) {
		throw SRPCSocketException("Fail to set local port.", true);
	}
}


void Socket::setLocalPort1() throw (SRPCSocketException) 
{
	sockaddr_in localAddr;

	memset(&localAddr, 0, sizeof(localAddr));
	localAddr.sin_family = AF_INET;
	localAddr.sin_addr.s_addr = htonl(INADDR_ANY);
	localAddr.sin_port = htons(0);

	if (bind(sockDesc, (sockaddr *) &localAddr, sizeof(sockaddr_in)) < 0) {
		throw SRPCSocketException("Fail to set local port.", true);
	}
}

void Socket::setLocalAddressAndPort(const string &localAddress,
		unsigned short localPort) throw (SRPCSocketException) {
	sockaddr_in localAddr;
	fillAddr(localAddress, localPort, localAddr);

	if (bind(sockDesc, (sockaddr *) &localAddr, sizeof(sockaddr_in)) < 0) {
		throw SRPCSocketException("Failed to set local address and port.",
				true);
	}
}

void Socket::cleanUp() throw (SRPCSocketException) {
#ifdef WIN32
	if (WSACleanup() != 0) {
		throw SocketException("WSACleanup() failed");
	}
#endif
}

unsigned short Socket::resolveService(const string &service,
		const string &protocol) {
	struct servent *serv;

	if ((serv = getservbyname(service.c_str(), protocol.c_str())) == NULL)
		return atoi(service.c_str());
	else
		return ntohs(serv->s_port);
}


ClientSocket::ClientSocket() throw (SRPCSocketException) :
		Socket(SOCK_STREAM,
		IPPROTO_TCP) {
}

ClientSocket::ClientSocket(const string &foreignAddress, unsigned short foreignPort)
		throw (SRPCSocketException) :
		Socket(SOCK_STREAM, IPPROTO_TCP) {
	connectToServer(foreignAddress, foreignPort);
}

ClientSocket::ClientSocket(int type, int protocol)
		throw (SRPCSocketException) :
		Socket(type, protocol) {
}

ClientSocket::ClientSocket(int newConnSD) :
		Socket(newConnSD) {
}

void ClientSocket::connectToServer(const string &peerAddress,
		unsigned short peerPort) throw (SRPCSocketException) {
	sockaddr_in destAddr;
	fillAddr(peerAddress, peerPort, destAddr);

	// Connect to the server
	if (::connect(sockDesc, (sockaddr *) &destAddr, sizeof(destAddr)) < 0) {
		throw SRPCSocketException("Fail to connect to peer.", true);
	}
}

void ClientSocket::sendData(const void *buffer, int bufferLen)
		throw (SRPCSocketException) {
	if (::send(sockDesc, (raw_type *) buffer, bufferLen, 0) < 0) {
		throw SRPCSocketException("Failed to send data to peer.", true);
	}
}
void ClientSocket::sendDataFile(void *buffer, int bufferLen)
		throw (SRPCSocketException) {
	if (::send(sockDesc, (raw_type *) buffer, bufferLen, 0) < 0) {
		throw SRPCSocketException("Failed to send data to peer.", true);
	}
}
int ClientSocket::receiveData(void *buffer, int bufferLen)
		throw (SRPCSocketException) {
	int rtn;
	if ((rtn = ::recv(sockDesc, (raw_type *) buffer, bufferLen, 0)) < 0) {
		throw SRPCSocketException("Failed to receive data from peer.", true);
	}
	return rtn;
}

int ClientSocket::receiveData2(void *buffer, int bufferLen)
		throw (SRPCSocketException) {
	struct timeval tv;

	tv.tv_sec = 5;  /* 30 Secs Timeout */
	tv.tv_usec = 0;  // Not init'ing this can cause strange errors

	setsockopt(sockDesc, SOL_SOCKET, SO_RCVTIMEO, (char *)&tv,sizeof(struct timeval));
	int rtn;
	if ((rtn = ::recv(sockDesc, (raw_type *) buffer, bufferLen, 0)) <0) {
		//cout<<"Timeout failed";
		
		throw SRPCSocketException("Failed to receive data from peer.", true);
	}
	return rtn;
}
void ClientSocket::close1() throw (SRPCSocketException)
{
	close(sockDesc);
}

string ClientSocket::getPeerAddress() throw (SRPCSocketException) {
	sockaddr_in addr;
	unsigned int addr_len = sizeof(addr);

	if (getpeername(sockDesc, (sockaddr *) &addr, (socklen_t *) &addr_len)
			< 0) {
		throw SRPCSocketException("Getting peer's address failed.",
				true);
	}
	return inet_ntoa(addr.sin_addr);
}

unsigned short ClientSocket::getPeerPort() throw (SRPCSocketException) {
	sockaddr_in addr;
	unsigned int addr_len = sizeof(addr);

	if (getpeername(sockDesc, (sockaddr *) &addr, (socklen_t *) &addr_len)
			< 0) {
		throw SRPCSocketException("Getting peer's port failed.",
				true);
	}
	return ntohs(addr.sin_port);
}


// server socket code

ServerSocket::ServerSocket(unsigned short localPort, int queueLen)
		throw (SRPCSocketException) :
		Socket(SOCK_STREAM, IPPROTO_TCP) {
	setLocalPort(localPort);
	setListener(queueLen);
	
	
}

ServerSocket::ServerSocket(unsigned short *localPort, int queueLen)
		throw (SRPCSocketException) :
		Socket(SOCK_STREAM, IPPROTO_TCP) {
	setLocalPort1();
	setListener(queueLen);
	*localPort = getLocalPort();
	
}

ServerSocket::ServerSocket(const string &localAddress,
		unsigned short localPort, int queueLen) throw (SRPCSocketException) :
		Socket(SOCK_STREAM, IPPROTO_TCP) {
	setLocalAddressAndPort(localAddress, localPort);
	setListener(queueLen);
}

ClientSocket *ServerSocket::accept() throw (SRPCSocketException) {
	int newConnSD;
	if ((newConnSD = ::accept(sockDesc, NULL, 0)) < 0) {
		throw SRPCSocketException("Failed to accept peer's request.", true);
	}

	return new ClientSocket(newConnSD);
}

void ServerSocket::setListener(int queueLen) throw (SRPCSocketException) {
	if (listen(sockDesc, queueLen) < 0) {
		throw SRPCSocketException("Failed to set socket listener.", true);
	}
}
