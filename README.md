# teller

The Hydrogen language compiler.

Current step to build, run, and access the result of program `test.hy`:

```
$ cmake -S . -B build/ && cmake --build build/
$ ./build/hydro <input.hy>
$ ./out
$ echo $?
```

Compiler made following the video series created by [_Pixeled_](https://www.youtube.com/@pixeled-yt).
