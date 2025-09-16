# OS Assignment - 2
## Simple Shell

This is a simple shell that executes simple commands (that are defined in /bin). Note that it does not implement commands like 'cd', etc and instead shows a user-friendly error (as its a simple shell for simple commands).  
The simple shell also supports pipes using `pipe()` command and file descriptors. Each command is executed within a child process using `fork()` and `execvp()`. We store each command in the history with information like its pid, execution time etc.

Then, we split the line about the pipes (`|`) (if any) and then create the pipes themselves. Then we parse each command by splitting it along white spaces. We then create a child process for each command using `fork()` and then we actually execute that command using `execvp()`. We also connect the pipes by replacing the stdin/stdout with the pipes of previous/next command. We also handle commands like `history` and print the history on termination (using Ctrl+C).

---

### Contributions
The work was equally divided with all aspects of the assignment done together.

**Group 44**
- Apaar Gulati - 2024095
- Bhavya Yadav - 2024152

### Github Repository
https://github.com/Krypton018/OS-Assignments/tree/main/OS-Assignment-2