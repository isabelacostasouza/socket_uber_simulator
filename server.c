#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <math.h>

// Estrutura para armazenar coordenadas (latitude e longitude)
typedef struct {
    double latitude;
    double longitude;
} Coordinate;

// Coordenadas do serviço de transporte
Coordinate coordServ = {-19.9227, -43.9451}; 

// Função para calcular a distância entre duas coordenadas usando a fórmula de Haversine
static double haversine(double lat1, double lon1, double lat2, double lon2) {
    // Conversão de graus para radianos
    double dLat = (lat2 - lat1) * M_PI / 180.0;
    double dLon = (lon2 - lon1) * M_PI / 180.0;
    lat1 = (lat1) * M_PI / 180.0;
    lat2 = (lat2) * M_PI / 180.0;

    // Cálculo da distância usando a fórmula de Haversine
    double a = pow(sin(dLat / 2), 2) + pow(sin(dLon / 2), 2) * cos(lat1) * cos(lat2);
    double rad = 6371; // Raio médio da Terra em quilômetros
    double c = 2 * asin(sqrt(a));
    return rad * c * 1000; // Convertendo para metros
}

int main(int argc, char *argv[]) {
    // Verificar se o número de argumentos é válido
    if (argc != 3) {
        fprintf(stderr, "Uso: %s <ipv4 | ipv6> <porta>\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    int server_fd, new_socket, valread;
    struct sockaddr_in6 address6;
    struct sockaddr_in address;
    int opt = 1;
    int addrlen;

    // Criar socket file descriptor
    if (strcmp(argv[1], "ipv4") == 0) {
        if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == 0) {
            perror("socket failed");
            exit(EXIT_FAILURE);
        }
        address.sin_family = AF_INET;
        address.sin_addr.s_addr = INADDR_ANY;
        address.sin_port = htons(atoi(argv[2]));
        addrlen = sizeof(address);
    } else if (strcmp(argv[1], "ipv6") == 0) {
        if ((server_fd = socket(AF_INET6, SOCK_STREAM, 0)) == 0) {
            perror("socket failed");
            exit(EXIT_FAILURE);
        }
        address6.sin6_family = AF_INET6;
        address6.sin6_addr = in6addr_any;
        address6.sin6_port = htons(atoi(argv[2]));
        addrlen = sizeof(address6);
    } else {
        fprintf(stderr, "Parâmetro de IP inválido. Use 'ipv4' ou 'ipv6'.\n");
        exit(EXIT_FAILURE);
    }

    // Permitir a reutilização do endereço e porta
    if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, &opt, sizeof(opt))) {
        perror("setsockopt");
        exit(EXIT_FAILURE);
    }

    // Vincular o socket ao endereço
    if (strcmp(argv[1], "ipv4") == 0) {
        if (bind(server_fd, (struct sockaddr *)&address, sizeof(address))<0) {
            perror("bind failed");
            exit(EXIT_FAILURE);
        }
    } else if (strcmp(argv[1], "ipv6") == 0) {
        if (bind(server_fd, (struct sockaddr *)&address6, sizeof(address6))<0) {
            perror("bind failed");
            exit(EXIT_FAILURE);
        }
    }

    // Esperar por conexões
    if (listen(server_fd, 3) < 0) {
        perror("listen");
        exit(EXIT_FAILURE);
    }

    while(1) {
        printf("Aguardando solicitação\n");
        if ((new_socket = accept(server_fd, (struct sockaddr *)&address, (socklen_t*)&addrlen))<0) {
            perror("accept");
            exit(EXIT_FAILURE);
        }

        int acceptDrive = 0;
        printf("Corrida disponível:\n0 - Recusar\n1 - Aceitar\n");
        scanf("%d", &acceptDrive);

        // O servidor envia se o motorista aceitou ou não a corrida
        char* message = "Não foi encontrado um motorista";
        if(acceptDrive == 1) message = "Motorista encontrado";
        send(new_socket, message, strlen(message), 0);

        if(acceptDrive == 1) {
            Coordinate coordCli;
            read(new_socket, &coordCli, sizeof(Coordinate));

            // Calcula a distância entre as coordenadas do motorista e do cliente
            double distancia = haversine(coordCli.latitude, coordCli.longitude, coordServ.latitude, coordServ.longitude);
            if(distancia <= 0) {
                printf("O motorista chegou!\n");
                send(new_socket, "O motorista chegou!", strlen("O motorista chegou!"), 0);
                break;
            }
            
            // Enquanto a distancia entre o motorista e o cliente for maior que zero, ela continua diminuindo de  200 em 200 metros
            while (distancia > 0) {
                // Se a distância for menor que 200m, o motorista chegou
                if (distancia <= 400) {
                    printf("O motorista chegou!\n");
                    send(new_socket, "O motorista chegou!", strlen("O motorista chegou!"), 0);
                    break;
                }
                // Senão, uma mensagem com a distância atual é enviada ao cliente
                else {
                    distancia -= 400;
                    char message[50] = "";
                    sprintf(message, "Motorista a %.2f metros", distancia);
                    send(new_socket, message, strlen(message), 0);
                    sleep(2); // Aguarda 2 segundos antes de enviar a próxima atualização
                }
            }
        }

        close(new_socket);
    }

    return 0;
}