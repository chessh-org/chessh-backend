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

#include <stdio.h>
#include <stdlib.h>

#include <util.h>
#include <client/chess.h>
#include <client/frontend.h>

/* notify codes */
#define draw_offer 0x00
#define white_wins 0x01
#define black_wins 0x02
#define forced_draw 0x03
#define internal_server_error 0x04
#define your_turn 0x05
#define illegal_move 0x06
#define move_needs_promotion 0x07

#define NOTIFY(code) \
	do { \
		putchar(0x07); \
		putchar(code); \
	} while (0)

#define CMD_LOGIN 0x00
#define CMD_MAKE_MOVE 0x01
#define CMD_GET_BOARD 0x02
#define CMD_GET_VALID_MOVES 0x03
#define CMD_INIT_GAME 0x04
#define CMD_BOARD_INFO 0x05
#define CMD_MOVE_INFO 0x06
#define CMD_NOTIFY 0x07

static char *get_move(void *aux, struct game *game, enum player player);
static void report_error(void *aux, int code);
static void report_msg(void *aux, int msg_code);
static void display_board(void *aux, struct game *game, enum player player);
static int api_get_move();
static void api_send_board(struct game *game);
static inline int get_code(struct game *game, int r, int c) {
}

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

static char *get_move(void *aux, struct game *game, enum player player) {
	struct move move;

	UNUSED(aux);
	UNUSED(player);

	NOTIFY(your_turn);
	for (;;) {
		int cmd;
		cmd = getchar();
		switch (cmd) {
		case CMD_MAKE_MOVE:
			if (api_get_move(&move) < 0) {
				return NULL;
			}
			goto got_move;
		case CMD_GET_BOARD:
			api_send_board(game);
			break;
		}
	}

got_move:

	return move_to_string(&move);
}

static void report_error(void *aux, int code) {
	UNUSED(aux);
	UNUSED(code);
}

static void report_msg(void *aux, int msg_code) {
	UNUSED(aux);

	switch (msg_code) {
	case MSG_UNKNOWN_ERROR: case MSG_IO_ERROR:
		NOTIFY(internal_server_error);
		break;
	case MSG_WAITING_FOR_OP:
		break;
	case MSG_WHITE_WIN:
		NOTIFY(white_wins);
		break;
	case MSG_BLACK_WIN:
		NOTIFY(black_wins);
		break;
	case MSG_FORCED_DRAW:
		NOTIFY(forced_draw);
		break;
	case MSG_WAITING_FOR_MOVE:
	/* this is handled by get_move() */
		break;
	case MSG_ILLEGAL_MOVE:
		NOTIFY(illegal_move);
		break;
	case MSG_FOUND_OP_WHITE:
		putchar(CMD_INIT_GAME);
		putchar(0);
		break;
	case MSG_FOUND_OP_BLACK:
		putchar(CMD_INIT_GAME);
		putchar(1);
		break;
	}
}

/* no-op, this function is handled by get_move() */
static void display_board(void *aux, struct game *game, enum player player) {
	UNUSED(aux);
	UNUSED(game);
	UNUSED(player);
}

static int api_get_move(struct move *ret) {
	int c1, c2;
	c1 = getchar();
	c2 = getchar();
	if (c1 == EOF || c2 == EOF) {
		return -1;
	}
	ret->r_i = (c1 >> 5) & 7;
	ret->c_i = (c1 >> 2) & 7;
	ret->r_f = (c2 >> 5) & 7;
	ret->c_f = (c2 >> 2) & 7;
	if (c1 & 2) {
		ret->promotion = c2 & 3;
	}
	else {
		ret->promotion = EMPTY;
	}
	return 0;
}

static void api_send_board(struct game *game) {
	for (int r = 0; r < 8; ++r) {
		for (int c = 0; c < 8; c += 2) {
			int code;
			/* we're collecting two pieces at a time here */
			code = get_code(game, r, c) << 4 | get_code(game, r, c+1);
			putchar(code);
		}
	}
	return;
}
