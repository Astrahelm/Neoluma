# <div align="center"><img src="neoluma_full.svg" alt="Neoluma" width="80%" /></div>
<p align="center" style="font-style: italic">(from et.: gr. neo- & lat. -lumen, aka "New Light")</p>

**A statically typed, multi-paradigm, general-purpose programming language** supporting both compiled and interpreted execution, with high-level ergonomics and low-level control, designed to be *a language for everything*.

### <div align="center">[Website](https://astrahelm.org/Neoluma) | [License](./LICENSE) | [Discord](https://discord.gg/zmrB9dbmy5)</div>

Neoluma aims to be a unified language that bridges low-level control and high-level ergonomics. It is designed for:

- Systems programming (including OS kernels)
- Cross-platform apps and utilities
- Educational tools and experimentation
- Scripting with performance
- Future self-hosting development
- ...and a lot more things, that modern languages can do everyday.

**Neoluma is in early development, though,** and not many features are available. Contributions and feedback are welcome!

## <span class="emoji" data-emoji="✨">✨</span> Features
- **Clean, modern syntax** - inspired by Python, Typescript, and C#;
- **Static typing with optional type inference** - for simplicity and safety;
- **Rich type system** - such as integers, floating-point and fixed-point numbers, booleans, strings, arrays, sets, dictionaries, error-catching `result` and more;
- Powerful decorators like `@entry`, `@comptime`, `@unsafe`, and others;
- Flexible preprocessor directives for low-level and modular control:
  - `#import` - import internal or external modules, even from other languages! (e.g., C++ bindings);
  - `#unsafe` - enable low-level operations;
  - `#macro` - define macros for compile-time logic.
- **Built-in async/await** - for concurrency;
- **Compile-time evaluation and an evolving macro system (WIP)**;
- **Configurable memory management** - *automatic* or *manual*, with *borrow checker* or *not*, you choose! (from what is available, of course);
- **Cross-platform compilation with LLVM backend** - allows for native performance and ability to run anywhere;
- **IDE support** - syntax highlighting, debugging, and project tools, to make development more enjoyable and efficient.

## <span class="emoji" data-emoji="📃">📃</span> File Formats

- `.nm` — **Neoluma Module**: source code
- `.nlp` — **Neoluma Project**: project structure/configuration
```
project/
├── main.nm
├── utils.nm
└── project.nlp
```

## <span class="emoji" data-emoji="🧩">🧩</span> Example

```neoluma
@entry
fn main() {
    name = "Neoluma";
    print("Hello, ${name}!")
}
```
> If the function doesn't have a return type, it is `void` by default — no `return` required.
> Also, if a function has an @entry decorator, the program starts with it's execution, otherwise it will search for main().
```bash
$ neoluma run main.nm         # Run it the interpreted way to check
Hello, Neoluma!
$ neoluma build project.nlp   # or compile it as an executable!
...
```

## <span class="emoji" data-emoji="🗺️">🗺️</span> Roadmap

- [x] Lexer
- [x] Parser
- [x] Semantic Analyzer
- [ ] LLVM IR Compiler *[In progress]*
- [ ] CLI Toolchain
- [ ] Cross-platform support (Linux / Windows / macOS / Android / iOS / etc.)
- [ ] Plugin system and editor integration
- [ ] Transpilation support (C++, JS, etc.)

## <span class="emoji" data-emoji="🔧">🔧</span> Building from source

Neoluma can be built from cloned source via `dotnet` command.
Just use `dotnet build` in the root directory to build your branch!

## <span class="emoji" data-emoji="📖">📖</span> Credits

**Created by [TsukimotoX](https://github.com/TsukimotoX) under [Astrahelm Project](https://github.com/Astrahelm/) and [Apache License](./LICENSE)**. 

**Neoluma™** is a [trademark](./TRADEMARK.md) of the **Neoluma** project; the name and branding are **not granted** under the Apache-2.0 license.

More documentation and compiler features coming soon!
**Join our [Discord server](https://discord.gg/zmrB9dbmy5)!**

This project uses [LLVM](https://llvm.org/) under the Apache License v2.0 with LLVM Exceptions.

<style>
.emoji {
    position: relative;
    display: inline-block;
    font-size: 28px;
    line-height: 1;
}
.emoji::after {
    content: attr(data-emoji);
    position: absolute;
    inset: 0;
    background: linear-gradient(#4A2BD6, #3b82f6);
    -webkit-background-clip: text;
    background-clip: text;
    color: transparent;
    -webkit-text-fill-color: transparent;
    mix-blend-mode: hard-light;
    pointer-events: none;
}

a {
    background: linear-gradient(90deg, #4A2BD6, #3b82f6);
    background-clip: text;
    -webkit-background-clip: text;
}
</style>