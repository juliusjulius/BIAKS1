#define WIN32_LEAN_AND_MEAN

#include <windows.h>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <stdlib.h>
#include <stdio.h>
#include <iostream>
#include <sstream> 

#pragma comment (lib, "Ws2_32.lib")
#pragma comment (lib, "Mswsock.lib")
#pragma comment (lib, "AdvApi32.lib")

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

int __cdecl main(int argc, char** argv)
{
	WSADATA wsaData;
	SOCKET connectionSocket = INVALID_SOCKET;
	struct addrinfo* result = NULL, * ptr = NULL, hints;
	std::string arr = "test";

	//DH
	int p = 0;  // 0 values will be recieved from server
	int g = 0;
	int serverPublicKey = 0;
	int clientPrivateKey = 6;


	//1. initialize Winsock
	int wsaInit = WSAStartup(MAKEWORD(2, 2), &wsaData);
	if (wsaInit != 0) {
		cout << "WSAStartup failed!";
		return 1;
	}

	ZeroMemory(&hints, sizeof(hints));
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_protocol = IPPROTO_TCP;


	wsaInit = getaddrinfo("localhost", PORT, &hints, &result);

	// Attempt to connect to an address until one succeeds
	for (ptr = result; ptr != NULL; ptr = ptr->ai_next) {

		//2. create socket
		connectionSocket = socket(ptr->ai_family, ptr->ai_socktype,
			ptr->ai_protocol);
		if (connectionSocket == INVALID_SOCKET) {
			cout << "socket failed!";
			WSACleanup();
			return 1;
		}

		//3. connect to server
		wsaInit = connect(connectionSocket, ptr->ai_addr, (int)ptr->ai_addrlen);
		if (wsaInit == SOCKET_ERROR) {
			closesocket(connectionSocket);
			connectionSocket = INVALID_SOCKET;
			continue;
		}

		cout << "connected to server" << endl;
		break;
	}

	freeaddrinfo(result);

	if (connectionSocket == INVALID_SOCKET) {
		cout << "Unable to connect to server!";
		WSACleanup();
		return 1;
	}

	// getting DH data from server
	char recvbuf[1024];
	int helpArray[10];
	int bRecieved = 0;

	bRecieved = recv(connectionSocket, recvbuf, 1024, 0);
	std::istringstream iss(recvbuf);

	int n;
	int temp = 0;
	while (iss >> n)
	{
		helpArray[temp] = n;
		temp++;
	}

	p = helpArray[0];
	g = helpArray[1];
	serverPublicKey = helpArray[2];

	//resolve client public key
	int clientPublicKey = pow(g, clientPrivateKey);
	clientPublicKey %= p;

	//send it tp server
	arr = std::to_string(clientPublicKey) + " ";
	send(connectionSocket, arr.c_str(), arr.size(), 0);  //send public key to server

	//get shared secret key
	int sharedSecretKey = pow(serverPublicKey, clientPrivateKey);
	sharedSecretKey %= p;

	//cout << "sharedKey client: " << sharedSecretKey << endl;

	bRecieved = recv(connectionSocket, recvbuf, 1024, 0);

	cout << decipher(recvbuf, sharedSecretKey);

	//toto tu necham len aby sa nepovedalo.. aj tak mi to funguje bez toho
	do {
		bRecieved = recv(connectionSocket, recvbuf, 1024, 0);
		if (bRecieved > 0) {
			//cout << "Bytes received: " << bRecieved << endl;
		//	cout << "Data recieved: " << recvbuf << endl;;
		}
		else if (bRecieved == 0)
			cout << "Connection closed" << endl;
		else
			cout << "recv failed!";

	} while (bRecieved > 0);

	closesocket(connectionSocket);
	WSACleanup();

	return 0;
}