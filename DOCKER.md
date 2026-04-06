# Installation (using Docker)

## Install Docker (with compose plugin)

* Debian users, follow https://linuxize.com/post/how-to-install-docker-on-debian-13/
* Ubuntu users, follow https://linuxiac.com/how-to-install-docker-on-ubuntu-24-04-lts/

## Clone the Repository

```bash
$ git clone -b docker https://github.com/eressea/server.git ~/eressea/source
$ cd ~/eressea/source
$ git submodule update --init
```

## Configuration

Copy .env.example to .env and modify it. The most importent bit here is using the
same uid and gid as your local user on the host, so files can be shared easily. Use
the id command to find your own values:

```sh
$ id
uid=1000(enno) gid=1000(enno) groups=1000(enno),4(adm),20(dialout),24(cdrom),25(floppy),27(sudo),29(audio),30(dip),44(video),46(plugdev),100(users),107(netdev),989(docker)
```

I am enno, my uid is 1000, and my default gid is also 1000. So my .env file looks like this:

```ini
## local user and group ids
WWWGROUP=1000
WWWUSER=1000

LOCALE=en_US

## cmake build flags
CMAKE_BUILD_TYPE=Release
```

You can also change the locale, which will give you system errors in a different language.

## Building the image

Before we can run a container, we need to build the image first. Use
the command `docker compose build` to do that. When you're done, `docker image ls` should contain there eressea/server:v1 image.

## Create a game directory and a tiny world.

The following commands set up your first game:

```bash
$ cd ~/eressea
$ mkdir game-1
$ docker run -v $HOME/eressea/game-1:/data eressea/server:v1 newgame.lua
```

After this, your ~/eressea/game-1 directory contains some new files:

`eressea.ini` contains a game configuration with sensible defaults for
a game with the E2 ruleset. Modify this with your own email address and
a different ruleset, if you want to. Only e2 and e3 currently supported,
custom rulesets TBD.

`data/0.dat` is a game file that contains a single-hex island with some ocean around it. There are no players in this world (yet).
