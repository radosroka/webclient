#include <iostream>
#include <string>
#include <cstring>
#include <cstdlib>

#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <unistd.h>

#include <vector>
#include <fstream>

using namespace std;

const int BUFFER_SIZE = 1024;

int main(int argc, char *argv[]){

	if (argc != 2) return EXIT_FAILURE;

	string arg = &argv[1][7];
	string port = "80";
	string path = "/";
	string file = "";

	string out_file = "";

	unsigned long int pos = 0;
	if ((pos = arg.find_first_of("/")) != string::npos){
		path = arg.substr(pos, arg.length());
		arg = arg.substr(0, pos);
	}

	pos = 0;
	if ((pos = arg.find_last_of(":")) != string::npos){
		port = arg.substr(pos+1, arg.length());
		arg = arg.substr(0, pos);
	}

	if (path[path.length()-1] == '/'){
		file = "";
		out_file = "index.html";
	}
	else if (path[0] == '/'){
		pos = path.find_last_of("/");
		file = path.substr(pos+1, path.length());
		path = path.substr(0, pos+1);
		out_file = file;
	}

	cout << arg << endl;
	cout << port << endl;
	cout << path << endl;
	cout << file << endl;

	int status;
	struct addrinfo host_info;
  	struct addrinfo *host_info_list, *i;

  	memset(&host_info, 0, sizeof host_info);
 
	cout << "Setting up the structs..."  << endl;
 
	host_info.ai_family = AF_UNSPEC;
	host_info.ai_socktype = SOCK_STREAM;

	status = getaddrinfo(arg.c_str(), port.c_str(), &host_info, &host_info_list);
	if (status != 0){
		cerr << "getaddrinfo error: " << gai_strerror(status) << endl;
		return EXIT_FAILURE;
	}

	int socketfd ;
	for (i = host_info_list; i != NULL; i = i->ai_next){
		cout << "Creating a socket..."  << endl;
		socketfd = socket(host_info_list->ai_family, host_info_list->ai_socktype, host_info_list->ai_protocol);
		if (socketfd == -1)
			continue;
		
		cout << "Connect()ing..."  << endl;
		status = connect(socketfd, host_info_list->ai_addr, host_info_list->ai_addrlen);
		if (status != -1)
			break;

		close(socketfd);
	}

	if (i == NULL){
		cerr << "couldn't connect\n";
		return EXIT_FAILURE;
	}


	cout << "send()ing message..."  << endl;
	string msg = "GET " + path + file + " HTTP/1.1\r\nhost: "
				 +  arg + "\r\nConnection: close\r\n" +
				 "\r\nAccept-Charset: ISO-8859-1,UTF-8;q=0.7,*;q=0.7\r\n\r\n";
	cout << msg;
	ssize_t bytes_sent;
	bytes_sent = send(socketfd, msg.c_str(), msg.length(), 0);
	cout << "bytes send : " << bytes_sent << endl ;
 

	cout << "Waiting to recieve data..."  << endl;
	ssize_t bytes_recieved = 0;
	ssize_t bytes;
	string data;
	char incoming_data_buffer[BUFFER_SIZE];
	
	while ((bytes = recv(socketfd, incoming_data_buffer, BUFFER_SIZE, 0))){
		bytes_recieved += bytes;
		if (bytes == 0) cout << "host shut down." << endl ;
		else if (bytes == -1){
			cerr << "recieve error!" << endl ;
			return EXIT_FAILURE;
		}
		else data.append(incoming_data_buffer, bytes);
	}
	cout << "bytes recieved : " << bytes_recieved << endl << endl ;

	string header = data.substr(0, data.find("\r\n\r\n"));
	string body = data.substr(data.find("\r\n\r\n")+4);

	cout << endl << header << endl;

	ofstream output;
	output.open(out_file, ios::out | ios::binary);
	output << body;
	output.close();

	return EXIT_SUCCESS;
}
