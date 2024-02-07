# ches.sh

It's chess over ssh! In your terminal, just run

    ssh guest@ches.sh

and use the password "guest" to play a game against another guest!

Alternatively, you can use telnet

    telnet ches.sh

Don't worry about the login, that doesn't affect anything for the moment. It may
in a future update, though ;)

This repository contains the ches.sh backend. It implements chess, some
plaintext frontends, and the API.

## Technical information

This program is intended to work with Docker. It contains hardcoded file paths
and port numbers for that reason. To run this program, just run

    ./build.sh
    docker compose up

## Related projects

I'm using [perftree](https://github.com/agausmann/perftree) for debugging. This
project would have been absolutely impossible without it.

I'm also using [Berkeley DB](https://en.wikipedia.org/wiki/Berkeley_DB) to
manage users.
