#!/bin/bash

# 6.6.2019 - Added codesigning on macOS, it grabs the identity
# fingerprint from $signteam environment variable

# TODO: Figure out how to include SDL2 in this bundle
# TODO: Implement and include version number

platform='unknown'
unamestr=$(uname)

if [[ "$unamestr" == 'Linux' ]]; then
	platform='linux'
elif [[ "$unamestr" == 'Darwin' ]]; then
	platform='macos'
elif [[ "$unamestr" == 'FreeBSD' ]]; then
	platform='freebsd'
fi

verstring="v$(grep '#define VERSION' src/c-ray.c | cut -d \" -f2)"

cmake . -DNO_SDL2=True -DCMAKE_BUILD_TYPE=Release
make

# date=$(date +"%Y-%m-%d")
git=$(git rev-parse --verify HEAD | cut -c1-8)
bundlename=cray-$git-$platform-$verstring

# Bundle the release
mkdir "$bundlename"
mkdir "$bundlename"/bin
mkdir "$bundlename"/output
cp ./bin/c-ray "$bundlename"/bin
if [[ "$platform" == 'macos' ]]; then
	if [[ "$signteam" != '' ]]; then
		echo "Code signing using $signteam"
		codesign -s "$signteam" "$bundlename"/bin/c-ray
		codesign -dv --verbose=2 "$bundlename"/bin/c-ray
	fi
fi
cp -r ./input "$bundlename"/
zip -r "$bundlename".zip "$bundlename"
