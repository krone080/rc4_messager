#include <stdio.h>
#include <stdlib.h>
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
#define TXTSIZE 256

int sock,sockls,sockcn,sockls_new;

struct msg_t
 {
 char log[LOGSIZE];
 char mtext[TXTSIZE];
 };

//Для программы месседжера выберем обмен массивами типа char,
//следовательно, n=8
void RC4crypt(const char *input, unsigned msg_len, char *output, const char *key, unsigned key_len);

//Пытался сделать протокол Диффи-Хелмана
//void simple256DH(char *gen_key, int sock)
// {
//A=g^a(mod p)
// mpz_t A[32],g,a,p;
// mpz_inits(g,p,a,NULL);
// for(unsigned i=0;i<32;++i)
//  mpz_init(A[i]);
//p=2^64, посчитано на Wolfram
// mpz_set_str(p,"18446744073709551616",10);
//p=2^31-1, walfram говорит, что число простое, является ли оно первообразным корнем сказать трудно,
//по крайней мере, оно точно взаимно просто с 2^256, и его показатель не 2 (среди делителей 2^64-2^63)
// mpz_set_str(g,"2147483647",10);
 
// gmp_randstate_t state;
// gmp_randinit_default(state);
// gmp_randseed_ui(state,time(NULL));
// for(unsigned i=0;i<32;++i)
//  {
//  mpz_urandomb(a,state,64);
//  }
// }

void* th0_func(void *arg);
void* th1_func(void *arg);
void* th2_func(void *arg);

int main()
 {
 setlocale(LC_ALL, "");
 //функция инициализации библиотеки ncurses и функция для включения прокручивания окна
 initscr();
 scrollok(stdscr,TRUE);

 struct sockaddr_in c1addr;
 u_int16_t port;
 
 struct msg_t snd_msg,buff_msg;

 printw("Input port:");
 refresh();
 scanw("%i",&port);

 //сокет, который будет пытаться соединиться через connect
 sockcn=socket(AF_INET,SOCK_STREAM,0);
 if(sockcn==-1)
  {
  printw("socket(): %s\n",strerror(errno));
  refresh();
  return -1;
  }

 //сокет, который будет слушать запросы на соединение через listen
 sockls=socket(AF_INET,SOCK_STREAM,0);
 if(sockls==-1)
  {
  printw("socket(): %s\n",strerror(errno));
  refresh();
  return -1;
  }

 memset((void*)&c1addr,0,sizeof(c1addr));
 c1addr.sin_family=AF_INET;
 c1addr.sin_port=htons(port);
 c1addr.sin_addr.s_addr=inet_addr("127.0.0.1");

 if(bind(sockls,(struct sockaddr*)&c1addr,sizeof(c1addr))==-1)
  {
  printw("bind(): %s\n",strerror(errno));
  refresh();
  return -1;
  }

 printw("Input your name (max lenght %i):",LOGSIZE);
 refresh();
 memset(buff_msg.log,0,LOGSIZE);
 getstr(buff_msg.log);

 void* retval=NULL;
 pthread_t thid[3];

 //либо подключаемся к удалённому сокету, либо подключаются к нашу сокету
 //пока одно из двух не выполнится, из цикла не выходим
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

 printw("\nEVERYTHING IS NICE\n\n"
        "Max message length is %i\n"
        "Input key (max length %i): ",TXTSIZE,256);

 //Ввод ключа, который потом будет использоваться при шифровании и расшифровании
 char key[257];
 memset((void*)key,0,257);
 getstr(key);

 //создаём поток для получения и вывода сообщений
 pthread_create(&thid[2],NULL,th2_func,(void*)key);

 time_t t;
 struct tm *tmtime;
 char time_buff[10];

 //Набор, шифрование и отправка сообщения, а также вывод его у себя на экране
 //в определённом формате
 while(1)
  {
  //набираем сообщение
  printw("you>> ");
  refresh();
  memset((void*)buff_msg.mtext,0,TXTSIZE);
  getstr(buff_msg.mtext);
  
  //шифруем
  memset((void*)snd_msg.log,0,LOGSIZE);
  RC4crypt(buff_msg.log,strlen(buff_msg.log),snd_msg.log,key,strlen(key));

  memset((void*)snd_msg.mtext,0,TXTSIZE);
  RC4crypt(buff_msg.mtext,strlen(buff_msg.mtext),snd_msg.mtext,key,strlen(key));
  //отправляем
  send(sock,(void*)&snd_msg,sizeof(snd_msg),0); 

  //выводим у себя отправленное сообщение
  t=time(NULL);
  tmtime=localtime(&t);
  strftime(time_buff, 10, "%T", tmtime);
  mvprintw(getcury(stdscr)-(1+(6+strlen(buff_msg.mtext))/getmaxx(stdscr)),0,
           "<you, %s> %s\n%s\n\n",buff_msg.log,time_buff,buff_msg.mtext);
  }

 endwin();
 return 0;
 }

void RC4crypt(const char *input, unsigned msg_len, char *output, const char *key, unsigned key_len)
 {
 unsigned char array[256];
 unsigned char j,tmp,t,i;

 for(unsigned k=0;k<256;++k)
  array[k]=k;

 j=0;

 for(unsigned k=0;k<256;++k)
  {
  j=j+array[k]+key[k%key_len];//%256

  tmp=array[k];
  array[k]=array[j];
  array[j]=tmp;
  }

 i=j=0;

 for(unsigned k=0;k<msg_len;++k)
  {
  ++i;//%256
  j+=array[i];//%256

  tmp=array[i];
  array[i]=array[j];
  array[j]=tmp;

  t=array[i]+array[j];
  output[k]=input[k]^array[t];
  }
 }

//В этом потоке программа пытается подключиться к удалённому сокету
void* th0_func(void *arg)
 {
 pthread_setcancelstate(PTHREAD_CANCEL_ENABLE,0);

 struct sockaddr_in c2addr;
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

//В этом потоке программа слушает запросы на соединение и соединяется, если надо
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
 echo();
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

//В этом потоке программа принимает сообщения, расшифровывает их и выводин на экран
void* th2_func(void *arg)
 {
 time_t t;
 struct tm *tmtime;
 char time_buff[10];
 struct msg_t rcv_msg,buff_msg;

 while(1)
  {
  if(recv(sock,(void*)&buff_msg,sizeof(rcv_msg),0)==-1)
   {
   printw("Probably connection is lost\nrecv(): %s\n",strerror(errno));
   refresh();
   exit(1);
   }
  t=time(NULL);
  tmtime=localtime(&t);
  strftime(time_buff, 10, "%T", tmtime);

  memset((void*)rcv_msg.log,0,LOGSIZE);
  RC4crypt(buff_msg.log,strlen(buff_msg.log),rcv_msg.log,(char*)arg,strlen((char*)arg));
  memset((void*)rcv_msg.mtext,0,TXTSIZE);
  RC4crypt(buff_msg.mtext,strlen(buff_msg.mtext),rcv_msg.mtext,(char*)arg,strlen((char*)arg));
  mvprintw(getcury(stdscr),0,"<%s> %s\n%s\n\nyou>> ",rcv_msg.log,time_buff,rcv_msg.mtext);
//  setsyx(getcury(stdscr)+(1+(6+strlen(rcv_msg.mtext))/getmaxx(stdscr)),6);
  refresh();
  }
 return NULL;
 }
