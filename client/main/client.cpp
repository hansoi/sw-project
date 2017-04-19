#include <cstdio>
#include <string>
#include <iostream>
#include <fstream>
#include <ctime>
#include "socketLayer.h"
#include "md5check.h"

using namespace std;

#define STRING_LENGTH 512

const int BUFFER_SIZE = SocketLayer::BUFFER_SIZE;

string sendFileToSocket(char *filename, SocketLayer &SL, SOCKET socket, long long *fileSize){
	ifstream is(filename, ios::in | ios::binary);
	if (!is.is_open()) {
		fprintf(stderr, "File is not exists: \"%s\"\n", filename);
		*fileSize = 0;
		return "";
	}
	char tag[BUFFER_SIZE + 30] = {};
	sprintf(tag, "<send-file>:%s", filename);
	SL.Send(socket, tag, strlen(tag));
	

	long long totalFileSize = is.tellg();
	is.seekg(0, ios::end);
	totalFileSize = is.tellg() - totalFileSize;
	is.seekg(0, ios::beg);

	char buffer[BUFFER_SIZE] = {};
	int sendCount = 0;
	string hashResult("");
	long long calculatedFileSize = 0LL;
	time_t lastTickTime = clock();
	long long lastTickFileSize = 0LL;
	do {
		memset(buffer, 0, sizeof(buffer));
		is.read((char *)&buffer, BUFFER_SIZE);
		hashResult = md5(hashResult + buffer);

		if (is.eof()){
			is.clear();
			is.seekg(0, ios::end);
			long long realRestSize = is.tellg() - (long long)(sendCount*BUFFER_SIZE);

			is.clear();
			is.seekg(-realRestSize, ios::end);
			memset(buffer, 0, sizeof(buffer));
			is.read((char *)&buffer, realRestSize);
			SL.Send(socket, buffer, realRestSize);
			calculatedFileSize += realRestSize;
			break;
		}
		else {
			Sleep(10);
			int byte = SL.Send(socket, buffer, BUFFER_SIZE);
			++sendCount;
			calculatedFileSize += BUFFER_SIZE;
		}

		double percentage = calculatedFileSize / (totalFileSize + 1e-9) * 100.0;
		time_t now = clock(), elapseSecond = (now - lastTickTime) / CLOCKS_PER_SEC;
		if (elapseSecond >= 1) {
			double kbps = ((calculatedFileSize - lastTickFileSize) / 1024LL) / (elapseSecond + (double)1e-9);
			printf("%s]... kbps: %.3lf KB/s (%3.1lf%%)\n", filename, kbps, percentage);
			lastTickTime = now;
			lastTickFileSize = calculatedFileSize;
		}
	} while (is.tellg() >= 0);
	is.close();
	SL.Send(socket, "</send-file>", 12);
	*fileSize = calculatedFileSize;
	return hashResult;
}

int main(int argc, char **argv){
	int port;
	char address[STRING_LENGTH];
	printf("IP: "); cin >> address;
	printf("PORT: "); cin >> port;
	getchar();
	fflush(stdin);

	SocketLayer SL;
	SOCKET client;
	if( SL.Create(&client) == INVALID_SOCKET ){
		perror("FAIL TO CREATE SOCKET");
		return 1;
	}

	sockaddr_in addressInfo = SL.MakeAddress(address, port);
	if( SL.Connect(client, addressInfo) == SOCKET_ERROR ){
		perror("FAIL TO CONNECT SOCKET");
		return 1;
	}

	SL.Send(client, "User Connected!", 15);

	char filename[STRING_LENGTH] = {};
	int retryCount = 0;
	do {
		// ��õ��� �ƴ϶�� ���ϸ��� �Է¹޾ƾ��Ѵ�.
		if( retryCount == 0 || retryCount > 5 ){
			if(retryCount > 5){
				puts("������ �����߽��ϴ�.");
			}
			retryCount = 0;

			printf("filename> ");
			if( fgets(filename, STRING_LENGTH, stdin) == NULL ) break;
			if (strncmp(filename, "quit\n", 5) == 0) break;
			filename[strlen(filename) - 1] = 0;
		}

		printf("Send File.. \"%s\"\n", filename);

		long long fileSize = 0;
		string fileHashValue = sendFileToSocket(filename, SL, client, &fileSize);

		char str[BUFFER_SIZE] = {};
		if( SL.Receive(client, str, BUFFER_SIZE) > 0 ){
			// �����κ��� ���𰡸� ���� ���� ������ۿ� ����.
			printf("Received Data from Server: %s\n", str);

			// ������ ���۵� ������ �ؽð��� �޴� ���
			if( strncmp(str, "<file-hash:", 11) == 0 ){
				printf("MyHashValue: %s and %s\n", str+11, fileHashValue);
				// ���� �����ڴ� �ڸ� ���� �ؽð�
				if( HasFileIntegrity(str+11, fileHashValue) == true ){
					printf("[%s] : original file\n[%s] : received file\n", str+11, fileHashValue.c_str());
					fprintf(stderr, "������ ���������� ���۵Ǿ����ϴ�.\n");
					retryCount = 0;
				} else {
					fprintf(stderr, "������ ����� ���۵��� ���߽��ϴ�.\n");
					fprintf(stderr, "5�� �� ��õ� (%d)\n", ++retryCount);
					Sleep(5 * 1000);
					continue;
				}
			}
		}
	} while( true );

	SL.Close(client);

	return 0;
}
