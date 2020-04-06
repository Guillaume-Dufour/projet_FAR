#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <string.h>


//Cette fonction nous sera utile plus tard
//cette fonction va couper en morceau les messages qui ont remplis le buffer, ici on découpe en paquet de 124
int recvTCP(int sock, char *msg, int sizeOctet, int option,int *cpt){
	int nbtotalrecv = 0;
	int res=-2;
	while(sizeOctet>nbtotalrecv && res!=0){
                res= recv(sock,msg,124,0);
		if(res==-1 || res == 0){
			return res;
		}else{
                        *cpt=*cpt+1;
			nbtotalrecv = nbtotalrecv + res;
		}
	}
	return nbtotalrecv;
}

//serveur
int main(int argc, char *argv[]){
        if(argc == 2){
                int dS = socket(PF_INET,SOCK_STREAM,0);
                if(dS==-1){
                        perror("erreur lors de la création de la socket");
                        exit(0);
                }
                printf("socket créée\n");
                struct sockaddr_in ad;
                ad.sin_family = AF_INET;
                ad.sin_addr.s_addr = INADDR_ANY;
                ad.sin_port = htons(atoi(argv[1]));
                printf("avant le bind\n");
                int resb = bind(dS, (struct sockaddr*)&ad,sizeof(struct sockaddr_in));
                if(resb == -1){
                        perror("erreur de bind\n");
                        exit(0);
                }else{
                        printf("bind ok\n");
                }
                printf("après le bind, serveur en écoute\n");
                int nb_u=0; //nb utilisateurs connectés
                while(nb_u<2){
                        listen(dS,7);
                        struct sockaddr_in aC;
                        socklen_t lg=sizeof(struct sockaddr_in);
                        
                        int dSC = accept(dS, (struct sockaddr*) &aC,&lg);
                        if(dSC == -1){
                                perror("l'utilisateur n'a pas été accepté\n");
                        }else{
                                printf("l'utilisateur à été accepté\n");
                        }
                        nb_u=nb_u+1;
                }
                

                char msg[2000];
                
                printf("attente de l'envoi d'un message\n");
                
                //On reçoit donc le bon nombre d'octets et les bons messages envoyés
                int *cpt = 0;
                int rc;
                int total=0;
                int cpt2=0;
                while(cpt2<100){

                        rc = recvTCP(dSC,msg,124,0,&cpt);
                        if(rc==-1){
                                perror("erreur de reception");
                        }else if(rc!=0){
                                printf("serveur reçoit %d octets\n", rc);
                                printf("nombre de passage %d \n", cpt2);
                                total=total+rc;
                                cpt2=cpt2+1;
                        }else{
                                break;
                        }
                        
                }
                printf("vous avez reçu %d octets en %d paquets\n",total,cpt2);
        
                close(dSC);
                close(dS);
        }else{
                printf("le nombre de paramètres doit être de 1 (PORT)\n");
                exit(0);
        }
        
          
        
        
}

