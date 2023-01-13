#!/bin/bash

ANSI_COLOR_DEFAULT=$(tput sgr0)
ANSI_COLOR_RED=$(tput setaf 1)
ANSI_COLOR_YELLOW=$(tput setaf 3)
ANSI_COLOR_GREEN=$(tput setaf 2)

# Depending on the version you have, those can be slightly different
ALL_GOOD_STRINGS=("no modified files to format" "clang-format did not modify any files")
# Show this text at the end
OVERRIDE_TEXT=$ANSI_COLOR_YELLOW"If you need to force this commit use '--no-verify'."$ANSI_COLOR_DEFAULT

# Make sure git-clang-format is installed and in the path
command -v git-clang-format >/dev/null 2>&1 || {
    echo -e $ANSI_COLOR_RED
    echo "Error: git-clang-format not installed, install 'clang-6.0' and add to path."
    echo -e $OVERRIDE_TEXT
    exit 1
}
commit_to_check='HEAD~0'

echo -e $ANSI_COLOR_YELLOW
echo "Running git-commit pre-hook"

# Keep track of the files included in this commit, so that we can update the index if needed
files=`git diff-index --cached HEAD --name-only --diff-filter=ACMTR`
formatting_issues=""

if [ ! -z "$files" ];
then
    echo -e "Running clang-format... $ANSI_COLOR_DEFAULT"
    echo

    # Run clang-format and prompting the user for auto-fixes, if needed
    #       Note: have to specify /dev/tty as stdin
    git-clang-format --commit "$commit_to_check" -p --style file < /dev/tty
    
    # Refresh index and include any changes clang-format may have made
    git update-index --again "$files"
    
    # Run git-clang-format again to check if everything has been fixed
    formatting_issues=$(git-clang-format --commit "$commit_to_check" --diff --style file)
    for msg in ${ALL_GOOD_STRINGS[*]}
    do
        formatting_issues=$(echo "$formatting_issues" | grep -v --color=never "$msg")
    done
else
    echo -e "No files modified; won't run clang-format $ANSI_COLOR_DEFAULT"
fi

if [ -z "$formatting_issues" ]; # No issues, let it go through
then
    echo -e $ANSI_COLOR_GREEN
    echo "Passed all formatting checks"
    echo -e $ANSI_COLOR_DEFAULT
    exit 0
else # Still have (more) issues
    echo "$formatting_issues"
    echo -e $ANSI_COLOR_RED
    echo "Please fix all code formatting issues before committing"
    echo -e $OVERRIDE_TEXT
    exit 1
fi


