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

typedef struct
{
	t_list* messages; // Type: queue_message*
	t_list* subscriptors; // Type: t_subscriptor*
} queue;

typedef enum
{
	FREE,
	OCCUPED,
} partition_status;

typedef struct {
	uint32_t begin;
	uint32_t id;
	partition_status status;
	uint32_t content_size;
	uint64_t lru_time; // Won't be using this by now until we implement compaction
} t_memory_partition;


pthread_t thread;
uint32_t message_count;
uint32_t partition_count;
t_list* answered_messages;
t_list* threads;
queue queue_new_pokemon;
queue queue_appeared_pokemon;
queue queue_catch_pokemon;
queue queue_caught_pokemon;
queue queue_get_pokemon;
queue queue_localized_pokemon;
int TAMANO_MINIMO_PARTICION;
int TAMANO_MEMORIA;
char* ALGORITMO_PARTICION_LIBRE;
void* memory;
t_list* memory_partitions;
pthread_mutex_t mutex_message_id;
t_log* logger;

void server_init(void);
void memory_init();
void queues_init();
void wait_for_client(uint32_t);
void process_request(uint32_t event_code, uint32_t socket);
void process_new_subscription(uint32_t client_socket);
void serve_client(uint32_t* socket);
void process_subscriptor(uint32_t* socket, t_subscription_petition* subscription_petition, queue queue);
void send_all_messages(uint32_t* socket, t_subscription_petition* subscription_petition, queue queue);

void store_message(t_message* message, queue queue, t_list* receivers);
uint32_t store_payload(void* payload, uint32_t size);
t_memory_partition* get_free_partition(uint32_t size);
t_memory_partition* get_free_partition_ff(uint32_t size);
t_memory_partition* get_free_partition_bf(uint32_t size);
void perform_compaction();

uint32_t get_message_id();

static void sig_usr(int signo);
void  err_sys(char* msg);
void dump_memory();

#endif
