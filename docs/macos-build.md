# macOS Build

`build.sh` can build the app bundle, deploy the Qt runtime into it, and install
the deployed bundle locally.

Build only:

```sh
./build.sh
```

Build and deploy a self-contained `build/qView.app`:

```sh
./build.sh --deploy-macos
```

Build, deploy, and install to `/Applications/qView.app`:

```sh
./build.sh --install-macos
```

Install somewhere else:

```sh
./build.sh --install-macos="$HOME/Applications/qView.app"
```

By default, deployed bundles are ad-hoc signed with `codesign --sign -`. Pass a
different identity with `--macos-codesign=<identity>`.
