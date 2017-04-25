#include <cstdio>
#include <string>
#include <iostream>
#include <fstream>
#include "socketLayer.h"

using namespace std;

#define BUFFER_SIZE 2048

const int BUFFER_SIZE = SocketLayer::BUFFER_SIZE;

string sendFileToSocket(char *filename, SocketLayer *SL, SOCKET socket, long long *fileSize){
	ifstream is(filename, ios::in | ios::binary);
	if (!is.is_open()) {
		fprintf(stderr, "File is not exists: \"%s\"\n", filename);
		*fileSize = 0;
		return "";
	}
	char tag[BUFFER_SIZE + 30] = {};
	sprintf(tag, "<send-file>:%s", filename);
	SL->Send(socket, tag, strlen(tag));
	

	long long totalFileSize = is.tellg();
	is.seekg(0, ios::end);
	totalFileSize = is.tellg() - totalFileSize;
	is.seekg(0, ios::beg);
	char buffer[BUFFER_SIZE] = {};
	int sendCount = 0;
	long long calculatedFileSize = 0LL;
	do {
		memset(buffer, 0, sizeof(buffer));
		is.read((char *)&buffer, BUFFER_SIZE);
		if (is.eof()){
			is.clear();
			is.seekg(0, ios::end);
			long long realRestSize = is.tellg() - (long long)(sendCount*BUFFER_SIZE);
			cout << realRestSize << '\n';

			is.clear();
			is.seekg(-realRestSize, ios::end);
			memset(buffer, 0, sizeof(buffer));
			is.read((char *)&buffer, realRestSize);
			SL->Send(socket, buffer, realRestSize);
			calculatedFileSize += realRestSize;
			break;
		}
		else {
			Sleep(10);
			int byte = SL->Send(socket, buffer, BUFFER_SIZE);
			++sendCount;
			calculatedFileSize += BUFFER_SIZE;
		}
	} while (is.tellg() >= 0);
	is.close();
	SL->Send(socket, "</send-file>", 12);
	*fileSize = calculatedFileSize;
	return hashResult;
}

int main(int argc, char **argv){
	int port;
	char address[STRING_LENGTH];
	SocketLayer *SL;
	printf("IP: "); cin >> address;
	printf("PORT: "); cin >> port;
	char suggestTCP[10];
	printf("Use TCP? (y/n): "); cin >> suggestTCP;
	bool usingTCP = suggestTCP[0] == 'Y' || suggestTCP[0] == 'y';
	if (usingTCP) {
		puts("TCP ���������� ����մϴ�.");
		SL = new SocketLayerTCP;
	}
	else {
		puts("UDP ���������� ����մϴ�.");
		SL = new SocketLayerUDP;
	}
	getchar();
	fflush(stdin);

	SOCKET client;
	if( SL->Create(&client) == INVALID_SOCKET ){
		perror("FAIL TO CREATE SOCKET");
		return 1;
	}

	sockaddr_in addressInfo = SL->MakeAddress(address, port);
	if( SL->Connect(client, addressInfo) == SOCKET_ERROR ){
		perror("FAIL TO CONNECT SOCKET");
		return 1;
	}

	SL->Send(client, "User Connected!", 15);

	char filename[STRING_LENGTH] = {};
	int retryCount = 0, maxRetry = 3;
	do {
		// ��õ��� �ƴ϶�� ���ϸ��� �Է¹޾ƾ��Ѵ�.
		if( retryCount <= 0 ){
			retryCount = 0;

			printf("filename> ");
			if( fgets(filename, STRING_LENGTH, stdin) == NULL ) break;
			if (strncmp(filename, "quit\n", 5) == 0) break;
			filename[strlen(filename) - 1] = 0;
		}

		filename[strlen(filename) - 1] = 0;
		printf("Send File.. \"%s\"\n", filename);

		ifstream inFile(filename, ios::in | ios::binary);
		if (!inFile.is_open()){
			fprintf(stderr, "File is not exists: \"%s\"\n", filename);
			continue;
		}

		char tag[BUFFER_SIZE + 30] = {};
		sprintf(tag, "<send-file>:%s", filename);
		SL.Send(client, tag, strlen(tag));
		sendFileToSocket(inFile, SL, client);
		SL.Send(client, "</send-file>", 12);

		char str[BUFFER_SIZE] = {};
		if( SL->Receive(client, str, BUFFER_SIZE) > 0 ){
			// �����κ��� ���𰡸� ���� ���� ������ۿ� ����.
			printf("Received Data from Server: %s\n", str);

			// ������ ���۵� ������ �ؽð��� �޴� ���
			if( strncmp(str, "<file-hash:", 11) == 0 ){
				printf("MyHashValue: %s and %s\n", str+11, fileHashValue.c_str());
				// ���� �����ڴ� �ڸ� ���� �ؽð�
				if( HasFileIntegrity(str+11, fileHashValue) == true ){
					printf("[%s] : original file\n[%s] : received file\n", str+11, fileHashValue.c_str());
					fprintf(stderr, "������ ���������� ���۵Ǿ����ϴ�.\n");
					retryCount = 0;
				} else {
					if (++retryCount > maxRetry) {
						puts("������ �����߽��ϴ�.");
						retryCount = -1;
					}
					else {
						fprintf(stderr, "������ ����� ���۵��� ���߽��ϴ�.\n");
						fprintf(stderr, "2�� �� ��õ� (%d/%d)\n", retryCount, maxRetry);
						Sleep(2 * 1000);
					}
					continue;
				}
			}
		}
		printf("filename> ");
	}
	SL.Send(client,"~EXIT~");
	
	
	fclose(file);

	SL->Close(client);

	return 0;
}
