#define _WINSOCK_DEPRECATED_NO_WARNINGS
#pragma comment (lib, "ws2_32")
#include <WinSock2.h>
#include <stdio.h>

#define BROADCASTIP "255.255.255.255"
#define BROADCASTPORT 50001
#define SHUTDOWNPORT 50000

//전역변수
BOOL shutdownActivate = FALSE;	//종료 과정 수행 여부를 저장한다.


//오류 메시지를 출력하고 프로그램을 종료하는 함수
void err_quit(const char *msg)
{
	//에러 코드로부터 오류 메시지를 만든다.
	LPSTR msgBuf;
	FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM, NULL,
		WSAGetLastError(), MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), (LPTSTR)& msgBuf, 0, NULL);

	//오류 메시지를 알림창으로 띄운다.
	MessageBox(NULL, msgBuf, msg, MB_ICONERROR);

	//버퍼 메모리 반환
	LocalFree(msgBuf);

	//프로그램 종료
	exit(1);
}

//오류 메시지를 출력하는 함수
void err_display(const char *msg)
{
	//에러 코드로부터 오류 메시지를 만든다.
	LPSTR msgBuf;
	FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM, NULL,
		WSAGetLastError(), MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), (LPTSTR)& msgBuf, 0, NULL);

	//오류 메시지를 알림창으로 띄운다.
	MessageBox(NULL, msgBuf, msg, MB_ICONERROR);

	//버퍼 메모리 반환
	LocalFree(msgBuf);
}

//일반 오류 메시지를 출력하는 함수
void err_print(const char *msg)
{
	//오류 메시지를 알림창으로 띄운다.
	MessageBox(NULL, msg, "error", MB_ICONERROR);
}

//브로드캐스팅 메시지를 송신하는 스레드 함수
DWORD WINAPI broadcastSender(LPVOID arg)
{
	int retval;

	//통신용 소켓을 생성한다.
	SOCKET sock = socket(AF_INET, SOCK_DGRAM, 0);
	if (sock == INVALID_SOCKET)
		err_quit("socket()");

	//브로드캐스팅을 활성화한다.
	BOOL bEnable = TRUE;
	retval = setsockopt(sock, SOL_SOCKET, SO_BROADCAST, (char*)& bEnable, sizeof(bEnable));
	if (retval == SOCKET_ERROR)
		err_quit("setsockopt()");

	//원격 IP, Port번호를 설정한다.
	SOCKADDR_IN remoteaddr = { 0 };
	remoteaddr.sin_family = AF_INET;
	remoteaddr.sin_addr.s_addr = inet_addr(BROADCASTIP);
	remoteaddr.sin_port = htons(BROADCASTPORT);

	//통신에 사용할 변수를 선언한다.
	char buffer[50] = { '\0' };

	//송신할 데이터를 복사한다.
	strcpy_s(buffer, sizeof(buffer), "Ready!");

	while (1)
	{
		//데이터를 송신한다.
		retval = sendto(sock, buffer, (int)strlen(buffer) + 1, 0, (SOCKADDR*)& remoteaddr, sizeof(remoteaddr));
		if (retval == SOCKET_ERROR)
		{
			err_display("sendto()");
			continue;
		}

		//10초동안 대기한다.
		Sleep(10000);
	}

	//통신용 소켓을 닫는다.
	retval = closesocket(sock);
	if (retval != 0)
		err_display("closesocket()");

	return 0;
}

//종료신호를 수신하는 함수
void shutdownMessageReceiver()
{
	int retval;

	//통신용 소켓을 생성한다.
	SOCKET sock = socket(AF_INET, SOCK_DGRAM, 0);
	if (sock == INVALID_SOCKET)
		err_quit("socket()");

	//지역 IP주소와 지역 포트번호를 설정한다.
	SOCKADDR_IN localaddr = { 0 };
	localaddr.sin_family = AF_INET;
	localaddr.sin_addr.s_addr = htonl(INADDR_ANY);
	localaddr.sin_port = htons(SHUTDOWNPORT);
	retval = bind(sock, (SOCKADDR*)& localaddr, sizeof(localaddr));
	if (retval == SOCKET_ERROR)
		err_quit("bind()");

	//통신에 사용할 변수를 선언한다.
	SOCKADDR_IN peeraddr;
	int addrlen = sizeof(peeraddr);
	char buffer[70] = { '\0' };

	while (1)
	{
		//데이터를 수신한다.
		addrlen = sizeof(peeraddr);
		retval = recvfrom(sock, buffer, sizeof(buffer), 0, (SOCKADDR*)& peeraddr, &addrlen);
		if (retval == SOCKET_ERROR)
		{
			err_display("recvfrom()");
			continue;
		}

		//수신한 데이터를 검사해서 틀리면 재전송을 요구한다.
		if (strcmp(buffer, "F39422345DF99D2EAD885FA4E80CF0AD3554D8600C33B5F0B158B54E9B67AFFD") != 0)
		{
			strcpy_s(buffer, sizeof(buffer), "Hash value is not match.");
			retval = sendto(sock, buffer, (int)strlen(buffer) + 1, 0, (SOCKADDR*)& peeraddr, sizeof(peeraddr));
			if (retval == SOCKET_ERROR)
				err_quit("sendto()");
			continue;
		}
		else
			break;
	}

	//종료 과정 수행 시작을 보고한다.
	strcpy_s(buffer, sizeof(buffer), "Shutdown process start.");
	retval = sendto(sock, buffer, (int)strlen(buffer) + 1, 0, (SOCKADDR*)& peeraddr, sizeof(peeraddr));
	if (retval == SOCKET_ERROR)
		err_quit("sendto()");

	//종료 과정 수행 여부를 True로 한다.
	shutdownActivate = TRUE;

	//통신용 소켓을 닫는다.
	retval = closesocket(sock);
	if (retval != 0)
		err_display("closesocket()");
}

//종료과정을 수행하는 함수
int shutdownProcess()
{
	int retval;

	//종료 과정 수행 여부가 True인지 검사한다.
	if (shutdownActivate != TRUE)
		return 1;

	//종료 과정을 수행한다.
	retval = system("shutdown /s /t 1");
	if (retval != 0)
		return 2;

	return 0;
}

//메인 함수
int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow)
{
	int retval;

	//윈속을 초기화한다.
	WSADATA wsa;
	retval = WSAStartup(MAKEWORD(2, 2), &wsa);
	if (retval != 0)
	{
		err_print("WSAStartup() 오류! \n");
		return 1;
	}

	//브로드캐스트 메시지를 송신하는 스레드를 생성한다.
	HANDLE hThread = CreateThread(NULL, 0, broadcastSender, NULL, 0, NULL);
	if (hThread == NULL)
		err_print("스레드 생성 오류! \n");
	else
		CloseHandle(hThread);

	//종료신호를 수신한다.
	shutdownMessageReceiver();

	//윈속을 종료한다.
	retval = WSACleanup();
	if (retval != 0)
		err_display("WSACleanup()");

	//종료 과정을 수행한다.
	retval = shutdownProcess();
	if (retval != 0)
	{
		switch (retval)
		{
		case 1:
			err_print("shutdownActivate 오류! \n");
			break;
		case 2:
			err_print("shutdown 명령어 오류! \n");
			break;
		default:
			err_print("정의되지 않은 오류코드입니다. \n");
			break;
		}
	}

	return 0;
}