#include <stdio.h>

#include "csapp.h"

/* Recommended max cache and object sizes */
#define MAX_CACHE_SIZE 1049000
#define MAX_OBJECT_SIZE 102400

/* You won't lose style points for including this long line in your code */
static const char *user_agent_hdr = "User-Agent: Mozilla/5.0 (X11; Linux x86_64; rv:10.0.3) Gecko/20120305 Firefox/10.0.3\r\n";
static char *ccstr = "Connection: close\r\n";
static char *pccstr = "Proxy-Connection: close\r\n";

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
    char buf[MAXLINE], method[MAXLINE], url[MAXLINE], version[MAXLINE];
    Rio_readinitb(&rio, acceptfd);

    Rio_readlineb(&rio, buf, MAXLINE); /* read request line */

    /* parse request line */
    sscanf(buf, "%s %s %s", method, url, version);
    printf("method: %s\nuri: %s\nversion: %s\n", method, url, version);
    /* TODO: if method is not GET then abort */

    /* Parse uri to get host and port */
    char host[MAXLINE], port[MAXLINE], file[MAXLINE];
    char *p = strchr(url, ':');
    ++p;
    p = strchr(p, ':');
    *p = 0;
    ++p;
    int portnum;
    sscanf(url, "http://%s", host);
    sscanf(p, "%d/", &portnum);
    sprintf(port, "%d", portnum);
    p = strchr(p, '/');
    strncpy(file, p, MAXLINE);
    printf("host: %s\nport: %s\nfile: %s\n", host, port, file);

    /* Open connection to target */
    int connectfd = Open_clientfd(host, port);
    /* TODO: check error */

    /* Process and send headers */
    sprintf(buf, "GET %s HTTP/1.0", file);
    Rio_writen(connectfd, buf, strlen(buf)); /* send request line */
    Rio_readlineb(&rio, buf, MAXLINE);
    while (strcmp(buf, "\r\n")) {
      if (strstr(buf, "Proxy-Connection")) {
        Rio_writen(connectfd, pccstr, strlen(pccstr));
      }
      else if (strstr(buf, "Connection")) {
        Rio_writen(connectfd, ccstr, strlen(ccstr));
      }
      else {
        Rio_writen(connectfd, buf, strlen(buf));
      }
      Rio_readlineb(&rio, buf, MAXLINE);
    }
    Rio_writen(connectfd, buf, strlen(buf)); /* send the end of request */

    /* tansfer content */
    int n = 0;
    while ((n = Rio_readn(connectfd, buf, MAXLINE))) {
      Rio_writen(acceptfd, buf, n);
    }
    Close(connectfd);

    Close(acceptfd);
  }
  /* printf("%s", user_agent_hdr); */
  return 0;
}
