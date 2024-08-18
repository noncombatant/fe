declarations at first use

const as much as possible

More example programs and tests

Use // comments

Replace `car` and `cdr` with `head` and `tail`.

Unit test all fe functions

When available, use `constexpr` instead of `#define`

Use readline in main

Reduce/eliminate C macros — replace the remaining lvalue accessors with `Set*`

Finish adding the rest of math.h.

Add (at least) `fread`, `fwrite`, `fflush`.

Consider namespaces, e.g. `math.*`, `io.*`, `sys.*`, et c.

Update docs given new names and types et c.

Turn the examples in the docs into scripts, and test them, and then refer to
them in the docs in place of the code.

help strings for symbols

Add GMP to Fex for better numbers.

Use bitfields instead of shifts and ors and so on

Ensure that everything declared in fe.h really needs to be public.

There's a minimum size for the arena; document and check it:

woop:~/code/fe % ./fe -s 1024 scripts/fib.fe 
UndefinedBehaviorSanitizer:DEADLYSIGNAL
==30053==ERROR: UndefinedBehaviorSanitizer: SEGV on unknown address 0x7ff75b000000 (pc 0x0001041be4a1 bp 0x7ff7bbd47370 sp 0x7ff7bbd47310 T2700168)
==30053==The signal is caused by a WRITE memory access.
    #0 0x1041be4a1 in fe_open+0x1c1 (fe:x86_64+0x1000064a1)
    #1 0x1041b87fc in main+0xec (fe:x86_64+0x1000007fc)
    #2 0x7ff80980c365 in start+0x795 (dyld:x86_64+0xfffffffffff5c365)

==30053==Register values:
rax = 0x0000000000000000  rbx = 0x00007ff75a809058  rcx = 0x0000000000000001  rdx = 0x00007ff75a809020  
rdi = 0x00000000007f6fa0  rsi = 0x00007ff75a809028  rbp = 0x00007ff7bbd47370  rsp = 0x00007ff7bbd47310  
 r8 = 0x00007ff75a809038   r9 = 0x0800000000000000  r10 = 0xf800000000000001  r11 = 0x00000001041c3c80  
r12 = 0x080000000007f6fa  r13 = 0x00007ff75afffff8  r14 = 0x00007ff75b000000  r15 = 0x00007ff75affffe8  
UndefinedBehaviorSanitizer can not provide additional info.
SUMMARY: UndefinedBehaviorSanitizer: SEGV (fe:x86_64+0x1000064a1) in fe_open+0x1c1
==30053==ABORTING
zsh: abort      ./fe -s 1024 scripts/fib.fe