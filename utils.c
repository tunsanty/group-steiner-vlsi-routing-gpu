/**
 * File: utils.c
 *
 * This is a collection of utility functions used by twostar-mpi
 *
 * @author Basileal Imana
 */

//Libraries
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>

//Library for sched_getcpu()
#define _GNU_SOURCE 
#include <utmpx.h>

//Header
#include "macros.h"
#include "utils.h"


/** Validates number of processes
 *
 * @param V total number of vertices
 * @param numProc total number of processes launched
 * @return true if numProc is greater than zero and less than V
 */
bool validNumProc(int V, int numProc) {
  return (numProc > 0 && numProc <= V);
}


/** Calculates how many roots each proc gets
 *    
 * @param numProc total number of processes launched 
 * @param V total number of vertices in graph
 * @return number of vertices per parent and per child in an array
 */
int* calcLaunchPar(int numProc, int V) {
  //par[0] = perChild, par[0] = perParent
  int* par = (int *) malloc(sizeof(int) * 2);
  
  int d = V/numProc; //difference
  int r = V%numProc; //remainder
  
  if (r == 0) {
    par[0] = d;
    par[1] = d;
  
  } else {
    par[0] = d;
    par[1] = d + r;
  }

  return par;
}


/** Calculates the ID of process that handled a given root when constructing onestar or twostar
 *    
 * @param root rootId
 * @param perChild number of processes assigned per child
 * @param perParent number of processes assigned per parent
 * @param V total number of vertices in graph
 * @return ID of process that handler the given root 
 */
int getProcId(int root, int perChild, int perParent, int numProc, int V) {
  int diff = perParent - perChild; //after the first diff number of roots (which are assigned to proc 0) round robin is used 
  int procId = 0;
  if(root < diff) {
    procId = 0;
  }
  else {
    int i = (root - diff)/(numProc); //get in which round of the round robin process this root get assigned to a process
    procId = root - ((i * numProc) + diff); //rearrange root = (perParent - perChild) + (i * numProc) + procId, taken from onestarwrapper function
  }  
  return procId;
}


/** Prints graph, metric closure and predessors matrices
 * 
 * @param G graph/matrix of size VxV
 * @param V size of matrix
 * @param name name of Graph to print
 */
void print(FILE * fout, int* G, int V, char* name) {
  fprintf(fout, "%s: \n\t", name);
  for(int i = 0; i < V; i++, fprintf(fout,"\n\t")) {
    for(int j = 0; j < V; j++) {
       int out = G[i * V + j];
       if(out  == INF)
          fprintf(fout, "%3s " , "INF");
       else
          fprintf(fout, "%3d " , out );
    }
  }
  fprintf(fout, "\n");
}


/** Print terminal vertices
 *
 * @param numTer number of terminals vertices in graph
 * @param terminals array of terminal vertices
 */
void printTerm(FILE * fout, int numTer, int* terminals) {
  fprintf(fout, "\nTerminal vertices: \n");

  fprintf(fout, "\t# of vertices: %d  ",numTer);
  fprintf(fout, "List of vertices: ");

  for(int i = 0; i < numTer; i++) {
    fprintf(fout, "%d ", terminals[i]);
  }
  fprintf(fout, "\n");
}

/** Print groups
 *
 * @param numGroups number of groups in graph
 * @param numTer number of terminals vertices in graph
 * @param groups array of groups
 */
void printGroups(FILE * fout, int numGroups, int numTer, int* groups) {
  fprintf(fout, "\nGroups: \n");

  fprintf(fout, "\tNum of groups: %d\n",numGroups);

  for(int i = 0; i < numGroups; i++) {
    int curr = groups[i * numTer + 0];
    fprintf(fout, "\t\tGroup %d (%d):", i,curr);
    for(int j = 1; j <= curr; j++) {
      fprintf(fout, " %d ", groups[i * numTer + j]);
    }
    fprintf(fout, "\n");
  }
  fprintf(fout, "\n");
}


/** Print partial stars
 *
 * @param partialstars an array of partial stars
 * @param numGroups number of groups in graph
 * @param count number of partial stars
 */
void printPartialStars(FILE * fout, int* partialstars, int numGroups, int count) {
  fprintf(fout, "\nParital stars: \n");

  for(int i = 0; i < count; i++) {
    int* curStar = partialstars + (i * (2 + numGroups));
    fprintf(fout, "\tIntermediate: %d # of Groups: %d Group IDs ",curStar[0],curStar[1]);
    for(int j = 0; j < curStar[1]; j++) {
      fprintf(fout, " %d ", curStar[j+2]);
    }
    fprintf(fout, "\n");
  }
  fprintf(fout, "\n");
}


/** Print one star
 *
 * @param onestar array of root to group costs
 * @param numGroups number of groups in graph
 * @param V number of vertices in graph
 * @param name name to print
 */
void printOnestar(FILE * fout, int* onestar, int numGroups, int V, char* name) {
  fprintf(fout,"\n%s: \n", name);
  
  for(int i = 0; i < V; i++) {
    fprintf(fout, "\tRoot%d:  ", i);
    for(int j = 0; j < numGroups; j++) {
      fprintf(fout, "%3d  ", onestar[i * numGroups + j]);
    }     
    fprintf(fout, "\n");
  }
}


/** Print two star cost
 *
 * @param root root ID of two star
 * @param cost cost of two star
 */
void printTwoStarCost(FILE * fout, int root, long cost) {
  fprintf(fout, "\nTWOSTAR: \n");
  fprintf(fout, "\tCost: %ld Root: %d\n", cost, root);
}


/** Print ID of CPU current proces is running on
 *
 * @param procId process ID of current processs
 */
void printCpuID(FILE * fout, int procId) {
  fprintf(fout, "Process ID = %d CPU ID = %d\n", procId, sched_getcpu());
}


/** Calculates total cost of a given graph
 *
 * @param G graph of size VxV
 * @param V size of graph
 * @return total graph cost
 */
long caclGraphCost(int* G, int V) {
  long sum = 0;
  int curr;
  for(int i = 0; i < V; i++) {
    for(int j = 0; j < V; j++) {
      if (i <= j) { //graph is undirected symmetric , just count upper or lower triangle
        continue;
      }
      int curr = G[i * V + j]; //get G[i][j]
      if(curr  == INF) { //if not edge between i and j, continue
        continue;
      }
      sum += curr; //add to sum
    }
  }
  return sum;
}

/** Counted the number of edges in a solution graph
 *
 * @param G graph of size VxV
 * @param V size of graph
 */
int countEdges(int * G, int V) {
  int count = 0;  
  for(int i =0; i < V; i++) {
    for(int j = 0; j < V; j++) {
      //since grpah is undirected hence symmetric, ignore elements below the diagonal
      if(i >= j) {
        continue;
      }
      if(G[i * V + j] != INF) {
        count++;
      }
    }
  }
  return count;
}

void writetoFile(int* S, int *C, int V, char* filename) {
  int E = countEdges(S,V);
  FILE *fp = fopen(filename, "w+");
  //write graph size
  fprintf(fp,"%d %d\n\n",V,E);

  //write edges
  for(int i = 0; i < V; i++) {
    for(int j = 0; j < V; j++) {
      if(i >=j)
        continue;
      int out = S[i * V + j];
      if(out  == INF)
         continue;
      else
        fprintf(fp,"%d %d %d\n",i+1,j+1,out);
    }
  }
  fprintf(fp,"\n");

  //write coordinates
  for(int i = 0; i < V; i++) {
    fprintf(fp,"%d %d %d\n",i+1,C[i * 2 +0], C[i * 2 +1]);
  }
}

/** Checks if a vertex is a terminal vertex
 *
 * @param v vertex in question
 * @param numTer number of terminal vertices
 * @param terminals array of all terminal vertices
 * @return 1 if is terminal and 0 if is non-terminal
 */
int isTerminal(int v, int numTer, int* terminals) {
  for(int i = 0; i < numTer; i++)
    if(v == terminals[i])
      return 1;
  return 0;
}


/** Counts number of non-terminals of a solution graph
 *
 * @param G graph of size VxV
 * @param V size of graph
 * @param numTer number of terminal verices
 * @param terminals number of terminals
 * @return total count of terminals vertices in solution graph G
 */
int countNonTerminals(int* G, int V, int numTer, int* terminals) {
  int count = 0;
  for(int i = 0; i < V; i++) { //for each vertex 'i' in the graph
    if(isTerminal(i,numTer,terminals)) { //if is known to be terminal skip 
      continue;
    }
    int edge = 0;
    for(int j = 0; j < V; j++) { //for each vetex 'j' in i'th row 
      if(G[i * V + j] != INF) { //if atlease one edge then count
        edge = 1;
        break;
      }
    }
    if(edge == 1) {
      count++;
    }
  }
  return count;
}


/** Copies a partial stars form A to B
 *
 * @param A partial stars array to copy from
 * @param V partial stars array to copy to
 * @param numGroups number of groups in graph
 * @param count number of parital stars in A
 */
void copypartialStar(int* A, int* B, int numGroups, int count) {
  //A - from, B - to
  for(int i = 0; i < count; i++) { //for each partial star
    //get a partial star
    int * curStarA = A + (i * (2 + numGroups));
    int * curStarB = B + (i * (2 + numGroups));

    //copy intermediate and num of groups
    curStarB[0] = curStarA[0];
    curStarB[1] = curStarA[1];

    //copy groups
    for(int j = 0; j < curStarA[1]; j++) {
      curStarB[j+2] = curStarA[j+2];
    }
  }
}

int getNextAvailableRoot(int * rootsAvail, int V, int procId) {
  for(int i = 0; i < V; i++) {
    if(rootsAvail[i]) {
      rootsAvail[i] = 0;
      return i;
    }
  }
  return -1;
}

int printAvailRoot(int * rootsAvail, int V) {
  printf("Roots avail: \n");
  for(int i = 0; i < V; i++) {
    printf(" %d",rootsAvail[i]);
  }
  printf("\n");
}
