#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <pthread.h>
#include <time.h>
#include <locale.h>
#include <ncurses.h>
#include <errno.h>

#define LOGSIZE 64
#define TXTSIZE 1024

int sock,sockls,sockcn,sockls_new;

//Для программы месседжера выберем обмен массивами типа char,
//следовательно, n=8
void rc4crypt(const char *input, unsigned msg_len, char *output, const char *key, unsigned key_len);

void* th0_func(void *arg);
void* th1_func(void *arg);
void* th2_func(void *arg);

int main(int argc, char *argv[])
 {
 setlocale(LC_ALL, "");
 initscr();
 scrollok(stdscr,TRUE);
 echo();

 //1. инициализация сокета ip-адресом
 int sock2;
 struct sockaddr_in c1addr;
 u_int16_t port;
 struct mymsgbuf
  {
  char log[LOGSIZE];
  char mtext[TXTSIZE];
  } snd_msg;

 printw("Input port:");
 refresh();
 scanw("%i",&port);

 sockcn=socket(AF_INET,SOCK_STREAM,0);
 if(sockcn==-1)
  {
  perror("socket()");
  return -1;
  }

 sockls=socket(AF_INET,SOCK_STREAM,0);
 if(sockls==-1)
  {
  perror("socket()");
  return -1;
  }
 memset((void*)&c1addr,0,sizeof(c1addr));
 c1addr.sin_family=AF_INET;
 c1addr.sin_port=htons(port);
 c1addr.sin_addr.s_addr=inet_addr("127.0.0.1");

 if(bind(sockls,(struct sockaddr*)&c1addr,sizeof(c1addr))==-1)
  {
  perror("bind()");
  return -1;
  }

 printw("Input your name:");
 refresh();
 getstr(snd_msg.log);

 //2. интерактив с выбором конечного узла (сохранённые)
 void* retval=NULL;
 pthread_t thid[3];

 while(1)
  {
  pthread_create(&thid[0],NULL,th0_func,(void*)&thid[1]);
  pthread_create(&thid[1],NULL,th1_func,(void*)&thid[0]);
  pthread_join(thid[0],&retval);
  if(retval==(void*)0)
   break;
  pthread_join(thid[1],&retval);
  if(retval==(void*)0)
   break;
  }

 printw("\nEVERYTHING IS NICE\n\n");
 refresh();

 pthread_create(&thid[2],NULL,th2_func,NULL);

 time_t t;
 struct tm *tmtime;
 char buff[10];
 while(1)
  {
  printw("you>> ");
  refresh();
  memset((void*)snd_msg.mtext,0,TXTSIZE);
  getstr(snd_msg.mtext);
  send(sock,(void*)&snd_msg,sizeof(snd_msg),0);

  t=time(NULL);
  tmtime=localtime(&t);
  strftime(buff, 10, "%T", tmtime);
  getmaxx(stdscr);
  mvprintw(getcury(stdscr)-(1+(6+strlen(snd_msg.mtext))/getmaxx(stdscr)),0,
           "<you, %s> %s\n%s\n\n",snd_msg.log,buff,snd_msg.mtext);
  }
 //3. fork(): один пишет, другой принимает
 endwin();
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

void* th0_func(void *arg)
 {
 pthread_setcancelstate(PTHREAD_CANCEL_ENABLE,0);

 struct sockaddr_in c2addr;
 socklen_t c2sock_len;
 u_int16_t port;
 char ip_addr[16];
 memset((void*)ip_addr,0,16);

 printw("\n\rInput ip-address to connect:");
 refresh();
 scanw("%s",ip_addr);
 printw("Input port:");
 refresh();
 scanw("%i",&port);

 memset((void*)&c2addr,0,sizeof(c2addr));
 c2addr.sin_family=AF_INET;
 c2addr.sin_port=htons(port);
 c2addr.sin_addr.s_addr=inet_addr(ip_addr);

 pthread_cancel(*(pthread_t*)arg);
 if(connect(sockcn,(struct sockaddr*)&c2addr,sizeof(c2addr))<0)
  {
  printw("connect(): %s\n",strerror(errno));
  refresh();
  pthread_exit((void*)1);
  }

 sock=sockcn;
 pthread_exit((void*)0);
 }

void* th1_func(void *arg)
 {
 pthread_setcancelstate(PTHREAD_CANCEL_ENABLE,0);

 struct sockaddr_in c2addr;
 socklen_t c2sock_len;
 char reply;

 listen(sockls,1);

 c2sock_len=sizeof(c2addr);
 sockls_new=accept(sockls,(struct sockaddr*)&c2addr,&c2sock_len);

 pthread_cancel(*(pthread_t*)arg);
 printw("\nThere is a request for connection from:\n"
        "ip %s:%i \naccept (Y/N)?:",inet_ntoa(c2addr.sin_addr),ntohs(c2addr.sin_port));
 refresh();
 while(1)
  {
  scanw("%c",&reply);
  if(reply=='Y')
   break;
  if(reply=='N')
   {
   close(sockls_new);
   pthread_exit((void*)1);
   }
  printw("Try again, accept (Y/N)?:");
  refresh();
  }

 sock=sockls_new;
 pthread_exit((void*)0);
 }

void* th2_func(void *arg)
 {
 time_t t;
 struct tm *tmtime;
 char buff[10];
 struct mymsgbuf
  {
  char log[LOGSIZE];
  char mtext[TXTSIZE];
  } rcv_msg;

 while(1)
  {
  recv(sock,(void*)&rcv_msg,sizeof(rcv_msg),0);
  t=time(NULL);
  tmtime=localtime(&t);
  strftime(buff, 10, "%T", tmtime);
  //расшифровка
  //getmaxyx(stdscr,row,col);
  mvprintw(getcury(stdscr),0,"<%s> %s\n%s\n\nyou>> ",rcv_msg.log,buff,rcv_msg.mtext);
  refresh();
  }
 return NULL;
 }
