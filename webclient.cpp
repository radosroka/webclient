#include <iostream>
#include <string>
#include <cstring>
#include <cstdlib>

#include <sys/socket.h>
#include <netdb.h>

using namespace std;

const int BUFFER_SIZE = 1024;

int main(int argc, char *argv[]){

	if (argc != 2) return EXIT_FAILURE;

	string arg = argv[1];
	arg = "fit.vutbr.cz";

	int status;
	struct addrinfo host_info;
  	struct addrinfo *host_info_list;

  	memset(&host_info, 0, sizeof host_info);
 
	cout << "Setting up the structs..."  << endl;
 
	host_info.ai_family = AF_UNSPEC;
	host_info.ai_socktype = SOCK_STREAM;

	status = getaddrinfo(arg.c_str(), "80", &host_info, &host_info_list);
	if (status != 0){
		cerr << "getaddrinfo error: " << gai_strerror(status) << endl;
		return EXIT_FAILURE;
	}

	cout << "Creating a socket..."  << endl;
	int socketfd ;
	socketfd = socket(host_info_list->ai_family, host_info_list->ai_socktype, host_info_list->ai_protocol);
	if (socketfd == -1){
		cout << "socket error" << endl;
	}

    cout << "Connect()ing..."  << endl;
	status = connect(socketfd, host_info_list->ai_addr, host_info_list->ai_addrlen);
	if (status == -1){
		cout << "connect error" << endl;
	}

	cout << "send()ing message..."  << endl;
	string msg = "GET / HTTP/1.1\nhost: " +  arg;
	ssize_t bytes_sent;
	bytes_sent = send(socketfd, msg.c_str(), msg.length(), 0);
	cout << "bytes send : " << bytes_sent << endl ;
 

	cout << "Waiting to recieve data..."  << endl;
	ssize_t bytes_recieved;
	char incoming_data_buffer[1000];
	
	bytes_recieved = recv(socketfd, incoming_data_buffer,1000, 0);

	if (bytes_recieved == 0) cout << "host shut down." << endl ;
	if (bytes_recieved == -1){
		cerr << "recieve error!" << endl ;
		return EXIT_FAILURE;
	}
	cout << "bytes recieved : " << bytes_recieved << endl ;
	cout << incoming_data_buffer << endl;

	return EXIT_SUCCESS;
}
