
#include<stdio.h>
#include<stdlib.h>
#include<string.h>
#include<stdbool.h>

typedef struct {
	int number;
	char direction;
	bool high_priority;
	float loading_time;
	float crossing_time;
	struct train* next;

} train;

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

void file_handler(char* path)	{

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
	train* head = NULL;

	int flag = 1;


	while(fscanf(file, "%c %f %f\n", &direction, &ld, &cross) != EOF)	{
		//tokenize line
		ld /= 10;
		cross /= 10;

		//printf("%c %f %f\n",direction, ld, cross );
		prior = isupper(direction);
		head = insert_at_end(head, create_train(num++, tolower(direction), prior, ld, cross));

	}

	print_queue(head);

	free(line);

}

int main(int argc, char* argv[])	{

	if(argc != 2)	{
		printf("Expected 2 argument.\n");
		exit(0);
	}

	file_handler(argv[1]);

	return 0;


}
