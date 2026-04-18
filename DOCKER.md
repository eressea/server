# Overview

At this point, the container can mount a game directory from the host, and run the server with a given script like so:
```bash
docker run -v $HOME/eressea/game:/data eressea/server:v1 myscript.lua
```

This mounts the host's `game` directory as a data volume in the container, and the server operated on the files in there, i.e. game data and reports go here.
The script can either be on the data volume, or one of the pre-made scripts in the scripts directory, like newgame.lua, reports.lua, or run-turn.lua. These get copied to /usr/local/share/eressea/scripts during the build process.

## Official Image

If you don't want to build the image yourself, use the latest official image from
the container registry:

```bash
docker pull ghcr.io/eressea/server:latest
```

# Installation (using Docker)

If you want to build the image yourself, because you're planning to make 
changes to the source code, follow these steps.

## Install Docker (with compose plugin)

* Debian users, follow https://linuxize.com/post/how-to-install-docker-on-debian-13/
* Ubuntu users, follow https://linuxiac.com/how-to-install-docker-on-ubuntu-24-04-lts/

## Building the image from source

### Clone the Repository

```bash
$ git clone -b develop https://github.com/eressea/server.git ~/eressea/source
$ cd ~/eressea/source
$ git submodule update --init
```

### Configuration

Copy .env.example to .env and modify it. The most importent bit here is using the
same uid and gid as your local user on the host, so files can be shared easily. Use
the id command to find your own values:

```sh
$ id
uid=1000(enno) gid=1000(enno) groups=1000(enno),4(adm),27(sudo),100(users),107(netdev),989(docker)
```

I am enno, my uid is 1000, and my default gid is also 1000. The eressea user in the
container uses 1337 by default, but if I want it to use 1000, that can be achieved
by changing the .env file like this:

```ini
## local user and group ids
WWWGROUP=1000
WWWUSER=1000

LOCALE=en_US

## cmake build flags
CMAKE_BUILD_TYPE=Release
```

You can also change the locale, which will give you system errors in a different language.

### Building the image

Before we can run a container, we need to first build the image. Use
the command `docker compose build` to do that. When you're done,
`docker image ls` should contain the eressea/server:v1 image. 


## Create a game directory and a tiny world.

The following steps assume you're using the official image.
If you built your oen image, I recommend that you tag it to
follow along:

```bash
docker image tag eressea/server:v1 eressea/server:latest
```

Create a directory for your first game:

```bash
cd ~/eressea
mkdir game
# for convenience, alias the "docker run" command
alias eressea='docker run -ti -e TERM -v $HOME/eressea/game:/data eressea/server:v1'
eressea newgame.lua
```

After this, your ~/eressea/game directory contains some new files:

`data/0.dat` is a game file that contains a single-hex island with some ocean around it. There are no players in this world (yet).

`eressea.ini` contains a game configuration with sensible defaults for
a game with the E2 ruleset. Modify this with your own email address and
a different ruleset, if you want to. The only built-in rulesets you can choose from are e2 and e3. 

## Let's Play
Now let's add a faction to our world and write initial reports.

in the game directory, add a short Lua program named start.lua:
```lua
require 'config'
eressea.read_game('0.dat')
r = get_region(0, 0)
f = faction.create('orc', 'orcs@example.com', 'de')
u = unit.create(f, r)
init_reports()
write_reports()
eressea.write_game(get_turn() .. '.dat')
```

Execute this program:
```
eressea start.lua
```
You should now have reports for your initial faction in the game/reports
directory!

If you would like to use the GM Mapper, you can start that with the built-in map.lua program, just run `eressea map.lua`.

## Custom Rulesets

You can choose to make your own rules, too! Just make the following modifications to your eressea.ini :
```ini
[lua]
;rules = e2
config = rules.json
```
This will instruct the game to not use the e2 ruleset, but a rules.json 
file from your game directory. This file could look like this:
```json
{
	"settings": {
		"game.name": "Discworld",
		"game.start": 0
	},
	"include": [
		"config://conf/keywords.json",
		"config://conf/calendar.json",
		"config://conf/e2/locales.json",
		"config://conf/e2/terrains.json",
		"config://conf/e2/items.json",
		"config://res/eressea/races.xml",
		"config://custom/terrains.json",
		"config://custom/strings.de.po"
	]
}
```
Here, we are instructing the game to borrow some rules from the core game,
like the names of keywords, the calendar, and the races. We also borrow the translations, terrains and items from E2. In addition, we include our own terrains and translations from the game/custom directoory. For their strucuture, refer to the original files and make your own changes!

