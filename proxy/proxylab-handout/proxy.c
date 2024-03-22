#include <stdio.h>

#include "csapp.h"

/* Recommended max cache and object sizes */
#define MAX_CACHE_SIZE 1049000
#define MAX_OBJECT_SIZE 102400

/* You won't lose style points for including this long line in your code */
static const char *user_agent_hdr = "User-Agent: Mozilla/5.0 (X11; Linux x86_64; rv:10.0.3) Gecko/20120305 Firefox/10.0.3\r\n";

int main(int argc, char* argv[])
{
  if (argc < 2) {
    printf("usage: %s [portnumber]\n", argv[0]);
    return 1;
  }

  int listenfd = Open_listenfd(argv[1]);
  int acceptfd;
  struct sockaddr_storage addr;
  socklen_t len = sizeof(addr);
  while ((acceptfd = Accept(listenfd, (struct sockaddr *)&addr, &len))) {
    /* read and process http request */
    rio_t rio;
    char buf[MAXLINE];
    Rio_readinitb(&rio, acceptfd);
    while (Rio_readlineb(&rio, buf, MAXLINE)) {
      printf("%s", buf);
    }
    /* connect to server */
    /* tansfer content */
    Close(acceptfd);
  }
  /* printf("%s", user_agent_hdr); */
  return 0;
}
