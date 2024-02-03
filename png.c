#include <errno.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>

void reverse_byte_order(void *buf, size_t buf_size){
  uint8_t *buf0 = buf;
  for (int i = 0; i<buf_size/2; ++i) {
    uint8_t temp = buf0[i];
    buf0[i] = buf0[buf_size - i -1];
    buf0[buf_size - i -1] = temp;
  }
}
off_t fsize(const char *file_path){
  struct stat st;
  if(stat(file_path, &st) == 0){
    return st.st_size;
  }
  fprintf(stderr, "Cannot determine size of %s: %s\n",
            file_path, strerror(errno));
  return -1;
}
uint8_t buffer[32*1024];

int main(int argc, char **argv){
  char* program = *argv++;
  if(*argv == NULL){
    fprintf(stderr, "Usage: %s <input.png> <output.png> <payload.png>\n", program);
    fprintf(stderr, "ERROR: No input file was given");
    exit(1);
  }
  char* file_path = *argv++;
  FILE* input_file = fopen(file_path, "rb");
  if(input_file == NULL){
    fprintf(stderr, "ERROR: Opening %s file was not succesful: %s\n", file_path, strerror(errno));
    exit(1);
  }
  char* out_file_path = *argv++;
  FILE* output_file = fopen(out_file_path, "wb");
  if(output_file == NULL){
    fprintf(stderr, "ERROR: Opening %s file was not succesful: %s\n", out_file_path, strerror(errno));
    exit(1);
  }
  char* injection_file_path = *argv++;
  FILE* injection_file = fopen(injection_file_path, "rb");
  if(injection_file_path == NULL){
    fprintf(stderr, "ERROR: Opening %s file was not succesful: %s\n", injection_file_path, strerror(errno));
    exit(1);
  }

  uint8_t sig[8];
  const uint8_t sig_base[8] = {137,80,78,71,13,10,26,10};
  fread(sig,sizeof(sig),1,input_file);
  for (int i = 0; i<8; ++i) {
    if(sig[i] != sig_base[i]){
      fprintf(stderr, "ERROR: %s file is not PNG format", file_path);
      exit(1);
    }
  }
  fwrite(sig_base, sizeof(sig_base), 1, output_file);
  
  bool quit = false;
  while(!quit){
    uint32_t chunk_size;
    fread(&chunk_size, sizeof(chunk_size), 1, input_file);
    fwrite(&chunk_size, sizeof(chunk_size), 1, output_file);
    reverse_byte_order(&chunk_size, sizeof(chunk_size));

    uint8_t chunk_type[4];
    fread(&chunk_type, sizeof(chunk_type), 1, input_file);
    fwrite(&chunk_type, sizeof(chunk_type), 1, output_file);
    if(*(uint32_t*)chunk_type == 0x444E4549){
      quit = true;
    }
    
    fread(&buffer, chunk_size, 1, input_file);
    fwrite(&buffer, chunk_size, 1, output_file);

    uint32_t chunk_crc;
    fread(&chunk_crc, sizeof(chunk_crc), 1, input_file);
    fwrite(&chunk_crc, sizeof(chunk_crc), 1, output_file);
    reverse_byte_order(&chunk_crc, sizeof(chunk_crc));
    
    fprintf(stdout, "Chunk size: %u byte\n" ,chunk_size);
    fprintf(stdout, "------------------------------------\n");
    fprintf(stdout, "Chunk type: %.*s (0x%08X)\n",(int)sizeof(chunk_type), chunk_type, *(uint32_t*)chunk_type);
    fprintf(stdout, "------------------------------------\n");
    fprintf(stdout, "Chunk checksum: 0x%08X\n", chunk_crc);
    fprintf(stdout, "------------------------------------\n");
    printf("\n");

    if(*(uint32_t*)chunk_type == 0x52444849){
     uint32_t injected_size = fsize(injection_file_path);
     reverse_byte_order(&injected_size, sizeof(injected_size));
     fwrite(&injected_size, sizeof(injected_size), 1, output_file);
     uint8_t injected_type[4] = "kuKI";
     fwrite(&injected_type, 4, 1, output_file);
     
     uint8_t temp_buff;
     while (fread(&temp_buff, sizeof(temp_buff), 1, injection_file)) {
      fwrite(&temp_buff, sizeof(temp_buff), 1, output_file); 
     }
     uint32_t injected_crc = 0;
     fwrite(&injected_crc, sizeof(injected_crc), 1, output_file);
    }

  }
  fclose(output_file);
  fclose(injection_file);
  fclose(input_file);

}
