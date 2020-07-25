#include "broker.h"

int main(void)
{
	if (signal(SIGUSR1, sig_usr) == SIG_ERR)
	{
		err_sys("Can't catch SIGUSR1");
	}

	pthread_mutex_init(&mutex_message_id, NULL);
	pthread_mutex_init(&mutex_memory_partition_id, NULL);
	pthread_mutex_init(&mutex_memory_partitions, NULL);

	t_config *config = config_create("/home/utnso/tp-2020-1c-Operavirus/broker/broker.config");
	IP = config_get_string_value(config, "IP_BROKER");
	PORT = config_get_string_value(config, "PUERTO_BROKER");
	ALGORITMO_PARTICION_LIBRE = config_get_string_value(config, "ALGORITMO_PARTICION_LIBRE");
	ALGORITMO_REEMPLAZO = config_get_string_value(config, "ALGORITMO_REEMPLAZO");
	ALGORITMO_MEMORIA = config_get_string_value(config, "ALGORITMO_MEMORIA");
	TAMANO_MEMORIA = config_get_int_value(config, "TAMANO_MEMORIA");
	TAMANO_MINIMO_PARTICION = config_get_int_value(config, "TAMANO_MINIMO_PARTICION");
	FRECUENCIA_COMPACTACION = config_get_int_value(config, "FRECUENCIA_COMPACTACION");
	LOG_FILE = config_get_string_value(config, "LOG_FILE");
	logger = logger_init_broker();

	server_init();

	return EXIT_SUCCESS;
}

void server_init(void)
{
	threads = list_create();		   // Subscriptor threads list
	message_count = 0;
	partition_count = 0;
	queues_init();
	int sv_socket;

	memory_init();

	struct addrinfo hints, *servinfo, *p;

	memset(&hints, 0, sizeof(hints));
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_flags = AI_PASSIVE;

	getaddrinfo(IP, PORT, &hints, &servinfo);

	for (p = servinfo; p != NULL; p = p->ai_next)
	{
		if ((sv_socket = socket(p->ai_family, p->ai_socktype, p->ai_protocol)) == -1)
			continue;

		if (bind(sv_socket, p->ai_addr, p->ai_addrlen) == -1)
		{
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

void memory_init()
{
	uint32_t first_partition_size;

	if (string_equals_ignore_case(ALGORITMO_MEMORIA, "PARTICIONES"))
	{
		first_partition_size = TAMANO_MEMORIA;
	}
	else
	{
		first_partition_size = 1;
		int reached_maximum_size = 0;

		while (first_partition_size < TAMANO_MEMORIA && reached_maximum_size == 0)
		{
			uint32_t new_partition_size = first_partition_size * 2;
			if (new_partition_size <= TAMANO_MEMORIA)
			{
				first_partition_size = new_partition_size;
			}
			else
			{
				reached_maximum_size = 1;
			}
		}
	}

	memory = malloc(first_partition_size);
	memory_partitions = list_create();

	t_memory_partition *first_memory_partition = malloc(sizeof(t_memory_partition));
	pthread_mutex_lock(&mutex_memory_partition_id);
	first_memory_partition->id = get_partition_id();
	pthread_mutex_unlock(&mutex_memory_partition_id);
	first_memory_partition->begin = 0;
	first_memory_partition->partition_size = first_partition_size;
	first_memory_partition->lru_timestamp = 0; // Free partitions should not have lru_timestamp value !== 0
	first_memory_partition->occupied_timestamp = 0;
	first_memory_partition->status = FREE;
	pthread_mutex_init(&(first_memory_partition->mutex_partition), NULL);
	if (!string_equals_ignore_case(ALGORITMO_MEMORIA, "PARTICIONES"))
	{

		first_memory_partition->father = string_new();
		first_memory_partition->side = string_new(); // The root doesn't have father nor side
	}

	list_add(memory_partitions, first_memory_partition);
}

uint32_t get_message_id()
{
	message_count++;
	return message_count;
}

uint32_t get_partition_id()
{
	partition_count++;
	return partition_count;
}

void queues_init()
{
	queue_new_pokemon = malloc(sizeof(queue));
	queue_new_pokemon->messages = list_create();
	queue_new_pokemon->subscriptors = list_create();
	pthread_mutex_init(&queue_new_pokemon->mutex_messages, NULL);
	pthread_mutex_init(&queue_new_pokemon->mutex_subscriptors, NULL);

	queue_appeared_pokemon = malloc(sizeof(queue));
	queue_appeared_pokemon->messages = list_create();
	queue_appeared_pokemon->subscriptors = list_create();
	pthread_mutex_init(&queue_appeared_pokemon->mutex_messages, NULL);
	pthread_mutex_init(&queue_appeared_pokemon->mutex_subscriptors, NULL);

	queue_catch_pokemon = malloc(sizeof(queue));
	queue_catch_pokemon->messages = list_create();
	queue_catch_pokemon->subscriptors = list_create();
	pthread_mutex_init(&queue_catch_pokemon->mutex_messages, NULL);
	pthread_mutex_init(&queue_catch_pokemon->mutex_subscriptors, NULL);

	queue_caught_pokemon = malloc(sizeof(queue));
	queue_caught_pokemon->messages = list_create();
	queue_caught_pokemon->subscriptors = list_create();
	pthread_mutex_init(&queue_caught_pokemon->mutex_messages, NULL);
	pthread_mutex_init(&queue_caught_pokemon->mutex_subscriptors, NULL);

	queue_get_pokemon = malloc(sizeof(queue));
	queue_get_pokemon->messages = list_create();
	queue_get_pokemon->subscriptors = list_create();
	pthread_mutex_init(&queue_get_pokemon->mutex_messages, NULL);
	pthread_mutex_init(&queue_get_pokemon->mutex_subscriptors, NULL);

	queue_localized_pokemon = malloc(sizeof(queue));
	queue_localized_pokemon->messages = list_create();
	queue_localized_pokemon->subscriptors = list_create();
	pthread_mutex_init(&queue_localized_pokemon->mutex_messages, NULL);
	pthread_mutex_init(&queue_localized_pokemon->mutex_subscriptors, NULL);
}

void wait_for_client(uint32_t sv_socket)
{
	struct sockaddr_in client_addr;

	uint32_t addr_size = sizeof(struct sockaddr_in);

	uint32_t* client_socket = malloc(sizeof(int));

	if ((*client_socket = accept(sv_socket, (void *)&client_addr, &addr_size)) != -1)
	{
		pthread_create(&thread, NULL, (void *)serve_client, client_socket);
		list_add(threads, &thread);
		pthread_detach(thread);
	}
}

void serve_client(uint32_t *socket)
{

	log_info(logger, "Proceso conectado al broker");
	event_code code;
	int bytes = recv(*socket, &code, sizeof(event_code), MSG_WAITALL);
	if (bytes == -1 || bytes == 0) {
		return;
	} else {
		process_request(code, *socket);
	}

}

void store_message(t_message *message, queue* queue, t_list *receivers)
{
	t_memory_message *memory_message = malloc(sizeof(t_memory_message));
	memory_message->id = message->id;
	memory_message->correlative_id = message->correlative_id;
	memory_message->event_code = message->event_code;

	//Store payload and get its containing partition id
	pthread_mutex_lock(&mutex_memory_partitions);
	if (string_equals_ignore_case(ALGORITMO_MEMORIA, "PARTICIONES"))
	{
		memory_message->memory_partition_id = store_payload_particiones(message->buffer->payload, message->buffer->size, memory_message->id);
	}
	else
	{
		memory_message->memory_partition_id = store_payload_bs(message->buffer->payload, message->buffer->size, memory_message->id);
	}
	pthread_mutex_unlock(&mutex_memory_partitions);

	queue_message *queue_msg = malloc(sizeof(queue_message));
	queue_msg->receivers = receivers;
	queue_msg->message = memory_message;

	pthread_mutex_lock(&(queue->mutex_messages));
	list_add(queue->messages, queue_msg);
	pthread_mutex_unlock(&(queue->mutex_messages));
}

uint32_t store_payload_bs(void *payload, uint32_t size, uint32_t message_id)
{
	// Search for partition and return it, if no partition is found return a partition with id=-1
	t_memory_partition *partition = get_free_partition_bs(size);

	uint32_t minimum_size = size;
	if (size <= TAMANO_MINIMO_PARTICION)
	{
		minimum_size = TAMANO_MINIMO_PARTICION;
	}

	uint32_t partition_size_to_use = partition->partition_size;

	while (partition_size_to_use / 2 >= minimum_size)
	{
		partition_size_to_use = partition_size_to_use / 2;
		// Divide partition in two, use the first one
		t_memory_partition *first_half = malloc(sizeof(t_memory_partition));
		pthread_mutex_lock(&mutex_memory_partition_id);
		first_half->id = get_partition_id();
		pthread_mutex_unlock(&mutex_memory_partition_id);
		first_half->begin = partition->begin;
		first_half->partition_size = partition_size_to_use;
		first_half->lru_timestamp = 0;
		first_half->occupied_timestamp = 0;
		first_half->status = FREE;
		first_half->father = malloc(strlen(partition->father) + 1);
		string_append(&(partition->father), partition->side);
		//strcpy(first_half->father, partition->father);
		first_half->father = string_from_format("%s", partition->father);
		first_half->side = "0";
		pthread_mutex_init(&(first_half->mutex_partition), NULL);

		list_add(memory_partitions, first_half);

		t_memory_partition *second_half = malloc(sizeof(t_memory_partition));
		pthread_mutex_lock(&mutex_memory_partition_id);
		second_half->id = get_partition_id();
		pthread_mutex_unlock(&mutex_memory_partition_id);
		second_half->begin = partition->begin + partition_size_to_use;
		second_half->partition_size = partition_size_to_use;
		second_half->lru_timestamp = 0;
		second_half->occupied_timestamp = 0;
		second_half->status = FREE;
		second_half->father = malloc(strlen(partition->father) + 1);
		string_append(&(partition->father), partition->side);
		//strcpy(second_half->father, partition->father);
		second_half->father = string_from_format("%s", partition->father);
		second_half->side = "1";
		pthread_mutex_init(&(second_half->mutex_partition), NULL);

		list_add(memory_partitions, second_half);

		for (int i = 0; i < list_size(memory_partitions); i++) {
			t_memory_partition *part = list_get(memory_partitions, i);
			if (part->id == partition->id)
			{
				list_remove(memory_partitions, i);
			}
		}

		partition = first_half;
	}

	// Copy into memory
	int offset = partition->begin;
	memcpy(memory + offset, payload, size);

	partition->status = OCCUPIED;
	partition->lru_timestamp = (unsigned long)time(NULL);
	partition->occupied_timestamp = (unsigned long)time(NULL);
	partition->content_size = size;

	for (int i = 0; i < list_size(memory_partitions); i++) {
		t_memory_partition *part = list_get(memory_partitions, i);
		if (part->id == partition->id)
		{
			list_replace(memory_partitions, i, partition);
		}
	}

	log_info(logger, "Almacenado mensaje con id %d en memoria, posicion de inicio: %d", message_id, partition->begin);

	return partition->id;
}

uint32_t store_payload_particiones(void *payload, uint32_t size, uint32_t message_id)
{
	// Search for partition and return it, if no partition is found return a partition with id=-1
	t_memory_partition *partition = get_free_partition_particiones(size);

	// Copy into memory
	int offset = partition->begin;
	memcpy(memory + offset, payload, size);

	// Create partition to reference the new content
	t_memory_partition *new_partition = malloc(sizeof(t_memory_partition));
	pthread_mutex_init(&(new_partition->mutex_partition), NULL);

	if (size <= TAMANO_MINIMO_PARTICION)
	{
		size = TAMANO_MINIMO_PARTICION;
	}
	new_partition->begin = partition->begin;
	new_partition->partition_size = size;
	new_partition->content_size = size;

	partition->begin = partition->begin + size;
	partition->partition_size = partition->partition_size - size;

	new_partition->status = OCCUPIED;
	new_partition->lru_timestamp = (unsigned long)time(NULL);
	new_partition->occupied_timestamp = (unsigned long)time(NULL);
	pthread_mutex_lock(&mutex_memory_partition_id);
	new_partition->id = get_partition_id();
	pthread_mutex_unlock(&mutex_memory_partition_id);
	list_add(memory_partitions, new_partition);

	log_info(logger, "Almacenado mensaje con id %d en memoria, posicion de inicio %d", message_id, new_partition->begin);

	return new_partition->id;
}

t_memory_partition *find_free_partition(uint32_t size)
{
	if (string_equals_ignore_case(ALGORITMO_PARTICION_LIBRE, "FF"))
	{
		return find_free_partition_ff(size);
	}
	else
	{
		return find_free_partition_bf(size);
	}
}

void delete_partition_and_consolidate()
{
	if (string_equals_ignore_case(ALGORITMO_REEMPLAZO, "FIFO"))
	{
		delete_partition_and_consolidate_fifo();
	}
	else
	{
		delete_partition_and_consolidate_lru();
	}
}

void delete_partition_and_consolidate_fifo()
{
	// Delete partition which ID is the lowest
	// Because we assign partition IDs incrementally
	// Causing older partitions to have lower ID values
	uint32_t smallest_id;
	uint64_t smallest_ocuppied_timestamp = 9999999999;
	int index_to_free = 0;

	for (int i = 0; i < list_size(memory_partitions); i++)
	{

		t_memory_partition *partition = list_get(memory_partitions, i);

		if (partition->occupied_timestamp < smallest_ocuppied_timestamp && partition->status == OCCUPIED)
		{
			smallest_id = partition->id;
			index_to_free = i;
			smallest_ocuppied_timestamp = partition->occupied_timestamp;
		}
	}


	// Set as FREE the partition which ID is memory_partition_id
	t_memory_partition *partition = list_get(memory_partitions, index_to_free);
	partition->status = FREE;
	partition->lru_timestamp = 0;
	partition->occupied_timestamp = 0;
	partition->content_size = 0;

	log_info(logger, "Eliminada particion con id %d, cuya posicion de inicio es %d", partition->id, partition->begin);
	delete_and_consolidate(smallest_id);
}

void delete_partition_and_consolidate_lru()
{
	// Delete partition which lru_timestamp is the lowest
	// We must update the lru_timestamp with every access to the partition
	// (Not taking into account when accessing for compacting)
	uint32_t smallest_id;
	int index_to_free;
	uint64_t smallest_lru_timestamp = 9999999999;

	for (int i = 0; i < list_size(memory_partitions); i++)
	{

		t_memory_partition *partition = list_get(memory_partitions, i);

		if (partition->lru_timestamp < smallest_lru_timestamp && partition->status == OCCUPIED)
		{
			index_to_free = i;
			smallest_id = partition->id;
			smallest_lru_timestamp = partition->lru_timestamp;
		}
	}

	// Set as FREE the partition which ID is memory_partition_id
	t_memory_partition *partition = list_get(memory_partitions, index_to_free);
	partition->status = FREE;
	partition->occupied_timestamp = 0;
	partition->content_size = 0;
	log_info(logger, "Eliminada particion con id %d, cuya posicion de inicio es %d y su tiempo de LRU es %d", partition->id, partition->begin, partition->lru_timestamp);
	partition->lru_timestamp = 0;
	delete_and_consolidate(smallest_id);
}

void delete_and_consolidate(uint32_t memory_partition_id)
{
	// Search for the associated message and delete it
	for (int i = 0; i < list_size(queue_new_pokemon->messages); i++)
	{
		queue_message *message = list_get(queue_new_pokemon->messages, i);
		if (message->message->memory_partition_id == memory_partition_id)
		{
			list_remove(queue_new_pokemon->messages, i);
			consolidate(memory_partition_id);
			return;
		}
	}

	for (int i = 0; i < list_size(queue_appeared_pokemon->messages); i++)
	{
		queue_message *message = list_get(queue_appeared_pokemon->messages, i);
		if (message->message->memory_partition_id == memory_partition_id)
		{
			list_remove(queue_appeared_pokemon->messages, i);
			consolidate(memory_partition_id);
			return;
		}
	}

	for (int i = 0; i < list_size(queue_catch_pokemon->messages); i++)
	{
		queue_message *message = list_get(queue_catch_pokemon->messages, i);
		if (message->message->memory_partition_id == memory_partition_id)
		{
			list_remove(queue_catch_pokemon->messages, i);
			consolidate(memory_partition_id);
			return;
		}
	}

	for (int i = 0; i < list_size(queue_caught_pokemon->messages); i++)
	{
		queue_message *message = list_get(queue_caught_pokemon->messages, i);
		if (message->message->memory_partition_id == memory_partition_id)
		{
			list_remove(queue_caught_pokemon->messages, i);
			consolidate(memory_partition_id);
			return;
		}
	}

	for (int i = 0; i < list_size(queue_get_pokemon->messages); i++)
	{
		queue_message *message = list_get(queue_get_pokemon->messages, i);
		if (message->message->memory_partition_id == memory_partition_id)
		{
			list_remove(queue_get_pokemon->messages, i);
			consolidate(memory_partition_id);
			return;
		}
	}

	for (int i = 0; i < list_size(queue_localized_pokemon->messages); i++)
	{
		queue_message *message = list_get(queue_localized_pokemon->messages, i);
		if (message->message->memory_partition_id == memory_partition_id)
		{
			list_remove(queue_localized_pokemon->messages, i);
			consolidate(memory_partition_id);
			return;
		}
	}
}

void consolidate(uint32_t memory_partition_to_consolidate_id)
{
	t_memory_partition *memory_partition_to_consolidate;
	for (int i = 0; i < list_size(memory_partitions); i++)
	{
		t_memory_partition *part = list_get(memory_partitions, i);
		if (part->id == memory_partition_to_consolidate_id)
		{
			memory_partition_to_consolidate = part;
		}
	}

	if (string_equals_ignore_case(ALGORITMO_MEMORIA, "PARTICIONES"))
	{
		for (int i = 0; i < list_size(memory_partitions); i++)
		{
			t_memory_partition *partition = list_get(memory_partitions, i);
			if (partition->status == FREE)
			{
				if ((partition->begin + partition->partition_size) == memory_partition_to_consolidate->begin)
				{
					for (int j = 0; j < list_size(memory_partitions); j++)
					{
						t_memory_partition *partition_to_consolidate = list_get(memory_partitions, j);
						if (memory_partition_to_consolidate->id == partition_to_consolidate->id)
						{
							list_remove(memory_partitions, j);
							partition->partition_size += memory_partition_to_consolidate->partition_size;
							memory_partition_to_consolidate = partition;
						}
					}
				}
			}
		}

		for (int i = 0; i < list_size(memory_partitions); i++)
		{
			t_memory_partition *partition = list_get(memory_partitions, i);
			if (partition->status == FREE)
			{
				if ((memory_partition_to_consolidate->begin + memory_partition_to_consolidate->partition_size) == partition->begin)
				{
					for (int j = 0; j < list_size(memory_partitions); j++)
					{
						t_memory_partition *partition_to_consolidate = list_get(memory_partitions, j);
						if (memory_partition_to_consolidate->id == partition_to_consolidate->id)
						{
							list_remove(memory_partitions, i);
							partition_to_consolidate->partition_size += partition->partition_size;
						}
					}
				}
			}
		}
	}
	else
	{
		int consolidated;
		do
		{
			consolidated = 0;

			for (int i = 0; i < list_size(memory_partitions); i++)
			{
				t_memory_partition *partition = list_get(memory_partitions, i);
				if (partition->status == FREE)
				{
					if (string_equals_ignore_case(memory_partition_to_consolidate->father, partition->father))
					{
						for (int j = 0; j < list_size(memory_partitions); j++)
						{
							t_memory_partition *partition_to_consolidate = list_get(memory_partitions, j);
							if (memory_partition_to_consolidate->id == partition_to_consolidate->id)
							{
								log_info(logger, "Asociada particion con id %d, cuya posicion de inicio es %d, con su buddy cuyo id es %d, y su posicion de inicio es %d", partition->id, partition->begin, memory_partition_to_consolidate->id, memory_partition_to_consolidate->begin);
								memory_partition_to_consolidate = list_remove(memory_partitions, j);
								partition->partition_size += memory_partition_to_consolidate->partition_size;
								int father_length = strlen(memory_partition_to_consolidate->father);
								partition->father = string_substring(memory_partition_to_consolidate->father, 0, father_length - 1);
								partition->side = string_substring(memory_partition_to_consolidate->father, father_length - 1, father_length);
								memory_partition_to_consolidate = partition;
								consolidated = 1;
							}
						}
					}
				}
			}
		} while (consolidated == 1);
	}
}

t_memory_partition *get_free_partition_particiones(uint32_t size)
{
	int search_count;
	t_memory_partition *partition_to_return = find_free_partition(size);

	if (partition_to_return->id != -1)
	{
		return partition_to_return;
	}

	while (true)
	{
		if (FRECUENCIA_COMPACTACION != -1 || list_size(memory_partitions) == 1)
		{
			perform_compaction();
			search_count = 0;
		}

		while (FRECUENCIA_COMPACTACION > search_count)
		{
			partition_to_return = find_free_partition(size);
			search_count++;

			if (partition_to_return->id != -1)
			{
				return partition_to_return;
			}

			delete_partition_and_consolidate();
		}

	}
}

t_memory_partition *get_free_partition_bs(uint32_t size)
{
	t_memory_partition *partition_to_return = find_free_partition(size);

	if (partition_to_return->id != -1)
	{
		return partition_to_return;
	}

	while (true)
	{
		delete_partition_and_consolidate();
		partition_to_return = find_free_partition(size);

		if (partition_to_return->id != -1)
		{
			return partition_to_return;
		}
	}
}

void perform_compaction()
{
	t_list *occupied_partitions = list_create();
	t_list *new_partitions = list_create();

	for (uint32_t i = 0; i < list_size(memory_partitions); i++)
	{
		t_memory_partition *partition = list_get(memory_partitions, i);
		if (partition->status == OCCUPIED)
		{
			list_add(occupied_partitions, partition);
		}
	}

	// Now that we have the list of occupied partitions, we proceed to re-write the partitions into the void* variable
	int offset = 0;
	void *compacted_memory = malloc(TAMANO_MEMORIA);

	for (uint32_t i = 0; i < list_size(occupied_partitions); i++)
	{
		t_memory_partition *partition = list_get(occupied_partitions, i);
		memcpy(compacted_memory + offset, memory + partition->begin, partition->partition_size);
		partition->begin = offset;
		offset += partition->partition_size;
		list_add(new_partitions, partition);
	}

	t_memory_partition *free_memory_partition = malloc(sizeof(uint64_t) * 2 + 4 * sizeof(uint32_t) + sizeof(partition_status));
	pthread_mutex_lock(&mutex_memory_partition_id);
	free_memory_partition->id = get_partition_id();
	pthread_mutex_unlock(&mutex_memory_partition_id);
	free_memory_partition->begin = offset;
	free_memory_partition->partition_size = TAMANO_MEMORIA - offset;
	free_memory_partition->lru_timestamp = 0;
	free_memory_partition->status = FREE;

	list_add(new_partitions, free_memory_partition);

	free(occupied_partitions);
	free(memory_partitions);
	memory_partitions = new_partitions;
	free(memory);
	memory = compacted_memory;
	log_info(logger, "Memoria compactada.");
}

t_memory_partition *find_free_partition_ff(uint32_t size)
{
	uint32_t i = 0;

	if (size <= TAMANO_MINIMO_PARTICION)
	{
		size = TAMANO_MINIMO_PARTICION;
	}

	while (i < list_size(memory_partitions))
	{
		t_memory_partition *partition = list_get(memory_partitions, i);
		if (partition->status == FREE && partition->partition_size >= size)
		{
			return partition;
		}

		i++;
	}

	t_memory_partition *partition_to_return = malloc(sizeof(t_memory_partition));
	partition_to_return->id = -1;

	return partition_to_return;
}

t_memory_partition *find_free_partition_bf(uint32_t size)
{
	uint32_t i = 0;
	t_memory_partition *best_partition;

	if (size <= TAMANO_MINIMO_PARTICION)
	{
		size = TAMANO_MINIMO_PARTICION;
	}

	while (i < list_size(memory_partitions))
	{
		t_memory_partition *partition = list_get(memory_partitions, i);
		if (best_partition == NULL && partition->status == FREE && partition->partition_size >= size)
		{
			best_partition = partition;
		}
		else if (partition->status == FREE && partition->partition_size >= size && best_partition->partition_size > partition->partition_size)
		{
			best_partition = partition;
		}

		i++;
	}

	if (best_partition == NULL)
	{
		best_partition = malloc(sizeof(t_memory_partition));
		best_partition->id = -1;
	}

	return best_partition;
}

void process_request(uint32_t event_code, uint32_t client_socket)
{
	t_message *msg = receive_message(event_code, client_socket);
	pthread_mutex_lock(&mutex_message_id);
	msg->id = get_message_id();
	pthread_mutex_unlock(&mutex_message_id);
	uint32_t size;
	uint32_t i;
	uint32_t bytes_to_send;
	void* to_send;
	switch (event_code)
	{
	case NEW_POKEMON:;
		t_new_pokemon *new_pokemon = deserialize_new_pokemon_message(client_socket, &size);
		msg->buffer = serialize_t_new_pokemon_message(new_pokemon);

		log_info(logger, "Llegada de mensaje para la queue %s", event_code_to_string(event_code));

		return_message_id(client_socket, msg->id);

		bytes_to_send = msg->buffer->size + sizeof(uint32_t)
				+ sizeof(event_code) + sizeof(uint32_t) + sizeof(uint32_t);
		to_send = serialize_message(msg, &(bytes_to_send));

		pthread_mutex_lock(&queue_new_pokemon->mutex_subscriptors);
		for (i = 0; i < list_size(queue_new_pokemon->subscriptors); i++)
		{
			t_subscriptor *subscriptor = list_get(
				queue_new_pokemon->subscriptors, i);
			send(subscriptor->socket, to_send, bytes_to_send, 0);
			log_info(logger, "Enviado mensaje id %d de tipo %s al subscriptor cuyo socket es %d", msg->id, event_code_to_string(event_code), subscriptor->socket);
		}
		pthread_mutex_unlock(&queue_new_pokemon->mutex_subscriptors);

		//Add message to memory
		store_message(msg, queue_new_pokemon, queue_new_pokemon->subscriptors);

		break;
	case APPEARED_POKEMON:;
		t_appeared_pokemon *appeared_pokemon = deserialize_appeared_pokemon_message(client_socket, &size);
		msg->buffer = serialize_t_appeared_pokemon_message(appeared_pokemon);

		log_info(logger, "Llegada de mensaje para la queue %s, Pokemon %s", event_code_to_string(event_code), appeared_pokemon->pokemon);

		return_message_id(client_socket, msg->id);

		bytes_to_send = msg->buffer->size + sizeof(uint32_t)
				+ sizeof(event_code) + sizeof(uint32_t) + sizeof(uint32_t);
		to_send = serialize_message(msg, &(bytes_to_send));

		pthread_mutex_lock(&queue_appeared_pokemon->mutex_subscriptors);
		for (i = 0; i < list_size(queue_appeared_pokemon->subscriptors); i++)
		{
			t_subscriptor *subscriptor = list_get(
				queue_appeared_pokemon->subscriptors, i);
			send(subscriptor->socket, to_send, bytes_to_send, 0);
			log_info(logger, "Enviado mensaje id %d de tipo %s al subscriptor cuyo socket es %d", msg->id, event_code_to_string(event_code), subscriptor->socket);
		}
		pthread_mutex_unlock(&queue_appeared_pokemon->mutex_subscriptors);

		//Add message to memory
		store_message(msg, queue_appeared_pokemon, queue_appeared_pokemon->subscriptors);
		break;
	case CATCH_POKEMON:;
		t_catch_pokemon *catch_pokemon = deserialize_catch_pokemon_message(client_socket, &size);
		msg->buffer = serialize_t_catch_pokemon_message(catch_pokemon);

		log_info(logger, "Llegada de mensaje para la queue %s", event_code_to_string(event_code));

		return_message_id(client_socket, msg->id);

		bytes_to_send = msg->buffer->size + sizeof(uint32_t)
				+ sizeof(event_code) + sizeof(uint32_t) + sizeof(uint32_t);
		to_send = serialize_message(msg, &(bytes_to_send));

		pthread_mutex_lock(&queue_catch_pokemon->mutex_subscriptors);
		for (i = 0; i < list_size(queue_catch_pokemon->subscriptors); i++)
		{
			t_subscriptor *subscriptor = list_get(
				queue_catch_pokemon->subscriptors, i);
			send(subscriptor->socket, to_send, bytes_to_send, 0);
			log_info(logger, "Enviado mensaje id %d de tipo %s al subscriptor cuyo socket es %d", msg->id, event_code_to_string(event_code), subscriptor->socket);
		}
		pthread_mutex_unlock(&queue_catch_pokemon->mutex_subscriptors);

		//Add message to memory
		store_message(msg, queue_catch_pokemon, queue_catch_pokemon->subscriptors);
		break;
	case CAUGHT_POKEMON:;
		t_caught_pokemon *caught_pokemon = deserialize_caught_pokemon_message(client_socket, &size);
		msg->buffer = serialize_t_caught_pokemon_message(caught_pokemon);

		log_info(logger, "Llegada de mensaje para la queue %s, result %d", event_code_to_string(event_code), caught_pokemon->result);

		return_message_id(client_socket, msg->id);

		bytes_to_send = msg->buffer->size + sizeof(uint32_t)
				+ sizeof(event_code) + sizeof(uint32_t) + sizeof(uint32_t);
		to_send = serialize_message(msg, &(bytes_to_send));

		pthread_mutex_lock(&queue_caught_pokemon->mutex_subscriptors);
		for (i = 0; i < list_size(queue_caught_pokemon->subscriptors); i++)
		{
			t_subscriptor *subscriptor = list_get(
				queue_caught_pokemon->subscriptors, i);
			send(subscriptor->socket, to_send, bytes_to_send, 0);
			log_info(logger, "Enviado mensaje id %d de tipo %s al subscriptor cuyo socket es %d", msg->id, event_code_to_string(event_code), subscriptor->socket);
		}
		pthread_mutex_unlock(&queue_caught_pokemon->mutex_subscriptors);

		//Add message to memory
		store_message(msg, queue_caught_pokemon, queue_caught_pokemon->subscriptors);
		break;
	case GET_POKEMON:;
		t_get_pokemon *get_pokemon = deserialize_get_pokemon_message(client_socket, &size);
		msg->buffer = serialize_t_get_pokemon_message(get_pokemon);

		log_info(logger, "Llegada de mensaje para la queue %s, Pokemon %s", event_code_to_string(event_code), get_pokemon->pokemon);

		bytes_to_send = msg->buffer->size + sizeof(uint32_t)
				+ sizeof(event_code) + sizeof(uint32_t) + sizeof(uint32_t);
		to_send = serialize_message(msg, &(bytes_to_send));

		pthread_mutex_lock(&queue_get_pokemon->mutex_subscriptors);
		for (i = 0; i < list_size(queue_get_pokemon->subscriptors); i++)
		{
			t_subscriptor *subscriptor = list_get(
				queue_get_pokemon->subscriptors, i);
			send(subscriptor->socket, to_send, bytes_to_send, 0);
			log_info(logger, "Enviado mensaje id %d de tipo %s al subscriptor cuyo socket es %d", msg->id, event_code_to_string(event_code), subscriptor->socket);
		}
		pthread_mutex_unlock(&queue_get_pokemon->mutex_subscriptors);

		//Add message to memory
		store_message(msg, queue_get_pokemon, queue_get_pokemon->subscriptors);
		break;
	case LOCALIZED_POKEMON:;
		t_localized_pokemon *localized_pokemon = deserialize_localized_pokemon_message(client_socket, &size);
		msg->buffer = serialize_t_localized_pokemon_message(localized_pokemon);

		log_info(logger, "Llegada de mensaje para la queue %s, Pokemon %s", event_code_to_string(event_code), localized_pokemon->pokemon);

		bytes_to_send = msg->buffer->size + sizeof(uint32_t)
				+ sizeof(event_code) + sizeof(uint32_t) + sizeof(uint32_t);
		to_send = serialize_message(msg, &(bytes_to_send));

		pthread_mutex_lock(&queue_localized_pokemon->mutex_subscriptors);
		for (i = 0; i < list_size(queue_localized_pokemon->subscriptors); i++)
		{
			t_subscriptor *subscriptor = list_get(
				queue_localized_pokemon->subscriptors, i);
			send(subscriptor->socket, to_send, bytes_to_send, 0);
			log_info(logger, "Enviado mensaje id %d de tipo %s al subscriptor cuyo socket es %d", msg->id, event_code_to_string(event_code), subscriptor->socket);
		}
		pthread_mutex_unlock(&queue_localized_pokemon->mutex_subscriptors);

		//Add message to memory
		store_message(msg, queue_localized_pokemon, queue_localized_pokemon->subscriptors);
		break;
	case NEW_SUBSCRIPTOR:;
		process_new_subscription(client_socket);
		break;
	case MESSAGE_RECEIVED:;
		process_message_received(client_socket);
		break;
	default:
		pthread_exit(NULL);
	}
}

void process_message_ack(queue* queue, char *subscriptor_id, uint32_t received_message_id)
{
	for (int i = 0; i < list_size(queue->subscriptors); i++)
	{
		t_subscriptor *subscriptor = (t_subscriptor *)list_get(queue->subscriptors, i);

		if (string_equals_ignore_case(subscriptor->subscriptor_info->subscriptor_id, subscriptor_id))
		{
			uint32_t subscriptor_socket = subscriptor->socket;

			for (int j = 0; j < list_size(queue->messages); j++)
			{
				queue_message *message = (queue_message *)list_get(queue->messages, j);

				if (message->message->id == received_message_id)
				{
					for (int k = 0; k < list_size(message->receivers); k++)
					{
						receiver *_receiver = (receiver *)list_get(message->receivers, k);

						if (_receiver->receiver_socket == subscriptor_socket)
						{
							_receiver->received = 1;
							log_info(logger, "Mensaje con id %d, de tipo %d, recibido por el suscriptor cuyo socket es %d, y su id es %s", received_message_id, message->message->event_code, subscriptor_socket, subscriptor_id);
							return;
						}
					}
				}
			}
		}
	}
}

void process_message_received(uint32_t socket)
{
	uint32_t subscriptor_len;
	recv(socket, &(subscriptor_len), sizeof(uint32_t), MSG_WAITALL);

	char *subscriptor_id = malloc(subscriptor_len);
	recv(socket, subscriptor_id, subscriptor_len,
		 MSG_WAITALL);

	event_code message_type;
	recv(socket, &(message_type), sizeof(event_code),
		 MSG_WAITALL);

	uint32_t received_message_id;
	recv(socket, &(received_message_id), sizeof(uint32_t), MSG_WAITALL);

	queue* queue;

	switch (message_type)
	{
	case NEW_POKEMON:;
		queue = queue_new_pokemon;
		break;
	case APPEARED_POKEMON:;
		queue = queue_appeared_pokemon;
		break;
	case CATCH_POKEMON:;
		queue = queue_catch_pokemon;
		break;
	case CAUGHT_POKEMON:;
		queue = queue_caught_pokemon;
		break;
	case GET_POKEMON:;
		queue = queue_get_pokemon;
		break;
	case LOCALIZED_POKEMON:;
		queue = queue_localized_pokemon;
		break;
	default:
		pthread_exit(NULL);
	}

	process_message_ack(queue, subscriptor_id, received_message_id);
}

void process_new_subscription(uint32_t socket)
{
	uint32_t subscriptor_len;
	recv(socket, &(subscriptor_len), sizeof(uint32_t), MSG_WAITALL);

	char *subscriptor_id = malloc(subscriptor_len);
	recv(socket, subscriptor_id, subscriptor_len,
		 MSG_WAITALL);

	event_code queue_type;
	recv(socket, &(queue_type), sizeof(event_code),
		 MSG_WAITALL);

	uint32_t ip_len;
	recv(socket, &(ip_len), sizeof(uint32_t), MSG_WAITALL);

	t_subscription_petition *subcription_petition = malloc(
		subscriptor_len + ip_len + 4 * sizeof(uint32_t) + sizeof(event_code));
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

	t_subscriptor *subscriptor = malloc(
		sizeof(t_subscription_petition) + sizeof(uint32_t));
	subscriptor->subscriptor_info = subcription_petition;
	subscriptor->socket = socket;

	queue* queue;

	switch (subcription_petition->queue)
	{
	case NEW_POKEMON:
		pthread_mutex_lock(&queue_new_pokemon->mutex_subscriptors);
		list_add(queue_new_pokemon->subscriptors, subscriptor);
		pthread_mutex_unlock(&queue_new_pokemon->mutex_subscriptors);
		queue = queue_new_pokemon;
		break;
	case APPEARED_POKEMON:
		pthread_mutex_lock(&queue_appeared_pokemon->mutex_subscriptors);
		list_add(queue_appeared_pokemon->subscriptors, subscriptor);
		pthread_mutex_unlock(&queue_appeared_pokemon->mutex_subscriptors);
		queue = queue_appeared_pokemon;
		break;
	case CATCH_POKEMON:
		pthread_mutex_lock(&queue_catch_pokemon->mutex_subscriptors);
		list_add(queue_catch_pokemon->subscriptors, subscriptor);
		pthread_mutex_unlock(&queue_catch_pokemon->mutex_subscriptors);
		queue = queue_catch_pokemon;
		break;
	case CAUGHT_POKEMON:
		pthread_mutex_lock(&queue_caught_pokemon->mutex_subscriptors);
		list_add(queue_caught_pokemon->subscriptors, subscriptor);
		pthread_mutex_unlock(&queue_caught_pokemon->mutex_subscriptors);
		queue = queue_caught_pokemon;
		break;
	case GET_POKEMON:
		pthread_mutex_lock(&queue_get_pokemon->mutex_subscriptors);
		list_add(queue_get_pokemon->subscriptors, subscriptor);
		pthread_mutex_unlock(&queue_get_pokemon->mutex_subscriptors);
		queue = queue_get_pokemon;
		break;
	case LOCALIZED_POKEMON:
		pthread_mutex_lock(&queue_localized_pokemon->mutex_subscriptors);
		list_add(queue_localized_pokemon->subscriptors, subscriptor);
		pthread_mutex_unlock(&queue_localized_pokemon->mutex_subscriptors);
		queue = queue_localized_pokemon;
		break;
	default:
		pthread_exit(NULL);
	}

	log_info(logger, "Subscripcion recibida de %s al broker para la queue %s", subcription_petition->subscriptor_id, event_code_to_string(subcription_petition->queue));

	//process_subscriptor(&(subscriptor->socket), subcription_petition, queue);
}

void process_subscriptor(uint32_t *socket, t_subscription_petition *subscription_petition, queue* queue_to_use)
{
	send_all_messages(socket, subscription_petition, queue_to_use);

	uint32_t end_subscription_time = (unsigned long)time(NULL);
	if (subscription_petition->duration < 0)
	{
		end_subscription_time += 60000;
	}
	else
	{
		end_subscription_time += subscription_petition->duration;
	}

	event_code code;

	while ((unsigned long)time(NULL) < end_subscription_time)
	{
		int bytes = recv(*socket, &code, sizeof(event_code), MSG_WAITALL);
		if (bytes == -1 || bytes == 0)
			code = -1;

		if ((unsigned long)time(NULL) < end_subscription_time)
		{
			process_request(code, *socket);
		}
	}

	pthread_exit(NULL);
}

void send_all_messages(uint32_t *socket, t_subscription_petition *subscription_petition, queue *queue_to_use)
{
	uint32_t i;
	uint32_t old_socket;
	uint32_t already_subscribed;

	for (int i = 0; i < list_size(queue_to_use->subscriptors); i++)
	{
		t_subscriptor *subscriptor = list_get(queue_to_use->subscriptors, i);

		if (string_equals_ignore_case(subscriptor->subscriptor_info->subscriptor_id, subscription_petition->subscriptor_id))
		{
			old_socket = subscriptor->socket;
			subscriptor->socket = *socket; // Override existing old socket with newly opened socket
			already_subscribed = 1;
		}
	}

	pthread_mutex_lock(&queue_to_use->mutex_messages);
	for (i = 0; i < list_size(queue_to_use->messages); i++)
	{
		queue_message *message = list_get(queue_to_use->messages, i);

		uint32_t already_received = 0;

		if (already_subscribed)
		{
			for (int j = 0; j < list_size(message->receivers); j++)
			{
				receiver *receiver = list_get(message->receivers, j);

				if (receiver->receiver_socket == old_socket && receiver->received == 1)
				{
					already_received = 1;
				}
			}
		}

		if (!already_received)
		{
			uint32_t partition_id = message->message->memory_partition_id;
			uint32_t j = 0;

			pthread_mutex_lock(&mutex_memory_partitions);
			while (j < list_size(memory_partitions) && (((t_memory_partition *)list_get(memory_partitions, j))->id != partition_id))
			{
				j++;
			}

			if (j >= list_size(memory_partitions))
			{
				pthread_mutex_unlock(&mutex_memory_partitions);
				return; //ERROR, MESSAGE NOT FOUND
			}
			else
			{
				t_memory_partition *partition = (t_memory_partition *)list_get(memory_partitions, j);
				void *content = malloc(partition->content_size);
				memcpy(content, memory + partition->begin, partition->content_size);

				t_buffer *buffer = malloc(sizeof(uint32_t) + partition->content_size);
				buffer->size = partition->content_size;
				buffer->payload = content;
				send_message(*socket, subscription_petition->queue, message->message->id,
							 message->message->correlative_id, buffer);
				log_info(logger, "Enviado mensaje id %d de tipo %s al subscriptor cuyo socket es %d", message->message->id, event_code_to_string(subscription_petition->queue), *socket);

				partition->lru_timestamp = (unsigned long)time(NULL);

				if (!already_subscribed)
				{
					receiver *receiver = malloc(sizeof(uint32_t) * 2);
					receiver->receiver_socket = *socket;
					list_add(message->receivers, receiver);
				}
				else
				{
					for (int n = 0; n < list_size(message->receivers); n++)
					{
						receiver *receiver = list_get(message->receivers, n);

						if (receiver->receiver_socket == old_socket)
						{
							receiver->receiver_socket = *socket;
						}
					}
				}
			}
			pthread_mutex_unlock(&mutex_memory_partitions);
		}
	}
	pthread_mutex_unlock(&queue_to_use->mutex_messages);
}

static void err_sys(char *msg)
{
	printf("%s \n", msg);
	exit(-1);
}

static void dump_memory()
{
	time_t rawtime;
	struct tm *timeinfo;

	time(&rawtime);
	timeinfo = localtime(&rawtime);

	FILE *dump_file;
	dump_file = fopen("dump.txt", "w");
	if (dump_file == NULL)
	{
		log_info(logger, "Error al crear el dump");
		exit(1);
	}

	fprintf(dump_file, "Dump: %d/%d/%d %d:%d:%d\n", timeinfo->tm_mday, timeinfo->tm_mon + 1, timeinfo->tm_year + 1900, timeinfo->tm_hour, timeinfo->tm_min, timeinfo->tm_sec);
	int i;
	for (i = 0; i < list_size(memory_partitions); i++)
	{
		uint32_t begin = ((t_memory_partition *)(list_get(memory_partitions, i)))->begin;
		uint32_t partition_size = ((t_memory_partition *)(list_get(memory_partitions, i)))->partition_size;
		uint32_t memory_partition_id = ((t_memory_partition *)(list_get(memory_partitions, i)))->id;
		uint32_t lru_timestamp = ((t_memory_partition *)(list_get(memory_partitions, i)))->lru_timestamp;
		uint32_t status = ((t_memory_partition *)(list_get(memory_partitions, i)))->status;
		//event_code queue;
		//uint32_t already_found_queue = 0;
/*
		for (int i = 0; i < list_size(queue_new_pokemon->messages); i++)
		{
			queue_message *message = list_get(queue_new_pokemon->messages, i);
			if (message->message->memory_partition_id == memory_partition_id)
			{
				queue = NEW_POKEMON;
				already_found_queue = 1;
			}
		}

		if (!already_found_queue)
		{
			for (int i = 0; i < list_size(queue_appeared_pokemon->messages); i++)
			{
				queue_message *message = list_get(queue_new_pokemon->messages, i);
				if (message->message->memory_partition_id == memory_partition_id)
				{
					queue = APPEARED_POKEMON;
					already_found_queue = 1;
				}
			}
		}

		if (!already_found_queue)
		{
			for (int i = 0; i < list_size(queue_catch_pokemon->messages); i++)
			{
				queue_message *message = list_get(queue_new_pokemon->messages, i);
				if (message->message->memory_partition_id == memory_partition_id)
				{
					queue = CATCH_POKEMON;
					already_found_queue = 1;
				}
			}
		}

		if (!already_found_queue)
		{
			for (int i = 0; i < list_size(queue_caught_pokemon->messages); i++)
			{
				queue_message *message = list_get(queue_new_pokemon->messages, i);
				if (message->message->memory_partition_id == memory_partition_id)
				{
					queue = CAUGHT_POKEMON;
					already_found_queue = 1;
				}
			}
		}

		if (!already_found_queue)
		{
			for (int i = 0; i < list_size(queue_get_pokemon->messages); i++)
			{
				queue_message *message = list_get(queue_new_pokemon->messages, i);
				if (message->message->memory_partition_id == memory_partition_id)
				{
					queue = GET_POKEMON;
					already_found_queue = 1;
				}
			}
		}

		if (!already_found_queue)
		{
			for (int i = 0; i < list_size(queue_localized_pokemon->messages); i++)
			{
				queue_message *message = list_get(queue_new_pokemon->messages, i);
				if (message->message->memory_partition_id == memory_partition_id)
				{
					queue = LOCALIZED_POKEMON;
					already_found_queue = 1;
				}
			}
		}*/

		fprintf(dump_file, "Partition %d: %#05x - %#05x.\t [%d]\t Size:%d b\t LRU:%d\t Queue:%s \t ID:%d\n", i, begin, (begin + partition_size), status, partition_size, lru_timestamp, "Cola", memory_partition_id);
	}
	fclose(dump_file);
}

static void sig_usr(int signo)
{
	if (signo == SIGUSR1)
	{
		dump_memory();
	}
	else
	{
		err_sys("received unexpected signal");
	}
	return;
}

t_log *logger_init_broker()
{
	return log_create(LOG_FILE, "broker", true,
					  LOG_LEVEL_INFO);
}
