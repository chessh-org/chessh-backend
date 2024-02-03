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
#include <string.h>
#include <stdint.h>

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
static void report_event(int code, void *aux, struct game *game, void *data);
static void report_msg(void *aux, int msg_code);
static void display_board(void *aux, struct game *game, enum player player);
static int api_get_move();
static void api_send_board(struct game *game);
static int count_valid_moves(struct game *game);
static void api_send_valid_moves(struct game *game);
static void print_move_if_valid(struct game *game,
		int r_i, int c_i, int r_f, int c_f);
static void print_move(struct move *move);
static bool move_is_valid(struct game *game, struct move *move);
static void putword(uint16_t word);

static inline int get_code(struct game *game, int r, int c) {
	struct piece *piece = &game->board.board[r][c];
	return piece->player << 3 | piece->type;
}

struct frontend *new_api_frontend(void) {
	struct frontend *ret;

	if ((ret = malloc(sizeof *ret)) == NULL) {
		return NULL;
	}

	ret->get_move = get_move;
	ret->report_error = report_error;
	ret->report_msg = report_msg;
	ret->report_event = report_event;
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
		fprintf(stderr, "%d\n", cmd);
		switch (cmd) {
		case CMD_MAKE_MOVE:
			if (api_get_move(&move) < 0) {
				return NULL;
			}
			goto got_move;
		case CMD_GET_BOARD:
			putchar(CMD_BOARD_INFO);
			api_send_board(game);
			fflush(stdout);
			break;
		case CMD_GET_VALID_MOVES:
			putchar(CMD_MOVE_INFO);
			/* TODO: Make this more efficient, we're counting every
			 * possible move twice here, I don't like that. */
			putword(count_valid_moves(game));
			api_send_valid_moves(game);
			fflush(stdout);
			break;
		default:
			return NULL;
		}
	}

got_move:

	return move_to_string(&move);
}

static void report_error(void *aux, int code) {
	UNUSED(aux);
	UNUSED(code);
}

static void report_event(int code, void *aux, struct game *game, void *data) {
	UNUSED(aux);
	UNUSED(game);

	switch (code) {
	case EVENT_OP_MOVE:
		fputs("TEST\n", stderr);
		print_move((struct move *) data);
		break;
	}
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
		fflush(stdout);
		break;
	case MSG_FOUND_OP_BLACK:
		putchar(CMD_INIT_GAME);
		putchar(1);
		fflush(stdout);
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
}

static int count_valid_moves(struct game *game) {
	int ret = 0;
	for (int r_i = 0; r_i < 8; ++r_i) {
		for (int c_i = 0; c_i < 8; ++c_i) {
			for (int r_f = 0; r_f < 8; ++r_f) {
				for (int c_f = 0; c_f < 8; ++c_f) {
					struct move move;
					move.r_i = r_i;
					move.c_i = c_i;
					move.r_f = r_f;
					move.c_f = c_f;
					if (move_is_valid(game, &move)) {
						++ret;
					}
				}
			}
		}
	}
	return ret;
}

static void api_send_valid_moves(struct game *game) {
	for (int r_i = 0; r_i < 8; ++r_i) {
		for (int c_i = 0; c_i < 8; ++c_i) {
			for (int r_f = 0; r_f < 8; ++r_f) {
				for (int c_f = 0; c_f < 8; ++c_f) {
					print_move_if_valid(game, r_i, c_i, r_f, c_f);
				}
			}
		}
	}
}

static void print_move_if_valid(struct game *game,
		int r_i, int c_i, int r_f, int c_f) {
	struct move move;
	move.r_i = r_i;
	move.c_i = c_i;
	move.r_f = r_f;
	move.c_f = c_f;
	move.promotion = QUEEN;
	if (move_is_valid(game, &move)) {
		print_move(&move);
	}
}

static void print_move(struct move *move) {
	putchar(move->r_i << 5 | move->c_i << 2);
	putchar(move->r_f << 5 | move->c_f << 2);
}

static bool move_is_valid(struct game *game, struct move *move) {
	struct game backup;
	memcpy(&backup, game, sizeof backup);
	return make_move(&backup, move) != ILLEGAL_MOVE;
}

static void putword(uint16_t word) {
	int c1 = (word >> 8) & 0xff;
	int c2 = (word)      & 0xff;
	putchar(c1);
	putchar(c2);
}
