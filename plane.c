#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <time.h>
#include <string.h>
int pthread_sleep (int seconds)
{

   pthread_mutex_t mutex;
   pthread_cond_t conditionvar;
   struct timespec timetoexpire;
   if(pthread_mutex_init(&mutex,NULL))
    {
      return -1;
    }
   if(pthread_cond_init(&conditionvar,NULL))
    {
      return -1;
    }
   struct timeval tp;
   //When to expire is an absolute time, so get the current time and add //it to our delay time
   gettimeofday(&tp, NULL);
   timetoexpire.tv_sec = tp.tv_sec + seconds; timetoexpire.tv_nsec = tp.tv_usec * 1000;

   pthread_mutex_lock (&mutex);
   int res =  pthread_cond_timedwait(&conditionvar, &mutex, &timetoexpire);
   pthread_mutex_unlock (&mutex);
   pthread_mutex_destroy(&mutex);
   pthread_cond_destroy(&conditionvar);

   //Upon successful completion, a value of zero shall be returned
   return res;

}


typedef struct Plane{
  long id;
  time_t arrival_time;
  pthread_cond_t p_cond;
  pthread_mutex_t p_lock;
}Plane;
/*
Plane * createPlane(int iden){
  Plane* Pln = (Plane *)malloc(sizeof(Plane));
  Pln->id = iden;
  Pln->arrival_time = time(NULL);
  pthread_mutex_init(&(Pln->p_lock), NULL);
  pthread_cond_init(&(Pln->p_cond), NULL);
  return Pln;
}
*/

typedef struct Queue{
  Plane **elements;
  int size;
  int front;
  int rear;
  int capacity;
}Queue;

Queue * createQueue(int si){
  Queue *Q;
  Q = (Queue *)malloc(sizeof(Queue));
  Q->elements = malloc(sizeof(Plane) * si + 2);
  Q->capacity = si;
  Q->elements[Q->capacity];
  Q->size = 0;
  Q->front = 0;
  Q->rear = -1;
  return Q;
}

Plane pop(Queue *q)
{
        if (q->size == 0){
          printf("queue is empty\n");
          return;
        }
        else {
                q->size--;
                q->front++;
                if(q->front==q->capacity) q->front=0;
        }

        return;
}

Plane * front(Queue *q){
  if(q->size ==0){
    printf("Queue is empty\n");
    exit(0);
  }
  return q->elements[q->front];
}

void push(Queue *q, Plane *p){
  if(q->size == q->capacity){
    printf("Queue is full\n");
  }else{
    q->size++;
    q->rear = q->rear + 1;
    if(q->rear == q->capacity){
      q->rear=0;
    }
    q->elements[q->rear]=p;
  }
  return;
}

Queue *Landing_Queue;
Queue *TakeOff_Queue;
Queue *Emergency_Queue;
pthread_mutex_t land_lock;
pthread_mutex_t takeOff_lock;
pthread_mutex_t emergency_lock;


int simulation_time=40;
long PlaneID = 0;
double p = 0.5;
time_t start;
char info[300][200];


void * air_traffic_control(void * arg){
  while(time(NULL) < simulation_time + start){
    if(Emergency_Queue->size>0){
      Plane * p = front(Emergency_Queue);
      pthread_mutex_lock(&emergency_lock);
      pop(Emergency_Queue);
      pthread_mutex_unlock(&emergency_lock);
      pthread_cond_signal(&p->p_cond);
    }else{
      int isTakeOffPrior = 0;
      if(TakeOff_Queue->size > 0){
        if(TakeOff_Queue->size >= 3 || (time(NULL)-front(TakeOff_Queue)->arrival_time) >= 10){
          isTakeOffPrior=1;
        }
        if((time(NULL)-front(Landing_Queue)->arrival_time) >= 20){
          isTakeOffPrior=0;
        }
      }
      if(Landing_Queue->size>0 && !isTakeOffPrior){
        Plane * p = front(Landing_Queue);
        pthread_mutex_lock(&land_lock);
        pop(Landing_Queue);
        pthread_mutex_unlock(&land_lock);
        char finalInfo[20];
        double elapsedTime = time(NULL)-start;
        sprintf(finalInfo, " Runway Time:%0.f", elapsedTime);
        strcat(info[p->id],finalInfo);
        pthread_cond_signal(&p->p_cond);
      }else if(TakeOff_Queue->size>0){
        Plane * p = front(TakeOff_Queue);
        pthread_mutex_lock(&takeOff_lock);
        pop(TakeOff_Queue);
        pthread_mutex_unlock(&takeOff_lock);
        char finalInfo[20];
        double elapsedTime = time(NULL)-start;
        sprintf(finalInfo, " Runway Time:%0.f", elapsedTime);
        strcat(info[p->id],finalInfo);
        pthread_cond_signal(&p->p_cond);
      }
    }
    pthread_sleep(2);
  }


}

void * lander(void* pid){
  long PlaneID = (long)pid;
  Plane p;
  p.id=PlaneID;
  p.arrival_time = time(NULL);

  pthread_cond_init(&p.p_cond, NULL);
  pthread_mutex_init(&p.p_lock, NULL);
  if((time(NULL)-start)%40==0 && (time(NULL)-start)!=0){
    double elapsedTime = time(NULL)-start;
    sprintf(info[p.id], "PlaneID:%ld Status: E Request Time:%0.f", p.id, elapsedTime);
    printf("Emergency\n");
    pthread_mutex_lock(&emergency_lock);
    push(Emergency_Queue, &p);
    pthread_mutex_unlock(&emergency_lock);
  }else{
    double elapsedTime = time(NULL)-start;
    sprintf(info[p.id], "PlaneID:%ld Status: L Request Time:%0.f", p.id, elapsedTime);
    pthread_mutex_lock(&land_lock);
    push(Landing_Queue, &p);
    pthread_mutex_unlock(&land_lock);
}
  pthread_cond_wait(&p.p_cond, &p.p_lock);
  pthread_mutex_destroy(&p.p_lock);
  pthread_cond_destroy(&p.p_cond);
}
void * departer(void* pid){
  long PlaneID = (long)pid;
  Plane p;
  p.id=PlaneID;
  p.arrival_time = time(NULL);
  pthread_cond_init(&p.p_cond, NULL);
  pthread_mutex_init(&p.p_lock, NULL);
  double elapsedTime = time(NULL)-start;
  sprintf(info[p.id], "PlaneID:%ld Status: D Request Time:%0.f", p.id, elapsedTime);
  pthread_mutex_lock(&takeOff_lock);
  push(TakeOff_Queue, &p);
  pthread_mutex_unlock(&takeOff_lock);
  pthread_cond_wait(&p.p_cond, &p.p_lock);
  pthread_mutex_destroy(&p.p_lock);
  pthread_cond_destroy(&p.p_cond);
}
int main(int argc, char **argv){
  int i;
  for(i=0; i<argc; ++i)
  {
    if(strcmp(argv[i], "-s")==0){
      simulation_time = atoi(argv[i+1]);
    }else if(strcmp(argv[i], "-p")==0){
      p = atof(argv[i+1]);
    }
  }

  start = time(NULL);

  Emergency_Queue= createQueue(simulation_time);
  TakeOff_Queue= createQueue(simulation_time);
  Landing_Queue = createQueue(simulation_time);
  pthread_mutex_init(&land_lock, NULL);
  pthread_mutex_init(&takeOff_lock, NULL);
  pthread_mutex_init(&emergency_lock, NULL);
  pthread_t atc;
  pthread_create(&atc, NULL, air_traffic_control, NULL);
  pthread_t init_plane_departing;
  pthread_t init_plane_landing;
  pthread_create(&init_plane_landing, NULL, lander, (void *)PlaneID);
  PlaneID++;
  pthread_create(&init_plane_departing, NULL, departer, (void *)PlaneID);
  PlaneID++;

  while(time(NULL) < simulation_time + start){
    srand(time(NULL));
    double rand_value = (double)rand() / RAND_MAX;

    if (rand_value <= p){
      pthread_t plane;
      pthread_create(&plane, NULL, lander, (void *)PlaneID);
      PlaneID++;
    }else{
      pthread_t plane;
      pthread_create(&plane, NULL, departer, (void *)PlaneID);
      PlaneID++;
    }
    pthread_sleep(1);
  }
  pthread_mutex_destroy(&land_lock);
  pthread_mutex_destroy(&takeOff_lock);
  pthread_mutex_destroy(&emergency_lock);
  int y;
  for(y=0; y<300; y++){
    if(strcmp(info[y], "")!=0)
    printf("%s\n", info[y]);
  }
  FILE *fp;
	 char output[]="log.txt";
	 int n;
	 fp=fopen(output,"w");
	 for(n=0;n<300;n++) {
	 fprintf(fp,"%s\n",info[n]);
	 }
	 fclose(fp);
  return 0;

}
