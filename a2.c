
#include<stdio.h>
#include<stdlib.h>
#include<string.h>
#include<stdbool.h>
#include <pthread.h>
#include <unistd.h>
#include <time.h>

typedef struct {
	int number;
	char direction;
	bool high_priority;
	float loading_time;
	float crossing_time;
	struct train* next;

} train;

train* hp_queue = NULL;
train* lp_queue = NULL;
pthread_mutex_t queues;
pthread_mutex_t track;
pthread_cond_t track_status;
pthread_cond_t nonempty_train_list;
pthread_cond_t start_loading;
int num_trains = 0;



void mutex_init()	{
	pthread_mutex_init(&queues, NULL);
	pthread_mutex_init(&track, NULL);
	pthread_cond_init(&track_status, NULL);
	pthread_cond_init(&nonempty_train_list, NULL);
	pthread_cond_init(&start_loading, NULL);
}


train* create_train(int num, char dir, bool h, float ld, float cross)	{
	train* new = (train*) malloc(sizeof(train));

	if(new == NULL)	{
		fprintf(stderr, "malloc failed in create_train\n" );
		exit(1);
	}

	new->number = num;
	new->direction = dir;
	new->high_priority = h;
	new->loading_time = ld;
	new->crossing_time = cross;
	new->next = NULL;

	return new;

}

bool isEmpty(train* head)	{
	return head == NULL;
}

void print_queue(train* head)	{
	train* curr = head;

	while(curr != NULL)	{
		printf("%d, %d, %c, %f, %f\n", curr->number, curr->high_priority, curr->direction, curr->loading_time, curr->crossing_time );
		curr = curr->next;
	}
}

train* remove_element(train* head, train* t)	{
	
	//case empty
	if(head == NULL)
		return NULL;

	//case multiple elements
	train* curr = head;
	train* prev = NULL;

	while(curr!=NULL)	{
		if(curr == t)	{

			if(prev !=NULL)	{			
				prev->next = curr->next;
				free(curr);
				return head;
			}

			else	{
				head = head->next;
				free(curr);
				return head;
			}
		}	

		prev = curr;
		curr = curr->next;
	}
	printf("element not found in remove.\n");
	return NULL;
}

train* insert_at_end(train* head, train* t)	{
	train* curr = head;
	//printf("inserting..\n");
	if(curr == NULL)	{
		//printf("head is null..\n");
		head = t;
		return head;
	}

	while(curr->next != NULL)	{
		curr = curr->next;
		//printf("while\n");
	}

	curr->next = t;

	return head;
}

void* load_train(void* param)	{

	train* t = (train*) param;

	pthread_mutex_lock(&queues);
	pthread_cond_wait(&start_loading, &queues);
	pthread_mutex_unlock(&queues);
	pthread_cond_broadcast(&start_loading);

	//printf("loading train:  %d\n", t->number );
	if(usleep(t->loading_time*100000) == -1)	{
		fprintf(stderr, "usleep failed in load_train().\n");
	}

	char* direction;

	if(t->direction == 'e')	
		direction = "East";
	else
		direction = "West";
	

	printf("Train %2d is ready to go %4s\n", t->number , direction );

	pthread_mutex_lock(&queues);

	//printf("inserting\n");

	if(t->high_priority)
		hp_queue = insert_at_end(hp_queue, create_train(t->number, t->direction, t->high_priority, t->loading_time, t->crossing_time));
	else
		lp_queue = insert_at_end(lp_queue, create_train(t->number, t->direction, t->high_priority, t->loading_time, t->crossing_time));

	//unlock mutex?
	pthread_cond_signal(&nonempty_train_list);
	pthread_mutex_unlock(&queues);

}

void* run_train(void* param)	{
	
	train* t = (train*) param;

	pthread_mutex_lock(&track);
	//pthread_cond_wait(&track_status, &track);

	char* direction;

	if(t->direction == 'e')	
		direction = "East";
	else
		direction = "West";

	printf("Train %2d is ON the main track going %4s\n", t->number, direction );

	if(usleep(t->crossing_time*100000) == -1)	{
		fprintf(stderr, "usleep failed in run_train().\n");
	}


	printf("Train %2d is OFF the main track after going %4s\n",t->number, direction );
	num_trains--;
	//pthread_cond_signal(&track_status);
	pthread_mutex_unlock(&track);

	if(num_trains==0)
		pthread_cond_signal(&nonempty_train_list);


}

train* file_handler(char* path)	{

	FILE *file = fopen(path, "r");

	if(file == NULL)	{
		fprintf(stderr, "fopen failed. Exiting.\n");
		exit(1);
	}

	char* line = NULL;
	size_t len = 0;

	int num = 0;
	char direction;
	float ld;
	float cross;
	bool prior = false;
	train* train_list = NULL;

	int flag = 1;


	while(fscanf(file, "%c %f %f\n", &direction, &ld, &cross) != EOF)	{
		//tokenize line

		//printf("%c %f %f\n",direction, ld, cross );
		prior = isupper(direction);
		train* new = create_train(num++, tolower(direction), prior, ld, cross);
		train_list = insert_at_end(train_list, new );

	}

	num_trains = num;

	free(line);
	return train_list;

}

int main(int argc, char* argv[])	{

	mutex_init();

	if(argc != 2)	{
		printf("Expected 2 argument.\n");
		exit(0);
	}

	train* t_list = file_handler(argv[1]);

	printf("train list:\n");
	print_queue(t_list);

	train* curr = t_list;
	
	while(curr != NULL)	{
		pthread_t id;
		pthread_create(&id, NULL, load_train, curr);
		//load_train(curr);
		curr = curr->next;
	}


	pthread_cond_broadcast(&start_loading);

	while(num_trains>0)	{

		pthread_mutex_lock(&queues);
		while(isEmpty(lp_queue) && isEmpty(hp_queue) && num_trains>0)	{
			pthread_cond_wait(&nonempty_train_list, &queues);
		}

		pthread_t id1, id2;
		if(!isEmpty(lp_queue))
		lp_queue = pthread_create(&id1, NULL, run_train, lp_queue);
		if(!isEmpty(hp_queue))
		hp_queue = pthread_create(&id1, NULL, run_train, hp_queue);

		pthread_mutex_unlock(&queues);


	}

	// usleep(2000000);

	printf("printing hp_queue\n");
	print_queue(hp_queue);
	printf("printing lp queue\n");
	print_queue(lp_queue);

	return 0;


}
