ls -1 orders.dir/turn* | sed -e 's/.*turn-\(.*\),gruenbaer.*/\1/' | sort -u
