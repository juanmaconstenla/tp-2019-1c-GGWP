/*
 * GossipingKernel.c
 *
 *  Created on: 15 jul. 2019
 *      Author: utnso
 */

#include "Planificador.h"

int pideRetardoGossiping() {
	tPaquete* msjeEnviado = malloc(sizeof(tPaquete));
	msjeEnviado->type = HANDSHAKE;
	strcpy(msjeEnviado->payload, "Pedido de RETARDO_GOSSIPING a Memoria");
	msjeEnviado->length = sizeof(msjeEnviado->payload);
	enviarPaquete(socketMemoria, msjeEnviado, logger,
			"Kernel realiza pedido de Retardo de Gossiping a Memoria");
	liberarPaquete(msjeEnviado);

	tPaquete* msjeRecibido = malloc(sizeof(tPaquete));
	recv(socketMemoria, msjeRecibido, sizeof(tPaquete), 0);
	int retardoGossiping = atoi(msjeRecibido->payload);
	liberarPaquete(msjeRecibido);

	return retardoGossiping;
}

void armarNodoMemoria(TablaGossip* nodo) {
	// Cargo datos faltantes del nodo
	if(nodo->IPMemoria != configuracion->IP_MEMORIA && nodo->puertoMemoria != configuracion->PUERTO_MEMORIA)
		nodo->socketMemoria = 1;
	else
		nodo->socketMemoria = socketMemoria;
	nodo->criterioSC = 0;
	nodo->criterioSHC = 0;
	nodo->criterioEC = 0;
	// Si el nodo no está en listaGossiping, lo agrego, me conecto y creo hilo de respuestas
	if (!nodoEstaEnLista(listaGossiping, nodo)) {
		list_add(listaGossiping, nodo);
		if(nodo->socketMemoria == 1)
			conectarConNuevaMemoria(nodo);
	} else
		free(nodo); // Si el nodo ya se encontraba en listaGossiping, lo libero
}

void pideListaGossiping(int socketMem) {
	tPaquete* msjeEnviado = malloc(sizeof(tPaquete));
	msjeEnviado->type = GOSSIPING;
	strcpy(msjeEnviado->payload, "Kernel pide Lista de Gossiping a Memoria");
	msjeEnviado->length = sizeof(msjeEnviado->payload);
	enviarPaquete(socketMem, msjeEnviado, logger,
			"Kernel realiza pedido de Lista de Gossiping a Memoria");
	liberarPaquete(msjeEnviado);

	int status;
	int cantElementosListaRecibida;

	recv(socketMem, &cantElementosListaRecibida, sizeof(int), 0);

	for (int i = 0; i < cantElementosListaRecibida; i++) {
		TablaGossip* nodoRecibido = malloc(sizeof(TablaGossip));
		status = recibirNodoYDeserializar(nodoRecibido, socketMem);
		if (status)
			armarNodoMemoria(nodoRecibido);
	}
}

void conectarConNuevaMemoria(TablaGossip* nodo) {
	// Cada vez que se conoce una nueva Memoria por Gossiping, Kernel se conecta a ella
	nodo->socketMemoria = connectToServer(nodo->IPMemoria, nodo->puertoMemoria,
			logger);
	// Si la conexión no falló, crea un hilo para las respuestas de esa nueva Memoria
	if (nodo->socketMemoria != 1) {
		sem_wait(&loggerSemaforo);
		log_info(logger, "Kernel se conectó correctamente a la Memoria %d",
				nodo->IDMemoria);
		sem_post(&loggerSemaforo);
		int *socket_m = malloc(sizeof(*socket_m));
		*socket_m = nodo->socketMemoria; //Solo cambia el socket de la memoria nueva
		crearHiloIndependiente(&hiloRespuestasRequest, (void*) respuestas,
				(void*) socket_m, "Kernel(Respuestas)");
		tPaquete* mensaje = malloc(sizeof(tPaquete));
		mensaje->type = DESCRIBE;
		strcpy(mensaje->payload,"DESCRIBE");
		mensaje->length = sizeof(mensaje->payload);
		int resultado = enviarPaquete(nodo->socketMemoria, mensaje,logger,"Ejecutar comando DESCRIBE desde Kernel.");
		liberarPaquete(mensaje);

		if (resultado > 0){
			sem_wait(&loggerSemaforo);
			log_info(logger, "'DESCRIBE' enviado exitosamente a Memoria");
			sem_post(&loggerSemaforo);
		}
	}
}

void gossipingKernel() {
	// Pide a la Memoria conectada por Archivo de Configuración el Retardo de Gossiping
	int retardoGossiping = pideRetardoGossiping();

	sem_wait(&loggerSemaforo);
	log_info(logger, "Kernel hace Gossiping con Memoria");
	sem_post(&loggerSemaforo);
	// Pide a la memoria del archivo de configuración la Lista de Gossiping
	pideListaGossiping(socketMemoria);

	while (1) {
		sleep(retardoGossiping / 1000);

		sem_wait(&loggerSemaforo);
		log_info(logger, "Kernel hace Gossiping con Memoria");
		sem_post(&loggerSemaforo);
		if (listaGossiping->elements_count > 0) {
			// Hago Gossiping siempre con la primera Memoria que esté en listaGossiping (Si no se desconectó, va a ser la que tenemos en el Archivo de Configuración)
			TablaGossip* nodoTablaGossipAux = listaGossiping->head->data;
			pideListaGossiping(nodoTablaGossipAux->socketMemoria);
		}
	}
}

int recibirNodoYDeserializar(TablaGossip *nodo, int socketMem) {
	int status;

	status = recv(socketMem, &(nodo->IDMemoria), sizeof(nodo->IDMemoria), 0);
	if (!status)
		return 0;

	status = recv(socketMem, nodo->IPMemoria, sizeof(nodo->IPMemoria), 0);
	if (!status)
		return 0;

	status = recv(socketMem, &(nodo->puertoMemoria),
			sizeof(nodo->puertoMemoria), 0);
	if (!status)
		return 0;

	return status;
}

int nodoEstaEnLista(t_list* lista, TablaGossip* nodo) {
	t_link_element* nodoActual = lista->head;
	TablaGossip* nodoAux;

	if (nodoActual)
		nodoAux = nodoActual->data;

	while (nodoActual && nodoAux->IDMemoria != nodo->IDMemoria) {
		nodoActual = nodoActual->next;
		if (nodoActual)
			nodoAux = nodoActual->data;
	}
	if (nodoActual)
		return 1;
	else
		return 0;
}
