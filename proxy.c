/*Proxy Server (proxy.c)
 *Name : Mayur Sharma
 *ID : mayurs
 * 
 *It is a proxy server that acts as the intermediary between the client and
 *the remote host. It starts by listening to a port assigned to it via the
 *initial argument.It then gets the request, appends or changes some
 *headers and then sends the request to the server. It then recieves the
 *response and sends it back to the requesting client.It does using 
 *multithreading
 */

#include <stdio.h>
#include "csapp.h"


/* You won't lose style points for including these long lines in your code */
static const char *user_agent_hdr = "User-Agent: Mozilla/5.0 (X11; Linux x86_64; rv:10.0.3) Gecko/20120305 Firefox/10.0.3\r\n";
static const char *accept_hdr = "Accept: text/html,application/xhtml+xml,application/xml;q=0.9,*/*;q=0.8\r\n";
static const char *accept_encoding_hdr = "Accept-Encoding: gzip, deflate\r\n";
static const char *connection_hdr = "Connection: close\r\n";
static const char *proxy_connection_hdr = "Proxy-Connection: close\r\n";
static const char *http_version_hdr = "HTTP/1.0\r\n";


/* Functions to be used */
void* doit(void* fd);
void parse_request_line(char *buf, char *method, char *protocol,
			char *host_port, char *resource, char *version);
void parse_host(char *host, char *remote_host, char *remote_port);
int recieve_and_send(int to_client_fd, int to_server_fd, char* request_buf) ;
void close_fd(int *to_client_fd, int *to_server_fd);

int main(int argc, char *argv [])
{
  int listenfd, *connfd, port, clientlen;
  struct sockaddr_in clientaddr;
    pthread_t pid;
  
  Signal(SIGPIPE,SIG_IGN); //Ignoring SIGPIPE

  /* Check command line args */
  if (argc != 2) {
    fprintf(stderr, "usage: %s <port>\n", argv[0]);
    exit(1);
  }
  port = atoi(argv[1]);
  
  listenfd = Open_listenfd(port);
  while (1) {
    clientlen = sizeof(clientaddr);
    *connfd = Accept(listenfd, (SA *)&clientaddr, (socklen_t *)&clientlen);
    printf("Old fd = %d\n",*connfd);
    Pthread_create(&pid,NULL,doit,(void*)connfd);
   

  }
  
}

/*
 * doit - handle GET request/response transaction with thread
 */

void* doit(void *fd_temp) 
{
  char buf[MAXLINE], request_buf[MAXLINE];
  char method[MAXLINE], protocol[MAXLINE];
  char host_port[MAXLINE];
  char remote_host[MAXLINE], remote_port[MAXLINE], resource[MAXLINE];
  char version[MAXLINE];
  char origin_request_line[MAXLINE];
  char origin_host_header[MAXLINE];
  
  int has_user_agent_str = 0, has_accept_str = 0;
  int has_accept_encoding_str = 0;
  int has_connection_str = 0, has_proxy_connection_str = 0;
  int has_host_str = 0;   
  
  rio_t rio_client;

  int to_server_fd,to_client_fd ;
  
  int fd=*(int*)fd_temp;
  Pthread_detach(pthread_self());
  printf("new fd = %d\n", fd);

  strcpy(origin_request_line, buf);
  
  strcpy(remote_host, "");
  strcpy(remote_port, "80");

  Rio_readinitb(&rio_client, fd);
  Rio_readlineb(&rio_client, buf, MAXLINE);
  parse_request_line(buf, method, protocol, host_port,
		     resource, version);
  parse_host(host_port, remote_host, remote_port);
 
  if (strstr(method, "GET") != NULL) {
    //Create request line
    strcpy(request_buf, method);
    strcat(request_buf, " ");
    strcat(request_buf, resource);
    strcat(request_buf, " ");
    strcat(request_buf, http_version_hdr);
   
     // process  header
        while (Rio_readlineb(&rio_client, buf, MAXLINE) != 0) {
            if (strcmp(buf, "\r\n") == 0) {
                break;
            } else if (strstr(buf, "User-Agent:") != NULL) {
                strcat(request_buf, user_agent_hdr);
                has_user_agent_str = 1;
            } else if (strstr(buf, "Accept-Encoding:") != NULL) {
                strcat(request_buf, accept_encoding_hdr);
                has_accept_encoding_str = 1;
            } else if (strstr(buf, "Accept:") != NULL) {
                strcat(request_buf, accept_hdr);
                has_accept_str = 1;
            } else if (strstr(buf, "Connection:") != NULL) {
                strcat(request_buf, connection_hdr);
                has_connection_str = 1;
            } else if (strstr(buf, "Proxy Connection:") != NULL) {
                strcat(request_buf, proxy_connection_hdr);
                has_proxy_connection_str = 1;
            } else if (strstr(buf, "Host:") != NULL) {
                strcpy(origin_host_header, buf);
                if (strlen(remote_host) < 1) {
		  //get host from host header
		  sscanf(buf, "Host: %s", host_port);
		  parse_host(host_port, remote_host, remote_port);
                }
                strcat(request_buf, buf);
                has_host_str = 1;
            } else {
	      strcat(request_buf, buf);
            }
        }
        // Append my headers
        if (has_user_agent_str != 1) {
            strcat(request_buf, user_agent_hdr);
        }
        if (has_accept_encoding_str != 1) {
            strcat(request_buf, accept_encoding_hdr);
        }
        if (has_accept_str != 1) {
            strcat(request_buf, accept_hdr);
        }
        if (has_connection_str != 1) {
            strcat(request_buf, connection_hdr);
        }
        if (has_proxy_connection_str != 1) {
            strcat(request_buf, proxy_connection_hdr);
        }
        if (has_host_str != 1) {
            
	  sprintf(buf, "Host: %s:%s\r\n", remote_host, remote_port);
            strcat(request_buf, buf);
        }
        strcat(request_buf, "\r\n");
	to_server_fd = Open_clientfd(remote_host, atoi(remote_port));
	to_client_fd = fd;
	printf("\n Request Buffer : %s\n ", request_buf);
	printf("to client fd %d\n", to_client_fd);
	printf("to server fd %d\n",to_server_fd);
	printf("\nRemote host : %s \n ", remote_host);
	printf("\n Remote port : %d \n", atoi(remote_port));
	
	if (recieve_and_send(to_client_fd, to_server_fd,request_buf)) {
	  close_fd(&to_client_fd, &to_server_fd);
	 
	}	
  }
  Pthread_exit(NULL);
  return (NULL);
}    

/*
 * parse_request_line - parse request line to different parts
 *
 */
void parse_request_line(char *buf, char *method, char *protocol,
			char *host_port, char *resource, char *version) {
  char url[MAXLINE];
  strcpy(resource, "/");
  sscanf(buf, "%s %s %s", method, url, version);
  if (strstr(url, "://") != NULL) {
    // has protocol
    sscanf(url, "%[^:]://%[^/]%s", protocol, host_port, resource);
  } else {
    // no protocols
    sscanf(url, "%[^/]%s", host_port, resource);
  }
}

/*
 * parse_host_port - parse host:port
 */
void parse_host(char *host_port, char *remote_host, char *remote_port) {
  char *tmp = NULL;
  tmp = index(host_port, ':');
  if (tmp != NULL) {
    *tmp = '\0';
    strcpy(remote_port, tmp + 1);
  } else {
    strcpy(remote_port, "80");
  }
  strcpy(remote_host, host_port);
}

/*recieve_and_send 
 * Recieves data from server
 * Sends to client
 */
int recieve_and_send(int to_client_fd, int to_server_fd,char* request_buf) {
  rio_t rio_server;
  char buf[MAXLINE];
  memset(buf,0,sizeof(buf));
  unsigned int length = 0; 
  
  Rio_readinitb(&rio_server, to_server_fd);
  Rio_writen(to_server_fd, request_buf, strlen(request_buf));
  
  while((length=Rio_readnb(&rio_server,buf,MAXLINE))>0)
    {
      Rio_writen(to_client_fd, buf, length);    
    }
  return 0;
}


/*
 * close_fd - safely close file descriptors
 */
void close_fd(int *to_client_fd, int *to_server_fd) {
    if (*to_client_fd >= 0) {
        Close(*to_client_fd);
    }
    if (*to_server_fd >= 0) {
        Close(*to_server_fd);
    }
}
