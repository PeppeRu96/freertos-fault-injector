#include <stdio.h>
#include <stdlib.h>
#include <string.h>
//#include <sys/uio.h>
//#include <unistd.h>
#include <errno.h>
//#include <err.h>
#include <time.h>

void inject(unsigned long start_address, unsigned char * buffer, int target_size);

//pid_t simulator_pid;

typedef struct{
    char* name;
    int type;
    unsigned long address;
} structure;

int main(){
    
    srand((unsigned)time(NULL));
    unsigned long a_variable_address;

    char *error;
    int err;
    unsigned char target_buffer[168];

    FILE *fp;
    int i=0, num, *luogo, *tempo, id;

    structure struct_array[114];


    if((fp=fopen("structures.txt", "r"))==NULL) {
        exit(1);
    }

    printf("Inserire quante iniezioni si vuole tentare:\n");
    scanf("%d",&num);

    luogo = (int*) malloc(num * sizeof(int));
    tempo = (int*) malloc(num * sizeof(int));

    printf("Le strutture che possono essere iniettate sono:\n");
    while(!feof(fp)){
        fscanf(fp,"%d %s %d %lu\n", &id, struct_array[id].name, &struct_array[id].type, &struct_array[id].address );
        printf("%d ..... %s\n", id, struct_array[id].name);
    }

    fclose(fp);
    printf("\n\n");

    for(i=0; i<num; i++){
        printf("Inserire l'ID del luogo %d' che si vuole iniettare:\n",i);
        scanf("%d",&luogo[i]);

        printf("Inserire il valore di tempo %d' della finestra di esecuzione in millisecondi:\n",i);
        scanf("%d",&tempo[i]);
    }

    for(i=0; i<num; i++){
        printf("Iniziezione %d nella struttura %s entro %d ms\n", i, struct_array[luogo[i]].name, tempo[i]);
    }

    /*
    printf("dimensione buffer: %d\n", sizeof(target_buffer));
    //local and remote must be always declared as arrays, even with 1 element, to be used with xxxx[0]
    struct iovec local[1];
    struct iovec remote[1];
    ssize_t nwrite;
    
    local[0].iov_base = &target_buffer;
    local[0].iov_len = 168;
    
    remote[0].iov_len = 168;
    
    simulator_pid = fork();
    
    if(simulator_pid != 0){
	//injector process
	printf("Inserire indirizzo di QueueHandle: ");
	scanf("%lx", &a_variable_address);
	
	    
	remote[0].iov_base = (void*) a_variable_address;
	
	nwrite = process_vm_readv(simulator_pid, local, 1, remote, 1, 0);
	if(nwrite == -1){
			switch(errno){
			    case EINVAL:
			    	printf("ERROR: INVALID ARGUMENTS.\n");
			    	break;
			    case EFAULT:
			    	printf("ERROR: UNABLE TO ACCESS TARGET MEMORY ADDRESS.\n");
			    	break;
			    case ENOMEM:
			    	printf("ERROR: UNABLE TO ALLOCATE MEMORY.\n");
			    	break;
			    case EPERM:
			    	printf("ERROR: INSUFFICIENT PRIVILEGES TO TARGET PROCESS.\n");
			    	break;
			    case ESRCH:
			    	printf("ERROR: PROCESS DOES NOT EXIST.\n");
			    	break;
			    default:
			    	printf("ERROR: AN UNKNOWN ERROR HAS OCCURRED.\n");
			}
	}
	
	printf("Contenuto letto: %d %d %d %d\n", target_buffer[0], target_buffer[1], target_buffer[2], target_buffer[3]);
    	printf("Iniezione al processo %d\n", simulator_pid);
    	
	while(1){
		sleep(1);
		inject(a_variable_address, target_buffer, 168);
	}
	
    }
    else{
        //simulator process
        printf("child");
        execl("./Demo", "Demo", (char*)NULL);
    }
    */
    free(luogo);
    free(tempo);
}
/*
void inject(unsigned long start_address, unsigned char * buffer, int target_size){
int nwrite;
    int target_byte_number = rand() % target_size;
    
    unsigned char target_byte = buffer[target_byte_number];
    
    int target_bit_number = rand() % 8;
    
    int new_byte = target_byte ^ (1<<target_bit_number);
    
    buffer[target_byte_number] = new_byte;
    
    struct iovec local[1];
    struct iovec remote[1];
    
    local[0].iov_base = buffer;
    local[0].iov_len = target_size;
    
    remote[0].iov_len = target_size;
    remote[0].iov_base = (void*) start_address;
    
    nwrite = process_vm_writev(simulator_pid, local, 1, remote, 1, 0);
		if(nwrite == -1){
			switch(errno){
			    case EINVAL:
			    	printf("ERROR: INVALID ARGUMENTS.\n");
			    	break;
			    case EFAULT:
			    	printf("ERROR: UNABLE TO ACCESS TARGET MEMORY ADDRESS.\n");
			    	break;
			    case ENOMEM:
			    	printf("ERROR: UNABLE TO ALLOCATE MEMORY.\n");
			    	break;
			    case EPERM:
			    	printf("ERROR: INSUFFICIENT PRIVILEGES TO TARGET PROCESS.\n");
			    	break;
			    case ESRCH:
			    	printf("ERROR: PROCESS DOES NOT EXIST.\n");
			    	break;
			    default:
			    	printf("ERROR: AN UNKNOWN ERROR HAS OCCURRED.\n");
			}
		}
		else
    printf("Iniettato byte %d\n", target_byte_number);
}
*/


