# X86 TSO
x86 使用的是 TSO (Total Store Ordering), 而 ARM 是 weak memory ordering 。
苹果在 Apple Silicon (M1/A12Z)中内置了一个状态寄存器来模拟 TSO