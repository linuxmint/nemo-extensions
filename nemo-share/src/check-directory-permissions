#!/usr/bin/python3

import os
import sys
import stat

if __name__ == "__main__":

    # Exit if anything is wrong with the arguments
    if len(sys.argv) != 2:
        print("Wrong number of arguments")
        sys.exit(1)

    (progname, path) = sys.argv

    if not os.path.exists(path):
        print("%s not found" % path)
        sys.exit(1)

    while True:
        # check permission
        stats = os.stat(path)
        valid = bool(stats.st_mode & stat.S_IROTH) and bool(stats.st_mode & stat.S_IXOTH)
        if not valid:
            print (path)
            sys.exit(1)
        # go to the parent
        parent = os.path.dirname(path)
        if parent == None or parent == path:
            break
        else:
            path = parent

    sys.exit(0)
