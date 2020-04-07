#include <stdlib.h>
#include <stdio.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <string.h>

int main(int argc, char* argv[]) {

	if (argc != 3) {
		printf("Erreur dans le nombre de paramètres\nLe premier paramètre est le numéro de PORT et le second paramètre est l'adresse IP du serveur");
		exit(1);
	}

	int dS = socket(PF_INET, SOCK_STREAM, 0);

	if (dS == -1) {
		perror("Erreur dans la création de la socket");
		exit(1);
	}

	struct sockaddr_in aS;
	aS.sin_family = AF_INET;
	aS.sin_port = htons(atoi(argv[1]));

	int res = inet_pton(AF_INET,argv[2],&(aS.sin_addr));
	if (res == -1) {
		perror("Erreur dans la création de la structure d'adresse réseau");
		exit(0);
	}
	else if (res == 0) {
		perror("Adresse réseau invalide");
		exit(0);
	}

	socklen_t lgA = sizeof(aS);
	res = connect(dS, (struct sockaddr *) &aS, lgA);

	if (res == -1) {
		perror("Erreur à la demande de connexion");
		exit(0);
	}
	else {
		printf("Connexion réussie\n");
		printf("%d\n", res);
	}

	int numeroClient;

	res = recv(dS, &numeroClient, sizeof(int), 0);

	if (res == -1) {
		perror("Erreur lors de la réception du numéro de client");
		exit(1);
	}
	else {
		printf("Vous êtes le client numéro %d\n", numeroClient);
	}


	while (1) {

		char message[100];

		if (numeroClient == 0) {

			printf("Message : ");
			gets(message);

			res = send(dS, message, strlen(message), 0);

			if (res == -1) {
				perror("Erreur lors de l'envoi");
			}
			else {
				if (strcmp(message, "fin\n") == 0) {
					break;
				}
			}

			res = recv(dS, message, sizeof(message), 0);

			if (res == -1) {
				perror("Erreur lors de l'envoi");
			}
			else {
				if (strcmp(message, "fin\n") == 0) {
					break;
				}
				else {
					printf("Message reçu : %s\n", message);
				}
			}
		}
		else {
			res = recv(dS, message, sizeof(message), 0);

			if (res == -1) {
				perror("Erreur lors de l'envoi");
			}
			else {
				if (strcmp(message, "fin\n") == 0) {
					break;
				}
				else {
					printf("Message reçu : %s\n", message);
				}
			}

			printf("Message : ");
			gets(message);

			res = send(dS, message, strlen(message), 0);

			if (res == -1) {
				perror("Erreur lors de l'envoi");
			}
			else {
				if (strcmp(message, "fin\n") == 0) {
					break;
				}
			}
		}
	}

	res = close(dS);

	if (res == -1) {
		perror("Erreur close socket");
		exit(1);
	}
	else {
		printf("Fin client\n");
	}

	return 0;
}