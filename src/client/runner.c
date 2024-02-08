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

#include <errno.h>
#include <stdio.h>
#include <ctype.h>
#include <stdlib.h>
#include <string.h>

#include <locale.h>
#include <unistd.h>

#include <legal.h>
#include <copyfd.h>
#include <client/sock.h>
#include <client/chess.h>
#include <client/runner.h>
#include <client/frontend.h>

static int parse_op_move(struct frontend *frontend, struct game *game, int fd);
static int get_player_move(struct frontend *frontend, struct game *game, int peer);

int run_client(char *sock_path) {
	int fds[2];
	int pid;
	enum player player;
	int recvlen;
	ssize_t pidlen;
	struct game *game;
	struct frontend *frontend;
	int end_msg;
	int sock_fd;

	frontend = new_api_frontend();
	if (frontend == NULL) {
		puts("Failed to initialize frontend, quitting");
		return 1;
	}

	frontend->report_msg(frontend->aux, MSG_WAITING_FOR_OP);

	if ((sock_fd = unix_connect(sock_path)) < 0) {
		return 1;
	}

	for (;;) {
		if ((recvlen = recvfds(sock_fd, fds, sizeof fds / sizeof *fds, &pid, sizeof pid, &pidlen)) < 2 ||
		    pidlen < (ssize_t) sizeof pid) {
			switch (errno) {
			case EINTR: case EAGAIN:
				continue;
			}
			return 1;
		}
		break;
	}

	player = pid == 0 ? WHITE : BLACK;

	frontend->report_msg(frontend->aux, player == WHITE ?
			MSG_FOUND_OP_WHITE: MSG_FOUND_OP_BLACK);

	game = new_game();

	frontend->display_board(frontend->aux, game, player);
	for (;;) {
		int move_code;
		int curr_player = get_player(game) == WHITE ? 0:1;
		if (curr_player == pid) {
			move_code = get_player_move(frontend, game, fds[1]);
		}
		else {
			frontend->report_msg(frontend->aux, MSG_WAITING_FOR_OP_MOVE);
			move_code = parse_op_move(frontend, game, fds[0]);
		}

		switch (move_code) {
		case WHITE_WIN:
			end_msg = MSG_WHITE_WIN;
			goto end;
		case BLACK_WIN:
			end_msg = MSG_BLACK_WIN;
			goto end;
		case FORCED_DRAW:
			end_msg = MSG_FORCED_DRAW;
			goto end;
		case IO_ERROR:
			end_msg = MSG_IO_ERROR;
			goto end;
		}
		if (move_code < 0) {
			end_msg = MSG_UNKNOWN_ERROR;
			goto end;
		}

		frontend->display_board(frontend->aux, game, player);
	}
end:
	frontend->report_msg(frontend->aux, end_msg);
	sleep(3);
	frontend->free(frontend);
	return 0;
}

static int parse_op_move(struct frontend *frontend, struct game *game, int fd) {
	char buff[1024];
	int code;
	ssize_t read_len;
	struct move move;
	if ((read_len = read(fd, buff, sizeof buff)) < 5) {
		return IO_ERROR;
	}
	if (buff[read_len-1] != '\0') {
		return IO_ERROR;
	}
	if ((code = parse_move(&move, buff)) < 0 ||
	    (code = make_move(game, &move)) < 0) {
		return code;
	}
	frontend->report_event(EVENT_OP_MOVE, frontend->aux, game, &move);
	return 0;
}

static int get_player_move(struct frontend *frontend, struct game *game, int peer) {
	char *move_text;
	int move_code;
	struct move move;
	ssize_t move_text_len;
	frontend->report_msg(frontend->aux, MSG_WAITING_FOR_MOVE);
	for (;;) {
		move_text = frontend->get_move(frontend->aux, game, get_player(game));
		if (move_text == NULL) {
			return IO_ERROR;
		}
		if (((move_code = parse_move(&move, move_text)) < 0 ||
		     (move_code = make_move(game, &move)) < 0) &&
		     (move_code != ILLEGAL_MOVE &&
		      move_code != MISSING_PROMOTION)) {
			return move_code;
		}
		switch (move_code) {
		case ILLEGAL_MOVE:
			frontend->report_msg(frontend->aux, MSG_ILLEGAL_MOVE);
			free(move_text);
			continue;
		case MISSING_PROMOTION:
			frontend->report_error(frontend->aux, MISSING_PROMOTION);
			free(move_text);
			continue;
		}
		break;
	}
	move_text_len = (ssize_t) strlen(move_text)+1;
	if (write(peer, move_text, move_text_len) < move_text_len) {
		free(move_text);
		return IO_ERROR;
	}
	free(move_text);
	return move_code;
}
