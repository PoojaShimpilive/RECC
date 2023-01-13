# Contributing to `recc`

If you'd like to help us improve and extend this project, then we welcome your contributions!

Below you will find some basic steps required to be able to contribute to the project. If
you have any questions about this process or any other aspect of contributing to a Bloomberg open
source project, feel free to send an email to opensource@bloomberg.net and we'll get your questions
answered as quickly as we can.

Before committing your changes, please make sure to run `clang-format` on them. For more information
take a look at the [Code Formatting](#code-formatting) section.

## Contribution Licensing

Since `recc` is distributed under the terms of the [Apache Version 2 license](LICENSE), contributions that you make
are licensed under the same terms. In order for us to be able to accept your contributions,
we will need explicit confirmation from you that you are able and willing to provide them under
these terms, and the mechanism we use to do this is called a Developer's Certificate of Origin
[DCO](DCO.md).  This is very similar to the process used by the Linux(R) kernel, Samba, and many
other major open source projects.

To participate under these terms, all that you must do is include a line like the following as the
last line of the commit message for each commit in your contribution:

    Signed-Off-By: Random J. Developer <random@developer.example.org>

The simplest way to accomplish this is to add `-s` or `--signoff` to your `git commit` command.

You must use your real name (sorry, no pseudonyms, and no anonymous contributions).

## Help / Documentation

The [README](README.md) contains instructions to help you get started with `recc`. For a more
detailed description of what `recc` does and how to customize its behavior, run `recc --help`.

If you need more or different information, please create an [Issue][].

[issue]: https://gitlab.com/BuildGrid/recc/issues

## Contribution tips

### Code Formatting

This project uses `clang-format` to ensure consistent code formatting. 
Please make sure you have installed `clang-format` version `>= 6` and that `git-clang-format`
is in your `$PATH`.

A git pre-commit hook is provided with the repo to make it easy for all the contributors to
automatically fix possible formatting issues. To install the pre-commit hook, simply run:

```sh
scripts/install-hooks.sh
```

