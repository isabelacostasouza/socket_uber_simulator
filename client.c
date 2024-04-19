#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

// Definição da estrutura para armazenar as coordenadas do cliente
typedef struct {
    double latitude;
    double longitude;
} Coordinate;

// Coordenadas fixas do cliente
Coordinate coordCli = {-19.9206, -43.9596};

int main(int argc, char *argv[]) {
    while(1) {
        int callDriver;
        printf("0 - Sair\n1 - Solicitar Corrida\n");
        scanf("%d", &callDriver);

        if(callDriver == 0) {
            return 0;
        }
        
        // Verificar o número correto de argumentos da linha de comando
        if (argc != 4) {
            fprintf(stderr, "Uso: %s <ipv4 | ipv6> <IP do servidor> <porta>\n", argv[0]);
            exit(EXIT_FAILURE);
        }

        struct sockaddr_in serv_addr; // Estrutura para endereços IPv4
        struct sockaddr_in6 serv_addr6; // Estrutura para endereços IPv6
        int sock = 0, valread;
        int addrlen;

        // Criar socket
        if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
            printf("\n Erro na criação do socket\n");
            return -1;
        }

        // Configurar a família de endereços e a porta
        if (strcmp(argv[1], "ipv4") == 0) {
            memset(&serv_addr, '0', sizeof(serv_addr));
            serv_addr.sin_family = AF_INET;
            serv_addr.sin_port = htons(atoi(argv[3]));
            addrlen = sizeof(serv_addr);
            if(inet_pton(AF_INET, argv[2], &serv_addr.sin_addr)<=0) {
                printf("\nEndereço inválido/Endereço não suportado \n");
                return -1;
            }
        } else if (strcmp(argv[1], "ipv6") == 0) {
            memset(&serv_addr6, '0', sizeof(serv_addr6));
            serv_addr6.sin6_family = AF_INET6;
            serv_addr6.sin6_port = htons(atoi(argv[3]));
            addrlen = sizeof(serv_addr6);
            if(inet_pton(AF_INET6, argv[2], &serv_addr6.sin6_addr)<=0) {
                printf("\nEndereço inválido/Endereço não suportado \n");
                return -1;
            }
        } else {
            fprintf(stderr, "Parâmetro de IP inválido. Use 'ipv4' ou 'ipv6'.\n");
            return -1;
        }

        // Conectar ao servidor
        if (strcmp(argv[1], "ipv4") == 0) {
            if (connect(sock, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0) {
                printf("\n Falha na conexão \n");
                return -1;
            }
        } else if (strcmp(argv[1], "ipv6") == 0) {
            if (connect(sock, (struct sockaddr *)&serv_addr6, sizeof(serv_addr6)) < 0) {
                printf("\n Falha na conexão \n");
                return -1;
            }
        }

		// Checar a resposta do motorista pra solicitação de corrida
        int continueDrive = 0;
        char buffer[1024] = {0};
        while(1) {
            valread = read(sock, buffer, 1024);
            if(strstr(buffer, "Motorista encontrado")) {
                continueDrive = 1;
            } else {
                printf("Não foi encontrado um motorista\n");
            }
            break;
        }

        if(continueDrive == 1) {
            // Enviar coordenadas ao servidor
            send(sock , &coordCli, sizeof(Coordinate) , 0 );

            // Receber e exibir resposta do servidor com a distância do motorista em tempo real
            while(1) {
                valread = read(sock, buffer, 1024);
                if (strstr(buffer, "O motorista chegou!") || strcmp(buffer, "O motorista chegou!") == 0) {
                    printf("O motorista chegou!\n");
                    return 0;
                }
                printf("%s\n", buffer);
                sleep(2);
            }
        }

        close(sock);
    }
    return 0;
}