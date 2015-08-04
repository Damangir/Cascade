import sys
if sys.version_info<(2,7,0):
    sys.stderr.write("You need python 2.7 or later to run the Cascade.\n")
    sys.stderr.write("You are using python"+sys.version+".\n")
    exit(1)
