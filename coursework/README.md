# Coursework. Chat over Shared Memory

## Task

A chat server and clients that communicate only through one POSIX shared memory segment.
The server owns the segment, assigns user slots, routes private messages and keeps the message history.
Each client logs in with a unique name and sends messages to other online users.

## Build and run

```bash
make run LAB=coursework APP=server
make run LAB=coursework APP=client
```

Both programs accept an optional segment name (`argv[1]` or `CHAT_SHM_NAME`, default `/chat_shm`).
Client commands: `<recipient>:<message>`, `/history <keyword>`, `quit`.

## Example

Client `alice` types `bob:hello`, the server prints:

```text
[system] Server started
[user] alice has logged in
[user] bob has logged in
[message] alice -> bob: hello
```

Client `bob` prints:

```text
[message] alice: hello
```

## Notes

- Logins go through the server: a client sends `LOGIN` and waits for a reply with its slot. Replies left by clients that died before reading them are reused.
- `/history <keyword>` returns up to 20 of the latest messages that the user sent or received and that contain the keyword; other users' conversations stay private.
- All queues are bounded ring buffers with robust process-shared mutexes and `not full`/`not empty` conditions.
- The server refuses to start while another server owns the segment and removes a segment left by a dead server.
- `Ctrl+C` stops the server (clients are notified) and logs a client out. Clients that die are reaped by the server, and clients notice a server that died without cleanup.
