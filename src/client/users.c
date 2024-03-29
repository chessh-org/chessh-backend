/* chessh - chess over ssh
 * Copyright (C) 2024  Nate Choe <nate@natechoe.dev>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Affero General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Affero General Public License for more details.
 *
 * You should have received a copy of the GNU Affero General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 * */

#include <db.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <util.h>
#include <client/crypt.h>
#include <client/users.h>

#define USER 0x00
#define GAME 0x01

typedef unsigned char datum_type;

/* A uuid is just 16 random binary bytes, not transmittable through plaintext. */
typedef char uuid[16];

static const uuid null_uuid = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};

struct chessh_db {
	DB_ENV *env;
	DB *user_dbp;
};

/* XXX: Run memset(&user, 0, sizeof user) before inserting/getting */
struct chessh_user {
	char username[256]; /* NULL terminated */
	char pass[88];      /* In /etc/shadow format */
	uuid last_game;
};

static int init_uuid(uuid *ret);

static void report_msg(unsigned char code, char *elaboration);

#define REGISTRATION_SUCCESSFUL 0x00
#define REGISTRATION_FAILED 0x01
#define AUTH_SUCCESSFUL 0x80
#define AUTH_FAILED 0x81

void *init_user_db() {
	struct chessh_db *ret;

	if ((ret = malloc(sizeof *ret)) == NULL) {
		goto error1;
	}
	if (db_env_create(&ret->env, 0) != 0) {
		goto error2;
	}
	if (ret->env->open(ret->env, "/chessh-data/environment",
				DB_INIT_LOCK |
				DB_INIT_TXN |
				DB_INIT_MPOOL |
				DB_CREATE, 0) != 0) {
		goto error3;
	}
	if (db_create(&ret->user_dbp, ret->env, 0) != 0) {
		goto error3;
	}
	if (ret->user_dbp->open(ret->user_dbp, NULL,
				"/chessh-data/users", NULL,
				DB_HASH, DB_CREATE | DB_AUTO_COMMIT, 0) != 0) {
		goto error4;
	}
	return (void *) ret;

error4:
	ret->user_dbp->close(ret->user_dbp, 0);
error3:
	ret->env->close(ret->env, 0);
error2:
	free(ret);
error1:
	return NULL;

	/* TODO: Remove me */
	init_uuid(NULL);
}

int register_user(void *dbp, char *user, char *pass) {
	struct chessh_db *db;
	DBT key, value;
	struct chessh_user new_user;
	size_t user_len, pass_len;
	char *pass_hashed;
	int error_code;

	user_len = strlen(user);

	if (user_len > 0xff) {
		report_msg(REGISTRATION_FAILED, "Username is too long (max len: 255)");
		return -1;
	}

	if (user[0] == '_') {
		report_msg(REGISTRATION_FAILED, "Usernames beginning with an underscore are reserved");
		return -1;
	}

	pass_hashed = crypt_salt(pass);
	if (pass_hashed == NULL) {
		report_msg(REGISTRATION_FAILED, "Failed to hash password");
		return -1;
	}
	pass_len = strlen(pass_hashed);
	if (pass_len + 1 >= sizeof new_user.pass) {
		report_msg(REGISTRATION_FAILED, "Hashed password too long? (internal server error)");
		return -1;
	}

	memset(&new_user, 0, sizeof new_user);
	memcpy(new_user.username, user, user_len);
	memcpy(new_user.pass, pass_hashed, pass_len + 1);

	memset(&key, 0, sizeof key);
	memset(&value, 0, sizeof value);
	key.data = user;
	key.size = user_len;
	value.data = &new_user;
	value.size = sizeof new_user;

	db = (struct chessh_db *) dbp;

	error_code = db->user_dbp->put(db->user_dbp, NULL, &key, &value, DB_NOOVERWRITE | DB_AUTO_COMMIT);
	if (error_code != 0) {
		char *msg;
		switch (error_code) {
		case DB_KEYEXIST:
			msg = "Username already registered :(";
			break;
		default:
			msg = "Unknown error while writing to database";
			break;
		}
		report_msg(REGISTRATION_FAILED, msg);
		return -1;
	}

	report_msg(REGISTRATION_SUCCESSFUL, "User registered, we did it reddit!");
	return 0;
}

bool user_is_valid(void *dbp, char *user, char *pass) {
	struct chessh_db *db;
	struct chessh_user user_data;
	DBT key, data;
	char *pass_encrypted;
	int code;

	db = (struct chessh_db *) dbp;
	memset(&user_data, 0, sizeof user_data);
	memset(&key, 0, sizeof key);
	memset(&data, 0, sizeof data);

	key.data = user;
	key.size = strlen(user);
	data.data = &user_data;
	data.ulen = sizeof user_data;
	data.flags = DB_DBT_USERMEM;
	if ((code = db->user_dbp->get(db->user_dbp, NULL, &key, &data, 0)) != 0) {
		char *msg;
		if (code == DB_NOTFOUND) {
			msg = "Couldn't find a user with that name";
		}
		else {
			msg = "Failed to retrieve user from database";
		}
		report_msg(AUTH_FAILED, msg);
		return false;
	}

	pass_encrypted = crypt(pass, user_data.pass);
	if (strcmp(pass_encrypted, user_data.pass) != false) {
		report_msg(AUTH_FAILED, "Incorrect username/password");
		return false;
	}

	report_msg(AUTH_SUCCESSFUL, "Authentication successful, we're in");
	return true;
}

static int init_uuid(uuid *ret) {
	static FILE *random_file = NULL;
	if (random_file == NULL) {
		if ((random_file = fopen("/dev/random", "r")) == NULL) {
			return -1;
		}
	}

	if (fread(*ret, sizeof *ret, 1, random_file) < 1) {
		return -1;
	}

	if (memcmp(*ret, null_uuid, sizeof *ret) == 0) {
		return -1;
	}

	return 0;
}

static void report_msg(unsigned char code, char *elaboration) {
	size_t elaboration_len;
	if ((elaboration_len = strlen(elaboration)) > 0xff) {
		fprintf(stderr, "Elaboration '%s' is too long!\n", elaboration);
		return;
	}
	putchar(0x09);
	putchar(code);
	putchar((unsigned char) elaboration_len);
	fputs(elaboration, stdout);
	fflush(stdout);
}
