#include <indigo/indigo_bus.h>
#include <indigo/indigo_server_tcp.h>
#include <pthread.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <stdio.h>
static void cb(int n) { }
static void *server(void *p) { indigo_server_start(cb); return NULL; }
int main(void) {
 indigo_start(); indigo_use_bonjour=false;indigo_use_blob_caching=true;indigo_server_tcp_port=0;
 indigo_client client={0}; indigo_device device={0};
 indigo_property *p=indigo_init_blob_property_p(NULL,"Audit synthetic","IMAGE","Audit","Image",INDIGO_OK_STATE,INDIGO_WO_PERM,1);
 indigo_init_blob_item(p->items,"IMAGE","Image");
 indigo_define_property_to_client(&device,&client,p,NULL);
 if(!indigo_validate_blob(p->items))return 3;
 pthread_t tid;pthread_create(&tid,NULL,server,NULL);
 for(int n=0;n<500&&indigo_server_tcp_port==0;n++)usleep(10000);
 int fd=socket(AF_INET,SOCK_STREAM,0);struct sockaddr_in a={.sin_family=AF_INET,.sin_port=htons(indigo_server_tcp_port),.sin_addr.s_addr=htonl(INADDR_LOOPBACK)};
 if(connect(fd,(void*)&a,sizeof a))return 4;
 char req[256];int n=snprintf(req,sizeof req,"PUT /blob/%p.raw HTTP/1.1\r\nContent-Length: -1\r\n\r\n",p->items);write(fd,req,n);
 usleep(500000);puts("PROCESS SURVIVED");return 5;
}
