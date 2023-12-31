#include <stdio.h>
#include "csapp.h"

/* 과제 조건: HTTP/1.0 GET 요청을 처리하는 기본 시퀀셜 프록시

  1) 클라이언트가 프록시에게 다음 HTTP 요청 보냄 
  2) GET http://www.google.com:80/index.html HTTP/1.1 
  3) 프록시가 이 요청을 파싱 -> server에게 전달해줄 /home.html을 선별
  4) Open_clientfd(getaddinfo, socket, connect)로 clientfd 소켓을 만든다.
  5) do_request를 통해 프록시에게 연결 요청을 한다.
  6) 프록시는 do_response를 통해 클라이언트에게 연결 요청 수락 메세지를 보낸다.
  7) close를 통해 socket을 닫는다.

  이렇게 프록시는 서버에게 다음 HTTP 요청으로 보내야함.
  GET /index.html HTTP/1.0   
  
  즉, proxy는 HTTP/1.1로 받더라도 server에는 HTTP/1.0으로 요청해야함 
*/

/* 최대 캐시 사이즈를 지정한다. */
#define MAX_CACHE_SIZE 1049000

/* HTTP 요청헤더인 "User_Agent"를 설정하는데 사용_해당 요청이 Mozilla Firefox 브라우저 클라이언트임. */
static const char *user_agent_hdr =
    "User-Agent: Mozilla/5.0 (X11; Linux x86_64; rv:10.0.3) Gecko/20120305 "
    "Firefox/10.0.3\r\n";

/* HTTP 버전 */
static const char *new_version = "HTTP/1.0";

/* 함수 모음 */
void do_it(int fd);
void do_request(int clientfd, char *method, char *uri_ptos, char *host);
void do_response(int connfd, int clientfd);
int parse_uri(char *uri, char *uri_ptos, char *host, char *port);
void *thread(void *vargp);

/* ./proxy < 포트번호 >를 입력한다. */
int main(int argc, char **argv) { 
  int listenfd, *connfdp;
  char hostname[MAXLINE], port[MAXLINE];
  socklen_t clientlen;
  struct sockaddr_storage clientaddr;
  pthread_t tid;

  /* 입력이 잘들어왔는지 확인 (첫번째 인자는 프로그램이름, 두번째인자부터 받는 값 -> 즉, 1개의 값을 받음) */
  if (argc != 2) {  
    fprintf(stderr, "usage: %s <port>\n", argv[0]);
    exit(1);    // 비정상종료이므로 1값을 받고, 종료
  }

  /* 해당 <포트 번호>를 인자로 받아 듣기 소켓 식별자를 열어준다. */
  listenfd = Open_listenfd(argv[1]);

  /* 클라이언트의 요청이 올 때마다 새로 연결 소켓을 만들어 doit()호출 */
  while(1) {
    clientlen = sizeof(clientaddr);
    /* 각 클라이언트에게서 받은 연결 요청마다 각각을 accept한다. connfd = proxy의 connfd */
    connfdp = Malloc(sizeof(int));
    *connfdp = Accept(listenfd, (SA *)&clientaddr, &clientlen);

    /* 연결이 성공했다는 메세지를 위해. Getnameinfo를 호출하면서 hostname과 port가 채워진다.*/
    Getnameinfo((SA *)&clientaddr, clientlen, hostname, MAXLINE, port, MAXLINE, 0);
    printf("Accepted connection from (%s, %s)\n", hostname, port);
	
	/* 
	* 각각의 클라이언트 요청을 처리하기 위해 새로운 스래드를 생성한다. -> thread 함수를 실행시킨다. 
	* 요청받을 때마다 매번 새로운 스래드를 생성함으로
	* 프록시 서버는 다중 클라이언트 요청을 동시에 처리할 수 있다.
	* 나중에는, 병렬 스레드 처리를 통해 여러 클라이언트의 요청을 동시에 처리할 수 있어서, 프록시 서버의 성능을 향상시킬 수 있다.
	*/
    Pthread_create(&tid, NULL, thread, connfdp);    
  }
}

/* 요청받은 소켓 식별자(confd)를 받아와서 그 소켓을 통해 do_it()함수를 실행한다. */
void *thread(void *vargp){
  int connfd = *((int *)vargp);
  /* 새로운 스래드를 메인 스레드와 분리시킨다. */
  Pthread_detach(pthread_self());
  Free(vargp);
  do_it(connfd);
  Close(connfd);
  return NULL;
}

void do_it(int connfd){
  int clientfd;
  char buf[MAXLINE],  host[MAXLINE], port[MAXLINE], method[MAXLINE], uri[MAXLINE], version[MAXLINE];
  char uri_ptos[MAXLINE];
  rio_t rio;

  /* Read request line and headers from Client */
  Rio_readinitb(&rio, connfd);                      // rio 버퍼와 fd(proxy의 connfd)를 연결시켜준다. 
  Rio_readlineb(&rio, buf, MAXLINE);          // 그리고 rio(==proxy의 connfd)에 있는 한 줄(응답 라인)을 모두 buf로 옮긴다. 
  printf("Request headers to proxy:\n");
  printf("%s", buf);
  sscanf(buf, "%s %s %s", method, uri, version);      // buf에서 문자열 3개를 읽어와 각각 method, uri, version이라는 문자열에 저장 

  /* Parse URI from GET request */
  parse_uri(uri, uri_ptos, host, port);

  clientfd = Open_clientfd(host, port);             // clientfd = proxy의 clientfd (연결됨)
  do_request(clientfd, method, uri_ptos, host);     // clientfd에 Request headers 저장과 동시에 server의 connfd에 쓰여짐
  do_response(connfd, clientfd);        
  Close(clientfd);                                  // clientfd 역할 끝
}

/* proxy => server 요청을 보낸다. */
void do_request(int clientfd, char *method, char *uri_ptos, char *host){
  char buf[MAXLINE];
  printf("Request headers to server: \n");     
  printf("%s %s %s\n", method, uri_ptos, new_version);

  /* Read request headers */        
  sprintf(buf, "GET %s %s\r\n", uri_ptos, new_version);     // GET /index.html HTTP/1.0
  sprintf(buf, "%sHost: %s\r\n", buf, host);                // Host: www.google.com     
  sprintf(buf, "%s%s", buf, user_agent_hdr);                // User-Agent: ~
  sprintf(buf, "%sConnections: close\r\n", buf);            // Connections: close
  sprintf(buf, "%sProxy-Connection: close\r\n\r\n", buf);   // Proxy-Connection: close

  /* Rio_writen: buf에서 clientfd로 strlen(buf) 바이트로 전송 */
  Rio_writen(clientfd, buf, (size_t)strlen(buf)); // => 적어주는 행위 자체가 요청
}

/* do_request에 대한 대답을 한다. server => proxy */
void do_response(int connfd, int clientfd){
  char buf[MAX_CACHE_SIZE];
  ssize_t n;
  rio_t rio;

  Rio_readinitb(&rio, clientfd);  
  n = Rio_readnb(&rio, buf, MAX_CACHE_SIZE);  
  Rio_writen(connfd, buf, n);
}

/* 파싱을 통해 -> host uri와 server로 보낼 uri를 나누어 저장한다. */
int parse_uri(char *uri, char *uri_ptos, char *host, char *port){ 
  char *ptr;

  /* 필요없는 http:// 부분 잘라서 host 추출 */
  if (!(ptr = strstr(uri, "://"))) 
    return -1;                        // ://가 없으면 unvalid uri 
  ptr += 3;                       
  strcpy(host, ptr);                  // host = www.google.com:80/index.html

  /* uri_ptos(proxy => server로 보낼 uri) 추출 */
  if((ptr = strchr(host, '/'))){  
    *ptr = '\0';                      // host = www.google.com:80 뒤에 '/0'을 붙여, ptr이 읽을 범위(끝)를 지정해준다.
    ptr += 1;
    strcpy(uri_ptos, "/");            // uri_ptos = /
    strcat(uri_ptos, ptr);            // uri_ptos = /index.html
  }
  else strcpy(uri_ptos, "/");		// 없다면, /를 붙여준다.

  /* port 추출 */
  if ((ptr = strchr(host, ':'))){     // host = www.google.com:80
    *ptr = '\0';                      // host = www.google.com 뒤에 '/0'을 붙여, ptr이 읽을 범위(끝)를 지정해준다.
    ptr += 1;     
    strcpy(port, ptr);                // port = 80 이 저장됨.
  }  
  else strcpy(port, "80");            // port가 없을 경우, "80"을 넣어줌

  /* 
  파싱 후에는 (Server로 보낼 Request Line을 uri_ptos에 담아둠.)
  => GET /index.html HTTP/11. 
  */ 

  return 0; // function int return => for valid check
}