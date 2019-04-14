/*
 * Memoria.h
 *
 *  Created on: 12 abr. 2019
 *      Author: utnso
 */

#ifndef MEMORIA_H_
#define MEMORIA_H_


//Ruta del archivo de configuraciones
#define RUTA_CONFIG "Memoria.config"
#define BACKLOG 5 //Creo que se pueden conectar solo 2 clientes pero hay que verificar

#include <stdio.h>

#include <openssl/md5.h>
#include <string.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <netdb.h>
#include <unistd.h>
#include <commons/config.h>
#include <commons/log.h>
#include <arpa/inet.h>

///---------------------VARIABLES A UTILIZAR-------------------------
int socketEscucha;
int	socketActivo;

///---------------------ESTRUCTURA DE CONFIGURACION DE LFS-------------------------

//Estructura para datos del archivo de configuracion
typedef struct {
	char PUERTO[10];
	char IP_FS[20];
	char PUERTO_FS[10];
	char IP_SEEDS[100]; //Verificar
	char PUERTO_SEEDS[100]; //Verificar
	int RETARDO_MEM;
	int RETARDO_FS;
	int TAM_MEM;
	int RETARDO_JOURNAL;
	int RETARDO_GOSSIPING;
	int MEMORY_NUMBER;
} ConfiguracionMemoria;
/*
PUERTO=8001
IP_FS="172.0.0.1"
PUERTO_FS=5003
IP_SEEDS=["172.0.0.2","172.0.0.3"]
PUERTO_SEEDS=[8000,8001]
RETARDO_MEM=600
RETARDO_FS=600
TAM_MEM=2048
RETARDO_JOURNAL=60000
RETARDO_GOSSIPING=30000
MEMORY_NUMBER=1
*/
//Estructura para guardar la configuracion del proceso
ConfiguracionMemoria* configuracion;

void journalization();

#endif /* MEMORIA_H_ */
