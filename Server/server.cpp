#undef UNICODE
#define WIN32_LEAN_AND_MEAN

#include <windows.h>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <stdlib.h>
#include <stdio.h>
#include <iostream>
#include <string> 
#include <sstream> 

#pragma comment (lib, "Ws2_32.lib")

#define PORT "4200"

using std::cout;
using std::endl;

std::string cipher(std::string data, int shift) {

	std::string shiftedMsg;

	for (int i = 0; i < data.length(); ++i)
		shiftedMsg += (int(data[i]) + shift);

	return shiftedMsg;
}

std::string decipher(std::string data, int shift) {

	std::string unShiftedMsg;

	for (int i = 0; i < data.length(); ++i)
		unShiftedMsg += (int(data[i]) - shift);

	return unShiftedMsg;
}


int __cdecl main(void)
{
	WSADATA wsaData;
	struct addrinfo* result = NULL;
	struct addrinfo hints;
	int iSendResult;

	//DH
	int p = 23; // prime number (should be bigger but for educational purposes lets say 23)
	int g = 5;
	int clientPublicKey = 0;
	int serverPrivateKey = 7;
	int serverPublicKey = pow(g, serverPrivateKey);
	serverPublicKey %= p;

	//1. initialize Winsock
	int wsaInit = WSAStartup(MAKEWORD(2, 2), &wsaData);
	if (wsaInit != 0) {
		cout << "WSAStartup failed!";
		return 1;
	}

	ZeroMemory(&hints, sizeof(hints));
	hints.ai_family = AF_INET;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_protocol = IPPROTO_TCP;
	hints.ai_flags = AI_PASSIVE;

	// Resolve the server address and port
	wsaInit = getaddrinfo(NULL, PORT, &hints, &result);

	//2. create socket
	SOCKET listeningSocket = socket(result->ai_family, result->ai_socktype, result->ai_protocol);
	if (listeningSocket == INVALID_SOCKET) {
		cout << "socket creation failed!";
		WSACleanup();
		return 1;
	}

	//3. bind the socket
	wsaInit = bind(listeningSocket, result->ai_addr, (int)result->ai_addrlen);
	if (wsaInit == SOCKET_ERROR) {
		cout << "socket binding failed!";
		freeaddrinfo(result);
		closesocket(listeningSocket);
		WSACleanup();
		return 1;
	}

	freeaddrinfo(result);

	//4. listen on the socket for a client
	cout << "Server is listening on port: " << PORT << endl;
	wsaInit = listen(listeningSocket, SOMAXCONN);
	if (wsaInit == SOCKET_ERROR) {
		cout << "listening failed !";
		closesocket(listeningSocket);
		WSACleanup();
		return 1;
	}

	SOCKET clientSocket;
	int sharedSecretKey;
	int helpArray[10];

	//5. accept a connection from a client
	clientSocket = accept(listeningSocket, NULL, NULL);
	if (clientSocket == INVALID_SOCKET) {
		cout << "accept failed!";
		closesocket(listeningSocket);
		WSACleanup();
		return 1;
	}

	//6. receive and send data
	char recvbuf[1024];
	int bRecieved = 0;
	std::string arr;

	arr = std::to_string(p) + " " + std::to_string(g) + " " + std::to_string(serverPublicKey) + " ";
	send(clientSocket, arr.c_str(), arr.size(), 0);  //send DH data to client


	//recieve client public key
	bRecieved = recv(clientSocket, recvbuf, 1024, 0);

	std::istringstream iss(recvbuf);

	int n;
	int temp = 0;
	while (iss >> n)
	{
		helpArray[temp] = n;
		temp++;
	}

	clientPublicKey = helpArray[0];

	//get shared secret key
	sharedSecretKey = pow(clientPublicKey, serverPrivateKey);
	sharedSecretKey %= p;

	//cout << "sharedKey Server: " << sharedSecretKey << endl;

	arr = "HTTP/1.1 200 OK\nContent-Type:text/html\nContent-Length: 200 \n\n<h1>testing</h1> \n<h1>fajne to funguje</h1> ";  //sending html
	arr = cipher(arr, sharedSecretKey);
	send(clientSocket, arr.c_str(), arr.size(), 0);

	//toto tu necham len aby sa nepovedalo.. aj tak mi to funguje bez toho
	do {
		bRecieved = recv(clientSocket, recvbuf, 1024, 0);
		if (bRecieved > 0) {
			iSendResult = send(clientSocket, arr.c_str(), arr.size(), 0);
			cout << "Data recieved: " << recvbuf << endl;
		}
		else if (bRecieved == 0)
			cout << "Connection closing" << endl;
		else
			cout << "recv failed!";


	} while (true);


	//7. disconnect
	wsaInit = shutdown(clientSocket, SD_SEND);
	closesocket(clientSocket);
	WSACleanup();

	return 0;
}

