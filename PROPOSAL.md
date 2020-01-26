# Project: Orchestra
Play music with each computer playing one part, together forming an electronic orchestra.

With school lab settings, people can walk around the room and listen to each part individually and also stand back to hear everything together.

# Usage
_Note: "[ins]" means insert instructions on how to do the described task._

Start the server program. Wait for clients to connect.

[ins]

Start the client program and connect to a server.

[ins]

Load file on server and start playing!

[ins]

# Notes to developers

## Prefix each commit message with one of the following 
(adopted from [Google's Angular JS project](https://github.com/angular/angular/blob/master/CONTRIBUTING.md#commit))

- __docs__: Documentation only changes
- __feat__: A new feature
- __fix__: A bug fix
- __perf__: A code change that improves performance
- __refactor__: A code change that neither fixes a bug nor adds a feature (ex. change in code structure)
- __style__: Changes that do not affect the meaning of the code (white-space, formatting, missing semi-colons, etc)
- __exp__: Relates to experiment code, which does not affect the main project

# Dev Plan

## Notes
Run fluidsynth with pulseaudio:
```bash
fluidsynth -a pulseaudio -m alsa_seq [midi] [soundfont]
```

Compile pulseaudio api code with -lpulse lib; if using simple, also use -lpulse-simple lib
Ex.
```bash
gcc driver.c -lpulse -lpulse-simple
```

Error in mftext: metaspecial should not have attr "type"

## Things you wish you knew before this project
sudo apt install libfluidsynth-dev

(I don't understand why this line isn't documented and it took me so long to figure out how to get the fluidsynth lib)

## Notes
- Use .mid (MIDI) files for musical info
  - some midi files to test: http://www.kunstderfuge.com/beethoven/variae.htm#Symphonies
- Use PulseAudio for playback (installed on school ubuntu computers)
- Soundfont (as of now): [timbres_of_heaven](http://midkar.com/soundfonts/)
Readline: 
- sudo apt-get install libreadline6 libreadline6-dev


- Use <assert.h> to assert.

TODOS:
1. implement frees
2. uses ncurses and readline
3. server
4. "modernize" midifile.c; it was from like 1989

## Libraries to use

- fluidsynth, for midi playback
  - http://www.fluidsynth.org/api/index.html
- readline, for command inputs and prompt display 
  - https://tiswww.case.edu/php/chet/readline/readline.html#SEC14
  - http://man7.org/linux/man-pages/man3/readline.3.html
- pulseaudio api, for managing audio playback
  - https://freedesktop.org/software/pulseaudio/doxygen/
  - http://gavv.github.io/articles/pulseaudio-under-the-hood/

- Roc for network audio streaming
  - https://roc-project.github.io/roc/docs/api.html
  - https://github.com/roc-project/roc/blob/master/src/lib/example/receiver_sox.c

## Nice features to have

- use a cli prompt to progress between each stage, and each stage corresponds to a command
- display info such as file status, connection status, in a full screen command line and refresh automatically and at the same time accepting commands, kind of like a cli file editor such as vim or emacs
--------------------------------------------
- easy: support for loopback

## Program stage outline

### Server stages: 
  - connecting clients
    - if no client, server will just play the music
  - load audio file
  - clients determine which part to play (or server autoselect)
  - load audio file or reload a different file
    - has to parse the file for info on how many parts there are
  - play the audio
  - close connections (so different clients can connect)
  - cleanup and exit (close all connections and files)

### Client stages:
  - on startup, connect to the audio driver on the computer
    - if this process fails, quit
  - allow for testing sound
    - possibility: play a short sound and let user confirm that audio is working fine
  - (most times) wait for server instructions
  - connect server
    - client cannot disconnect, so the server doesn't have to keep track of clients real-time

## Dev stages
___NOTE___: TOO hard to anticipate completion dates. Hopefully it will be added later.

Implement server

  - read a file
  - parse file to get midi info
  - play midi with audio driver
  - play one part of midi file
  - split midi file to different parts
---------------------------------------
  - load file into mem (for sockets)
  - network sockets
---------------------------------------
  - handle signals (add feature at this point only because program before is largely incomplete)
  - go implement client
---------------------------------------
  - refactor program to make into stages
  - cli support
    - add help messages with "help" cmd
  - full screen status display

Implement client
  - ***DO THIS AFTER IMPLEMENTING A BASIC SERVER***
  - audio driver connection and detection on startup
  - server connection and wait
  - audio testing
  - signal handling

# Technical Design

## How topics from class is used
- allocates memory for just about eveything
- use files for music storage and playback
- handles signals to make sure program runs smoothly
- doesn't use processes, but fluidsynth uses threads for playback
  - shared memory is used since threads in the same process share memory
- uses network to send playback information across computers

## Data structures / algorithms / techniques that will be used

- array for loading music data
- custom structs for music communication
- needs some sort of algorithm to coordinate music communication so the music sounds good when listened together

- will use the most important technique of "incremental program testing" :) to make sure one thing is fully working before moving on to the next

# Credits
Developers: (as of now) Ruoshui

Thanks to Peter Brooks for inspiration