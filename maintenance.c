#define _GNU_SOURCE
#include <sqlite3.h>
#include <dirent.h>
#include <string.h> // strcpy() strchr()
#include <stdio.h>  // printf() fprintf() fgets()

#define MAX_CHARS 100

/* print each string in a nested character array followed by 
its comparison value to its predecessor */
void print_names(char names[][MAX_CHARS], int size)
{
    for (int i = 1; i < size; i++)
    {
        printf("%s %d\n", names[i], strcmp(names[i], names[i - 1]));
    }
}

/* swap two strings by index in a nested character array. Called by
sort_names */
void swap(char names[][MAX_CHARS], int i, int j)
{
    char temp[MAX_CHARS];
    strcpy(temp, names[i]);
    strcpy(names[i], names[j]);
    strcpy(names[j], temp);
}

/* an implementation of insertion sort called by sync_folder */
void sort_names(char names[][MAX_CHARS], int size)
{
    int i, j;
    for (i = size - 1; i > 0; i--)
        for (j = i; j < size && strcmp(names[j], names[j - 1]) < 0; j++)
            swap(names, j, j - 1);
}

/* confirm the existance of database items.db and database table clothing */
int confirm_tables()
{
    sqlite3 *db;
    /* flag SQLITE_OPEN_READWRITE | SQLITE_OPEN_CREATE will create database if
    it does not yet exist */
    int rc = sqlite3_open_v2("items.db", &db, SQLITE_OPEN_READWRITE | SQLITE_OPEN_CREATE, NULL);
    if (rc)
    {
        fprintf(stderr, "Cannot open database: %s\n", sqlite3_errmsg(db));
        return 1;
    }
    sqlite3_stmt *stmt;

    char *confirm_clothing = "SELECT name "
                             "FROM sqlite_master "
                             "WHERE type='table' AND name='clothing';";

    rc = sqlite3_prepare_v2(db, confirm_clothing, -1, &stmt, NULL);

    if (rc != SQLITE_OK) // confirm query 
    {
        printf("error: %s\n", sqlite3_errmsg(db));
        return 1;
    }
    rc = sqlite3_step(stmt);

    /* if return code is not 101 then table does not yet exist. Create it */
    if (rc != SQLITE_ROW) 
    {
        /* PRIMARY KEY implies UNIQUE but in SQLite does not imply NOT NULL */
        char *clothing = "CREATE TABLE clothing ("
                         "name          CHAR(100)  NOT NULL PRIMARY KEY, "
                         "days_passed   INT        NOT NULL, "
                         "coldest_t     INT        NOT NULL, "
                         "warmest_t     INT        NOT NULL, "
                         "layer_value   INT        NOT NULL);";

        sqlite3_reset(stmt); //NEW post test | rc =
        rc = sqlite3_prepare_v2(db, clothing, -1, &stmt, NULL);

        if (rc != SQLITE_OK)
        {
            printf("error: %s\n", sqlite3_errmsg(db));
            return 1;
        }

        rc = sqlite3_step(stmt);
        //printf("attempt to create: %d\n", rc);
    }
    sqlite3_finalize(stmt);
    sqlite3_close(db);
    return 0;
}

/* add one item to table whose file name is the argument */
int insert_to_database(char name[MAX_CHARS])
{
    sqlite3 *db;
    int rc = sqlite3_open_v2("items.db", &db, SQLITE_OPEN_READWRITE, NULL);
    if (rc) 
    {
        printf("Cannot open database: %s\n", sqlite3_errmsg(db));
        return 1;
    }

    /* collect auxillary table fields.
    Do absolutely no
    sanitizing 
    type checking 
    ref https://stackoverflow.com/a/2693826*/

    char min[4] = {'\0'}, max[4] = {'\0'}, type[4] = {'\0'};
    printf("%s\n", name);
    char *pos;
    printf("Type the coldest temperature appropriate for this article of clothing\n");
    scanf("%3s", min);
    if ((pos = strchr(min, '\n')) != NULL)
        *pos = '\0';
    printf("Type the warmest temperature appropriate for this article of clothing\n");
    scanf("%3s", max);
    if ((pos = strchr(max, '\n')) != NULL)
        *pos = '\0';
    /* simple number scheme in order to make quick edits to the
    classification structure */
    char q[] = "To properly categorize this item assign it a layer value."
               "For tops"
               " 1 if this can be worn below another layer "
               " for example a tee shirt."
               " 2 if this is worn by itself."
               " 3 if this must be worn above another layer "
               " e.g. a jacket."
               " 0 for bottoms.";
    printf("%s\n", q);
    scanf("%3s", type);
    if ((pos = strchr(type, '\n')) != NULL)
        *pos = '\0';

    //printf("%s\n", type);

    sqlite3_stmt *stmt;
    /* will fail silently if I try to insert a
    duplicate name */
    char *insert_query = sqlite3_mprintf("INSERT INTO clothing"
                                         "(name, days_passed, coldest_t, warmest_t, layer_value) "
                                         "VALUES"
                                         "('%s', 0, %s, %s, %s);",
                                         name, min, max, type);

    //printf("%s\n", insert_query);
    rc = sqlite3_prepare_v2(db, insert_query, -1, &stmt, NULL);
    if (rc != SQLITE_OK) 
    {
        printf("error: %s\n", sqlite3_errmsg(db));
        return 0;
    }

    rc = sqlite3_step(stmt);

    sqlite3_finalize(stmt);
    sqlite3_close(db);
    return 0;
}

/* remove one item from table whose table field name is the argument */
int remove_from_database(char name[MAX_CHARS])
{
    sqlite3 *db;
    int rc = sqlite3_open_v2("items.db", &db, SQLITE_OPEN_READWRITE, NULL);
    if (rc)
    {
        fprintf(stderr, "Cannot open database: %s\n", sqlite3_errmsg(db));
        return 1;
    }

    sqlite3_stmt *stmt;
    char *delete_query = sqlite3_mprintf("DELETE FROM clothing "
                                         "WHERE name='%s';",
                                         name); 
    //printf("%s\n", delete_query);

    rc = sqlite3_prepare_v2(db, delete_query, -1, &stmt, NULL);

    if (rc != SQLITE_OK)
    {
        printf("error: %s\n", sqlite3_errmsg(db));
        return 1;
    }

    rc = sqlite3_step(stmt);

    sqlite3_finalize(stmt);
    sqlite3_close(db);
    return 0;
}

/* compare a list of table entries e to a list of file names d and 
act appropriately at each comparison. Almost
exactly the merge routine of merge sort */
int compare(char d_names[][MAX_CHARS], int d_size,
            char e_names[][MAX_CHARS], int e_size)
{
    int d_idx = 0, e_idx = 0;
    while (d_idx < d_size && e_idx < e_size)
    {
        int comparison = strcmp(d_names[d_idx], e_names[e_idx]);
        if (comparison == 0)
        {
            d_idx++;
            e_idx++;
        }
        else if (comparison < 0)
        {
            // the directory has a name the database does not
            // add it and its associated data to the database
            if (insert_to_database(d_names[d_idx]))
            {
                return 1;
            }
            d_idx++;
        }
        else
        {
            // the database has a name the directory does not
            // remove it from the database
            if (remove_from_database(e_names[e_idx]))
            {
                return 1;
            }
            e_idx++;
        }
    }
    while (d_idx < d_size)
    {
        // there are still files in the directory not
        // in the database
        // add them all
        insert_to_database(d_names[d_idx]);
        d_idx++;
    }
    while (e_idx < e_size)
    {
        // there are still files left to be removed
        // remove each
        remove_from_database(e_names[e_idx]);
        e_idx++;
    }
    return 0;
}

/* use the functions above to make the table in our database
items.db match exactly those items of clothing with an image in the 
directory /images. Called by main.c if flag -s is received */
int sync_folder()
{
    /* first create a sorted list of strings from the names of
    files in /images */
    struct dirent *ent;
    DIR *dptr;

    dptr = opendir("images");
    if (dptr == NULL)
    {
        printf("Cannot open directory");
        return 1;
    }

    int f_count = 0;
    while ((ent = readdir(dptr)) != NULL)
        if (ent->d_type != DT_DIR)
            f_count++;

    char files[f_count][MAX_CHARS];
    rewinddir(dptr);

    int i = 0;
    while ((ent = readdir(dptr)) != NULL)
    {
        if (ent->d_type != DT_DIR)
        {
            strcpy(files[i], ent->d_name);
            i++;
        }
    }
    closedir(dptr);
    sort_names(files, f_count);

    /* second, create an order list of strings from the names of entries 
    in the database */
    if ( confirm_tables() ) return 1;
    sqlite3 *db;
    int rc = sqlite3_open_v2("items.db", &db, SQLITE_OPEN_READONLY, NULL);
    if (rc)
    {
        fprintf(stderr, "Cannot open database: %s\n", sqlite3_errmsg(db));
        return 1;
    }

    char *query_number = "SELECT COUNT(*) FROM clothing;";
    sqlite3_stmt *stmt;
    rc = sqlite3_prepare_v2(db, query_number, -1, &stmt, NULL);
    if (rc != SQLITE_OK)
    {
        printf("error: %s\n", sqlite3_errmsg(db));
        return 1;
    }
    sqlite3_reset(stmt); 

    int e_count = (rc = sqlite3_step(stmt) == SQLITE_ROW) ? sqlite3_column_int(stmt, 0) : 0;
    char entries[e_count][MAX_CHARS];

    char *query_names = "SELECT name FROM clothing ORDER BY name ASC;"; //" COLLATE NOCASE ASC;";
    rc = sqlite3_prepare_v2(db, query_names, -1, &stmt, NULL);
    if (rc != SQLITE_OK)
    {
        printf("error: %s\n", sqlite3_errmsg(db));
        return 1;
    }
    /* copy rows into array entries */
    i = 0;
    while ((rc = sqlite3_step(stmt)) == SQLITE_ROW)
    {
        strcpy(entries[i], sqlite3_column_text(stmt, 0));
        i++;
    }
    if (rc != SQLITE_DONE)
    {
        printf("error: %s", sqlite3_errmsg(db));
    }

    sqlite3_finalize(stmt);
    sqlite3_close(db);
    // print_names(entries, e_count);

    /* finally, compare the two lists to correct discrepancies */
    if (compare(files, f_count, entries, e_count))
    {
        return 1;
    }
    return 0;
}

/*
int main()
{
	confirm_tables();
    sync_folder();
}
*/
