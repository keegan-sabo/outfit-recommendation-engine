#define _GNU_SOURCE
#include <stdio.h>	// asprintf() printf() fprintf()
#include <time.h>	// time()
#include <stdlib.h> // atoi() system() srand() rand()
#include <string.h> // strstr()
#include <sqlite3.h>
#include "weather.h"

#define MAX_CHARS 100

/* an implementation of binary search called by perform_query */
int binary_search(int *numbers, int size, int target)
{
	int k = 0;
	for (int b = size / 2; b >= 1; b /= 2)
		while (k + b < size && numbers[k + b] <= target)
			k += b;
	return k;
}

/* choose an article of clothing given a category set captured in sequence 
and a random number, pseudo */
int perform_query(char name[MAX_CHARS], char *sequence, int temp, int r_pseudo)
{
	sqlite3 *db;
	int rc;
	rc = sqlite3_open_v2("items.db", &db, SQLITE_OPEN_READONLY, NULL);
	if (rc) 
	{
		fprintf(stderr, "Cannot open database: %s\n", sqlite3_errmsg(db));
		return 0;
	}

	int total = 0; // total number of matching items
	int row;       // row number to select

	sqlite3_stmt *stmt;
	char *query_number = sqlite3_mprintf("SELECT COUNT(*) "
										 "FROM clothing "
										 "WHERE layer_value %s "
										 "AND coldest_t <= %d AND %d <= warmest_t;",
										 sequence, temp, temp);
	rc = sqlite3_prepare_v2(db, query_number, -1, &stmt, NULL);
	if (rc != SQLITE_OK) // confirm formatted
	{
		printf("error: %s\n", sqlite3_errmsg(db));
		return 0;
	}
	rc = sqlite3_step(stmt);
	if (rc != SQLITE_ROW && rc != SQLITE_DONE)
	{
		printf("error:%s \n", sqlite3_errmsg(db));
		sqlite3_finalize(stmt);
	}
	if (rc == SQLITE_ROW)
	{
		total = sqlite3_column_int(stmt, 0);
	}
	if (total == 0)
	{
		printf("No items match the weather\n");
		return 1;
	}
	// printf("total number is %d\n", total);
	sqlite3_reset(stmt);

	/* compute a distribution from which to choose a 
	clothing item */
	int weighted_array[total];
	int i;
	weighted_array[0] = 1;
	for (i = 0; i < total; i++)
		weighted_array[i] = ((i + 1) * 2) + weighted_array[i-1];

	r_pseudo %= weighted_array[total - 1];
	row = binary_search(weighted_array, total, r_pseudo);
	
	/* 
	printf("%d\n", r_pseudo);
	for (int i = 0; i < total; i++)
		printf("%d ", weighted_array[i]);
	printf("\n"); 
	printf("The row is %d\n", row); 
	*/

	char *choose = sqlite3_mprintf("SELECT name "
								   "FROM clothing "
								   "WHERE layer_value %s "
								   "AND coldest_t <= %d AND %d <= warmest_t "
								   "ORDER BY days_passed ASC, name ASC;",
								   sequence, temp, temp);
	// printf("%s", choose);
	rc = sqlite3_prepare_v2(db, choose, -1, &stmt, NULL);
	if (rc != SQLITE_OK)
	{
		printf("prepare error: %s\n", sqlite3_errmsg(db));
		return 0;
	}
	for (i = 0; i <= row; i++) // row may be 0
		rc = sqlite3_step(stmt);
	if (rc != SQLITE_ROW && rc != SQLITE_DONE)
	{
		printf("step error:%s \n", sqlite3_errmsg(db));
		sqlite3_finalize(stmt);
	}
	if (rc == SQLITE_ROW)
	{
		strcpy(name, sqlite3_column_text(stmt, 0));
	}
	sqlite3_finalize(stmt);
	sqlite3_close(db);
	return 0;
}

/* get the layer_value field of an entry given its name */
int get_layer(char *name)
{
	int value = -1;
	int rc;
	sqlite3 *db;
	rc = sqlite3_open_v2("items.db", &db, SQLITE_OPEN_READONLY, NULL);
	if (rc)
	{
		fprintf(stderr, "Cannot open database: %s\n", sqlite3_errmsg(db));
		return 0;
	}
	sqlite3_stmt *stmt;
	char *query_layer = sqlite3_mprintf("SELECT layer_value "
										"FROM clothing "
										"WHERE name = '%s';",
										name);
	rc = sqlite3_prepare_v2(db, query_layer, -1, &stmt, NULL);
	if (rc != SQLITE_OK)
	{
		printf("error: %s\n", sqlite3_errmsg(db));
		return 0;
	}
	rc = sqlite3_step(stmt);
	if (rc != SQLITE_ROW && rc != SQLITE_DONE)
	{
		printf("error:%s \n", sqlite3_errmsg(db));
		sqlite3_finalize(stmt);
	}
	if (rc == SQLITE_ROW)
	{
		value = sqlite3_column_int(stmt, 0);
	}
	sqlite3_finalize(stmt);
	sqlite3_close(db);
	return value;
}

/* increment the field days_passed of all entries. Called after the user
confirms a selection of clothing */
int update_all()
{
	sqlite3 *db;
	int rc;
	rc = sqlite3_open_v2("items.db", &db, SQLITE_OPEN_READWRITE, NULL);
	if (rc)
	{
		fprintf(stderr, "Cannot open database: %s\n", sqlite3_errmsg(db));
		return 0;
	}
	sqlite3_stmt *stmt;
	char *update = "UPDATE clothing "
				   "SET days_passed = days_passed + 1;";

	rc = sqlite3_prepare_v2(db, update, -1, &stmt, NULL);
	if (rc != SQLITE_OK)
	{
		printf("error: %s\n", sqlite3_errmsg(db));
		return 0;
	}
	rc = sqlite3_step(stmt);
	if (rc != SQLITE_ROW && rc != SQLITE_DONE)
	{
		printf("error:%s \n", sqlite3_errmsg(db));
	}
	sqlite3_finalize(stmt);
	sqlite3_close(db);
	return 0;
}

/* set the days_passed field to 0 for any chosen item of
clothing */
int update_chosen(char *name)
{
	sqlite3 *db;
	int rc;
	rc = sqlite3_open_v2("items.db", &db, SQLITE_OPEN_READWRITE, NULL);
	if (rc)
	{
		fprintf(stderr, "Cannot open database: %s\n", sqlite3_errmsg(db));
		return 0;
	}
	sqlite3_stmt *stmt;
	char *update_choice = sqlite3_mprintf("UPDATE clothing "
										  "SET days_passed = 0 "
										  "WHERE name = '%s';",
										  name);

	rc = sqlite3_prepare_v2(db, update_choice, -1, &stmt, NULL);
	if (rc != SQLITE_OK)
	{
		printf("error: %s\n", sqlite3_errmsg(db));
		return 0;
	}
	rc = sqlite3_step(stmt);
	if (rc != SQLITE_ROW && rc != SQLITE_DONE)
	{
		printf("error:%s \n", sqlite3_errmsg(db));
	}
	sqlite3_finalize(stmt);
	sqlite3_close(db);
	return 0;
}

/* chooses a complete set of clothing if possible subject to temperature
and displays an image corresponding to each item before prompting the user 
to confirm the set */
int poll_database()
{
	Weather w;
	weather_initialize(&w);

	/* to ignore weather.c
	w.temperature = 50;
	w.humidity = 90;
	end ignore */

	char bottom[MAX_CHARS];
	char top[MAX_CHARS];
	char base[MAX_CHARS];
	char buff;

	do
	{
		srand((unsigned int)time(NULL)); // start random number sequence

		if (perform_query(bottom, "= 0", w.temperature, rand()))
		{
			printf("query failed");
			return 1;
		}

		perform_query(top, "!= 0", w.temperature, rand());

		char *feh;

		// TODO: allow a custom resolution as an argument
		if (get_layer(top) == 3)
		{
			perform_query(base, "= 1", w.temperature, rand());
			asprintf(&feh, "feh -g 800x800 -. images/'%s' images/'%s' images/'%s' &", bottom, base, top);
		}
		else
			asprintf(&feh, "feh -g 800x800 -. images/'%s' images/'%s' &", bottom, top); // & has the effect of making 
																							// system() noni-blocking (at least 
																							// for this use case)

		printf("This work? (Y/n)");
		system(feh);
		scanf("%c", &buff);
	} while (buff == 'n' || buff == 'N');

	update_chosen(base); // allowed to return 1
	update_chosen(top);
	update_chosen(bottom);

	update_all();

	return 0;
}

/*
int main() {
	poll_database();
}
*/
