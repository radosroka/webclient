#include <iostream>
#include <string>
#include <cstring>
#include <cstdlib>

#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <unistd.h>

#include <locale> 
#include <fstream>

#include <map>

using namespace std;

const int BUFFER_SIZE = 1024;


string escape(const string address){
	string arg = "";
	for(int i = 0 ; address[i] != 0 ; i++){
		switch (address[i]){
			case ' ':
				arg += "%20";
				break;
			case '~':
				arg += "%7E";
				break;
			default:
				arg += address[i];
		} 
	}
	return arg;
}


int get(const char * address, int counter, map<string, string> &redir, char version = '1'){

	if (counter > 5){
		cerr << "Error: More than 5 redirections" << endl;
		return EXIT_FAILURE;
	}

	locale loc;

	string arg = &address[7];
	string port = "80";
	string path = "/";
	string file = "";

	string out_file = "";

	unsigned long int pos = 0;
	if ((pos = arg.find_first_of("?")) != string::npos){
		arg = arg.substr(0, pos);
	}

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
		out_file = path.substr(pos+1, path.length());
		path = escape(path.substr(0, pos+1));
		file = escape(out_file);
	}

	int status;
	struct addrinfo host_info;
  	struct addrinfo *host_info_list, *i;

  	memset(&host_info, 0, sizeof host_info);
 
	host_info.ai_family = AF_UNSPEC;
	host_info.ai_socktype = SOCK_STREAM;

	status = getaddrinfo(arg.c_str(), port.c_str(), &host_info, &host_info_list);
	if (status != 0){
		cerr << "getaddrinfo error: " << gai_strerror(status) << endl;
		return EXIT_FAILURE;
	}

	int socketfd ;
	for (i = host_info_list; i != NULL; i = i->ai_next){
		socketfd = socket(host_info_list->ai_family, host_info_list->ai_socktype, host_info_list->ai_protocol);
		if (socketfd == -1)
			continue;

		status = connect(socketfd, host_info_list->ai_addr, host_info_list->ai_addrlen);
		if (status != -1)
			break;

		close(socketfd);
	}

	if (i == NULL){
		cerr << "couldn't connect\n";
		freeaddrinfo(host_info_list);
		return EXIT_FAILURE;
	}


	string msg = "GET " + path + file + " HTTP/1." + version + "\r\nhost: "
				 +  arg + "\r\nConnection: close\r\n" +
				 "Accept-Charset: ISO-8859-1,UTF-8;q=0.7,*;q=0.7\r\n\r\n";

	send(socketfd, msg.c_str(), msg.length(), 0);
 
	ssize_t bytes_recieved = 0;
	ssize_t bytes;
	string data = "";
	char incoming_data_buffer[BUFFER_SIZE];
	memset(incoming_data_buffer, 0, BUFFER_SIZE);
	
	while ((bytes = recv(socketfd, incoming_data_buffer, BUFFER_SIZE, 0))){
		bytes_recieved += bytes;
		if (bytes == -1){
			cerr << "recieve error!" << endl ;
			close(socketfd);
			freeaddrinfo(host_info_list);
			return EXIT_FAILURE;
		}
		else data.append(incoming_data_buffer, bytes);
		memset(incoming_data_buffer, 0, BUFFER_SIZE);
	}

	string header = data.substr(0, data.find("\r\n\r\n"));
	string body = data.substr(data.find("\r\n\r\n")+4);

	string upper_header = "";
	for (unsigned long int i = 0 ; i < header.length() ; i++)
		upper_header.push_back(toupper(header[i], loc));


	pos = 0;
	string response;
	string type;
	string code;
	if ((pos = upper_header.find("HTTP")) != string::npos){
		if (upper_header[pos + 7] != version){
			if (get(address, counter, redir, '0'))return EXIT_FAILURE;
			return EXIT_SUCCESS;
		}
		type = upper_header.substr(pos, pos+9);
		pos += 9;
		int u = upper_header.find("\r\n");
		response = upper_header.substr(pos, u - pos);
		code = response.substr(0, 3);
	}

	
	if (upper_header.find("TRANSFER-ENCODING: CHUNKED") != string::npos){
		pos = 0;
		unsigned long int start = 0;
		unsigned long int size = 0;
		string tmp = "";
		while(1){
			while(body.length() > pos && body[pos] != '\r')pos++;
			if (start == pos)break;
			size = stoi(body.substr(start, pos), nullptr, 16);
			if (pos + 2 < body.length())pos += 2;
			else break;
			tmp += body.substr(pos, size);
			if (pos + size < body.length()) pos += size + 2;
			else break;
			start = pos;
		}
		body = tmp;
	}

	if (code == "200"){
		ofstream output;
		output.open(out_file, ios::out | ios::binary);
		output << body;
		output.close();
		close(socketfd);
		freeaddrinfo(host_info_list);
		return EXIT_SUCCESS;
	}
	else if (code == "301" || code == "302"){
		pos = 0;
		if ((pos = upper_header.find("LOCATION")) != string::npos){
			pos += 10;
			int u = header.find("\r\n", pos);
			string location = header.substr(pos, u - pos);
			if (code == "302"){
				if(redir.count(address) == 1)location = redir[address];
				else redir[address] = location;
			}
			if (get(location.c_str(), counter+1, redir)){
				close(socketfd);
				freeaddrinfo(host_info_list);
				return EXIT_FAILURE;
			}
		}
	}
	else {
		cerr << "Error: " << response << endl;
		close(socketfd);
		freeaddrinfo(host_info_list);
		return EXIT_FAILURE;
	}

	return EXIT_SUCCESS;

}

int main(int argc, char *argv[]){

	if (argc != 2) {
		cerr << "Only one parameter is allowed" << endl;
		return EXIT_FAILURE;
	}

	map<string, string> redir;

	if (get(argv[1], 1, redir))return EXIT_FAILURE;

	return EXIT_SUCCESS;
}
