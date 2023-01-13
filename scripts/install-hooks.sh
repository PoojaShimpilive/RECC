#!/bin/bash

scriptsdir="scripts"
hookprefix="git-"

gitdir="$(git rev-parse --git-dir)"
projectdir=$(dirname "$gitdir")
scriptsdir="$projectdir/$scriptsdir"

cd $projectdir

find "$scriptsdir" -name "$hookprefix*" -type f | while read script; do
    echo "Processing $script"

    # have to remove the "prefix" so that git recognizes the appropriate trigger
    hook_name=$(basename "$script" | sed -e "s/$hookprefix//g")
    githook_relpath="$gitdir/hooks/$hook_name"

    echo Symlinking ../../$script $githook_relpath
    ln -s ../../$script $githook_relpath

    chmod +x "$script"
done

