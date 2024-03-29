/*
 * API_Kernel.h
 *
 *  Created on: 17 abr. 2019
 *      Author: utnso
 */

#ifndef API_KERNEL_H_
#define API_KERNEL_H_

#include <readline/readline.h>
#include <readline/history.h>
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
#include <sockets.h>
#include <configuraciones.h>
#include <commons/string.h>
#include <parser.h>
#include "Planificador.h"

#define PATH_SCRIPT "LQL.script" //para hacer pruebas

int socketElegido;

/* --------------------    Definición de Funciones    -------------------- */

void API_Kernel(void);
int ejecutarSelect(char* instruccion);
int ejecutarInsert(char* instruccion);
void ejecutarCreate(char* instruccion);
void ejecutarDescribe(char* instruccion);
int ejecutarDrop(char* instruccion);
void ejecutarJournal(char* instruccion);
void ejecutarAdd(char* instruccion);
int ejecutarRun(char* instruccion, int requestEjecutadas);
void respuestas(void* socket_Mem);
int elegirSocketMemoria(char* tabla, int key);
int elegirSocketMemoria_CREATE(char* criterio);
#endif /* API_KERNEL_H_ */
