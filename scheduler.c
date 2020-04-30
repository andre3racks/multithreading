
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

struct timespec start, stop; 

#define BILLION 1000000000.0; 

train* hp_queue = NULL;
train* lp_queue = NULL;
pthread_mutex_t queues;
pthread_mutex_t track;
pthread_cond_t track_status;
pthread_cond_t train_selection;
pthread_cond_t start_loading;
pthread_cond_t done_with_Qs;
int num_trains = 0;
bool queues_in_use = false;
bool on_track = false;


void mutex_init()	{
	pthread_mutex_init(&queues, NULL);
	pthread_mutex_init(&track, NULL);
	pthread_cond_init(&track_status, NULL);
	pthread_cond_init(&train_selection, NULL);
	pthread_cond_init(&start_loading, NULL);
	pthread_cond_init(&done_with_Qs, NULL);
}

void print_time()	{

	if( clock_gettime( CLOCK_REALTIME, &stop) == -1 ) {      
		perror( "clock gettime" );      
		exit( EXIT_FAILURE );    
	} 

	double total_seconds = ( stop.tv_sec - start.tv_sec ) + ( stop.tv_nsec - start.tv_nsec )/ BILLION;
	int hours = 0;
	int minutes = 0;

	while(total_seconds>=3600)	{
		hours++;
		total_seconds-=3600;
	}	

	while(total_seconds>60)	{
		minutes++;
		total_seconds-=60;
	}

	printf("%02d:%02d:%04.1f ", hours, minutes, total_seconds);

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
	//utility function for queues
	return head == NULL;
}

void print_queue(train* head)	{
	//utility function for queues
	train* curr = head;

	while(curr != NULL)	{
		printf("%d, %d, %c, %f, %f\n", curr->number, curr->high_priority, curr->direction, curr->loading_time, curr->crossing_time );
		curr = curr->next;
	}
}

train* remove_element(train* head, train* t)	{
	//utility function for queues
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
				//free(curr);
				return head;
			}

			else	{
				head = head->next;
				//free(curr);
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
	//utility function for queues
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

	//wait for loading broadcast, then release mutex and load train

	train* t = (train*) param;

	if(pthread_mutex_lock(&queues))	{
		fprintf(stderr, "mutex_lock failed in load_train.\n" );
		exit(1);
	}

	if(pthread_cond_wait(&start_loading, &queues) != 0)	{
		fprintf(stderr, "cond_wait failed in load_train\n" );
		exit(1);
	}
	
	if(pthread_mutex_unlock(&queues))	{
		fprintf(stderr, "mutex-unlock failed in load_train\n" );
		exit(1);
	}
	
	if(pthread_cond_broadcast(&start_loading))	{
		fprintf(stderr, "cond_wait failed in load_train\n" );
		exit(1);
	}

	//printf("loading train:  %d\n", t->number );
	if(usleep(t->loading_time*100000) == -1)	{
		fprintf(stderr, "usleep failed in load_train().\n");
		exit(1);
	}

	char* direction;

	if(t->direction == 'e')	
		direction = "East";
	else
		direction = "West";

	//reacquire mutex to add train to queue

	if(pthread_mutex_lock(&queues))	{
		fprintf(stderr, "mutex_lock failed in load_train.\n" );
		exit(1);
	}
	
	print_time();
	printf("Train %2d is ready to go %4s\n", t->number , direction );

	
	
	while(queues_in_use)
		if(pthread_cond_wait(&done_with_Qs, &queues) != 0)	{
			fprintf(stderr, "cond_wait failed in load_train\n" );
			exit(1);
		}
	//printf("inserting\n");
	queues_in_use = true;

	if(t->high_priority)
		hp_queue = insert_at_end(hp_queue, create_train(t->number, t->direction, t->high_priority, t->loading_time, t->crossing_time));
	else
		lp_queue = insert_at_end(lp_queue, create_train(t->number, t->direction, t->high_priority, t->loading_time, t->crossing_time));

	//unlock mutex?
	queues_in_use = false;
	pthread_cond_signal(&done_with_Qs);

	if(!on_track)
		pthread_cond_signal(&train_selection);

	if(pthread_mutex_unlock(&queues))	{
		fprintf(stderr, "mutex-unlock failed in load_train\n" );
		exit(1);
	}

}

void* run_train(void* param)	{

	//acquire mutex, no need to wait, due to the synchronization in main

	//run train across track
	
	if(pthread_mutex_lock(&track))	{
		fprintf(stderr, "mutex_lock failed in run_train.\n" );
		exit(1);
	}

	train* t = (train*) param;

	on_track = true;

	char* direction;

	if(t->direction == 'e')	
		direction = "East";
	else
		direction = "West";

	print_time();
	printf("Train %2d is ON the main track going %4s\n", t->number, direction );

	if(usleep(t->crossing_time*100000) == -1)	{
		fprintf(stderr, "usleep failed in run_train().\n");
	}

	print_time();
	printf("Train %2d is OFF the main track after going %4s\n",t->number, direction );
	num_trains--;

	//free(t);

	on_track = false;

	if(pthread_mutex_unlock(&track))	{
		fprintf(stderr, "mutex_unlock failed in run_train.\n" );
		exit(1);
	}

	if(!isEmpty(lp_queue) || !isEmpty(hp_queue) || num_trains == 0)
		pthread_cond_signal(&train_selection);



}

train* file_handler(char* path)	{

	//create trains from file for loading

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

train* find_next(train* head, char last_train_direction)	{
	//logic to find the next train if not in starvation 

	if(isEmpty(head))
		return NULL;

	int least_L_time_e = 2147483647;
	int least_L_time_w = 2147483647;
	int lowest_train_num_e = 2147483647;
	int lowest_train_num_w = 2147483647;
	train* curr = head;
	train* eastbound = NULL;
	train* westbound = NULL;

	while(curr != NULL)	{

		if(curr->direction == 'e')	{
			if(curr->loading_time <= least_L_time_e && curr->number < lowest_train_num_e)	{
				lowest_train_num_e = curr->number;
				least_L_time_e = curr->loading_time;
				eastbound = curr;
			}
		}

		else if(curr->direction == 'w')	{
			if(curr->loading_time <= least_L_time_w && curr->number < lowest_train_num_w)	{
				lowest_train_num_w = curr->number;
				least_L_time_w = curr->loading_time;
				westbound = curr;
			}
		}

		else 	{
			fprintf(stderr, "unexpected direction in find_next()\n" );
		}

		curr = curr->next; 
	
	}

	// no we have highest priority trains for each direction

	//if there loading times are the same, we send the opposite direction as last train
	//otherwise, send the least loading time;

	if(eastbound != NULL && westbound != NULL)	{

		if(eastbound->loading_time == westbound->loading_time)	{
			if(last_train_direction == 'e')
				return westbound;
			else if(last_train_direction == 'w')
				return eastbound;
			else
				return eastbound;
		}

		if(eastbound->loading_time<westbound->loading_time)
			return eastbound;
		else
			return westbound;
	}

	if(eastbound == NULL)
			return westbound;
	else
		return eastbound;

	fprintf(stderr, "No train found in nonempty list, find_next().\n");
	return NULL;
	

}

train* starvation(train* hp, train* lp, char last_train_direction)	{
	//logic to find next train if starved

	if(isEmpty(hp) && isEmpty(lp))
		return NULL;

	int least_L_time = 2147483647;
	int lowest_train_num = 2147483647;
	train* curr = hp;
	train* next = NULL;

	char direction;

	if(last_train_direction == 'e')
		direction = 'w';
	else
		direction = 'e';

	//HP iteration
	while(curr != NULL)	{

		if(curr->direction == direction)	{
			if(curr->loading_time <= least_L_time && curr->number < lowest_train_num)	{
				least_L_time = curr->loading_time;
				lowest_train_num = curr->number;
				next = curr;
			}
		}

		curr = curr->next;
	}

	if(next != NULL)
		return next;

	curr = lp;
	//LP iteration
	while(curr != NULL)	{

		if(curr->direction == direction)	{
			if(curr->loading_time <= least_L_time && curr->number < lowest_train_num)	{
				least_L_time = curr->loading_time;
				lowest_train_num = curr->number;
				next = curr;
			}
		}
		curr = curr->next;
	}

	if(next != NULL)
		return next;

	next = find_next(hp, last_train_direction);

	if(next != NULL)
		return next;

	next = find_next(lp, last_train_direction);

	if(next != NULL)
		return next;

	fprintf(stderr, "starvation failed to find any trains.\n" );

	return NULL;

}

int main(int argc, char* argv[])	{

	mutex_init();

	if(argc != 2)	{
		printf("Expected 2 argument.\n");
		exit(0);
	}

	//create list of trains
	train* t_list = file_handler(argv[1]);
	train* curr = t_list;

	//call load train for each train	
	while(curr != NULL)	{
		pthread_t id;
		if(pthread_create(&id, NULL, load_train, curr))	{
			fprintf(stderr, "create failed in main\n" );
			exit(1);
		}
		//load_train(curr);
		curr = curr->next;
	}

	//wait to ensure all trains have been parsed
	usleep(2000000);

	//broadcast to start loading
	if(pthread_cond_broadcast(&start_loading))	{
		fprintf(stderr, "broadcast failed in main\n" );
		exit(1);
	}

	//start time
	if( clock_gettime( CLOCK_REALTIME, &start) == -1 ) {      
		perror( "clock gettime" );      
		exit( EXIT_FAILURE );    
	}

	//initial vars for next train and starvation logic
	int starv = 0;
	char last_train_direction = "n";
	train* next_train = NULL;
	train* prev_train = NULL;

	pthread_t id1;

	//while not all trains have crossed

	while(num_trains>0)	{

		if(pthread_mutex_lock(&queues))	{
			fprintf(stderr, "mutex_lock failed in main.\n" );
			exit(1);
		}

		//wait for non-empty queues
		while((isEmpty(lp_queue) && isEmpty(hp_queue)))	{
			if(pthread_cond_wait(&train_selection, &queues))	{
				fprintf(stderr, "cond_wait failed in main.\n" );
				exit(1);
			}
		}

		if(num_trains == 0)
			break;

		if(next_train != NULL)
			last_train_direction = next_train->direction;

		prev_train = next_train;
		next_train = NULL;

		//if starved

		if(starv >= 2)	{
			//printf("starvation called !\n");
			next_train = starvation(hp_queue, lp_queue, last_train_direction);

			if(next_train->direction != last_train_direction)
				starv = 0;
			else
				starv++;

			if(prev_train != NULL)
				free(prev_train);

			if(next_train->high_priority)
				hp_queue = remove_element(hp_queue, next_train);
			else
				lp_queue = remove_element(lp_queue, next_train);

			if(pthread_create(&id1, NULL, run_train, next_train))	{
				fprintf(stderr, "create failed in main\n" );
				exit(1);
			}
			
			if(pthread_mutex_unlock(&queues))	{
				fprintf(stderr, "mutex-unlock failed in main\n" );
				exit(1);
			}
			
			if(pthread_join(id1, NULL))	{
				fprintf(stderr, "join failed in main\n" );
				exit(1);
			}

			continue;
		}

		//hp check
		if(!isEmpty(hp_queue))	{
			next_train = find_next(hp_queue, last_train_direction);
			//remove train?
			if(next_train != NULL)	{
				hp_queue = remove_element(hp_queue, next_train);
				if(next_train->direction == last_train_direction)
					starv++;
				else
					starv = 0;

				if(prev_train != NULL)
					free(prev_train);
			}

		}
		//if no hp, lp check
		//lp
		else if(!isEmpty(lp_queue))	{
			next_train = find_next(lp_queue, last_train_direction);
			//remove train?
			if(next_train != NULL)	{
				lp_queue = remove_element(lp_queue, next_train);
				if(next_train->direction == last_train_direction)
					starv++;
				else
					starv = 0;

				if(prev_train != NULL)
					free(prev_train);
			}
		}

		if(next_train == NULL)	{
			fprintf(stderr, "next_train is NULL in main, exiting.\n" );
			return 1;
		}

		// printf("train selected is: %d\n", next_train->number );

		// printf("starvation is: %d\n", starv);

		// printf("last train: %c\n", last_train_direction);

		// printf("next train: %c\n", next_train->direction);

		if(pthread_create(&id1, NULL, run_train, next_train))	{
			fprintf(stderr, "create failed in main\n" );
			exit(1);
		}

		if(pthread_mutex_unlock(&queues))	{
			fprintf(stderr, "mutex-unlock failed in main\n" );
			exit(1);
		}

		if(pthread_join(id1, NULL))	{
			fprintf(stderr, "join failed in main\n" );
			exit(1);
		}
	}


	// printf("printing hp_queue\n");
	// print_queue(hp_queue);
	// printf("printing lp queue\n");
	// print_queue(lp_queue);

	return 0;


}
