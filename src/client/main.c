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

#include <getopt.h>

#include <legal.h>
#include <client/users.h>
#include <client/perft.h>
#include <client/runner.h>

struct client_args {
	char *dir;
	char *user;
	char *pass;

	int perft;
	char *start_pos;
	char *start_sequence;
	bool autotest;

	bool register_user;
};

static void parse_args(int argc, char *argv[], struct client_args *ret);
static void print_help(char *progname);

int main(int argc, char *argv[]) {
	struct client_args args;
	char sock_path[4096];
	void *dbp;

	parse_args(argc, argv, &args);

	if (args.perft != -1) {
		return run_perft(args.perft, args.start_pos, args.start_sequence, args.autotest);
	}

	if ((dbp = init_user_db()) == NULL) {
		return 1;
	}

	if (args.register_user) {
		return register_user(dbp, args.user, args.pass);
	}

	if (!user_is_valid(dbp, args.user, args.pass)) {
		return 1;
	}

	snprintf(sock_path, sizeof sock_path, "%s/matchmaker", args.dir);
	sock_path[sizeof sock_path - 1] = '\0';

	return run_client(sock_path);
}

static void parse_args(int argc, char *argv[], struct client_args *ret) {
	ret->dir = ret->user = ret->pass = NULL;
	ret->perft = -1;
	ret->start_pos = ret->start_sequence = NULL;
	ret->autotest = false;
	ret->register_user = false;

	for (;;) {
		int opt = getopt(argc, argv, "hld:u:p:t:i:s:amr");
		switch (opt) {
		case -1:
			goto got_args;
		case 'h':
			print_help(argv[0]);
			exit(EXIT_SUCCESS);
		case 'l':
			print_legal();
			exit(EXIT_SUCCESS);
		case 'd':
			ret->dir = optarg;
			break;
		case 'u':
			ret->user = optarg;
			break;
		case 'p':
			ret->pass = optarg;
			break;
		case 't':
			ret->perft = atoi(optarg);
			break;
		case 'i':
			ret->start_pos = optarg;
			break;
		case 's':
			ret->start_sequence = optarg;
			break;
		case 'a':
			ret->autotest = true;
			break;
		case 'r':
			ret->register_user = true;
			break;
		default:
			print_help(argv[0]);
			exit(EXIT_FAILURE);
		}
	}
got_args:

	if (ret->perft != -1) {
		return;
	}

	if ((!ret->register_user && ret->dir == NULL) ||
			ret->user == NULL || ret->pass == NULL) {
		fprintf(stderr, "%s: missing required argument\n", argv[0]);
		print_help(argv[0]);
		exit(EXIT_FAILURE);
	}
}

static void print_help(char *progname) {
	printf("Usage: %s -d [dir] -u [username] -p [password]\n", progname);
	puts("OTHER FLAGS:");
	puts("  -h: Show this help and quit");
	puts("  -l: Show a legal notice and quit");
	puts("  -t [level]: Run a perft test with [level] levels");
	puts("  -i [start]: Use [start] as the starting position for the perft test");
	puts("  -s [sequence]: Run [sequence] before beginning the perft test");
	puts("  -a: Produce a test output suitable for automatic testing with perftree");
	puts("  -r: Don't play chess, register this user instead");
}
