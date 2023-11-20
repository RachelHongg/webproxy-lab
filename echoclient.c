/*
* Echo 클라이언트의 메인 루틴
* 서버와의 연결을 수립한 이후에 클라이언트는 표준 입력에서 텍스트 줄을 반복해서 읽는 루프에 진입하고,
* 서버에 텍스트 줄을 전송하고,
* 서버에서 echo 줄을 읽어서 그 결과를 표준 출력으로 인쇄한다.
*/

/* 종료조건
* fgets가 EOF 표준 입력을 만나면 종료
* 그 이유는 사용자가 Ctrl + D를 눌렀거나, 파일로 텍스트 줄을 모두 소진했기 때문에,
*/

/*
* 루프가 종료한 후에는 클라이언트는 식별자를 닫는다 -> 서버로 EOF라는 통지가 전송됨.
* 서버는 rio_readlineb함수에서 리턴 코드 0을 받으면, 이 사실을 감지한다.
* 자신의 식별자를 닫은 후, 클라이언트는 종료된다.
*/

/* EOF란?
* EOF는 "End of File"의 약자로, 
* 파일 끝에 도달했음을 나타내는 특수한 상태를 나타냅니다. 
* C 언어에서 EOF는 파일 끝 또는 입력 소스의 끝을 나타내는 값으로 사용됩니다.
* 입력 작업을 수행하는 함수들 중 일부(예: `fgets`, `fscanf`, `getchar` 등)은 파일 또는 표준 입력에서 데이터를 읽고, 
* 파일 끝에 도달하거나 입력 소스의 끝을 나타내는 특별한 값인 EOF를 반환합니다.
* 표준 입력에서 Ctrl + D (Unix 및 Linux 시스템에서)를 누르면, 입력 소스의 끝을 표시하는데, 
* 이는 일반적으로 EOF 값으로 해석됩니다. 
* Ctrl + D를 누르면, 입력 소스로부터 더 이상 데이터를 읽을 수 없음을 나타내며, 
* 이는 `fgets` 또는 다른 입력 함수에 EOF를 반환하여 프로그램이 종료되거나 반복 루프에서 빠져나오는 데 사용됩니다.
* 따라서 종료 조건에서 `fgets` 함수가 EOF(즉, 파일의 끝 또는 입력 소스의 끝)을 만나면 
* 프로그램이 종료되거나 반복이 중단될 수 있습니다.
*/

/* fgets 함수가 입력 스트림에서 한 줄씩 데이터를 읽어와 저장된 버퍼('buf')에 저장한다.
* 이때, MAXLINE은 버퍼('buf')의 최대 크기를 나타낸다.
* 아래 해당 while문은 fgets함수가 NULL이 아닌 값을 반환 할 때까지 반복한다.
* fgets 함수는 성공적으로 데이터를 읽어오면 문자열의 첫번째 문자에 대한 포인터, 
* 즉, 버퍼에 대한 주소를 반환하고,
* 만약, EOF에 도달하거나 오류가 발생하면 NULL을 반환한다.
*/

/* 버퍼란?
* 데이터를 일시적으로 저장하는 메모리 공간을 가리킴.
* 컴퓨터에서 데이터를 처리할 때
* 메모리 상의 임시 공간에 데이터를 저장하고 조작하는데, 이러한 임시 저장소가 바로 버퍼이다
*/

#include "csapp.h"

int main(int argc, char **argv) // 동적 할당 argc(argument count), argv(argument value)
{
    int clientfd;
    char *host, *port, buf[MAXLINE];
    rio_t rio;

    if (argc != 3) 
	{
        fprintf(stderr, "usage: %s <host> <port>\n", argv[0]);
        exit(0);
    }
    host = argv[1]; 
    port = argv[2];

    clientfd = open_clientfd(host, port);
    Rio_readinitb(&rio, clientfd);

    while (fgets(buf, MAXLINE, stdin) != NULL) 
	{
        Rio_writen(clientfd, buf, strlen(buf));
        Rio_readlineb(&rio, buf, MAXLINE);
        fputs(buf, stdout);
    }
    Close(clientfd);
    exit(0);
}
