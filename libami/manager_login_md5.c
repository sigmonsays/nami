int manager_login_md5(Manager *m, char *username, char *password) {
	int i;
	char challenge[64];

	unsigned char md5hash[16];
	unsigned char hexdigest[33];
	unsigned char buf[32];

	ManagerMessage msg, resp;

	manager_init_message( &msg );

	/* Get the challenge */
	manager_build_message(&msg, "Action", "Challenge");
	manager_build_message(&msg, "AuthType", "MD5");
	if (manager_send_message(m, &msg ) == -1) return -1;

	manager_init_message( &resp );
	if (manager_recv_message(m, &resp) == -1) return -1;
	if (manager_message_get( &resp, "Challenge", challenge) != 0) return -1;

	/* hash the password */
	strcpy(buf, challenge);
	strcat(buf, password);
	md5_buffer( buf, strlen(buf), md5hash);

	/* make it hex */
	strcpy(hexdigest, "");
	for(i=0; i<16; i++) {
		sprintf(buf, "%02x", md5hash[i]);
		strcat(hexdigest, buf);
	}

	/* Send it back */
	manager_init_message( &msg );
	manager_build_message(&msg, "Action", "Login");
	manager_build_message(&msg, "AuthType", "MD5");
	manager_build_message(&msg, "Username", username);
	manager_build_message(&msg, "Key", hexdigest);
	manager_build_message(&msg, "Events", "off");

	if (manager_send_message(m, &msg ) == -1) return -1;

	manager_init_message( &resp );
	if (manager_recv_message(m, &resp) == -1) return -1;

	if (manager_message_get( &resp, "Response", buf) == -1) return -1;

	if (strcmp(buf, "Error") == 0) return -1;

	m->logged = 1;

	return 0;
}
