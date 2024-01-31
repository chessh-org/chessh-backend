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

#include <util.h>
#include <client/frontend.h>

static char *get_move(void *aux, enum player player);
static void report_error(void *aux, int code);
static void report_msg(void *aux, int msg_code);
static void display_board(void *aux, struct game *game, enum player player);

struct frontend *new_api_frontend(void) {
	struct frontend *ret;

	if ((ret = malloc(sizeof *ret)) == NULL) {
		return NULL;
	}

	ret->get_move = get_move;
	ret->report_msg = report_msg;
	ret->report_error = report_error;
	ret->display_board = display_board;
	ret->free = (void (*)(struct frontend *)) free;
	ret->aux = NULL;

	return ret;
}

static char *get_move(void *aux, enum player player) {
	UNUSED(aux);
	UNUSED(player);
	return NULL;
}

static void report_error(void *aux, int code) {
	UNUSED(aux);
	UNUSED(code);
}

static void report_msg(void *aux, int msg_code) {
	UNUSED(aux);
	UNUSED(msg_code);
}

static void display_board(void *aux, struct game *game, enum player player) {
	UNUSED(aux);
	UNUSED(game);
	UNUSED(player);
}
