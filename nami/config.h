#ifndef HAVE_CONFIG_H
#define HAVE_CONFIG_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define CONFIG_MAX_PROFILES 10

/* structures */

typedef struct {
	int number;
	char name[255], hostname[255], username[255], password[255];

} profile_t;

typedef struct {

	char config_filename[4096];

	char default_hostname[4096];

	profile_t profiles[CONFIG_MAX_PROFILES];
	int profile_count;

	char date_format[128];

} config_t;


/* prototypes */

int load_config( char *file, config_t *cfg);
int init_config( config_t *cfg);
int init_profile( profile_t *cfg);
int parse_profile_line(char *conn_str, profile_t *profile);

int print_config( config_t *cfg );

#endif
