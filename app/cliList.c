// Cliente de usuários

/* Guia:


  Básico
  1- Conecta com o servidor
  2- Espera comandos
    2.1- Se for bump
      2.1.1- Solicita listas
      2.1.2- Recebe lista
      2.1.3- Escreve lista
      2.1.4- Volta pra 2
    2.2- Se for bye
      2.2.1- Finaliza a conexão

  Extras: Autobump, mensagem de autobump.
*/

#include "mysocket.h"
//Registo de par passivo: Nome com 20 caracteres, Ip com 16, porta com 4 caracteres

   
TSocket sock;
char *servIP;                /* server IP */
unsigned short servPort;     /* server port */
char str[100];
int n;

void bumpClientList() {

   printf("\n");

   int actives;
   int i;

   n = strlen(str);
   str[n] = '\n';

   printf("Eu vo mandar pro servidor %s", str);
   if (WriteN(sock, str, ++n) <= 0)
      { ExitWithError("WriteN() failed"); }// envia o bump

   memset(str,0,strlen(str));

   //Como vai ser a resposta: 1 mensagem com o número de clientes e N mensagens para cada cliente. não esquecer de tratar concorrência no servidor

   if (ReadLine(sock, str, 99) < 0)
      { ExitWithError("ReadLine() failed");}

   actives = atoi(str);

   memset(str,0,strlen(str));

   if(!actives)
   {
      printf("Server vazio!\n");
      return;

   }
   else{

      printf("Lista de clientes disponiveis:\n");
      for(i = 0; i < actives; i++)
      {
         if (ReadLine(sock, str, 99) < 0)
            { ExitWithError("ReadLine() failed");} // O que ele vai receber do Server? Nome
         printf("%s",str );
         memset(str,0,strlen(str));
      }

      printf("Fim da lista\n");

   }

   return;
}

void quitClientList() {

   printf("\n");

   n = strlen(str);
   str[n] = '\n';
   if (WriteN(sock, str, ++n) <= 0)
      { ExitWithError("WriteN() failed"); }

   return;
}


int main(int argc, char *argv[]) {

   printf("Aye aye cap'n \n");

   if (argc != 2) {
      ExitWithError("Esse é o cliente da lista de usuários.\nUsage: \\.cliList <remote server IP> <remote server Port>");    
   }

   servIP = argv[1];
   servPort = 2016;
   int goodbye = 0;

   /* Create a connection */
   sock = ConnectToServer(servIP, servPort);

   while(!goodbye) {
      memset(str,0,strlen(str));
   /* Write msg */
      scanf("%99[^\n]%*c",str);

      if(strncasecmp(str,"2",1) == 0)
      {// é um bump
         printf("Bump!\n");
         bumpClientList();
      }

      if(strncasecmp(str,"bye",3) == 0)
      {
         quitClientList();
         goodbye = 1;
      }

      //o mais hot dos hotfixes

      if (strncasecmp(str,"1 Silvana",9) == 0)
      {
         printf("Não fui criado pra isso aqui não, ô preguiçoso\n");
         n = strlen(str);
         str[n] = '\n';

         printf("Eu vo mandar pro servidor %s", str);
         if (WriteN(sock, str, ++n) <= 0)
            { ExitWithError("WriteN() failed"); }

      }

   }

   close(sock);
   return 0;
  
}
