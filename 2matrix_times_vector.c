#include "mpi.h"
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#define min(x, y) ((x)<(y)?(x):(y))


//MPI_Send(data , count  , datatype , destination , tag , communicator?)
//
//When the root process (in our example, it was process zero) calls MPI_Bcast, 
//the data variable will be sent to all other processes.
//
//When all of the receiver processes call MPI_Bcast,
//the data variable will be filled in with the data from the root process.

int main(int argc, char* argv[])
{
  int nrows, ncols;
  double *aa;//matrix 
  double *b;// I think this is the vector we will use to multiply, no values ever given to it though?
 // i just added some code to make an example vector. see below
  double *c; //when we receive from process we store the received value here. this is the result vector from the multiplication
  double *buffer, ans;
  double *times;
  double total_times;
  int run_index;
  int nruns;
  int myid, master, numprocs;
  double starttime, endtime;
  MPI_Status status;
  int i, j, numsent, sender;
  int anstype, row;
  srand(time(0));
  MPI_Init(&argc, &argv);
  MPI_Comm_size(MPI_COMM_WORLD, &numprocs);
  MPI_Comm_rank(MPI_COMM_WORLD, &myid);


  if (argc > 1) {//if more than one processor
    nrows = atoi(argv[1]);
    ncols = nrows;
    aa = (double*)malloc(sizeof(double) * nrows * ncols);

    //if rows and cols are the same why did this person use rows for one and cols for another? y tho
    b = (double*)malloc(sizeof(double) * ncols);
    c = (double*)malloc(sizeof(double) * nrows);
    buffer = (double*)malloc(sizeof(double) * ncols);
    master = 0;
    if (myid == master) {// Master Code goes here

        //////////////////////A
        //Create random Matrix
        for (i = 0; i < nrows; i++) {
            for (j = 0; j < ncols; j++) {
              //aa[i*ncols + j] = (double)rand()/RAND_MAX;//creates  
              aa[i*ncols + j] = (i*ncols + j);
              printf("\nElement \n row:%d \n col:%d \n is %f" , i , j  , aa[i*ncols + j] );
              b[j] = aa[i*ncols + j];//I added this, this makes the vector the last row of the matrix(so we have something to work with)
            }
        }
        /////////////////////A
        
        //This starts a timer 
        starttime = MPI_Wtime();
        numsent = 0;

        //MPI_Bcast(data , count  , datatype , root ,  communicator?)
        MPI_Bcast(b, ncols, MPI_DOUBLE, master, MPI_COMM_WORLD);

        /////////////////B
        //This takes rows and sends each row to a process 
        for (i = 0; i < min(numprocs-1, nrows); i++) {
            for (j = 0; j < ncols; j++) {
                //loads the buffer with the row
                buffer[j] = aa[i * ncols + j]; 
                //buffer goes to MPI send
            }  
         
            ////gives row to process i while there are still rows/processes
            //MPI_Send(data , count  , datatype , destination , tag , communicator?)
            MPI_Send(buffer, ncols, MPI_DOUBLE, i+1, i+1, MPI_COMM_WORLD);//i+1 because we are in process 0 so we want to start at 1
            numsent++;//keep track of which rows we sent out, then when we cycle again we know which row we left off on
        }
        ///////////////B


        //////////////////////////C
        //A process has finished calculating his row(since he sent a MPI_send).
        //let's decide what to do with him
        for (i = 0; i < nrows; i++) {
          //MPI_Recv(data , count  , datatype , source , tag , communicator?)
            MPI_Recv(&ans, 1, MPI_DOUBLE, MPI_ANY_SOURCE, MPI_ANY_TAG, MPI_COMM_WORLD, &status);
            sender = status.MPI_SOURCE;
            anstype = status.MPI_TAG;
            c[anstype-1] = ans;

        /////////////////////////////////////////D
        //This means there was more rows than processes from up there^ 
        //It means we need to start cycling to give out rows again 
        //now that the receive happened we know that process is done so we can give him a new row
        if (numsent < nrows) {
          for (j = 0; j < ncols; j++) {
            //load up the row we left off at into buffer
            buffer[j] = aa[numsent*ncols + j];
          }  

          //MPI_Send(data , count  , datatype , destination , tag , communicator?)
          MPI_Send(buffer, ncols, MPI_DOUBLE, sender, numsent+1, MPI_COMM_WORLD);
          numsent++;
        }
        /////////////////////////////////D
        
        ////////////////////////////E
        //We're done since all rows were consumed. Send a 0 to notify consumers to stop consuming
        else {
          //MPI_Send(data , count  , datatype , destination , tag , communicator?)
            MPI_Send(MPI_BOTTOM, 0, MPI_DOUBLE, sender, 0, MPI_COMM_WORLD);
            }
        /////////////////E
        } 
        ///////////////////C

        ///////////////////////////////////F
        //This is the last thing to execute
         endtime = MPI_Wtime();
         printf("\n\nTime to calculate is: %f seconds\n\n",(endtime - starttime));
         /*
         for(int geo=0; geo < sizeof(buffer); geo++  ){
            printf("\n\nBuffer:%f\n\n" , buffer[geo]);
         }
         */
         for(int geo=0; geo<nrows; geo++){//ex: 3*3 matrix times 3*1 vector can only return a 3*1 vector. So we need to only loop through the amount of rows(or columns)
            printf("\n\nc:%f\n\n" , c[geo]);
         }
        ///////////////////////////////////F
    }
   
    else { // Slave Code goes here
     //MPI_Bcast(data , count  , datatype , root , communicator?)
      MPI_Bcast(b, ncols, MPI_DOUBLE, master, MPI_COMM_WORLD);//I'm ready to receive!
        
      //if I'm not the last row to be calculated. Actually I can be the last row too.
        if (myid <= nrows) {
            ///////////
            //This loop ends when sender sends 0 --->when is that? found it look up there ^
            while(1) {
              MPI_Recv(buffer, ncols, MPI_DOUBLE, master, MPI_ANY_TAG,MPI_COMM_WORLD, &status);//receive from root
              if (status.MPI_TAG == 0){
                break;
                }
              row = status.MPI_TAG;//this is to store which row I'm calculating
              ans = 0.0;

              ////////////////////
              //This is where the actual math happens(matrix row * vector)
              for (j = 0; j < ncols; j++) {
                printf("\nThis is thread:%d\n" , row);
                printf("\nHere is the vector element %i I think:%f\n" , j,  b[j]);
                ans += buffer[j] * b[j];
              }
              //MPI_Send(data , count  , datatype , destination , tag , communicator?)
              printf("\nSending this ans: %f\n" , ans);
              MPI_Send(&ans, 1, MPI_DOUBLE, master, row, MPI_COMM_WORLD);
            }
        }
    }//end of slave code 
  }
  else {
    fprintf(stderr, "Usage matrix_times_vector <size>\n");
  }
  MPI_Finalize();
  return 0;
}
