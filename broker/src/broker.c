#include "broker.h"

int main(void) {
	server_init();

	return EXIT_SUCCESS;
}

void server_init(void) {
	answered_messages = list_create(); // Answered messages list
	threads = list_create(); // Subscriptor threads list
	message_count = 0;
	queues_init();
	int sv_socket;
	t_config* config = config_create("broker.config");
	char* IP = config_get_string_value(config, "IP_BROKER");
	char* PORT = config_get_string_value(config, "PUERTO_BROKER");
	ALGORITMO_PARTICION_LIBRE = config_get_string_value(config, "ALGORITMO_PARTICION_LIBRE");
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
	first_memory_partition->id = 0;
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

t_log* iniciar_logger(void) {
	return log_create("broker.log", "broker", true,
			LOG_LEVEL_INFO);
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
		new_partition->id = list_size(memory_partitions);
		list_add(memory_partitions, new_partition);
		return new_partition->id;
	}

	// If id==1, create new partition, store data into it and add it to partitions list

	return -1;
}



t_memory_partition* get_free_partition(uint32_t size) {
	if (string_equals_ignore_case(ALGORITMO_PARTICION_LIBRE, "FF"))
	{
		return get_free_partition_ff(size);
	} else {
		return get_free_partition_lru(size);
	}
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

t_memory_partition* get_free_partition_lru(uint32_t size) {
	t_memory_partition* partition_to_return = malloc(sizeof(uint64_t) + 3 * sizeof(uint32_t) + sizeof(partition_status));
	partition_to_return->id = -1;

	return partition_to_return;
}

void process_request(uint32_t event_code, uint32_t client_socket) {
	t_message* msg = receive_message(event_code, client_socket);
	// Implement mutex, critical region:
	msg->id = get_message_id();
	// End of critical region
	uint32_t size;
	uint32_t i;
	
	switch (event_code) {
	case NEW_POKEMON:
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
	case APPEARED_POKEMON:;
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
	case CATCH_POKEMON:
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
	case CAUGHT_POKEMON:
		msg->buffer->payload = deserialize_caught_pokemon_message(client_socket,
				&size);
		msg->buffer->size = size;

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
	case GET_POKEMON:
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
	case LOCALIZED_POKEMON:
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
	case NEW_SUBSCRIPTOR:
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
	//TODO: Add logic to send all messages in the queue of the new subscription

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
			memcpy(content, memory + partition->begin, sizeof(partition->content_size));

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
