# suitui

A terminal UI library for [Seal](https://codeberg.org/seal-lang/seal).

Early days. Things break and change daily.

```
git clone --recursive https://github.com/Huseynteymurzade28/suitui
cd suitui
make
make run EX=01
```

The library is written in Seal (`tui.seal`, `key.seal`). `termio.so` is a small C module for
the few things Seal can't do on its own yet: raw mode, reading a byte with a timeout and the
terminal size. The goal is to shrink it as Seal grows.
