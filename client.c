/**
 * @author Sandra Lorena Pisamina <psandra@unicauca.edu.co> Oscar Ordoñez <oscarordonez@unicauca.edu.co>
 * @brief Gestiona el envio y almacenamiento de ficheros.
 *  */

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
void getNombreArchivo(char * rtArchivo);

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
      get_file(s, argv[3]);
      accionEncontrada = 1;
    }
    if (!accionEncontrada) {
      fprintf(stderr, "\nOpcion invalida.");
      exit(EXIT_FAILURE);
    }

  close(s);
   exit(EXIT_SUCCESS);
}

/**
 * @brief envia un archivo al servidor
 * @param sockfd descriptor del socket aceptado por el cual se realiza la comunicación con el servidor.
 * @param path ruta local del archivo que se envia al servidor.
 * @param server_path ruta en el servidor donde se alojará el archivo que se envia.
 * */
void put_file(int sockfd, char * path, char * server_path) {  
	struct stat st;
  char buffer[BUFFER_SIZE], auxPath[strlen(path)];
	transaccion cabecera;
  int bytesLeidos, fdArchivo;
  strcpy(auxPath, path);
  getNombreArchivo(auxPath);
  if (stat(path, &st) != 0) {
    perror("stat");
    exit(EXIT_FAILURE);    
  }
	cabecera.tipo = 1;
	cabecera.tamanio = st.st_size;
	strcpy(cabecera.ruta, server_path);
  strcpy(cabecera.nombreArchivo, auxPath);	
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
/**
 * @brief Se encarga de recibir un archivo del servidor
 * @param sockfd descriptor del socket aceptado por el cual se realiza la comunicación con el servidor.
 * @param ruta en el servidor donde se aloja el archivo a pedir.
 * */
void get_file(int sockfd, char * server_path) {
  int fdEscritura, bytesLeidos = 0, finished = 0, acumBytesLeidos = 0;
  struct stat st;
  char buffer[BUFFER_SIZE];
  transaccion cabeceraSolicitud;
  cabeceraSolicitud.tipo = 2;
	strcpy(cabeceraSolicitud.ruta, server_path);
  write(sockfd, &cabeceraSolicitud, sizeof(transaccion));
  transaccion cabeceraRespuesta;
  read(sockfd, &cabeceraRespuesta, sizeof(transaccion));
  if (cabeceraRespuesta.tipo == 0) {
    fprintf(stderr, "\nEl archivo no se encuentra alojado en el servidor, verifique la ruta");
    exit(EXIT_FAILURE);
  }
  if (stat(cabeceraRespuesta.nombreArchivo, &st) == 0) {
    fprintf(stderr, "\nEl archivo a recibir ya se encuentra almacenado en este directorio");
    exit(EXIT_FAILURE);
  }
  printf("\nRecibiendo %s con tamanio %d\n", cabeceraRespuesta.nombreArchivo, cabeceraRespuesta.tamanio);
  fdEscritura = open(cabeceraRespuesta.nombreArchivo, O_CREAT | O_WRONLY, 00660);
  do {
    memset(&buffer, 0, BUFFER_SIZE);
    bytesLeidos = read(sockfd, buffer, BUFFER_SIZE);
    acumBytesLeidos = acumBytesLeidos + bytesLeidos; 
    if (bytesLeidos <= 0) {
      finished = 1;
    } else {
      write(fdEscritura, &buffer, bytesLeidos);
    }
  } while (finished != 1 && acumBytesLeidos != cabeceraRespuesta.tamanio);
  close(fdEscritura);
  fflush(stdout);
}
/**
 * @brief Obtiene la última cadena depues de la última ocurrencia del caracter '/'.
 * @param rtArchivo cadena de la que se extrae el nombre del archivo.
 * */
void getNombreArchivo(char * rtArchivo) {
	char * sbcadena;	
	int idc;
	sbcadena = strrchr(rtArchivo, '/');
	if (sbcadena != NULL) {
		idc = sbcadena - rtArchivo + 1;
		sbcadena = strchr(sbcadena, rtArchivo[idc]);
		strcpy(rtArchivo, sbcadena);
	}
}
