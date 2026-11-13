# suitui

A terminal UI library for [Seal](https://codeberg.org/seal-lang/seal).

Early days. Things break and change daily.

```
git clone --recursive https://github.com/Huseynteymurzade28/suitui
cd suitui
make
make run EX=01
```

`term.so` is the C core (raw mode, input, drawing). It is loaded from Seal with `include "term"`.
