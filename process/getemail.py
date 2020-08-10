#!/usr/bin/env python

import sys, re
from epasswd import EPasswd

if len(sys.argv)<3:
    sys.exit(-2)

filename=sys.argv[1]
myfaction=sys.argv[2]

pw_data = EPasswd.load_any(filename)
faction = pw_data.get_faction(myfaction)

if not faction:
    sys.exit(-1)

print(faction.email)
