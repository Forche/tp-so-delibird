#include "create_tall_grass.h"

int main(int arg_quantity, char* argv[]) {
	char* magic_number;
	read_config();

	if(arg_quantity != 4){
		printf("Debe informar cantidad de bloques, size del bloque y magic number");
		return -1;
	}

	int blocks = atoi(argv[1]);
	int block_size = atoi(argv[2]);
	magic_number = argv[3];


	printf("Creado Tall-Grass correctamente con %d bloques, de tamaño %d", blocks, block_size);

	return EXIT_SUCCESS;
}

void read_config() {
	config = config_create("/home/utnso/workspace/tp-2020-1c-Operavirus/gamecard/gamecard.config");

	PUNTO_MONTAJE = config_get_string_value(config, "PUNTO_MONTAJE_TALLGRASS");

	config_destroy(config);
}



void create_bitmap(char* path_metadata, int blocks_quantity, int blocks_size){

	char* path_bitmap_file = (char*) string_from_format("%s/Bitmap.bin", path_metadata);
	FILE* fpFile = fopen(path_bitmap_file,"wb");
	fclose(fpFile);

	int bitarray_size = blocks_quantity/8;

	truncate(path_bitmap_file, bitarray_size);

	int fd = open(path_bitmap_file, O_RDWR);
	if (fstat(fd, &mystat) < 0) {
		printf("Error al establecer fstat\n");
		close(fd);
	}

	char *bmap = mmap(NULL, mystat.st_size, PROT_READ | PROT_WRITE, MAP_SHARED,	fd, 0);

	t_bitarray* bitarray = bitarray_create_with_mode(bmap, bitarray_size, MSB_FIRST);


	for(int cont=0; cont < blocks_quantity*8; cont++){
		bitarray_clean_bit(bitarray, cont);
	}
	msync(bmap, sizeof(bitarray), MS_SYNC);

	munmap(bmap, mystat.st_size);

	free(path_bitmap_file);
	bitarray_destroy(bitarray);
}


