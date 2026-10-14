# qView IPC

qView can open an opt-in local IPC socket for newline-delimited JSON messages.
The server is disabled by default.

## Starting the Server

Start qView with `--ipc-server`:

```sh
qview --ipc-server
```

Without a value, the Unix socket path defaults to:

```sh
${TMPDIR:-/tmp}/qview-${UID}.sock
```

You can pass an explicit socket path either with `=` or as the following
argument:

```sh
qview --ipc-server=/tmp/qview.sock
qview --ipc-server /tmp/qview.sock
```

If you need to pass a file path after the default socket flag, put the file
before the flag:

```sh
qview image.jpg --ipc-server
```

## Messages

Each request and response is one compact JSON object followed by a newline.

Current file path request:

```json
{"method":"currentFilePath"}
```

Success response:

```json
{"ok":true,"path":"/absolute/current/file.jpg"}
```

Failure response when no file is open:

```json
{"ok":false,"error":"no_current_file"}
```

Unknown methods return:

```json
{"ok":false,"error":"unknown_method"}
```

Invalid JSON returns:

```json
{"ok":false,"error":"invalid_json"}
```

## Zsh Example

`qview-path-get` prints the current file path and returns non-zero if qView is
not reachable, the response is malformed, or no current file exists.

```zsh
qview-path-get() {
  emulate -L zsh
  local socket="${1:-${QVIEW_IPC_SOCKET:-${TMPDIR:-/tmp/}qview-${UID}.sock}}"
  local response

  response=$(
    printf '%s\n' '{"method":"currentFilePath"}' |
      socat - "UNIX-CONNECT:${socket}" 2>/dev/null
  ) || return 1

  local ok path
  ok=$(printf '%s\n' "$response" | jq -er '.ok') || return 1
  [[ "$ok" == true ]] || return 1

  path=$(printf '%s\n' "$response" | jq -er '.path // empty') || return 1
  [[ -n "$path" ]] || return 1

  print -r -- "$path"
}
```
