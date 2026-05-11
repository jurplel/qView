# Command-line input paths

qView accepts multiple command-line paths in a single invocation:

```sh
qview 1.png ~/Pictures/7.png image-dir/
```

File arguments are opened as one navigation sequence in the order given. Directory arguments expand
to the supported images in that directory using qView's file filtering and sort settings. Missing
paths, unsupported files, and directories without supported images are skipped with diagnostics on
standard error.

Use `-` to read additional paths from standard input. Input is line-delimited, and empty lines are
ignored. The `-` marker expands in place:

```sh
printf '%s\n' ~/Pictures/a.png ~/Pictures/b.png | qview before.png - after.png
```
