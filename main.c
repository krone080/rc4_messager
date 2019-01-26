#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <pthread.h>
#include <ncurses.h>

#define TXTSIZE 1024
#define PWDSIZE 1024

//Для программы месседжера выберем обмен массивами типа char,
//следовательно, n=8

void rc4crypt(const char *input, char *output, const char *key, unsigned length=256);

int main(int argc, char *argv[])
 {
 //1. инициализация сокета ip-адресом
 int sock1, sock2;
 struct sockaddr_in c1addr,c2addr;
 u_int16_t port;
 struct mymsgbuf
  {
  char mtext[TXTSIZE];
  char pwd[PWDSIZE];
  } msg;

 printf("Input port:");
 scanf("%i",&port);
 port[4]=0;

 sock1=socket(AF_INET,SOCK_STREAM,0);
 if(sock1==-1)
  {
  perror("socket()");
  return -1;
  }
 memset((void*)&c1addr,0,sizeof(c1addr));
 c1addr.sin_family=AF_UNIX;
 c1addr.sin_port=htons(port);
 c1addr.sin_addr.s_addr=inet_addr("127.0.0.1");

 if(bind(sock1,(struct sockaddr*)&c1addr,sizeof(c1addr))==-1)
  {
  perror("bind()");
  return -1;
  }

 //2. интерактив с выбором конечного узла (сохранённые)
 int pfd[2];
 pipe(pfd);
 if(fork()==0)
  {
  close(1);
  close(3);
  dup(4);

  socklen_t c2sock_len;

  listen(sock1,1);
  c2sock_len=sizeof(c2addr);
  accept(sock1,(struct sockaddr*)&c2addr,&c2sock_len);
  while(1)
   {
   recv(sock1,(void*)&msg,sizeof(msg),0);

   }
  }
 close(4);
 //3. fork(): один пишет, другой принимает

 return 0;
 }

void rc4crypt(const char *input, unsigned msg_len, char *output, const char *key, unsigned key_len)
 {
 char S[256];

 for(unsigned i=0;i<256;++i)
  S[i]=i;

 char j=0,X;

 for(unsigned i=0;i<256;++i)
  {
  j=j+S[i]+key[i%key_len];//%256

  X=S[i];
  S[i]=S[j];
  S[j]=X;
  }

 char i=0,K;
 j=0;
 for(unsigned n=0;n<msg_len;++n)
  {
  ++i;//%256
  j=j+S[i];//%256

  X=S[i];
  S[i]=S[j];
  S[j]=X;

  output[n]=input[n]^S[S[i]+S[j]];
  }
 }
