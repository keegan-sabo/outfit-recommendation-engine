#define _GNU_SOURCE
#include <stdio.h> 
#include <argp.h> 
#include "maintenance.h"
#include "weather.h"
#include "display.h"

static int parse_opt(int key, char* arg, struct argp_state* state)
{
	switch (key)
	{
		case 'k': write_api_key(arg); break;
		case 's': sync_folder(); break;
	}
	return 0;
}

int main(int argc, char** argv)
{
	struct argp_option options[] = 
	{
		{ "key", 'k', "KEY", 0, "Set API key"},
		{ "sync", 's', 0, 0, "Sync folder and database"},
		{ 0 }
	};

	struct argp argp = { options, parse_opt };

	if ( argp_parse(&argp, argc, argv, 0, 0, 0) ) 
	{
		fputs("error: fialed to parse arguments", stderr);
		return 1;
	}

	if ( poll_database() )
	{
		fputs("error: failed to poll database", stderr);
		return 1;
	}

	return 0;
}
