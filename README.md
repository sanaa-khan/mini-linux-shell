# mini-linux-shell
A small shell for Linux, implements some of the features found in typical shells such as bash and and csh. Written in C and C++.
Once started, presents a prompt to the user where they can enter commands.

## Basic Commands
- exit : quits the shell
- pwd : prints user's current working directory
- clear : clears the screen

These are the most simple commands. All other commands are parsed and treated differently, as they may or may not have arguments.

## Other Commands
- ls \<directory> : lists the contents of the directory specified by \<directory>
- cd \<directory> : changes to the directory specified by \<directory>. If no directory is specified, changes to home directory
- environment :
  - environ : lists all the environment strings currently defined
  - setenv \<envar> \<value> : set the environment variable \<envar> to \<value>. If setenv is used with \<envar> only, then the value is an empty string
  - unsetenv \<envar> : undefine environment variable \<envar>
  
All other command line input is interpreted as program invocation which is done by the shell forking and executing the programs as its own child processes e.g *top*, *ps* etc
  
## Additional Features
- Support for I/O redirection for *stdin* and *stdout*
- *stdout* redirection also suported for internal commands e.g. pwd, ls, environ.
- Background execution of prpograms (if the command has an ampersand & at the end)
- Piping between programs via '|' e.g \<cmd1> | \<cmd2>
- Shell ignores the SIGINT signal
