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

#include <util.h>
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

struct chessh_user {
	char username[256]; /* NULL terminated */
	char pass[88];      /* In /etc/shadow format */
	uuid last_game;
};

static int init_uuid(uuid *ret);

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

int register_user(void *dbp, char *user, char *pass, bool use_api) {
	struct chessh_db *db;
	DB_TXN *transaction;
	UNUSED(user);
	UNUSED(pass);
	UNUSED(use_api);

	db = (struct chessh_db *) dbp;
	if (db->env->txn_begin(db->env, NULL, &transaction, 0) != 0) {
		goto error1;
	}

	return 0;

error1:
	return -1;
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
