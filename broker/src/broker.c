#include "broker.h"

int main(void) {
	if (signal(SIGUSR1, sig_usr) == SIG_ERR){
		err_sys("Can't catch SIGUSR1");
	}

	pthread_mutex_init(&mutex_message_id, NULL);

	server_init();

	return EXIT_SUCCESS;
}

void server_init(void) {
	answered_messages = list_create(); // Answered messages list
	threads = list_create(); // Subscriptor threads list
	message_count = 0;
	partition_count = 0;
	queues_init();
	int sv_socket;
	t_config* config = config_create("broker.config");
	char* IP = config_get_string_value(config, "IP_BROKER");
	char* PORT = config_get_string_value(config, "PUERTO_BROKER");
	ALGORITMO_PARTICION_LIBRE = config_get_string_value(config, "ALGORITMO_PARTICION_LIBRE");
	ALGORITMO_REEMPLAZO = config_get_string_value(config, "ALGORITMO_REEMPLAZO");
	TAMANO_MEMORIA = config_get_int_value(config, "TAMANO_MEMORIA");
	TAMANO_MINIMO_PARTICION = config_get_int_value(config, "TAMANO_MINIMO_PARTICION");

	memory_init();

	struct addrinfo hints, *servinfo, *p;

	memset(&hints, 0, sizeof(hints));
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_flags = AI_PASSIVE;

	getaddrinfo(IP, PORT, &hints, &servinfo);

	for (p = servinfo; p != NULL; p = p->ai_next) {
		if ((sv_socket = socket(p->ai_family, p->ai_socktype, p->ai_protocol))
				== -1)
			continue;

		if (bind(sv_socket, p->ai_addr, p->ai_addrlen) == -1) {
			close(sv_socket);
			continue;
		}
		break;
	}

	listen(sv_socket, SOMAXCONN);

	freeaddrinfo(servinfo);

	while (1)
		wait_for_client(sv_socket);
}

void memory_init() {
	memory = malloc(TAMANO_MEMORIA);
	memory_partitions = list_create();

	t_memory_partition* first_memory_partition = malloc(sizeof(uint64_t) + 3 * sizeof(uint32_t) + sizeof(partition_status));
	first_memory_partition->id = get_partition_id();
	first_memory_partition->begin = 0;
	first_memory_partition->content_size = TAMANO_MEMORIA;
	first_memory_partition->lru_time = 0; // We'll use this later for compaction
	first_memory_partition->status = FREE;

	list_add(memory_partitions, first_memory_partition);
}

uint32_t get_message_id() {
	message_count++;
	return message_count;
}

uint32_t get_partition_id() {
	partition_count++;
	return partition_count;
}

void queues_init() {
	queue_new_pokemon.messages = list_create();
	queue_new_pokemon.subscriptors = list_create();

	queue_appeared_pokemon.messages = list_create();
	queue_appeared_pokemon.subscriptors = list_create();

	queue_catch_pokemon.messages = list_create();
	queue_catch_pokemon.subscriptors = list_create();

	queue_caught_pokemon.messages = list_create();
	queue_caught_pokemon.subscriptors = list_create();

	queue_get_pokemon.messages = list_create();
	queue_get_pokemon.subscriptors = list_create();

	queue_localized_pokemon.messages = list_create();
	queue_localized_pokemon.subscriptors = list_create();
}

void wait_for_client(uint32_t sv_socket) {
	struct sockaddr_in client_addr;

	uint32_t addr_size = sizeof(struct sockaddr_in);

	uint32_t client_socket;

	if ((client_socket = accept(sv_socket, (void*) &client_addr, &addr_size))
			!= -1) {
		pthread_create(&thread, NULL, (void*) serve_client, &client_socket);
		list_add(threads, &thread);
		pthread_detach(thread);
	}
}

void serve_client(uint32_t* socket) {
	event_code code;
	if (recv(*socket, &code, sizeof(event_code), MSG_WAITALL) == -1)
		code = -1;

	process_request(code, *socket);
}

void store_message(t_message* message, queue queue, t_list* receivers) {
	t_memory_message* memory_message = malloc(sizeof(t_memory_message));
	memory_message->id = message->id;
	memory_message->correlative_id = message->correlative_id;
	memory_message->event_code = message->event_code;

	//Store payload and get its containing partition id
	memory_message->memory_partition_id = store_payload(message->buffer->payload, message->buffer->size);

	queue_message* queue_message = malloc(sizeof(queue_message));
	queue_message->receivers = receivers;
	queue_message->message = memory_message;

	list_add(queue.messages, queue_message);
}

uint32_t store_payload(void* payload, uint32_t size) {
	// Search for partition and return it, if no partition is found return a partition with id=-1
	t_memory_partition* partition = get_free_partition(size);

	if (partition->id == -1)
	{
		// Compact and search again
		// If no partition is found, delete partition, compact and search again n times
	} else {
		// Copy into memory
		int offset = partition->begin;
		memcpy(memory + offset, payload, sizeof(size));

		// Create partition to reference the new content
		t_memory_partition* new_partition = malloc(sizeof(uint64_t) + 3 * sizeof(uint32_t) + sizeof(partition_status));

		if (size <= TAMANO_MINIMO_PARTICION)
		{
			size = TAMANO_MINIMO_PARTICION;
		}
		new_partition->begin = partition->begin;
		partition->begin = partition->begin + size;

		new_partition->content_size = size;
		partition->content_size = partition->content_size - size;

		new_partition->status = OCCUPED;
		new_partition->lru_time = 0;
		new_partition->id = get_partition_id();
		list_add(memory_partitions, new_partition);
		return new_partition->id;
	}

	return -1;
}

t_memory_partition* get_free_partition(uint32_t size) {
	t_memory_partition* partition_to_return;

	if (string_equals_ignore_case(ALGORITMO_PARTICION_LIBRE, "FF"))
	{
		partition_to_return = get_free_partition_ff(size);
	} else {
		partition_to_return = get_free_partition_bf(size);
	}

	if (partition_to_return->id == -1)
	{
		perform_compaction();

		if (string_equals_ignore_case(ALGORITMO_PARTICION_LIBRE, "FF"))
		{
			partition_to_return = get_free_partition_ff(size);
		} else {
			partition_to_return = get_free_partition_bf(size);
		}
	}

	

	return partition_to_return;
}

void perform_compaction() {
	uint32_t i = list_size(memory_partitions);
	t_list* occuped_partitions = list_create();
	t_list* new_partitions = list_create();

	while (i >= 0) {
		t_memory_partition* partition = list_get(memory_partitions, i);
		if (partition->status == OCCUPED)
		{
			list_add(occuped_partitions, partition);
		}

		i--;
	}

	// Now that we have the list of occuped partitions, we proceed to re-write the partitions into the void* variable
	int offset = 0;
	void* compacted_memory = malloc(TAMANO_MEMORIA);

	for (uint32_t i = 0; i < list_size(occuped_partitions); i++)
	{
		t_memory_partition* partition = list_get(occuped_partitions, i);
		memcpy(compacted_memory + offset, memory + partition->begin, partition->content_size);
		partition->begin = compacted_memory + offset;
		offset += partition->content_size;
		list_add(new_partitions, partition);
	}

	t_memory_partition* free_memory_partition = malloc(sizeof(uint64_t) + 3 * sizeof(uint32_t) + sizeof(partition_status));
	free_memory_partition->id = get_partition_id();
	free_memory_partition->begin = compacted_memory + offset;
	free_memory_partition->content_size = TAMANO_MEMORIA - offset;
	free_memory_partition->lru_time = 0; // We'll use this later for compaction
	free_memory_partition->status = FREE;

	list_add(new_partitions, free_memory_partition);

	free(occuped_partitions);
	free(memory_partitions);
	memory_partitions = new_partitions;
	free(memory);
	memory = compacted_memory;
}

t_memory_partition* get_free_partition_ff(uint32_t size) {
	uint32_t i = 0;

	if (size <= TAMANO_MINIMO_PARTICION)
	{
		size = TAMANO_MINIMO_PARTICION;
	}

	while (i < list_size(memory_partitions)) {
		t_memory_partition* partition = list_get(memory_partitions, i);
		if (partition->status == FREE && partition->content_size >= size)
		{
			return partition;
		}

		i++;
	}

	t_memory_partition* partition_to_return = malloc(sizeof(uint64_t) + 3 * sizeof(uint32_t) + sizeof(partition_status));
	partition_to_return->id = -1;

	return partition_to_return;
}

t_memory_partition* get_free_partition_bf(uint32_t size) {
	uint32_t i = 0;
	t_memory_partition* best_partition;

	if (size <= TAMANO_MINIMO_PARTICION)
	{
		size = TAMANO_MINIMO_PARTICION;
	}

	while (i < list_size(memory_partitions)) {
		t_memory_partition* partition = list_get(memory_partitions, i);
		if (best_partition == NULL && partition->status == FREE && partition->content_size >= size)
		{
			best_partition = partition;
		} else if (partition->status == FREE && partition->content_size >= size && best_partition->content_size > partition)
		{
			best_partition = partition;
		}

		i++;
	}

	if (best_partition == NULL)
	{
		best_partition = malloc(sizeof(uint64_t) + 3 * sizeof(uint32_t) + sizeof(partition_status));
		best_partition->id = -1;
	}

	return best_partition;
}

void process_request(uint32_t event_code, uint32_t client_socket) {
	t_message* msg = receive_message(event_code, client_socket);
	pthread_mutex_lock(&mutex_message_id);
	msg->id = get_message_id();
	pthread_mutex_unlock(&mutex_message_id);
	uint32_t size;
	uint32_t i;
	
	switch (event_code) {
	case NEW_POKEMON: ;
		msg->buffer->payload = deserialize_new_pokemon_message(client_socket,
				&size);
		msg->buffer->size = size;

		return_message_id(client_socket, msg->id);

		for (i = 0; i < list_size(queue_new_pokemon.subscriptors); i++) {
			t_subscriptor* subscriptor = list_get(
					queue_new_pokemon.subscriptors, i);
			send_message(subscriptor->socket, event_code, msg->id,
					msg->correlative_id, msg->buffer);
		}

		//Add message to memory
		store_message(msg, queue_new_pokemon, queue_new_pokemon.subscriptors);

		break;
	case APPEARED_POKEMON: ;
		t_appeared_pokemon* appeared_pokemon = deserialize_appeared_pokemon_message(client_socket, &size);
		msg->buffer = serialize_t_appeared_pokemon_message(appeared_pokemon);

		return_message_id(client_socket, msg->id);

		for (i = 0; i < list_size(queue_appeared_pokemon.subscriptors); i++) {
			t_subscriptor* subscriptor = list_get(
					queue_appeared_pokemon.subscriptors, i);
			send_message(subscriptor->socket, event_code, msg->id,
					msg->correlative_id, msg->buffer);
		}

		//Add message to memory
		store_message(msg, queue_appeared_pokemon, queue_appeared_pokemon.subscriptors);
		break;
	case CATCH_POKEMON: ;
		msg->buffer->payload = deserialize_catch_pokemon_message(client_socket,
				&size);
		msg->buffer->size = size;
		return_message_id(client_socket, msg->id);

		for (i = 0; i < list_size(queue_catch_pokemon.subscriptors); i++) {
			t_subscriptor* subscriptor = list_get(
					queue_catch_pokemon.subscriptors, i);
			send_message(subscriptor->socket, event_code, msg->id,
					msg->correlative_id, msg->buffer);
		}
		//Add message to memory
		store_message(msg, queue_catch_pokemon, queue_catch_pokemon.subscriptors);
		break;
	case CAUGHT_POKEMON: ;
		t_caught_pokemon* caught_pokemon = deserialize_caught_pokemon_message(client_socket,
				&size);
		msg->buffer = serialize_t_caught_pokemon_message(appeared_pokemon);

		return_message_id(client_socket, msg->id);

		for (i = 0; i < list_size(queue_caught_pokemon.subscriptors); i++) {
			t_subscriptor* subscriptor = list_get(
					queue_caught_pokemon.subscriptors, i);
			send_message(subscriptor->socket, event_code, msg->id,
					msg->correlative_id, msg->buffer);
		}

		//Add message to memory
		store_message(msg, queue_caught_pokemon, queue_caught_pokemon.subscriptors);
		break;
	case GET_POKEMON: ;
		msg->buffer->payload = deserialize_get_pokemon_message(client_socket,
				&size);
		msg->buffer->size = size;

		for (i = 0; i < list_size(queue_get_pokemon.subscriptors); i++) {
			t_subscriptor* subscriptor = list_get(
					queue_get_pokemon.subscriptors, i);
			send_message(subscriptor->socket, event_code, msg->id,
					msg->correlative_id, msg->buffer);
		}

		//Add message to memory
		store_message(msg, queue_get_pokemon, queue_get_pokemon.subscriptors);
		break;
	case LOCALIZED_POKEMON: ;
		msg->buffer->payload = deserialize_localized_pokemon_message(
				client_socket, &size);
		msg->buffer->size = size;

		for (i = 0; i < list_size(queue_localized_pokemon.subscriptors); i++) {
			t_subscriptor* subscriptor = list_get(
					queue_localized_pokemon.subscriptors, i);
			send_message(subscriptor->socket, event_code, msg->id,
					msg->correlative_id, msg->buffer);
		}
		//Add message to memory
		store_message(msg, queue_localized_pokemon, queue_localized_pokemon.subscriptors);
		break;
	case NEW_SUBSCRIPTOR: ;
		process_new_subscription(client_socket);
		break;
	default:
		pthread_exit(NULL);
	}
}

void process_new_subscription(uint32_t socket) {
	uint32_t subscriptor_len;
	recv(socket, &(subscriptor_len), sizeof(uint32_t), MSG_WAITALL);

	char* subscriptor_id = malloc(subscriptor_len);
	recv(socket, subscriptor_id, subscriptor_len,
	MSG_WAITALL);

	event_code queue_type;
	recv(socket, &(queue_type), sizeof(event_code),
	MSG_WAITALL);

	uint32_t ip_len;
	recv(socket, &(ip_len), sizeof(uint32_t), MSG_WAITALL);

	t_subscription_petition* subcription_petition = malloc(
			subscriptor_len + ip_len + 4 * sizeof(uint32_t)
					+ sizeof(event_code));
	subcription_petition->subscriptor_len = subscriptor_len;
	subcription_petition->subscriptor_id = subscriptor_id;
	subcription_petition->ip_len = ip_len;

	subcription_petition->queue = queue_type;

	subcription_petition->ip = malloc(subcription_petition->ip_len);
	recv(socket, subcription_petition->ip, subcription_petition->ip_len,
	MSG_WAITALL);

	recv(socket, &(subcription_petition->port), sizeof(uint32_t), MSG_WAITALL);

	recv(socket, &(subcription_petition->duration), sizeof(uint32_t),
	MSG_WAITALL);

	t_subscriptor* subscriptor = malloc(
			sizeof(t_subscription_petition) + sizeof(uint32_t));
	subscriptor->subscriptor_info = subcription_petition;
	subscriptor->socket = socket;

	queue queue;

	switch (subcription_petition->queue) {
	case NEW_POKEMON:
		list_add(queue_new_pokemon.subscriptors, subscriptor);
		queue = queue_new_pokemon;
		break;
	case APPEARED_POKEMON:
		list_add(queue_appeared_pokemon.subscriptors, subscriptor);
		queue = queue_appeared_pokemon;
		break;
	case CATCH_POKEMON:
		list_add(queue_catch_pokemon.subscriptors, subscriptor);
		queue = queue_catch_pokemon;
		break;
	case CAUGHT_POKEMON:
		list_add(queue_caught_pokemon.subscriptors, subscriptor);
		queue = queue_caught_pokemon;
		break;
	case GET_POKEMON:
		list_add(queue_get_pokemon.subscriptors, subscriptor);
		queue = queue_get_pokemon;
		break;
	case LOCALIZED_POKEMON:
		list_add(queue_localized_pokemon.subscriptors, subscriptor);
		queue = queue_localized_pokemon;
		break;
	default:
		pthread_exit(NULL);
	}

	t_log* logger = logger_init();
	log_info(logger, "Nuevo suscriptor");
	log_info(logger, subcription_petition->subscriptor_id);
	log_destroy(logger);

	process_subscriptor(&(subscriptor->socket), subcription_petition, queue);
}

void process_subscriptor(uint32_t* socket, t_subscription_petition* subscription_petition, queue queue) {
	send_all_messages(socket, subscription_petition, queue);

	//TODO: Add logic to subscribe for 'duration' seconds.

	event_code code;

	while (1) {
		if (recv(*socket, &code, sizeof(event_code), MSG_WAITALL) == -1)
			code = -1;

		process_request(code, *socket);
	}
}

void send_all_messages(uint32_t* socket, t_subscription_petition* subscription_petition, queue queue) {
	uint32_t i;

	for (i = 0; i < list_size(queue.messages); i++)
	{
		queue_message* message = list_get(queue.messages, i);
		uint32_t partition_id = message->message->memory_partition_id;
		uint32_t j = 0;

		while (j < list_size(memory_partitions) && (((t_memory_partition*)list_get(memory_partitions, j))->id != partition_id))
		{
			j++;
		}

		if (j >= list_size(memory_partitions))
		{
			return; //ERROR, MESSAGE NOT FOUND
		} else {
			t_memory_partition* partition = (t_memory_partition*) list_get(memory_partitions, j);
			void* content = malloc(partition->content_size);
			memcpy(content, memory + partition->begin, partition->content_size);

			t_buffer* buffer = malloc(sizeof(uint32_t) + partition->content_size);
			buffer->size = partition->content_size;
			buffer->payload = content;
			send_message(*socket, subscription_petition->queue, message->message->id,
											message->message->correlative_id, buffer);
		}

		receiver* receiver = malloc(sizeof(uint32_t) * 2);
		receiver->receiver_socket = *socket;
		receiver->received = 1;
		list_add(message->receivers, receiver);
	}
}

static void sig_usr(int signo){
	if (signo == SIGUSR1){
		dump_memory();
	} else{
		err_sys("received unexpected signal");
	}
	return;
}

void  err_sys(char* msg) {
  printf("%s \n", msg);
  exit(-1);
}

void dump_memory(){
	 time_t rawtime;
	 struct tm * timeinfo;

	 time (&rawtime);
	 timeinfo = localtime(&rawtime);

	FILE* dump_file;
	dump_file = fopen("dump.txt","w");
	if(dump_file == NULL){
		logger = logger_init();
		log_info(logger,"Error al crear el dump");
		log_destroy(logger);
		exit(1);
	}

		fprintf(dump_file,"Dump: %d/%d/%d %d:%d:%d\n",timeinfo->tm_mday, timeinfo->tm_mon + 1, timeinfo->tm_year + 1900, timeinfo->tm_hour, timeinfo->tm_min, timeinfo->tm_sec);
	int i;
	for(i=0;i<list_size(memory_partitions);i++){
		uint32_t begin = ((t_memory_partition*)(list_get(memory_partitions,i)))->begin;
		uint32_t content_size = ((t_memory_partition*)(list_get(memory_partitions,i)))->content_size;
		uint32_t id = ((t_memory_partition*)(list_get(memory_partitions,i)))->id;
		uint32_t lru_time = ((t_memory_partition*)(list_get(memory_partitions,i)))->lru_time;
		uint32_t status = ((t_memory_partition*)(list_get(memory_partitions,i)))->status;
		fprintf(dump_file,"Particion %d: %X - %X.\t [%d]\t Size:%d b\t LRU:%d\t Cola:%d(FALTA EL TIPO DE COLA)\t ID:%d\n", i, begin, (begin+content_size), status, content_size, lru_time, content_size, id);
	 }
	fclose(dump_file);
}