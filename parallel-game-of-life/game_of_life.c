/*
@file page_rank.c
@author https://github.com/syrigonaki

@brief game of life implementation using openmp
Spring Semester 2024
*/

#include <omp.h>
#include <stdio.h>
#include <stdlib.h>

#define NUM_OF_THRDS 4
int ** grid;
int ** next_state;
int rows_n;
int cols_n;

/*parse input file and init grid with 0/1s*/
void read_input_file(char * input_file) {
    FILE *file = fopen(input_file, "r");
    if (file==NULL) {
        perror("error when opening file");
        exit(1);
    }

    fscanf(file, "%d %d\n", &rows_n, &cols_n); //first line is array size

    grid = malloc(rows_n * sizeof(int *));
    next_state = malloc(rows_n * sizeof(int *));

    if (grid==NULL || next_state==NULL) {
        perror("malloc error");
        exit(1);
    }

    for (int i = 0; i<rows_n; i++) {
        grid[i]=malloc(cols_n * sizeof(int));
        if (grid[i]==NULL) {
            perror("malloc error");
            exit(1);
        }

        next_state[i] = malloc(cols_n * sizeof(int));
        if (next_state[i]==NULL) {
            perror("malloc error");
            exit(1);
        }
    }

    int r=0, c=0;
    char ch=fgetc(file);

    while (ch!=EOF && r<rows_n) {     
        if (ch=='\n') {
            r++;
            c=0;
        } else if (ch=='|') {
            ;
        } else if (ch=='*') {
            grid[r][c] = 1;
            c++;
        } else if (ch==' ') {
            grid[r][c] = 0;
            c++;
        } 
        ch=fgetc(file);
    }

        fclose(file);

}

void write_output(char * output_file) {

    FILE * file = fopen(output_file, "w");
    if (file==NULL) {
        perror("error opening output file");
        exit(1);
    }

    fprintf(file, "%d %d\n", rows_n, cols_n);

    for (int i=0; i<rows_n; i++) {
        for (int j=0; j<cols_n; j++) {
            fprintf(file, "|%c", grid[i][j]==1 ? '*':' ');
        }
        fprintf(file, "|\n");
    } 

    fclose(file);
}

void free_grid() {

    for (int i = 0; i < rows_n; i++) free(grid[i]);
    free(grid);
    for (int i = 0; i < rows_n; i++) free(next_state[i]);
    free(next_state);
}

int get_alive_n(int row, int col) {
    int count=0;
    for (int r=row-1; r<=row+1; r++) {
        for (int c=col-1; c<=col+1; c++) {
            if (r==row && c==col) continue; //skip current cell
            if (r>-1 && c>-1 && r<rows_n && c<cols_n) count += grid[r][c]; //check bounds before adding neighbor
        }
    }
    return count;
}

/*compute the new state for next_state array by checking game rules on grid array*/
void compute_next_state() {
    #pragma omp parallel for shared(grid, next_state, rows_n, cols_n) collapse(2)
    for (int i=0; i<rows_n; i++) {
        for (int j=0; j<cols_n; j++) {
            int alive_neighbors = get_alive_n(i,j);
            int new_state=0;

            if (grid[i][j]==1) { 
                if (alive_neighbors < 2 || alive_neighbors > 3) new_state=0;
                else new_state = 1;
            } else {
                if (alive_neighbors==3) new_state=1;
            }

            next_state[i][j]=new_state;
        }
    }
}

/*after computing all cells' new state, update the grid array*/
void update_state() {
    #pragma omp parallel for shared(grid, next_state, rows_n, cols_n) collapse(2)
    for (int i=0; i<rows_n; i++) {
        for (int j=0; j<cols_n; j++) {
            grid[i][j]=next_state[i][j];
        }
    }
}

int main(int argc, char *argv[]) {

    if (argc!=4) {
        printf("Not enough arguments: Usage: %s, <input_file>, <number_of_rounds>, <output_file>\n", argv[0]);
        return 1;
    }
    
    read_input_file(argv[1]);

    int generations = atoi(argv[2]);

    omp_set_num_threads(NUM_OF_THRDS); //need to remove this line to compile serial program

    for(int gen=0; gen<generations; gen++) {
        compute_next_state();
        update_state();
    }

    write_output(argv[3]);

    free_grid();

    return 0;
}
