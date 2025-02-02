#ifndef BROKER_H_
#define BROKER_H_

#include<stdio.h>
#include<stdlib.h>
#include<commons/string.h>
#include<pthread.h>
#include<operavirus-commons.h>
#include<sys/socket.h>
#include<unistd.h>
#include<netdb.h>
#include<commons/log.h>
#include<commons/collections/list.h>
#include<string.h>
#include<pthread.h>
#include<signal.h>
#include<commons/config.h>
#include<signal.h>
#include<time.h>

typedef struct
{
	uint32_t receiver_socket;
	uint32_t received; // 1: received, 0: not received
} receiver;

typedef struct
{
	t_memory_message* message;
	t_list* receivers; // Type: receiver*
} queue_message;

t_subscriptor* sub;

typedef struct
{
	t_list* messages; // Type: queue_message*
	t_list* subscriptors; // Type: t_subscriptor*
	pthread_mutex_t mutex_messages;
	pthread_mutex_t mutex_subscriptors;
} queue;

typedef enum
{
	FREE,
	OCCUPIED,
} partition_status;

typedef struct {
	uint32_t begin;
	int id;
	partition_status status;
	uint32_t content_size;
	uint32_t partition_size;
	uint64_t lru_timestamp;
	uint64_t occupied_timestamp;
	char* father;
	char* side; // '0' for left and '1' for right;
	pthread_mutex_t mutex_partition;
} t_memory_partition;


char *IP;
char *PORT;
pthread_t thread;
t_log *logger;
uint32_t message_count;
uint32_t partition_count;
queue* queue_new_pokemon;
queue* queue_appeared_pokemon;
queue* queue_catch_pokemon;
queue* queue_caught_pokemon;
queue* queue_get_pokemon;
queue* queue_localized_pokemon;
int TAMANO_MINIMO_PARTICION;
int TAMANO_MEMORIA;
char* ALGORITMO_PARTICION_LIBRE;
char* ALGORITMO_REEMPLAZO;
char* ALGORITMO_MEMORIA;
int FRECUENCIA_COMPACTACION;
char* LOG_FILE;
void* memory;
t_list* memory_partitions;

t_log* logger;

pthread_mutex_t mutex_message_id;
pthread_mutex_t mutex_memory_partition_id;
pthread_mutex_t mutex_memory_partitions;

void server_init(void);
void memory_init();
void queues_init();
void wait_for_client(uint32_t);
void process_request(uint32_t event_code, uint32_t socket);
void process_new_subscription(uint32_t client_socket);
uint32_t exist_in_queue(queue* queue, char* id);
void replace_socket_in_queue_and_messages(queue* queue, char* subscription_to_add_id, uint32_t socket);
void process_message_received(uint32_t client_socket);
void serve_client(uint32_t* socket);
void process_subscriptor(uint32_t* socket, t_subscription_petition* subscription_petition, queue* queue_to_use);
void send_all_messages(uint32_t* socket, t_subscription_petition* subscription_petition, queue* queue_to_use);

void store_message(t_message* message, queue* queue, t_list* receivers);
uint32_t store_payload_particiones(void* payload, uint32_t size, uint32_t message_id);
uint32_t store_payload_bs(void* payload, uint32_t size, uint32_t message_id);
t_memory_partition* get_free_partition_particiones(uint32_t size);
t_memory_partition* get_free_partition_bs(uint32_t size);
t_memory_partition* find_free_partition(uint32_t size);
t_memory_partition* find_free_partition_ff(uint32_t size);
t_memory_partition* find_free_partition_bf(uint32_t size);
void delete_partition_and_consolidate();
void delete_partition_and_consolidate_fifo();
void delete_partition_and_consolidate_lru();
void perform_compaction();
void delete_and_consolidate(uint32_t memory_partition_id);
void consolidate(uint32_t memory_partition_to_consolidate_id);

uint32_t get_message_id();
uint32_t get_partition_id();

static void sig_usr(int signo);
static void sig_pipe(int signo);
static void err_sys(char* msg);
static void dump_memory();
void process_message_ack(queue* queue, char *subscriptor_id, uint32_t received_message_id);

t_log* logger_init_broker();

#endif
