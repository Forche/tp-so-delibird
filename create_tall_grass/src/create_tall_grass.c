#include "create_tall_grass.h"

int main(int arg_quantity, char* argv[]) {

	read_config();

	if(arg_quantity != 4){
		printf("Debe informar cantidad de bloques, size del bloque y magic number \n");
		return -1;
	}
	int blocks = atoi(argv[1]);
	int block_size = atoi(argv[2]);
	char* magic_number = argv[3];

	create_tall_grass_directory();

	char* dir_metadata = string_from_format("%s/Metadata", PUNTO_MONTAJE);
	mkdir(dir_metadata, 0777);
	char* dir_files = string_from_format("%s/Files", PUNTO_MONTAJE);
	mkdir(dir_files, 0777);
	char* dir_blocks = string_from_format("%s/Blocks", PUNTO_MONTAJE);
	mkdir(dir_blocks, 0777);

	create_bitmap(dir_metadata, blocks, block_size);
	create_metadata_file(dir_metadata, blocks, block_size, magic_number);
	create_blocks(dir_blocks, blocks, block_size);

	free(dir_metadata);
	free(dir_files);
	free(dir_blocks);

	printf("Creado Tall-Grass correctamente con %d bloques, de tamaño %d \n", blocks, block_size);
	config_destroy(config);
	return EXIT_SUCCESS;
}

void read_config() {
	config = config_create("/home/utnso/workspace/tp-2020-1c-Operavirus/gamecard/gamecard.config");
	PUNTO_MONTAJE = config_get_string_value(config, "PUNTO_MONTAJE_TALLGRASS");
}

void create_tall_grass_directory() {
	char **tall_grass_path_splited = string_split(PUNTO_MONTAJE, "/");
	char* base_path = string_new();
	string_append(&base_path, "/home/utnso/");

	//las posicion 0 es home, posicion 1 utnso
	if(tall_grass_path_splited[2]!=NULL){
		int i=2;
		while(tall_grass_path_splited[i]!=NULL){
			string_append(&base_path, tall_grass_path_splited[i]);
			mkdir(base_path, 0777);
			string_append(&base_path, "/");
			i=i+1;
		}
	}
	free(base_path);

	int i = 0;
	while(tall_grass_path_splited[i]!=NULL){
		free(tall_grass_path_splited[i]);
		i = i +1;
	}
	free(tall_grass_path_splited);
}

void create_metadata_file(char* dir_metadata, int blocks, int block_size, char* magic_number){
	char* path_metadata_file = string_from_format("%s/Metadata", dir_metadata);
	FILE* file = fopen(path_metadata_file, "wb+");
	fclose(file);

	char* magic = string_new();
	string_append(&magic, magic_number);
	t_config* metadata = config_create(path_metadata_file);
	dictionary_put(metadata->properties,"BLOCK_SIZE", string_itoa(block_size) );
	dictionary_put(metadata->properties, "BLOCKS", string_itoa(blocks));
	dictionary_put(metadata->properties, "MAGIC_NUMBER", magic);

	config_save(metadata);
	config_destroy(metadata);
	free(path_metadata_file);
}

void create_bitmap(char* path_metadata, int blocks_quantity, int blocks_size){
	// Creo el archivo
	char* path_bitmap_file = (char*) string_from_format("%s/Bitmap.bin", path_metadata);
	FILE* fpFile = fopen(path_bitmap_file,"wb");
	fclose(fpFile);

	//le doy el tamaño de mi cantidad de bloques (bits que contendra el array), expresada en bytes (1 byte = 8 bits)
	int bitarray_size = blocks_quantity/8;

	truncate(path_bitmap_file, bitarray_size);

	//Abro el archivo con libreria mmap para setear los valores iniciales en cero.
	int fd = open(path_bitmap_file, O_RDWR);
	if (fstat(fd, &mystat) < 0) {
		printf("Error al establecer fstat\n");
		close(fd);
	}

	char *bmap = mmap(NULL, mystat.st_size, PROT_READ | PROT_WRITE, MAP_SHARED,	fd, 0);
	t_bitarray* bitarray = bitarray_create_with_mode(bmap, bitarray_size, MSB_FIRST);


	for(int cont=0; cont < blocks_quantity; cont++){
		bitarray_clean_bit(bitarray, cont);
	}
	msync(bmap, sizeof(bitarray), MS_SYNC);
	munmap(bmap, mystat.st_size);

	free(path_bitmap_file);
	bitarray_destroy(bitarray);
}

void create_blocks(char* dir_blocks, int blocks_quantity, int block_size){
    for(int i=0; i<blocks_quantity; i++){
        char *block_file_path = string_from_format("%s/%d.bin", dir_blocks, i);
        FILE* fpFile = fopen(block_file_path,"wb+");
        fclose(fpFile);
        truncate(block_file_path, block_size);
        free(block_file_path);
    }
}


