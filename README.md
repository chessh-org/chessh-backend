# ches.sh

It's chess over ssh! In your terminal, just run

    ssh chessh@ches.sh

and use the password "chessh" to play a game against another user!

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

## Acknowledgements/Related Projects

I'm using [perftree](https://github.com/agausmann/perftree) for debugging. This
project would have been absolutely impossible without it.

I'm also using [Berkeley DB](https://en.wikipedia.org/wiki/Berkeley_DB) to
manage users. I've been heavily referencing [this
website](https://web.mit.edu/Ghudson/dev/third/rpm/db/docs/api_c/c_index.html)
hosted by MIT containing Berkeley DB documentation.
