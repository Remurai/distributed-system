#include <stdio.h>
#include <mpi.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

int num[10];
int num2[1000];
int n;

void getNumText(char* argv[]){
	FILE *fl;
	fl = fopen(argv[0], "r");
	char *token;
	char str[1000];
	int i;
	
	if (fl==NULL){
		printf("File %s does not exist.\n", argv[0]);
		MPI_Abort(MPI_COMM_WORLD, 1);
	}
	
	fscanf(fl, "%s", str);
	token = strtok(str, ",");
	i=0;
	while(token!=NULL){
		num[i] = atoi(token);
		token = strtok(NULL, ",");
		i++;
	}
	n=i;
	fclose(fl);
}

int main(int argc, char* argv[]){
	int rank, size, name_len, sendNum, recvNum;
	char procname[MPI_MAX_PROCESSOR_NAME];

	MPI_Status status;
	MPI_Init(&argc, &argv);
	
	MPI_Comm_rank(MPI_COMM_WORLD, &rank);
	MPI_Comm_size(MPI_COMM_WORLD, &size);
	MPI_Get_processor_name(procname, &name_len);
	
	getNumText(&argv[1]);

	//Master process
	if (rank==0){
		int index, i;
		sendNum = n/size;
		
		if(size>1){
			for (i=1; i<size-1; i++){
				//Send the data to slave processes
				index = i*sendNum;
				MPI_Send(&sendNum, 1, MPI_INT, i, 0, MPI_COMM_WORLD);
				printf("(%s) Process 0 sends %d numbers to Process %d\n", procname, sendNum, i);
				MPI_Send(&num[index], sendNum, MPI_INT, i, 0, MPI_COMM_WORLD);
			}

		//Send the left data to last slave process
		index = i*sendNum;
		int leftNum = n - index;
		MPI_Send(&leftNum, 1, MPI_INT, i, 0, MPI_COMM_WORLD);
		printf("(%s) Process 0 sends %d numbers to Process %d\n", procname, leftNum, i);
		MPI_Send(&num[index], leftNum, MPI_INT, i, 0, MPI_COMM_WORLD);
		}
		
		MPI_Barrier(MPI_COMM_WORLD);
		
		//Master calculate its own array
		int sum=0;
		for (i=0; i<sendNum; i++){
			sum+= num[i];
			printf("(%s) Process %d has: %d\n", procname, rank, num[i]);
		}
		
		MPI_Barrier(MPI_COMM_WORLD);
		
		printf("(%s) Process %d calculated: %d\n", procname, rank, sum);
		
		//Receive partial sums from slave processes
		int temp;
		for (i=1; i<size; i++){
			MPI_Recv(&temp, 1, MPI_INT, MPI_ANY_SOURCE, 0,  MPI_COMM_WORLD, &status);
			int sender = status.MPI_SOURCE;
			sum+=temp;
			printf("(%s) Partial sum received from Process %d: %d\n", procname, sender, temp);
		}
		
		printf("Total is: %d\n", sum);
	}
	
	//Slave process
	else{
		//Runs the slave processes orderly
		for (int i=1; i<size; i++){
			if (i==rank){
				MPI_Barrier(MPI_COMM_WORLD);
				
				//Receive a array of number from master process
				MPI_Recv(&recvNum, 1, MPI_INT, 0, 0, MPI_COMM_WORLD, &status);
				MPI_Recv(&num2, recvNum, MPI_INT, 0, 0, MPI_COMM_WORLD, &status);
				
				//Calculate partial sum
				int partial_sum = 0;
				for (int i=0; i<recvNum; i++){
					partial_sum+=num2[i];
					printf("(%s) Process %d has: %d\n", procname, rank, num2[i]);
				}
				
				//Send partial sum back to master process
				MPI_Send(&partial_sum, 1, MPI_INT, 0, 0, MPI_COMM_WORLD);
				MPI_Barrier(MPI_COMM_WORLD);
			}
		}
	}
	
	MPI_Finalize();
	return 0;
}
