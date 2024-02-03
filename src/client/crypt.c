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

#include <stdlib.h>
#include <unistd.h>

static inline char gen_salt_char();

char *crypt_salt(char *key) {
	char salt[] = "$5$XXXXXXXXXXXXXXXX$";

	for (int i = 0; salt[i] != '\0'; ++i) {
		if (salt[i] != 'X') {
			continue;
		}
		salt[i] = gen_salt_char();
	}

	return crypt(key, salt);
}

/* TODO: Make this cryptographically secure? */
static inline char gen_salt_char() {
	return "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789./"[random() & 63];
}
