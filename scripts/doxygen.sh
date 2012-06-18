#!/bin/sh

DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"
cd $DIR/..

# Make sure we have the latest commits.
git pull -f

echo "Generating Doxygen documentation..."
doxygen doc/Doxyfile > /dev/null 2>&1
echo "Complete."
