
#ifndef __PRACTICALSOCKET_INCLUDED__
#define __PRACTICALSOCKET_INCLUDED__

#include <string>
#include <exception>

using namespace std;



/**
 *   Used to handle socket exceptions.
 */
class SRPCSocketException : public exception {
public:
  /**
   *   Constructor
   */
	SRPCSocketException(const string &msg, bool isSysMsg = false) throw();

  ~SRPCSocketException() throw();

  /**
   *   Get the exception message
   */
  const char *info() const throw();

private:
  string exceptionInfo;
};


//====================================================================socket



/**
 *   Socket class
 */
class Socket {
public:
	/**
	   *   Destroy constructor
	   */
  ~Socket();

  /**
     *   Get the local address
     */
  string getLocalAddress() throw(SRPCSocketException);

  /**
     *   Get the local port
     */
  unsigned short getLocalPort() throw(SRPCSocketException);

  /**
     *   Set the local port
     */
  void setLocalPort(unsigned short localPort) throw(SRPCSocketException);
//get port dynamically
void setLocalPort1() throw (SRPCSocketException);
  /**
     *   Set the local address and port
     */
  void setLocalAddressAndPort(const string &localAddress, 
    unsigned short localPort = 0) throw(SRPCSocketException);

  /**
     *   Clear all resources
     */
  static void cleanUp() throw(SRPCSocketException);

  /**
     *   Resolve the service for TCP
     */
  static unsigned short resolveService(const string &service,
                                       const string &protocol = "tcp");

private:

  Socket(const Socket &sock);
  void operator=(const Socket &sock);

protected:
  int sockDesc;
  Socket(int type, int protocol) throw(SRPCSocketException);
  Socket(int sockDesc);
};

//===========================================================clientsocket implements socket

/**
 *   ClientSocket
 */
class ClientSocket : public Socket {
public:
	ClientSocket() throw(SRPCSocketException);
	ClientSocket(const string &foreignAddress, unsigned short foreignPort)
	      throw(SRPCSocketException);
	/**
	   *   Establish a socket connection
	   */
  void connectToServer(const string &peerAddress, unsigned short peerPort)
    throw(SRPCSocketException);

  /**
     *   Send data
     */
  void sendData(const void *buffer, int bufferLen) throw(SRPCSocketException);

  void sendDataFile(void *buffer, int bufferLen) throw(SRPCSocketException);
  /**
     *   Receive the response
     */
  int receiveData(void *buffer, int bufferLen) throw(SRPCSocketException);

  /**
     *   Get the peer address.
     */
int receiveData2(void *buffer, int bufferLen) throw(SRPCSocketException);
void close1() throw(SRPCSocketException);
  string getPeerAddress() throw(SRPCSocketException);

  /**
     *   Get the peer port.
     */
  unsigned short getPeerPort() throw(SRPCSocketException);

protected:
  ClientSocket(int type, int protocol) throw(SRPCSocketException);
  friend class ServerSocket;
  ClientSocket(int newConnSD);
};

//========================================================serversocket implements socket

/**
 *   ServerSocket
 */
class ServerSocket : public Socket {
public:
	/**
	   *   Constructor for a TCP socket
	   */
	ServerSocket(unsigned short *localPort, int queueLen = 5)
      throw(SRPCSocketException);

ServerSocket(unsigned short localPort, int queueLen = 5)
      throw(SRPCSocketException);
	/**
	   *   Constructor for a TCP socket
	   */
	ServerSocket(const string &localAddress, unsigned short localPort,
      int queueLen = 5) throw(SRPCSocketException);

	/**
	 *   Listen until there is client request
	 */
	ClientSocket *accept() throw(SRPCSocketException);

private:
  void setListener(int queueLen) throw(SRPCSocketException);
};


#endif
