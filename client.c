#include <netdb.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>

#define BUFFER_SIZE 255

void put_file(int sockfd, char * path, char * server_path);
void get_file(int sockfd, char * server_path);

struct cabecera_transaccion {
	int tipo;
	int tamanio;
	char ruta[150];
  char nombreArchivo[50];
};

typedef struct cabecera_transaccion transaccion;

int main(int argc, char * argv[]) {
  int s;
  struct addrinfo hints;
  struct addrinfo * info;
  int res;

  //TODO Validar argumentos de linea de comandos
  if (argc > 5) {
    fprintf(stderr, "Argumentos insuficientes\n");
    exit(EXIT_FAILURE);
  }

  //Configurar la direccion del servidor
  memset(&hints, 0, sizeof(struct addrinfo));
  hints.ai_family = PF_INET; //IP v4
  hints.ai_socktype = SOCK_STREAM;

  res = getaddrinfo(argv[2], "1234", &hints, &info); //Puerto 1234
  if (res != 0) {
    gai_strerror(res);
    exit(EXIT_FAILURE);
  }
  //crear un socket TCP
  s = socket(info->ai_family, info->ai_socktype, info->ai_protocol);
  if (s < 0) {
    perror("socket");
    exit(EXIT_FAILURE);
  }
  //conectar al servidor
  if (connect(s, info->ai_addr, info->ai_addrlen) != 0) {
    perror("bind");
    exit(EXIT_FAILURE);
  }	
  //armar el encabezado y enviar al servidor write(s, ...)

  //si argv[1] es "put"
    //path = argv[3]
    //server_path = argv[4] o vacío
    // put_file(s, path, server_path);
  //si argv[1] es "get"
    //server_path = argv[3]
    // get_file(s, server_path);
    int accionEncontrada = 0;
    if (strcmp(argv[1], "put") == 0) {
      if (argc > 4) {
        put_file(s, argv[3], argv[4]);
      } else {
        put_file(s, argv[3], "");
      }      
      accionEncontrada = 1;
    }
    if (strcmp(argv[1], "get") == 0) {
      accionEncontrada = 1;
    }
    if (!accionEncontrada) {
      fprintf(stderr, "\nOpcion invalida.");
      exit(EXIT_FAILURE);
    }

  close(s);
   exit(EXIT_SUCCESS);
}

void put_file(int sockfd, char * path, char * server_path) {
	struct stat st;
  char buffer[BUFFER_SIZE];
	transaccion cabecera;
  int bytesLeidos, fdArchivo;
  printf("\npath: %s", path);
  fflush(stdout);
  if (stat(path, &st) != 0) {
    perror("stat");
    exit(EXIT_FAILURE);    
  }
	cabecera.tipo = 1;
	cabecera.tamanio = st.st_size;
	strcpy(cabecera.ruta, server_path);
  strcpy(cabecera.nombreArchivo, path);	
	write(sockfd, &cabecera, sizeof(transaccion));
  fdArchivo = open(path, O_RDONLY);
  if (fdArchivo == -1) {
    perror("open");
    exit(EXIT_FAILURE);
  }
  do {
    bytesLeidos = read(fdArchivo, buffer, BUFFER_SIZE);
    if (bytesLeidos > 0) {
      write(sockfd, buffer, bytesLeidos);
    }
  } while (bytesLeidos > 0);

}
void get_file(int sockfd, char * server_path) {
	
}
