# SWordle! (Socket-Wordle) 🌐🔠
Simulating multiple clients playing Wordle on a server.

## How it works
A server is created to host a Wordle game, and clients are created to connect and play. The server creates a TCP socket to listen for requests, and the client creates a TCP socket to try and connect. Multiple clients can connect and all play concurrently in different threads! Every client gets their own different wordle word.

## Communication
The client sends a guess:

[5 letter word] - 5 bytes

In response the server sends:

['Y' or 'N' if in word list | # or guesses remaining | wordle result] - 8 bytes

The rules of wordle are used: [https://www.nytimes.com/games/wordle/index.html](https://www.nytimes.com/games/wordle/index.html)

## How to play
There are 4 commmand-line arguments:

&lt;listener-port&gt; &lt;seed&gt; &lt;word-filename&gt; &lt;num-words&gt;

Representing the server port, seed for random word choosing, the wordle file name, and the number of words in that file, respectively.

Server side w/ arguments: &lt;8192&gt; &lt;117&gt; &lt;word_words.txt&gt; &lt;5757&gt;, compile and run like so:

```bash
gcc -o server.out server.c helper.c -pthread && ./server.out 8192 117 wordle_words.txt 5757
```

Client side, compile and run like so:

```bash
gcc -o client.out client.c && ./client.out
```

## Example
Here is a game with the arguments from above and two clients:

Server side:

```txt
MAIN: opened wordle_words09.txt (5757 words)
MAIN: Wordle server listening on port {8199}
MAIN: rcvd incoming connection request
THREAD 139743450404416: waiting for guess
THREAD 139743450404416: rcvd guess: stare
THREAD 139743450404416: sending reply: s---E (5 guesses left)
THREAD 139743450404416: waiting for guess
THREAD 139743450404416: rcvd guess: pulse
THREAD 139743450404416: sending reply: ---SE (4 guesses left)
THREAD 139743450404416: waiting for guess
THREAD 139743450404416: rcvd guess: house
THREAD 139743450404416: sending reply: ho-SE (3 guesses left)
THREAD 139743450404416: waiting for guess
MAIN: rcvd incoming connection request
THREAD 139743442011712: waiting for guess
THREAD 139743442011712: rcvd guess: stern
THREAD 139743442011712: sending reply: --e-- (5 guesses left)
THREAD 139743442011712: waiting for guess
THREAD 139743442011712: rcvd guess: fleck
THREAD 139743442011712: sending reply: --eC- (4 guesses left)
THREAD 139743442011712: waiting for guess
THREAD 139743442011712: rcvd guess: voice
THREAD 139743442011712: sending reply: ---CE (3 guesses left)
THREAD 139743442011712: waiting for guess
THREAD 139743442011712: rcvd guess: bocce
THREAD 139743442011712: sending reply: --cCE (2 guesses left)
THREAD 139743442011712: waiting for guess
THREAD 139743442011712: rcvd guess: deice
THREAD 139743442011712: sending reply: DE-CE (1 guess left)
THREAD 139743442011712: waiting for guess
THREAD 139743442011712: client gave up; closing TCP connection...
THREAD 139743442011712: game over; word was DEUCE!
THREAD 139743450404416: rcvd guess: mouse
THREAD 139743450404416: sending reply: -o-SE (2 guesses left)
THREAD 139743450404416: waiting for guess
THREAD 139743450404416: rcvd guess: moose
THREAD 139743450404416: sending reply: -oOSE (1 guess left)
THREAD 139743450404416: waiting for guess
THREAD 139743450404416: rcvd guess: worse
THREAD 139743450404416: sending reply: -o-SE (0 guesses left)
THREAD 139743450404416: game over; word was CHOSE!
MAIN: SIGUSR1 rcvd; Wordle server shutting down...
MAIN: Wordle server shutting down...

MAIN: guesses: 11
MAIN: wins: 0
MAIN: losses: 2

MAIN: word(s) played: CHOSE DEUCE
```

Client side:

```txt
CLIENT: connecting to server...
CLIENT: sending to server: stare
CLIENT: response: s---E -- 5 guesses remaining
CLIENT: sending to server: pulse
CLIENT: response: ---SE -- 4 guesses remaining
CLIENT: sending to server: house
CLIENT: response: ho-SE -- 3 guesses remaining
CLIENT: connecting to server...
CLIENT: sending to server: stern
CLIENT: response: --e-- -- 5 guesses remaining
CLIENT: sending to server: fleck
CLIENT: response: --eC- -- 4 guesses remaining
CLIENT: sending to server: voice
CLIENT: response: ---CE -- 3 guesses remaining
CLIENT: sending to server: bocce
CLIENT: response: --cCE -- 2 guesses remaining
CLIENT: sending to server: deice
CLIENT: response: DE-CE -- 1 guess remaining
CLIENT: disconnecting...
CLIENT: sending to server: mouse
CLIENT: response: -o-SE -- 2 guesses remaining
CLIENT: sending to server: moose
CLIENT: response: -oOSE -- 1 guess remaining
CLIENT: sending to server: worse
CLIENT: response: -o-SE -- 0 guesses remaining
CLIENT: game over!
CLIENT: disconnecting...
```
