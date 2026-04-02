// tmtb2
// 2026-03-22
// spaghetti code

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
// #include <termios.h> // TODO

#define OPT_START 0
#define OPT_END 99

#define OPTARG_START 100
#define OPTARG_END 199

enum option_enum {
	AUTO,
	NOSAVE,
	DUMMY,
	QUIET,
	REMOVE,
	RM,
	DELETE,
	DEL,
	DESTROY,
	RENAME,
	STOP,
	PAUSE,
	START,
	UNPAUSE,
	RUN,
	EDIT,
	FIND,
	MOVE,
	TREE,
	FLIP,
	NEW,
	CREATE,
	ADD,
	RESET,
	SHOW,
	LIST,
	AND
};

enum misc {
	ARG = 200,
	NOP = 600,
	EMPTY = 999
};

enum optarg_enum {
	RECORD = 100,
	REC,
	R,
	ACTIVITY,
	ACTV,
	A,
	ALL
};

typedef struct {
	unsigned int show_full;
	unsigned int color;
	unsigned int how_many_rec_to_show;
	unsigned int show_rec_count;
	unsigned int show_streak;
	int time_display_mode;

	unsigned int write_to_db;
	// int last_selected_actv;
	char last_selected_actv[128];

	int tokens[128];
	unsigned int is_user_specified_arg;

	char *db_path;
	char *conf_path;
} options;

typedef struct {
	time_t time;
	time_t created;
	time_t ended;
} rec;

typedef struct {
	unsigned int is_run;
	time_t total;
	time_t timer;
	time_t created;
	int goal;
	int rec_count;
	int rec_size;
	char name[128];
	rec *rec_arr;
} actv;

typedef struct {
	int count;
	int size;
	actv *arr;
} actv_arr;

// char *help = "db file not found. run `tt help` for more info."

void dealloc_all (actv_arr *actv_arr) {
	// if (actv_arr->arr == NULL) return;

	if (actv_arr->arr != NULL) {
		for (int i = 0; i < actv_arr->count; i++) {
			if (actv_arr->arr[i].rec_arr != NULL) {
				free(actv_arr->arr[i].rec_arr);
			}
		}
		free(actv_arr->arr);
	}
}

void kys (char *message);

int is_in_range (int x, int a, int b)
{
	if (x >= a && x <= b) return 1;
	return 0;
}

// if eg. only 2 actv are called to be listed
// dont load the whole db to memory
void actv_arr_mem_resize (actv_arr *actv_arr, int add)
{
	if (actv_arr->arr == NULL) {
		actv_arr->arr = (actv*)malloc(sizeof(actv) * add);

		if (actv_arr->arr == NULL) return; // some error idk

		actv_arr->size += add;
	}

	while (actv_arr->count >= actv_arr->size) {
		actv *tmp = NULL;
		tmp = (actv*)realloc(actv_arr->arr,
	sizeof(actv) * 
	(actv_arr->size + add)
		);

		if(tmp == NULL) return; // some error
		actv_arr->size += add;
		actv_arr->arr = tmp;
	}
}

void rec_arr_mem_resize (actv *actv, int add)
{
	if (actv->rec_arr == NULL) {
		actv->rec_arr = (rec*)malloc(sizeof(rec) * add);

		if (actv->rec_arr == NULL) { puts("memory error"); return; } // some error idk

		actv->rec_size += add;
	}

	while (actv->rec_count >= actv->rec_size) {
		rec *tmp = NULL;
		tmp = (rec*)realloc(actv->rec_arr,
	sizeof(rec) * 
	(actv->rec_size + add)
		);

		if(tmp == NULL) { puts("memory error"); return; } // some error
		actv->rec_size += add;
		actv->rec_arr = tmp; // im a dumbass
	}
}

void actv_new (actv_arr *actv_arr, char *name)
{
	// make creating actv with name same as reserved words illegal
	actv_arr->count += 1;
	actv_arr_mem_resize(actv_arr, 2);

	int cur = actv_arr->count - 1;
	actv new = (*actv_arr).arr[cur];

	// *actv_arr->arr[cur] = (actv) {
	new = (actv) {
		.created = time(NULL),
		.total = 0,
		.is_run = 0,
		.rec_count = 0,
		.rec_size = 0,
		.rec_arr = NULL
	};

	strcpy(new.name, name);

	actv_arr->arr[cur] = new;
}

void rec_new (actv *actv)
{
	actv->rec_count += 1;
	rec_arr_mem_resize(actv, 30);
	int cur = actv->rec_count - 1;
	rec new = (*actv).rec_arr[cur];
	// rec new = {0, 0, 0}; // this bad

	new = (rec) {
		.created = time(NULL),
		.ended = 0,
		.time = 0
	};

	actv->rec_arr[cur].created = time(NULL);
	actv->rec_arr[cur].ended = 0;
	actv->rec_arr[cur].time = 0;

	actv->rec_arr[cur] = new; // this bad
}

void actv_rm (actv_arr *actv_arr, int index)
{
	if (actv_arr->arr == NULL) return;

	actv *tmp = actv_arr->arr;
	if (tmp[index].rec_arr != NULL) { free(tmp[index].rec_arr); }
	actv_arr->count--;

	for (; index < actv_arr->count; index++) {
		tmp[index] = tmp[index + 1];
	}

	actv_arr_mem_resize(actv_arr, -2);
}

void rec_rm (actv *actv, int index)
{
	if (actv->rec_arr == NULL) return;

	rec *tmp = actv->rec_arr;
	actv->rec_count--;

	for (; index < actv->rec_count; index++) {
		tmp[index] = tmp[index + 1];
	}

	rec_arr_mem_resize(actv, -20);
}

void make_time_cooler ();

void rec_timer (char mode, actv *actv)
{
	rec tmp;

	if (actv->rec_arr == NULL || mode == 'c') {
		if (mode == 'p' || mode == 'e') {
			printf("no records for \"%s\" exist. can't stop/edit non-existent timers.\n", actv->name);
			return;
		}
		// if (mode == 'c') break;
		if (mode == 'b' || mode == 'f') {
			printf("no records for \"%s\" exist yet. creating a new one.\n", actv->name);
			mode = 'c'; 
		}

	} else {
		tmp = actv->rec_arr[actv->rec_count - 1];
	}

	switch (mode) {
		case 'b': // begin
			if (actv->is_run) { printf("timer is already active.\n"); break; }
			actv->timer = time(NULL);
			actv->is_run = 1;
			printf("timer started for \"%s\". %lds\n", actv->name, tmp.time);
			break;
		case 'c': // create and begin
			if (actv->is_run) {
				printf("another timer is active! stop it first to create a new one.\n");
				break;
			}
			rec_new(actv);
			tmp = actv->rec_arr[actv->rec_count - 1];
			actv->timer = time(NULL);
			actv->is_run = 1;
			printf("timer created and started for \"%s\". %lds\n",actv->name, tmp.time);
			break;
		case 'p': // pause
			if (!actv->is_run) { printf("timer not active.\n"); break; }
			actv->is_run = 0;
			actv->total += (time(NULL) - actv->timer);
			tmp.time += (time(NULL) - actv->timer);
			tmp.ended = time(NULL);
			printf("timer stopped for \"%s\". %lds\n", actv->name, tmp.time);
			break;
		case 'f': // flip
			break;
		case 'e': // edit
			break;
	}

	actv->rec_arr[actv->rec_count - 1] = tmp;
}

void my_time_format (float *clock, char *sign, time_t time) 
{
	if (time > 86399) {
		*clock = ((float)time / 86400);
		*sign = 'd';
	} else if (time > 3599) {
		*clock = ((float)time / 3600);
		*sign = 'h';
	} else if (time > 59) {
		*clock = ((float)time / 60);
		*sign = 'm';
	} else {
		*clock = (float)time;
		*sign = 's';
	}
}

void actv_show (actv actv, options options)
{
	struct tm *tm;
	char date[32];
	char sign;
	float clock;

	printf("\n# %s\n", actv.name);
	// printf(" - %d record(s)\n", actv.rec_count);
	if (actv.rec_arr != NULL) { // print records
		for (int i = actv.rec_count - 1; i >= 0; i--) {
			tm = localtime(&actv.rec_arr[i].created);
			strftime(date, 15, "%Y-%m-%d", tm);
			if (i < actv.rec_count - 7) {
				printf("and %d more\n", actv.rec_count - 7);
				break;
			}

			my_time_format(&clock, &sign, actv.rec_arr[i].time);
			if (sign == 's')
				printf("%s | %1.f%c", date, clock, sign);
			else
				printf("%s | %.1f%c", date, clock, sign);

			if (actv.is_run && i == actv.rec_count - 1)
				printf(" active\n");
			else
				printf("\n");
		}
	}

	tm = gmtime(&actv.created);
	strftime(date, 32, "%Y-%m-%d at %H:%M", tm);
	printf("created: %s\n", date);

	my_time_format(&clock, &sign, actv.total);
	if (sign == 's')
		printf("total time: %1.f%c\n", clock, sign);
	else
		printf("total time: %.1f%c\n", clock, sign);
	// printf("streak: %s\n", "none yet"); // TODO
	// printf("highest streak: %s\n", "none yet"); // TODO
	// printf("week average: %s\n", "none yet"); // TODO
}

void db_read (FILE *db, actv_arr *actv_arr, options *options)
{
	if (db == NULL) return;

	// alloc memory
	actv_arr->count = 0;
	actv_arr->size = 0;
	actv_arr_mem_resize(actv_arr, 2);

	// helper pointers
	int *ac = &actv_arr->count;
	int *rc = NULL;
	actv *tmp_a = NULL;
	rec *tmp_r = NULL;

	int sign = 0;
	while (sign != EOF) {
		sign = fgetc(db);
		switch (sign) {
			case 'H':
				fscanf(db, ",%[^,],\n",
					&options->last_selected_actv
				);
				break;
			case 'A':
				*ac += 1;
				actv_arr_mem_resize(actv_arr, 2);
				tmp_a = actv_arr->arr;

				fscanf(db, ",%[^,],%d,%ld,%ld,%ld,%d,\n",
					 tmp_a[*ac - 1].name,
					&tmp_a[*ac - 1].is_run,
					&tmp_a[*ac - 1].timer,
					&tmp_a[*ac - 1].created,
					&tmp_a[*ac - 1].total,
					&tmp_a[*ac - 1].goal
				);
				tmp_a[*ac - 1].rec_size = 0;
				tmp_a[*ac - 1].rec_count = 0;
				tmp_a[*ac - 1].rec_arr = NULL;
				rc = &tmp_a[*ac - 1].rec_count;
				break;
			case 'R':
				*rc += 1;
				rec_arr_mem_resize(&actv_arr->arr[*ac - 1], 20);
				tmp_r = tmp_a[*ac - 1].rec_arr;

				fscanf(db, ",%ld,%ld,%ld,\n",
					&tmp_r[*rc - 1].time,
					&tmp_r[*rc - 1].created,
					&tmp_r[*rc - 1].ended
				);
				break;
		}
	}
}

void db_write (FILE *db, actv_arr *actv_arr, options options) 
{
	if (actv_arr->count == 0) return;

	fprintf(db, "H,%s,\n", options.last_selected_actv); // header

	for (int i = 0; i < actv_arr->count; i++) {
		fprintf(db, "A,%s,%d,%ld,%ld,%ld,%d,\n", 
			actv_arr->arr[i].name,
			actv_arr->arr[i].is_run,
			actv_arr->arr[i].timer,
			actv_arr->arr[i].created,
			actv_arr->arr[i].total,
			actv_arr->arr[i].goal
		);
		if (actv_arr->arr[i].rec_count != 0) {
			// for (int j = actv_arr->arr[i].rec_count - 1; j >= 0; j--) {
			for (int j = 0; j < actv_arr->arr[i].rec_count; j++) {
				// printf("ACTV WRITTEN\n");
				fprintf(db, "R,%ld,%ld,%ld,\n", 
					actv_arr->arr[i].rec_arr[j].time,
					actv_arr->arr[i].rec_arr[j].created,
					actv_arr->arr[i].rec_arr[j].ended
				);
			}
		}
	}

	// dealloc_all(actv_arr);
	// dealloc
}

void completion (char *token, char **target_list);

int actv_find (actv_arr *actv_arr, char *query)
{
	if (actv_arr->arr == NULL) { return -1; }

	for (int i = 0; i < actv_arr->count; i++) {
		if (!strncmp(query, actv_arr->arr[i].name, 128))
			return i;
	}

	return -1;
}

void set_options (int argc, char **argv, options *options)
{
	if (argc > 127) { puts("too many args!"); return; }

	char option_list[][10] = {
		"auto",
		"nosave", // don't write
		"dummy", // same as nosave
		"quiet", // no echo
		"remove",
		"rm", // same as remove
		"delete", // same as remove
		"del", // same as remove
		"destroy", // same as remove
		"rename", // rename actv.
		"stop", // stop timer
		"pause", // same as stop
		"start", // start timer
		"unpause", // same as start
		"run", // same as start
		"edit", // edit time for record
		"find", // search for record by date
		"move", // move actv. up or down in array
		"tree", // show in tree mode
		"flip", // flip timer
		"new",
		"create",
		"add",
		"reset", // reset timer
		"show",
		"list",
		"and"
	};

	char optarg_list[][10] = {
		"record",
		"rec", // same as record
		"r",
		"activity",
		"actv", // same as activity
		"a",
		"all"
	};

	int len = sizeof(option_list) / sizeof(option_list[0]);
	int optarg_len = sizeof(optarg_list) / sizeof(optarg_list[0]);

	for (int i = 1; i < argc; i++) { // get options into array
 		for (int j = 0; j < len; j++) {

			if (!strncmp(argv[i], option_list[j], 10)) {
				options->tokens[i] = j;
				break;
			}


			if (j == len - 1) { // get args into array
				options->tokens[i] = ARG;
			}
		}

		if (options->tokens[i] == ARG) { // little optimization
			for (int j = 0; j < optarg_len; j++) {
				if (!strncmp(argv[i], optarg_list[j], 10)) {
					options->tokens[i] = j + OPTARG_START;
					break;
				}
			}
		}
	}
}

void ui (int argc, char **argv, actv_arr *actv_arr, options *options)
{
	unsigned int opt_arg = EMPTY;
	unsigned int option = EMPTY;
	int arg_start = EMPTY;
	int arg_end = EMPTY;
	int bl = 0; // block_len 

	for (int i = 0; i < argc; i++) {

		opt_arg = EMPTY;
		option = EMPTY;
		arg_start = EMPTY;
		arg_end = EMPTY;
		bl = 0; // block_len 

		while (i < argc) {
			// if (options->tokens[i] >= OPTARG_START && options->tokens[i] <= OPTARG_END) // check if opt_arg
			if (is_in_range(options->tokens[i], OPTARG_START, OPTARG_END)) { // check if opt_arg
				if (opt_arg != EMPTY) { puts("only one option argument is allowed!"); return; } // TODO error
				opt_arg = i;
			}
			if (options->tokens[i] == ARG) { // argument queue 
				if (arg_start != EMPTY) { puts("arguments (probably activity names) specified twice!"); return; } // TODO error
				arg_start = i;
				arg_end = i;
				while (options->tokens[arg_end + 1] == ARG) { 
					bl++;
					i++;  
					arg_end++;
				}
				break; // end sentence on last arg
			}
			// if (options->tokens[i] >= OPT_START && options->tokens[i] <= OPT_END) { // check if opt
			if (is_in_range(options->tokens[i], OPT_START, OPT_END)) { // check if opt
				if (option != EMPTY) { puts("option error!"); return; } // TODO error
				option = i; 
			}

			bl++;

			if (options->tokens[i + 1] == EMPTY) { // exit if next argv is blank
				if (option == EMPTY) { 
					option = i;
					options->tokens[i] = NOP;
					break;
				} // error
				break;
			}

			if (option != EMPTY) { // exit if next argv is another option
				if (is_in_range(options->tokens[i + 1], OPT_START, OPT_END)) { 
					printf("is in range of opt: %d\n", is_in_range(options->tokens[i+1], OPT_START, OPT_END));
					printf("argv: %s\n", argv[i+1]);
					printf("token: %d\n", options->tokens[i + 1]);
					break;
				}
			}

			i++;
		}

		if (strncmp(options->last_selected_actv, "none", 128)) { 
			if (arg_start == EMPTY) {

			}
		}
		 
		switch (options->tokens[option]) {
			case NEW:
			case ADD:
				if (opt_arg == EMPTY) { puts("you need to specify type that you want to create. (activity or record)"); break; }
				if (arg_start == EMPTY) { puts("no activity or session was specified. try `tt new [type] [name]`"); break; }
				switch (options->tokens[opt_arg]) {
					case ACTIVITY:
					case ACTV:
						for (int j = arg_start; j <= arg_end; j++) {
							actv_new(actv_arr, argv[j]);
							printf("activity \"%s\" created.\n", argv[j]);
						}
						break;

					case RECORD:
					case REC:
						for (int j = arg_start; j <= arg_end; j++) {
							int tmp = actv_find(actv_arr, argv[j]);
							if (tmp != -1) {
								if (actv_arr->arr[tmp].is_run) {
									printf("stop the timer first.\n");
									break; 
								}
								rec_new(&actv_arr->arr[tmp]);
								printf("new record created.\n");
							} else { printf("activity \"%s\" wasn't found.\n", argv[j]); }
						}
						break;

					default: 
						puts("incorrect option argument! try `activity` or `record`");
						break;
				}
				break;
			case REMOVE:
			case DELETE:
			case DEL:
			case RM:
				if (opt_arg == EMPTY) { puts("you need to specify type that you want to delete. (activity or record)"); break; }
				if (arg_start == EMPTY) { puts("no activity or record was specified. try `tt del [type] [name]`"); break; }
				switch (options->tokens[opt_arg]) {
					case ACTIVITY:
					case ACTV:
						for (int j = arg_start; j <= arg_end; j++) {
							int tmp = actv_find(actv_arr, argv[j]);
							if (tmp != -1) {
								actv_rm(actv_arr, tmp);
								printf("actv \"%s\" deleted.\n", argv[j]);
							} else { printf("activity \"%s\" wasn't found.\n", argv[j]); }
						}
						break;
					case RECORD:
					case REC:
						// for (int j = arg_start; j <= arg_end; j++) {
						// 	int tmp = actv_find(actv_arr, argv[j]);
						// 	if (tmp != -1) {
						// 		rec_rm(&actv_arr->arr[tmp], 0);
						// 		printf("record deleted.\n", argv[j]);
						// 	} else { printf("activity \"%s\" wasn't found.\n", argv[j]); }
						// }
						break;
					default: 
						puts("incorrect option argument! try `activity` or `record`");
						break;
				}
				break;
			case EDIT:
				puts("tbd"); // TODO
				break;
			case START:
				// printf("%s\n", options->last_selected_actv); // debug
				if (strcmp(options->last_selected_actv, "none")) { 
					if (arg_start == EMPTY) {
						int tmp = actv_find(actv_arr, options->last_selected_actv);
						rec_timer('b', &actv_arr->arr[tmp]);
						break;
					}
				}

				for (int j = arg_start; j <= arg_end; j++) {
					int tmp = actv_find(actv_arr, argv[j]);
					if (tmp != -1) { rec_timer('b', &actv_arr->arr[tmp]); }
					else { printf("activity \"%s\" wasn't found.\n", argv[j]); }
				}
				break;
			case STOP:
			case PAUSE:
				if (strcmp(options->last_selected_actv, "none")) { 
					if (arg_start == EMPTY) {
						int tmp = actv_find(actv_arr, options->last_selected_actv);
						rec_timer('p', &actv_arr->arr[tmp]);
						break;
					}
				}

				for (int j = arg_start; j <= arg_end; j++) {
					int tmp = actv_find(actv_arr, argv[j]);
					if (tmp != -1) { rec_timer('p', &actv_arr->arr[tmp]); }
					else { printf("activity \"%s\" wasn't found.\n", argv[j]); }
				}
				break;
			case FLIP:
				if (strcmp(options->last_selected_actv, "none")) { 
					if (arg_start == EMPTY) {
						int tmp = actv_find(actv_arr, options->last_selected_actv);
						if (actv_arr->arr[tmp].is_run)
							rec_timer('p', &actv_arr->arr[tmp]);
						else
							rec_timer('b', &actv_arr->arr[tmp]);
						break;
					}
				}

				for (int j = arg_start; j <= arg_end; j++) {
					int tmp = actv_find(actv_arr, argv[j]);
					if (tmp != -1) { 
						if (actv_arr->arr[tmp].is_run)
							rec_timer('p', &actv_arr->arr[tmp]); 
						else
							rec_timer('b', &actv_arr->arr[tmp]); 
					}
					else { printf("activity \"%s\" wasn't found.\n", argv[j]); }
				}
				break;
			case SHOW:
				if (arg_start != EMPTY) {
					for (int j = arg_start; j <= arg_end; j++) {
						int tmp = actv_find(actv_arr, argv[j]);

						if (tmp != -1) { actv_show(actv_arr->arr[tmp], *options); }
						else { printf("activity \"%s\" wasn't found.\n", argv[j]); }
					}
				}

				if (opt_arg != EMPTY) {
					switch (options->tokens[opt_arg]) {
						case EMPTY:
						break;
						case ALL:
							for (int j = 0; j < actv_arr->count; j++) {
								actv_show(actv_arr->arr[j], *options);
							}
						break;
						// case FULL:
						// break;
						default:
							printf("incorrect option argument! try `all`\n");
						break;
					}
				}

				break;
			case NOP:
				if (arg_start == EMPTY) {
					printf("no activity is selected. here are available choices:");
					printf("there are no available choices.\ncreate an activity with: `tt new activity [name]`\nit will remain selected on next invoke.\n");
				} else {
				}
				break;
			default:
				printf("i don't know what to do with this option: \"%s\" sorry!! :(\n", argv[i]);
				break;
		}
	}

	// search for last specified option and keep it as default for next operation // TODO
	if (arg_end != EMPTY) {
		strncpy(options->last_selected_actv, argv[arg_end], 128);
	}
}


void edit_actv ();
void get_config_file ();

int main (int argc, char **argv)
{
 	FILE *db;
	actv_arr actv_arr = {0, 0, NULL};
	char *conf_path;
	options options;
	for (int i = 0; i < 128; i++) {
		options.tokens[i] = EMPTY;
	}
	options.is_user_specified_arg = 0;
	strncpy(options.last_selected_actv, "none", 128);

	// get config
	 
	set_options(argc, argv, &options);

	// open and read db
	db = fopen("./db/test.db", "r");
	db_read(db, &actv_arr, &options);
	fclose(db);

	// create a backup

	ui(argc, argv, &actv_arr, &options);

	// write and close db
	db = fopen("./db/test.db", "w");
	db_write(db, &actv_arr, options);
	fclose(db);

	// get config path
	// if no config then set default
	// passing arguments will temporarly modify the config struct

	// printf("\nworks so far! count: %d size: %d \n", actv_arr.count, actv_arr.size); // debug
	// printf("command count: %d\n", argc - 1); // debug
	// printf("last selected actv: %s\n\n", options.last_selected_actv);

	// if (actv_arr.arr != NULL) {
	// 	for (int i = 0; i < actv_arr.count; i++) {
	// 		if (actv_arr.arr[i].rec_arr != NULL) {
	// 			free(actv_arr.arr[i].rec_arr);
	// 		}
	// 	}
	// 	free(actv_arr.arr);
	// }

	// check if db exists
	// parse args/decide how much memory to alloc
	// read and load db to memory
	// flag if any change was made
	// if yes then write to db and dealloc

	dealloc_all(&actv_arr);

	return 0;
}
