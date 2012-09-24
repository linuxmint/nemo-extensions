import sys
import datetime

# heeeheee
env = {"__name__":"__notmain__"}
execfile("dropbox", env)
commands = env["commands"]

f = open("AUTHORS", "r")
authors = '| ' + f.read().replace('\n', '\n| ')
f.close()

formatted_commands = ""
for cmd in commands:
    split = commands[cmd].__doc__.split('\n', 2)
    formatted_commands += split[1].decode('ascii').replace(cmd, "`%s`" % cmd).replace("dropbox", "``dropbox``")
    formatted_commands += split[2].decode('ascii').replace('\n', '\n  | ')
    formatted_commands += '\n\n'

sys.stdout.write(sys.stdin.read().replace\
                     ('@AUTHORS@', authors).replace\
                     ('@DATE@', datetime.date.today().isoformat()).replace\
                     ('@PACKAGE_VERSION@', sys.argv[1]).replace\
                     ('@SYNOPSIS@', '| '+'\n| '.join(commands[cmd].__doc__.split('\n', 2)[1].decode('ascii').replace(cmd, "`%s`" % cmd).replace("dropbox", "``dropbox``") for cmd in commands)).replace\
                     ('@COMMANDS@', formatted_commands))
