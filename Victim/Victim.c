#define _WINSOCK_DEPRECATED_NO_WARNINGS
#pragma comment (lib, "ws2_32")
#include <WinSock2.h>
#include <stdio.h>

#define BROADCASTIP "255.255.255.255"
#define BROADCASTPORT 50000


//오류 메시지를 출력하고 프로그램을 종료하는 함수
void err_quit(char* msg)
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
void err_display(char* msg)
{
	//에러 코드로부터 오류 메시지를 만든다.
	LPSTR msgBuf;
	FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM, NULL,
		WSAGetLastError(), MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), (LPTSTR)& msgBuf, 0, NULL);

	//오류 메시지를 콘솔창에 출력한다.
	printf("[%s] %s\n", msg, msgBuf);

	//버퍼 메모리 반환
	LocalFree(msgBuf);
}

//메인 함수
int main()
{
	int retval;

	//윈속을 초기화한다.
	WSADATA wsa;
	retval = WSAStartup(MAKEWORD(2, 2), &wsa);
	if (retval != 0)
	{
		printf("WSAStartup() 오류: %d\n", retval);
		return 1;
	}

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
	char buffer[512] = { '\0' };

	while (1)
	{
		//전송할 데이터를 입력한다.
		printf("Send: ");
		gets_s(buffer, sizeof(buffer));

		//데이터를 전송한다.
		retval = sendto(sock, buffer, (int)strlen(buffer) + 1, 0, (SOCKADDR*)& remoteaddr, sizeof(remoteaddr));
		if (retval == SOCKET_ERROR)
		{
			err_display("sendto()");
			continue;
		}

		//10초동안 대기한다.
		//Sleep(10000);
	}

	//통신용 소켓을 닫는다.
	retval = closesocket(sock);
	if (retval != 0)
		err_display("closesocket()");

	//윈속을 종료한다.
	retval = WSACleanup();
	if (retval != 0)
		err_display("WSACleanup()");

	return 0;
}