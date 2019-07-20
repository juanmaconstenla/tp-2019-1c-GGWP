/*
 * GossipingMemoria.c
 *
 *  Created on: 15 jul. 2019
 *      Author: utnso
 */

#include "Memoria.h"

void conectarConNuevaMemoria(TablaGossip* nodo) {
	// Cada vez que se conoce una nueva Memoria del pool por Gossiping, hay que conectarse a ella
	nodo->socketMemoria = connectToServer(nodo->IPMemoria, nodo->puertoMemoria,
			logger);

	// Si la conexión no falló, crea un hilo para las respuestas de esa nueva Memoria
	if (nodo->socketMemoria != 1) {
		sem_wait(&loggerSemaforo);
		log_trace(logger, "Memoria se conectó correctamente a la Memoria %d",
				nodo->IDMemoria);
		sem_post(&loggerSemaforo);
	} else {
		sem_wait(&loggerSemaforo);
		log_error(logger, "Falló la conexión con la Memoria %d",
				nodo->IDMemoria);
		sem_post(&loggerSemaforo);
	}
}

void armarNodoMemoria(TablaGossip* nodo) {
	// Cargo dato faltante del nodo
	nodo->socketMemoria = 1;

	for (int i = 0; i < CANT_MAX_SEEDS; i++) {
		if (nodo->IPMemoria == configuracion->IP_SEEDS[i]
				&& nodo->puertoMemoria == configuracion->PUERTO_SEEDS[i])
			nodo->socketMemoria = socketSEED[i];
	}

	// Si el nodo no está en listaGossiping, lo agrego y me conecto
	if (!nodoEstaEnLista(listaGossiping, nodo)) {
		list_add(listaGossiping, nodo);
		if (nodo->socketMemoria == 1)
			conectarConNuevaMemoria(nodo);
	} else
		free(nodo); // Si el nodo ya se encontraba en listaGossiping, lo libero
}

void recibeLista(int socketMem) {
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

void pideListaGossiping(int socketMem) {
	tPaquete* msjeEnviado = malloc(sizeof(tPaquete));
	msjeEnviado->type = GOSSIPING;
	strcpy(msjeEnviado->payload, "Memoria pide Lista de Gossiping");
	msjeEnviado->length = sizeof(msjeEnviado->payload);
	enviarPaquete(socketMem, msjeEnviado, logger,
			"Memoria realiza pedido de Lista de Gossiping");
	liberarPaquete(msjeEnviado);

	recibeLista(socketMem);
}

void enviarListaGossiping(int socketEnvio) {
	TablaGossip* nodoGossipAux;
	int tamanioPaquete = sizeof(nodoGossipAux->IDMemoria)
			+ sizeof(nodoGossipAux->IPMemoria)
			+ sizeof(nodoGossipAux->puertoMemoria);
	char* paqueteSerializado = malloc(tamanioPaquete);
	t_link_element* nodoAux = listaGossiping->head;

	send(socketEnvio, string_itoa(listaGossiping->elements_count), sizeof(int), 0);

	for (int i = 0; i < listaGossiping->elements_count; i++) {
		nodoGossipAux = nodoAux->data;

		serializarNodo(nodoGossipAux, paqueteSerializado);
		send(socketEnvio, paqueteSerializado, tamanioPaquete, 0);

		nodoAux = nodoAux->next;
	}
	free(paqueteSerializado);
	sem_post(&mutexMemoria);
}

void enviaLista(int socketMem) {
	tPaquete* msjeEnviado = malloc(sizeof(tPaquete));
	msjeEnviado->type = GOSSIPING_RECIBE;

	TablaGossip* nodoGossipAux;
	int tamanioPaquete = sizeof(nodoGossipAux->IDMemoria)
			+ sizeof(nodoGossipAux->IPMemoria)
			+ sizeof(nodoGossipAux->puertoMemoria);
	char* paqueteSerializado = malloc(tamanioPaquete);
	t_link_element* nodoAux = listaGossiping->head;

	//send(socketMem, string_itoa(listaGossiping->elements_count), sizeof(int), 0);

	strcpy(msjeEnviado->payload, string_itoa(listaGossiping->elements_count));
		msjeEnviado->length = sizeof(msjeEnviado->payload);
		enviarPaquete(socketMem, msjeEnviado, logger,
				"Memoria realiza envío de Lista de Gossiping");

	for (int i = 0; i < listaGossiping->elements_count; i++) {
		nodoGossipAux = nodoAux->data;

		serializarNodo(nodoGossipAux, paqueteSerializado);
		send(socketMem, paqueteSerializado, tamanioPaquete, 0);

		nodoAux = nodoAux->next;
	}
	free(paqueteSerializado);

	liberarPaquete(msjeEnviado);

}

void armarPropioNodo() {
	TablaGossip* nodoMem = malloc(sizeof(TablaGossip));
	//puts("ASIGNÉ MEMORIA NODO MEM");
	nodoMem->IDMemoria = configuracion->MEMORY_NUMBER;
	//puts("ASIGNÉ ID MEMORIA");
	strcpy(nodoMem->IPMemoria, configuracion->IP_PROPIA); // CHEQUEAR
	//puts("ASIGNÉ IP MEMORIA");
	nodoMem->puertoMemoria = configuracion->PUERTO;
	//puts("ASIGNÉ PUERTO MEMORIA");
	nodoMem->socketMemoria = 1;
	//puts("ASIGNÉ SOCKET MEMORIA");

	// Agrego al nodo correspondiente a la propia memoria en la primera posición de la listaGossiping de dicha memoria
	list_add(listaGossiping, nodoMem);
	//puts("AGREGUÉ NODO A LISTA");
}

int nodoSocketEstaEnLista(int socketID) {
	t_link_element* nodoActual = listaGossiping->head;
	TablaGossip* nodoAux;

	if (nodoActual)
		nodoAux = nodoActual->data;

	while (nodoActual && nodoAux->socketMemoria != socketID) {
		nodoActual = nodoActual->next;
		if (nodoActual)
			nodoAux = nodoActual->data;
	}
	if (nodoActual)
		return 1;
	else
		return 0;
}

void* gossipingMemoria() {
	armarPropioNodo();
	// Memoria intercambia listaGossiping con todos sus seeds
	int seed = 0;
	while (configuracion->PUERTO_SEEDS[seed] != 0 && seed < CANT_MAX_SEEDS) {
		if (socketSEED[seed] != 1) {
			sem_wait(&loggerSemaforo);
			log_trace(logger,
					"Memoria hace Gossiping con su seed en la IP: %s y Puerto: %d",
					configuracion->IP_SEEDS[seed],
					configuracion->PUERTO_SEEDS[seed]);
			sem_post(&loggerSemaforo);
			pideListaGossiping(socketSEED[seed]);
			enviaLista(socketSEED[seed]);
		} else { // Si no puede conectarse, informa error
			sem_wait(&loggerSemaforo);
			log_error(logger,
					"La memoria no pudo conectarse con su seed en la IP: %s y Puerto: %d",
					configuracion->IP_SEEDS[seed],
					configuracion->PUERTO_SEEDS[seed]);
			sem_post(&loggerSemaforo);
		}
		seed++;
	}

	while (1) {
		sleep(configuracion->RETARDO_GOSSIPING / 1000);
		int hizoGossipingConSeed = 0;

		seed = 0;
		while (configuracion->PUERTO_SEEDS[seed] != 0 && seed < CANT_MAX_SEEDS) {
			if (nodoSocketEstaEnLista(socketSEED[seed])) {
				sem_wait(&loggerSemaforo);
				log_trace(logger,
						"Memoria hace Gossiping con su seed en la IP: %s y Puerto: %d",
						configuracion->IP_SEEDS[seed],
						configuracion->PUERTO_SEEDS[seed]);
				sem_post(&loggerSemaforo);
				pideListaGossiping(socketSEED[seed]);
				enviaLista(socketSEED[seed]);
				hizoGossipingConSeed = 1;
			}
			seed++;
		}
		// Si no pudo hacer Gossiping con ningún seed, hago con la segunda Memoria de la listaGossiping, si existe (Porque la primera va a ser sí misma)
		if (hizoGossipingConSeed == 0) {
			if (listaGossiping->elements_count > 1) { // Hay alguna otra memoria además de la actual
				TablaGossip* nodoTablaGossipAux =
						listaGossiping->head->next->data;
				sem_wait(&loggerSemaforo);
				log_trace(logger, "Memoria hace Gossiping con la Memoria %d",
						nodoTablaGossipAux->IDMemoria);
				sem_post(&loggerSemaforo);
				pideListaGossiping(nodoTablaGossipAux->socketMemoria);
				enviaLista(nodoTablaGossipAux->socketMemoria);
			}
		}
	}
}

void serializarNodo(TablaGossip* nodo, char* paqueteSerializado) {
	int offset = 0;

	memcpy(paqueteSerializado + offset, &(nodo->IDMemoria),
			sizeof(nodo->IDMemoria));
	offset += sizeof(nodo->IDMemoria);

	memcpy(paqueteSerializado + offset, nodo->IPMemoria,
			sizeof(nodo->IPMemoria));
	offset += sizeof(nodo->IPMemoria);

	memcpy(paqueteSerializado + offset, &(nodo->puertoMemoria),
			sizeof(nodo->puertoMemoria));
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
