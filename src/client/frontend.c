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

#include <client/frontend.h>

char *frontend_strerror(int code) {
	switch (code) {
	case MSG_WAITING_FOR_OP:
		return "Waiting on opponent's move";
	case MSG_WHITE_WIN:
		return "White wins!";
	case MSG_BLACK_WIN:
		return "Black wins!";
	case MSG_FORCED_DRAW:
		return "It's a draw!";
	case IO_ERROR:
		return "I/O error";
	case MSG_WAITING_FOR_MOVE:
		return "Make your move";
	case MSG_ILLEGAL_MOVE:
		return "Illegal move!";
	case MSG_UNKNOWN_ERROR: default:
		return "An unknown error has occured";
	}
}
