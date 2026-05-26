#!/bin/bash

# Usage: make_source_tarball.sh <git-tag> [<tarball-version>]
# <git-tag>         the exact git tag to archive (e.g. 3.0-1_beta1)
# <tarball-version> the version string used in the tarball/filename
#                   (e.g. 3.0-1~beta1); defaults to <git-tag> when omitted

if [ -z "$1" ]
then
	echo "Please specify git tag"
	exit 1
fi

GIT_TAG="$1"
TARBALL_VER="${2:-$1}"

if [ -z "$(git tag | grep -w "$GIT_TAG")" ]
then
	echo "Specified tag not found, building tarball from HEAD"
	git archive --format=tar --prefix=indigo-$TARBALL_VER/ HEAD | gzip >indigo-$TARBALL_VER.tar.gz
	exit 0
fi

echo "Building tarball from tag $GIT_TAG"
git archive --format=tar --prefix=indigo-$TARBALL_VER/ "$GIT_TAG" | gzip >indigo-$TARBALL_VER.tar.gz
