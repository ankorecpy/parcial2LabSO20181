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
void getNombreArchivo(char * rtArchivo);

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

/**
 * @brief Gestiona la petición recibida por parte del cliente
 * @param sockfd descriptor del socket aceptado por el cual se realiza la comunicación con el cliente
 * */
void handle_client(int sockfd) {
  printf("Handle client\n");
  
  transaccion cabecera;
  if (read(sockfd, &cabecera, sizeof(transaccion)) != sizeof(transaccion)) {
		fprintf(stderr, "\nError al recibir encabezado");
		close(sockfd);
		return;		
  }  

  switch (cabecera.tipo) {
	  case 1:
    printf("\nRecibiendo: %s de tamanio %d bytes en el directorio %s", cabecera.nombreArchivo, cabecera.tamanio, cabecera.ruta);
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

/**
 * @brief Se encarga del envio de informacion acerca del archivo que se enviara al cliente, la información se envia por medio de una cabecera struct transaccion, posteriormente se envia el archivo al cliente
 * @param sockfd descriptor del socket aceptado por el cual se realiza la comunicación con el cliente
 * @param cabeceraSolicitud cabecera de peticion de envio de archivo, contiene la referencia de la ruta donde se encuentra el archivo en el servidor
 * */
void send_file(int sockfd, transaccion cabeceraSolicitud) {
	struct stat st;
  int fdEscritura, bytesLeidos, archivoEncontrado = 0, fdArchivo;
	char buffer[BUFFER_SIZE];
  char * ruta = unirCadenas(configuracionPath, cabeceraSolicitud.ruta);
  transaccion cabeceraEnvio;
  printf("\nSolicitud de envio de archivo: %s\n", ruta);
  if (stat(ruta, &st) != 0) {
    fprintf(stderr, "el archivo buscado no existe\n");
    cabeceraEnvio.tipo = 0;
    write(sockfd, &cabeceraEnvio, sizeof(transaccion));
  } else {
    archivoEncontrado = 1;
    cabeceraEnvio.tipo = 1;
    cabeceraEnvio.tamanio = st.st_size;
    char nombreArchivo[strlen(cabeceraSolicitud.nombreArchivo) + 1];
    strcpy(nombreArchivo, cabeceraSolicitud.ruta);
    getNombreArchivo(nombreArchivo);  
    strcpy(cabeceraEnvio.nombreArchivo, nombreArchivo);
    write(sockfd, &cabeceraEnvio, sizeof(transaccion));    
    fdArchivo = open(ruta, O_RDONLY);
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
}
/**
 * @brief Se encarga de recibir informacion acerca del archivo que enviara el cliente, la información se recibe por medio de una cabecera struct transaccion, posteriormente se recibe el arhivo
 * @param sockfd descriptor del socket aceptado por el cual se realiza la comunicación con el cliente
 * @param cabeceraSolicitud cabecera de peticion de recepción de archivo, contiene la referencia de la ruta donde se desea almacenar el archivo en el servidor, el tamaño y su nombre.
 * */
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
  if (stat(ruta, &st) == 0) {
    fprintf(stderr, "el archivo recibido ya existe, no se sobreescribira\n");
    exit(EXIT_FAILURE);
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
/**
 * @brief Se encarga de concatenar una cadena de caracteres seguida de otra.
 * @param cadena1 cadena que sera la primera en la cadena resultante.
 * @param cadena2 cadena que se añade al final de la cadena1.
 * @return la cadena resultante de concatenar cadena1 con cadena2.
 * */
char * unirCadenas(char * cadena1, char * cadena2) {
  int longitud = strlen(cadena1) + strlen(cadena2) + 1;
  char * resultado = (char * ) malloc (sizeof(char) * longitud);
  memset(resultado, 0, longitud);
  strcpy(resultado, cadena1);
  strcat(resultado, cadena2);
  return resultado;
}
/**
 * @brief ejecuta un comando shell
 * @param comando comando shell que se ejecuta.
 * */
void ejecutarComando(char * comando) {
  FILE * elemento = popen(comando, "w");
  pclose(elemento);
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
