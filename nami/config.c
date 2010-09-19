#include "config.h"


/* Parse configuration file
 * return -1 on error, 0 on success
 */
int load_config( char *file, config_t *cfg) {

	profile_t profile;
	char conn_str[255];

	char section[128];

	int l, lineno = 0;
	char key[256], value[256], buf[4096];

	FILE *f;

	f = fopen(file, "r");
	if (!f) return -1;

	init_config(cfg);

	strcpy(cfg->config_filename, file);
	strcpy(section, "globals");

	while (fgets(buf, sizeof(buf) - 1, f)) {

		lineno += 1;

		l = strlen(buf);
		if (buf[l - 1] == '\n') buf[--l] = 0;
		if (buf[l - 1] == '\r') buf[--l] = 0;

		if (l == 0 || *buf == '#' || *buf == ';') continue;

		if (*buf == '[' && sscanf(buf, "[%128[^]]", section) ) {
			continue;
		}


		if (strcmp(section, "globals") == 0) {

			if (sscanf (buf, "%255[a-zA-Z0-9_-] = '%255[^']'", key, value) < 2) {
				fprintf(stderr, "WARNING: bad global format at line %d: '%s'\n", lineno, buf);
				continue;
			}


			if (strcmp( key, "default_hostname") == 0) {
				strcpy(cfg->default_hostname, value);

			} else if (strcmp( key, "date_format") == 0) {
				strcpy(cfg->date_format, value);

			} else {

				fprintf(stderr, "WARNING: Skipping unknown directive: %s\n", key);
			}


		} else if (strcmp(section, "profiles") == 0) {

			if (sscanf(buf, "%255[a-zA-z0-9_-] => '%255[^']'$", profile.name, conn_str ) < 2) {
				fprintf(stderr, "WARNING: Bad profile format at line %d: %s\n", lineno, buf);
				continue;
			}


			if (parse_profile_line( conn_str, &profile) != 0) {
				fprintf(stderr, "WARNING: Skipping bad profile line at %d: %s\n", lineno, buf);
				continue;
			}


			if (strlen(profile.hostname) && strlen(profile.username)) {

				if (cfg->profile_count < CONFIG_MAX_PROFILES) {

					profile.number = cfg->profile_count + 1;
					memcpy( cfg->profiles + cfg->profile_count++, &profile, sizeof(profile_t));

				} else {
					fprintf(stderr, "Max profiles reached (limit %d); line %d: Skipping: %s\n", CONFIG_MAX_PROFILES, lineno, buf);
				}

			}


			strcpy(profile.hostname, buf);

		} else {


			fprintf(stderr, "WARNING: Unknown section: '%s'\n", section);
		}


	}

	fclose(f);
	return 0;
}

int init_config( config_t *cfg) {

	profile_t *profile;

	int i;

	strcpy(cfg->config_filename, "");
	strcpy(cfg->default_hostname, "");

	cfg->profile_count = 0;

	for(i=0; i<CONFIG_MAX_PROFILES; i++) {

		profile = cfg->profiles + i;
		init_profile(profile);
		
	}
	return 0;
}

int print_config( config_t *cfg ) {
	int i;
	profile_t *profile;

	printf("-- Global --\n");
	printf("Configuration (%s):\n", (strlen(cfg->config_filename) ? cfg->config_filename : "(none)") );
	printf(" Default Hostname: %s\n", cfg->default_hostname);
	printf(" Date Format: '%s'\n", cfg->date_format);

	printf("-- Profiles --\n");
	for(i=0; i<CONFIG_MAX_PROFILES; i++) {

		profile = cfg->profiles + i;

		if (strlen(profile->hostname) == 0) break;

		printf("Profile %d) '%s'\n", i + 1, profile->name);
		printf(" Hostname: %s\n", profile->hostname);
		printf(" Username: %s\n", profile->username);
		printf(" Password: %s\n", profile->password);

	}
	return 0;

}

int init_profile(profile_t *profile) {
	profile->number = 0;

	strcpy(profile->name, "");
	strcpy(profile->hostname, "");
	strcpy(profile->username, "");
	strcpy(profile->password, "");
	return 0;
}

/* parse profile line in format 'user:pw@host' 
 * return -1 on error, 0 on success
 */
int parse_profile_line(char *conn_str, profile_t *profile) {
	char *p;

	char *username, *password, *hostname;

	username = conn_str;

	// 'sig:pw@host'
	p = strstr(conn_str, ":");
	if (!p) return -1;
	*p = 0;

	if (p++ && *p == 0) return -1;
	password = p;

	p = strstr(password, "@");
	if (!p) return -1;

	*p = 0;
	if (p++ && *p == 0) return -1;
	hostname = p;

	strcpy(profile->hostname, hostname);
	strcpy(profile->username, username);
	strcpy(profile->password, password);

	return 0;
}
