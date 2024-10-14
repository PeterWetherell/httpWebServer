// **************************************************************************************
// * Web Server (web_server.cc)
// * -- Accepts TCP connections with HTTP and then sends HTML and JPEG files back.
// **************************************************************************************
#include "web_server.h"

using namespace std;

// **************************************************************************************
// * extractFilePath()
// * - Takes the GET request and returns the url from it
// **************************************************************************************
string extractFilePath(const string& httpRequest) {
    //GET + " " + _____ + " " + HTTP
    regex pattern("GET+\\s+(/[^ ]*)\\s+HTTP.");
    smatch matches;
    if (regex_search(httpRequest, matches, pattern)) {
        return matches[1];
    }
    return ""; // No match
}


// **************************************************************************************
// * readRequest(int sockFd, string *filename)
// * - reads the entire header and then returns back with the appropriate code and the url
// **************************************************************************************
int readRequest(int sockFd, string *filename){
  string request = "";
  int size = 0;
  
  int BUFFER_SIZE = 5;
  char buffer[BUFFER_SIZE+1];

  regex closingPattern("\\r\\n\\r\\n");

  while (true) {
    int bytesRead = -1;
		memset(buffer, 0, BUFFER_SIZE+1);    

		if ((bytesRead = read(sockFd, buffer, BUFFER_SIZE)) < 0){
			cout << "Failed to read: " << strerror(errno) << endl;
      return 400;
    }

    if (LOG_LEVEL > 3){
      cout << "Calling read(" << sockFd << ", " << &buffer << ", " << BUFFER_SIZE << ") (web_server.cc:43)" << endl;
      cout << "Received " << bytesRead << " bytes, containing the string \"" << (char *)buffer << "\". (web_server.cc:50)" << endl;
    }

    //Check if the connection was closed -> read 0 bytes
    if (bytesRead == 0) {
        cout << "Client closed the connection." << endl;
        return 400;
    }

    //push the bytes from the connection buffer into the buffer for the command
    request += buffer;
    size += bytesRead;

    //exit when there are /r/n/r/n next to eachother in the buffer
    if (regex_search(request, closingPattern)) {
      break;
    }
  }
  cout << "Received request of size " << size << " bytes: \n" << request << "\n (web_server.cc:77)" << endl;

  //we have now read the entirety of the header and put it into the request buffer

  //Check if it is a get request
  if (request[0] == 'G'&& request[1] == 'E' && request[2] == 'T'){
    //find the filename
    string str(request);
    *filename = extractFilePath(str); //yoink the filename
    cout << "Asked for file " << *filename << endl;

    //Check if this is a valid filename style
    //fileX.html
    //imageX.jpg
    string cwdString = filesystem::current_path().string();
    cout << "current file path: " << cwdString << endl;
    regex filePathPattern(cwdString + "/file\\d\\.html"); //Don't use \d+ because that allows for more than 1 digit -> explicitly what the rubric says
    regex imagePathPattern(cwdString + "/image\\d\\.jpg");

    if (regex_search(*filename, filePathPattern) || regex_search(*filename, imagePathPattern)) {
      cout << "file matches the correct format" << endl;
      return 200; //return with the filename assuming that it is correct
    }
    cout << "file didn't match the correct format" << endl;
    return 404; //filename doesn't match the pattern -> 404
  } 
  
  // If its not a get request we say that it was malformatted.
  return 400;
}

// ****************************************************************************
// * sendLine()
// * Send one line (including the line terminator <LF><CR>)
// ****************************************************************************
void sendLine(int socketFD, const char* stringToSend, int size){

  if (write (socketFD, stringToSend, size) < 0){
    cout << "Failed to send data back: " << strerror(errno) << endl;
  }
  if (LOG_LEVEL > 3){
    cout << "Wrote " << size << " bytes back to client (web_server.cc:118)" << endl;
  }
}

void send404(int sockFd){
  sendLine(sockFd, "HTTP/1.0 404 Not Found\r\nContent-Type: text/html\r\nContent-Length: 92\r\n\r\n<html><body><h1>404 Not Found</h1><p>The requested resource was not found.</p></body></html>", 163);
}

void send400(int sockFd){
  sendLine(sockFd, "HTTP/1.0 400 Bad Request\r\nContent-Type: text/html\r\nContent-Length: 108\r\n\r\n<html><body><h1>400 Bad Request</h1><p>Your request could not be understood by the server.</p></body></html>", 182);
}

void send200(int sockFd, string &filename){

  struct stat fileStat;
  if (stat(filename.c_str(), &fileStat) < 0){ //404 if we cannot access the file
    send404(sockFd);
    return;
  }
  int size = fileStat.st_size; // get the file size

  sendLine(sockFd, "HTTP/1.0 200 OK\r\nContent-Type: ", 31);

  
  string cwdString = filesystem::current_path().string();
  regex filePathPattern(cwdString + "/file\\d\\.html");
  if (regex_search(filename, filePathPattern)){ //am file -> text
    sendLine(sockFd, "text/html", 9);
  }
  else {
    sendLine(sockFd, "image/jpeg", 10); //otherwise am jpg -> image
  }

  sendLine(sockFd, "\r\nContent-Length: ", 18);
  sendLine(sockFd, to_string(size).c_str(), to_string(size).length());
  sendLine(sockFd, "\r\n\r\n", 4);

  int BUFFER_SIZE = 10;
  char buffer[BUFFER_SIZE];
  int bytesRead = -1;

  int fd = open(filename.c_str(), O_RDONLY); //get the FD for the file

  while ((bytesRead = read(fd, buffer, BUFFER_SIZE)) > 0){
    sendLine(sockFd, buffer, bytesRead);
    memset(buffer, 0, BUFFER_SIZE);
  }
  
  close(fd);
}

// **************************************************************************************
// * processConnection()
// * - Handles reading the line from the network and sending it back to the client.
// * - calls the readRequest and sends the appropriate 400, 404, or 200 back to the client
// **************************************************************************************
void processConnection(int sockFd) {
  string filename;
  int code = readRequest(sockFd, &filename);
  switch (code){
    case 200: send200(sockFd, filename); break;
    case 404: send404(sockFd); break;
    case 400: send400(sockFd); break;
  }
}
    


// **************************************************************************************
// * main()
// * - Sets up the sockets and accepts new connection until processConnection() returns 1
// **************************************************************************************

int main (int argc, char *argv[]) {

  // ********************************************************************
  // * Process the command line arguments
  // ********************************************************************
  int opt = 0;
  while ((opt = getopt(argc,argv,"d:")) != -1) {
    
    switch (opt) {
    case 'd':
      LOG_LEVEL = std::stoi(optarg);;
      break;
    case ':':
    case '?':
    default:
      std::cout << "useage: " << argv[0] << " -d <num>" << std::endl;
      exit(-1);
    }
  }

  // *******************************************************************
  // * Creating the inital socket is the same as in a client.
  // ********************************************************************
  int listenFd = -1;
  // Call socket() to create the socket you will use for lisening.
	if ((listenFd = socket(AF_INET, SOCK_STREAM,0)) < 0){
		cout << "Failed to create listening socket: " << strerror(errno) << endl;
		exit(-1);
	}
  if (LOG_LEVEL > 3){
    cout << "Calling Socket() assigned file descriptor " << listenFd << " (web_server.cc:221)" << endl;
  }
  
  // ********************************************************************
  // * The bind() and calls take a structure that specifies the
  // * address to be used for the connection. On the cient it contains
  // * the address of the server to connect to. On the server it specifies
  // * which IP address and port to lisen for connections.
  // ********************************************************************
  struct sockaddr_in servaddr;
  memset(&servaddr, 0, sizeof(servaddr));

  // *** assign 3 fields in the servadd struct sin_family, sin_addr.s_addr and sin_port
  // *** the value your port can be any value > 1024.

	servaddr.sin_family = AF_INET;

	servaddr.sin_addr.s_addr = htonl(INADDR_ANY);

  int port = 1024;

	servaddr.sin_port = htons(port);

  // ********************************************************************
  // * Binding configures the socket with the parameters we have
  // * specified in the servaddr structure.  This step is implicit in
  // * the connect() call, but must be explicitly listed for servers.
  // ********************************************************************
  bool bindSuccesful = false;
  while (!bindSuccesful) {
    // ** Call bind()
    // You may have to call bind multiple times if another process is already using the port
    // your program selects.
    if (LOG_LEVEL > 3){
      cout << "Calling bind(" << listenFd << ", " << &servaddr << ", " << sizeof(servaddr) << ") on port: " << port << " (web_server.cc:255)" << endl;
    }
    if (bind(listenFd, (sockaddr *) &servaddr, sizeof(servaddr)) < 0){
      cout << "port " << port << " not avialable" << endl;
      port ++;
	    servaddr.sin_port = htons(port);
    }
    else{
      bindSuccesful = true;
    }
  }

  // *** DON'T FORGET TO PRINT OUT WHAT PORT YOUR SERVER PICKED SO YOU KNOW HOW TO CONNECT.
  cout << "Using port: " << port << endl;

  // ********************************************************************
  // * Setting the socket to the listening state is the second step
  // * needed to being accepting connections.  This creates a queue for
  // * connections and starts the kernel listening for connections.
  // ********************************************************************
  int listenQueueLength = 1;
  // ** Call listen()

  if (LOG_LEVEL > 3){
    cout << "Calling listen(" << listenFd << ", " << listenQueueLength << ") (web_server.cc:279)" << endl;
  }
	if (listen(listenFd, listenQueueLength) < 0){
		cout << "listen() failed: " << strerror(errno) << endl;
		exit(-1);
	}

  // ********************************************************************
  // * The accept call will sleep, waiting for a connection.  When 
  // * a connection request comes in the accept() call creates a NEW
  // * socket with a new fd that will be used for the communication.
  // ********************************************************************
  while (true) {
    int connFd = 0;

    // Call the accept() call to check the listening queue for connection requests.
    // If a client has already tried to connect accept() will complete the
    // connection and return a file descriptor that you can read from and
    // write to. If there is no connection waiting accept() will block and
    // not return until there is a connection.
    // ** call accept() 
    if (LOG_LEVEL > 3){
      cout << "Calling accept(" << listenFd << ", NULL, NULL) (web_server.cc:301)" << endl;
    }
		if ((connFd = accept(listenFd, (sockaddr *) NULL, NULL)) < 0){
			cout << "accept() failed: " << strerror(errno) << endl;
			exit(-1);
		}
    if (LOG_LEVEL > 3){
      cout << "We have a connection on " << connFd << " (web_server.cc:308)" << endl;
    }
    
    // Now we have a connection, so you can call processConnection() to do the work.
    processConnection(connFd);
   
    close(connFd);
  }

  close(listenFd);

}
