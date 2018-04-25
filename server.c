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
#define TAM_CONFIG_PATH 50

struct cabecera_transaccion {
	int tipo;
	int tamanio;
	char ruta[150];
  char nombreArchivo[50];
};

typedef struct cabecera_transaccion transaccion;

void handle_sigterm(int sig);
void handle_client(int sockfd);

void send_file(int sockfd, transaccion cabecera);
void receive_file(int sockfd, transaccion cabecera);

char * unirCadenas(char * cadena1, char * cadena2);
void ejecutarComando(char * comando);

int configured;
char configuracionPath[TAM_CONFIG_PATH];
char * comandoCrearDirectorio = "mkdir -p ", * comandoCrearFichero = "touch ";
int finished;


int main(int argc, char * argv[]) {
  struct sigaction act;
  struct addrinfo hints;
  struct addrinfo * info;
  int res, fdConfig;
  int s;
  int c;
  char * rutaConfig = "config.txt";

  //TODO Validar el archivo de configuracion

  //Configurar el manejador de señal SIGTERM
  memset(&act, 0, sizeof(struct sigaction));
  act.sa_handler = handle_sigterm;
  sigemptyset(&act.sa_mask);

  if (sigaction(SIGTERM, &act, NULL) != 0) {
    perror("sigaction");
    exit(EXIT_FAILURE);
  }
  
  //Configurar la direccion del servidor
  memset(&hints, 0, sizeof(struct addrinfo));
  hints.ai_flags = AI_PASSIVE; //Servidor
  hints.ai_family = PF_INET; //IP v4
  hints.ai_socktype = SOCK_STREAM;

  res = getaddrinfo(0, "1234", &hints, &info); //Puerto 1234
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
  //asociar una direccion al socket
  if (bind(s, info->ai_addr, info->ai_addrlen) != 0) {
    perror("bind");
    exit(EXIT_FAILURE);
  }
  //poner disponible el socket
  if (listen(s, 10) != 0) {
    perror("listen");
    exit(EXIT_FAILURE);
  }
  struct stat st;
  if (stat("config.txt", &st)) {
    fprintf(stderr, "ERROR: No se encontró la ruta de configuración. \n");
    exit(EXIT_FAILURE);
  } else {
    fdConfig = open(rutaConfig, O_RDONLY);
    read(fdConfig, configuracionPath, TAM_CONFIG_PATH);
    close(fdConfig);
  }
  finished = 0;
  while (!finished) {
    //aceptar conexiones 
    printf("A la espera de conexion");//LINEA DE GUIA: BORRAR EN CUANTO SE PUEDA
    fflush(stdout);
    c = accept(s, NULL, 0);
    if (c < 0) {
      finished = 1;
      continue;
    }
    //crear una copia, que atendera al cliente handle_client(c);
    if (fork() == 0) {
      handle_client(c);
    }
  }

  exit(EXIT_SUCCESS);
}

void handle_sigterm(int sig) {
  finished = 1;
}

void handle_client(int sockfd) {
  printf("Handle client\n");
  //Si la operacion es "put": receive_file(sockfd);
  //Si la operacion es "get": send_file(sockfd);
  
  transaccion cabecera;
  if (read(sockfd, &cabecera, sizeof(transaccion)) != sizeof(transaccion)) {
		fprintf(stderr, "\nError al recibir encabezado");
		close(sockfd);
		return;		
  }  

  switch (cabecera.tipo) {
	  case 1:
    printf("\nRecibiendo: %s de tamanio %d en %s", cabecera.nombreArchivo, cabecera.tamanio, cabecera.ruta);
		receive_file(sockfd, cabecera);
	  break;
	  case 2:
		send_file(sockfd, cabecera);
	  break;
	  default:
	  break;
  }
  
  fflush(stdout);
  close(sockfd);
  exit(EXIT_SUCCESS);
}

void send_file(int sockfd, transaccion cabecera) {
	
}

void receive_file(int sockfd, transaccion cabecera) {
  struct stat st;
  int fdEscritura, bytesLeidos;
	char buffer[BUFFER_SIZE];
  char * ruta = unirCadenas(configuracionPath, cabecera.ruta);
  char * auxComandoCreacion = unirCadenas(comandoCrearDirectorio, configuracionPath);
  auxComandoCreacion = unirCadenas(auxComandoCreacion, cabecera.ruta);
  if (strlen(cabecera.ruta) > 0) {
    ruta = unirCadenas(ruta, "/");
  }
  ruta = unirCadenas(ruta, cabecera.nombreArchivo);
  if (!stat(ruta, &st)) {
    printf("el archivo recibido ya existe\n");
    fflush(stdout);
    exit(EXIT_SUCCESS);
  } else {
    ejecutarComando(auxComandoCreacion);
  }  
  fdEscritura = open(ruta, O_CREAT | O_WRONLY, 00660);
  do {
    memset(&buffer, 0, BUFFER_SIZE);
    bytesLeidos = read(sockfd, buffer, BUFFER_SIZE);
    if (bytesLeidos <= 0) {
      finished = 1;
    } else {
      write(fdEscritura, &buffer, bytesLeidos);
    }
  } while (finished != 1);
  close(fdEscritura);  
}

char * unirCadenas(char * cadena1, char * cadena2) {
  int longitud = strlen(cadena1) + strlen(cadena2) + 1;
  char * resultado = (char * ) malloc (sizeof(char) * longitud);
  memset(resultado, 0, longitud);
  strcpy(resultado, cadena1);
  strcat(resultado, cadena2);
  return resultado;
}

void ejecutarComando(char * comando) {
  FILE * elemento = popen(comando, "w");
  pclose(elemento);
}
