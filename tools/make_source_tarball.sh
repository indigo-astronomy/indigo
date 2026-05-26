#!/bin/bash

# Build a source tarball for a given release version.
#
# Usage: make_source_tarball.sh <version>
#   <version>  the deb-style version string, e.g. 3.0-1~beta1 or 2.0-370
#
# Tag format:
#   Releases    2.0-370       (no pre-release suffix)
#   Pre-releases 3.0-1_beta1  ('_' used because git does not allow '~' in tags)
#
# The script converts '~' to '_' to locate the git tag, but keeps the original
# '~' form for the tarball name so the resulting file matches the deb version.
#
# Fallback: if the tag is not found the tarball is built from HEAD, which is
# useful for local/CI builds that are not on a tagged commit.

if [ -z "$1" ]
then
	echo "Usage: $0 <version>  (e.g. 3.0-1~beta1 or 2.0-370)"
	exit 1
fi

TARBALL_VER="$1"
GIT_TAG="${TARBALL_VER//\~/_}"

if [ -z "$(git tag | grep -w "$GIT_TAG")" ]
then
	echo "Specified tag not found, building tarball from HEAD"
	git archive --format=tar --prefix=indigo-$TARBALL_VER/ HEAD | gzip >indigo-$TARBALL_VER.tar.gz
	exit 0
fi

echo "Building tarball from tag $GIT_TAG"
git archive --format=tar --prefix=indigo-$TARBALL_VER/ "$GIT_TAG" | gzip >indigo-$TARBALL_VER.tar.gz
