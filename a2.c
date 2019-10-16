
#include<stdio.h>
#include<stdlib.h>
#include<string.h>
#include<stdbool.h>
#include <pthread.h>

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
	}

	curr->next = t;

	return head;
}

void* load_train(void* param)	{
	train* list = (train*) param;
	print_queue(list);

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

	free(line);
	return train_list;

}

int main(int argc, char* argv[])	{

	if(argc != 2)	{
		printf("Expected 2 argument.\n");
		exit(0);
	}

	train* t_list = file_handler(argv[1]);
	load_train(t_list);

	return 0;


}
