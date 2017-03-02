teeterm
========
A utility that splits the I/O of one process into two pseudoterminals.

Install
-------
```
make install
```
Uninstall with:
```
make uninstall
```

Usage
-----
`teeterm COMMAND [ARGS]`

teeterm will launch `COMMAND [ARGS]`, print out the paths of two pseudoterminals, create links `pty0` and `pty1` to them in the current directory, then wait for a SIGINT (ctrl-c).
Upon termination, the links `pty0` and `pty1` will be removed.

The pseudoterminals each have access to the I/O of COMMAND. The input from either will be sent to the process, and the output from the process will be sent to both terminals.

An example usage given here is to "duplicate" a terminal:
```
teeterm picocom /dev/ttyUSB0 -b115200
(In another shell) picocom pty0
(In another shell) picocom pty1
```
For a typical serial terminal, commands may be entered into either pty and the same output is seen on both terminals.
Even more useful, other programs such as `expect` should be able to interact with one of the terminals. This would enable the user to keep a serial terminal open for development, but also run scripts that automate some of the work.

References
----------
I asked [this question](http://unix.stackexchange.com/questions/346194/how-do-i-duplicate-a-serial-terminal) on the unix stack exchange. The user "dirkt" provided an excellent related code example for the subject.
[This](http://rachid.koucha.free.fr/tech_corner/pty_pdip.html) is also a useful read on controlling programs with pseudoterminals, though a [forkpty](https://www.gnu.org/software/libc/manual/html_node/Pseudo_002dTerminal-Pairs.html) call turned out to be far more convenient than their examples.

License
-------
[The Unlicense](http://unlicense.org/). This project is truly free, and public domain.