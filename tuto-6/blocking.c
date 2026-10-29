#include <mpi.h>
#include <stdio.h>
int main(int argc, char **argv) {
 int rank, size;
 int number=0;
 MPI_Init(&argc, &argv);
 MPI_Comm_rank(MPI_COMM_WORLD, &rank); 
 MPI_Comm_size(MPI_COMM_WORLD, &size);
 MPI_Send(&number,1,MPI_INT,1,0,MPI_COMM_WORLD);
 MPI_Recv(&number,1,MPI_INT,0,0,MPI_COMM_WORLD,MPI_STATUS_IGNORE);
 printf("Process 1 received number %d from process 0\n", number);
 printf("the rank is %d with size %d\n",rank,size);
 MPI_Finalize(); 
return 0;
}