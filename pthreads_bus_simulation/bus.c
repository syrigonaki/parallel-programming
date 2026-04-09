#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <semaphore.h>
#include <unistd.h>
#include <time.h>
#include <string.h>

#define BUS_MAX 12
#define TRAVEL_TIME 10


typedef enum {MATH=0, PHYS=1, CSD=2, CHEM=3} dep_t;
char * dep_name[4] = {"Math", "Physics", "Computer Sceince", "Chemistry"};
int students_n=0;
sem_t print_sem;

int arrived=0;

typedef struct student {
    int id;
    dep_t department;
    sem_t arrival_uni;
    sem_t arrival_home;
    unsigned int study_time;
    struct student * next;
} student_t;

student_t ** uni;
    


struct stop {
    char name;
    int front;
    int back;
    student_t ** stop;
    sem_t sem;
} stop_a, stop_b;


struct bus_t {
    student_t * bus_seats[BUS_MAX];
    int count;
    int dep_count[4];
} bus;


void print_info() {
    int i=0;
    printf("Stop A: ");

    for (i=stop_a.front; i!=stop_a.back; i=(i+1)%(students_n+1)) {
        if (stop_a.stop[i]==NULL) continue;
        printf("[%d, %s] ", stop_a.stop[i]->id, dep_name[stop_a.stop[i]->department]);
    }
    printf("\n");
    
    printf("Bus: ");
    for (i=0; i<BUS_MAX; i++) {
        if (bus.bus_seats[i]==NULL) continue;
        printf("[%d, %s] ", bus.bus_seats[i]->id, dep_name[bus.bus_seats[i]->department]);
    }
    printf("\n");


    printf("University: ");
    for (i=0; i<students_n; i++) {
        if (uni[i]==NULL) continue;
        printf("[%d, %s] ", uni[i]->id, dep_name[uni[i]->department]);
    }
    printf("\n");

    printf("Stop B: ");
    for (i=stop_b.front; i!=stop_b.back; i=(i+1)%(students_n+1)) {
        if (stop_b.stop[i]==NULL) continue;
        printf("[%d, %s] ", stop_b.stop[i]->id, dep_name[stop_b.stop[i]->department]);
    }
    printf("\n\n");
}

void add_to_stop(struct stop * stop, student_t * student) {

    
    if (stop->front==stop->back) {
        stop->front=0;
        stop->back=0;
    }

    stop->stop[stop->back] = student;
    stop->back = (stop->back + 1)%(students_n+1);
   
    sem_wait(&print_sem);
    printf("Student (%d, %s) arrived at stop %c.\n", student->id, dep_name[student->department], stop->name);
    print_info();
    sem_post(&print_sem); 

}

int board_bus(struct stop * stop) {
    student_t * student;
    int i;
    bus.count=0;
    memset(bus.dep_count, 0, sizeof(bus.dep_count));
    memset(bus.bus_seats, 0, BUS_MAX*sizeof(student_t *));

    sem_wait(&stop->sem);

    i=stop->front;
    while(i!=stop->back) {
        student = stop->stop[i];
        
        if (stop->stop[i]!=NULL) {
           if (bus.count == BUS_MAX) { //if bus is full
                sem_wait(&print_sem);
                printf("Student %d (%s) couldn't board the bus :(\n", student->id, dep_name[student->department]);
                sem_post(&print_sem);
           } else {
                if (bus.dep_count[student->department]==BUS_MAX/4) { //if the department conut is N/4
                    sem_wait(&print_sem);
                    printf("Student %d (%s) couldn't board the bus :(\n\n", student->id, dep_name[student->department]);
                    sem_post(&print_sem);
                } else {
                    sem_wait(&print_sem);
                    bus.bus_seats[bus.count] = student;
                    bus.count++;
                    bus.dep_count[student->department]++;
                    stop->stop[i]=NULL;
                    printf("Student %d (%s) has boarded the bus at Stop %c!!\n", student->id, dep_name[student->department], stop->name);
                    print_info();
                    sem_post(&print_sem);    
                }
           }
        }
        i=(i+1)%(students_n + 1);
    }

    //remove empty spaces from stop queue

    int new_back = stop->front;
    for (i=stop->front; i!=stop->back; i=(i+1)%(students_n+1)) {
        if (stop->stop[i]!=NULL) {
            stop->stop[new_back]=stop->stop[i];
            new_back = (new_back + 1)%(students_n + 1);
        }
    }

    stop->back = new_back;
   
    sem_post(&stop->sem);
    
    return 0;

}


void * bus_ride() {  

    sem_t * temp_sem;
    while(1) {

        
        sem_wait(&print_sem);
        printf("Bus is on the way to Stop A...\n\n");
        sem_post(&print_sem);
        
        sleep(TRAVEL_TIME);
        
        sem_wait(&print_sem);
        printf("Bus arrived to Stop A!\n\n");
        sem_post(&print_sem);

        for (int i=0; i<bus.count; i++) {
	    temp_sem=&bus.bus_seats[i]->arrival_home;
	    bus.bus_seats[i]=NULL;
            sem_post(temp_sem); //notify student threads of arrival
        }

        arrived = arrived + bus.count; 

        if (arrived >= students_n) break;

        board_bus(&stop_a);

        sem_wait(&print_sem);
        printf("Bus is on the way to University...\n\n");
        sem_post(&print_sem);

        sleep(TRAVEL_TIME);

        sem_wait(&print_sem);
        printf("Bus arrived to University!\n\n");
        sem_post(&print_sem);


        for (int i=0; i<bus.count; i++) {
	    temp_sem = &bus.bus_seats[i]->arrival_uni;
	    bus.bus_seats[i]=NULL;
            sem_post(temp_sem);
            
        }

        board_bus(&stop_b);

    }

    return NULL;
}


void * student(void * st) {
   
    student_t * student = (student_t *)st;

    sem_wait(&print_sem);
    printf("Student %d (%s) created.\n\n", student->id, dep_name[student->department]);
    sem_post(&print_sem); 
    
    sem_wait(&stop_a.sem); 
    add_to_stop(&stop_a, student);
    sem_post(&stop_a.sem);

    sem_wait(&student->arrival_uni); //wait to be notified by bus thread

    sem_wait(&print_sem); 
    uni[student->id-1]=student; 
    printf("Student %d (%s) arrived at university.\n", student->id, dep_name[student->department]);
    print_info();
    sem_post(&print_sem);

    sleep(student->study_time);
    
    uni[student->id-1]=NULL;

    sem_wait(&print_sem);
    printf("Student %d (%s) studied for %d seconds, and headed to Stop B.\n\n", student->id, dep_name[student->department], student->study_time);
    sem_post(&print_sem);

    sem_wait(&stop_b.sem); 
    add_to_stop(&stop_b, student);
    sem_post(&stop_b.sem);
    
    sem_wait(&student->arrival_home); //wait to be notified by bus thread

    sem_wait(&print_sem);
    printf("Student %d (%s) went home.\n", student->id, dep_name[student->department]);
    print_info();

    sem_destroy(&student->arrival_home);
    sem_destroy(&student->arrival_uni);
    free(student);

    sem_post(&print_sem); 

   

    return NULL;
}

void initialize() {

    stop_a.front=0; stop_a.back=0;
    stop_b.front=0;stop_b.back=0;
    stop_a.name = 'A'; stop_b.name ='B';

    bus.count=0;
   
    sem_init(&stop_a.sem, 0, 1);
    sem_init(&stop_b.sem, 0, 1);
    sem_init(&print_sem, 0, 1);
    
    
    printf("Enter the number of students:\n");
    scanf("%d", &students_n);
    stop_a.stop = malloc((students_n+1) * sizeof(student_t *));
    stop_b.stop = malloc((students_n+1) * sizeof(student_t *));
    uni = malloc(students_n * sizeof(student_t *));

    memset(uni, 0, students_n*sizeof(student_t *));

    return;
}


int main() {

    srand(time(0));
    student_t * stud;

    initialize();

    pthread_t bus_thread; 
    pthread_t * student_threads = (pthread_t *)malloc(students_n*sizeof(pthread_t));

   
    for (long int i=0; i<students_n; i++) {
        stud = malloc(sizeof(student_t));
        if (stud==NULL) exit(EXIT_FAILURE);
        stud->department = rand() % 4;
        stud->id = i+1;
        stud->study_time = rand() % 11 + 5;
        sem_init(&stud->arrival_uni, 0, 0);
        sem_init(&stud->arrival_home, 0, 0);
        pthread_create(&student_threads[i], NULL, student, (void *)stud);
    }
    
    bus_thread = pthread_create(&bus_thread, NULL, bus_ride, NULL );
    
    for (int i=0; i<students_n; i++) {
        pthread_join(student_threads[i], NULL);
    }
    pthread_join(bus_thread, NULL); 


    sem_destroy(&stop_a.sem);
    sem_destroy(&stop_b.sem);
    sem_destroy(&print_sem);

    free(stop_a.stop);
    free(stop_b.stop);
    free(uni);
    free(student_threads);



    return 0;
}
