namespace Neoluma.Core;

/// Compiler settings for the project tell the compiler what to set up before building
public class CompilerSettings {
    ///<c>Verbose</c> is an option that allows extra keywords, syntactic sugar. Not recommended for people who loves keeping shape of a language. Recommended for people who don't give a heck and love copypasting answers from StackOverflow.
    ///<br/> <c>true</c> - enables this option
    ///<br/> <c>false</c> - disables this option
    public bool verbose = false; 
        
    ///<c>Baremetal</c> is an option that toggles the ability to develop software for bare metal hardware. It disables platform-based ABI (including std) and compiles into binary.
    ///<br/> <c>true</c> - enables this option
    ///<br/> <c>false</c> - disables this option
    public bool baremetal = false;
    
    public class Memory {
        public enum MemoryOptions { None, Rusty, ARC, Default };
        
        ///<c>level</c> is an option of <c>Memory</c> class that goes through types of Memory options and, depending on chosen option, will manage the memory in the application the preferred way.
        ///<br/> <c>Default</c> - enables default Garbage Collector (Java, C#, others)
        ///<br/> <c>ARC</c> - enables Automatic Reference Counter (Python, JS, others)
        ///<br/> <c>Rusty</c> - enables Borrow Checker (Rust)
        ///<br/> <c>None</c> - No memory management tools (C, C++, maybe others)
        public MemoryOptions level = MemoryOptions.Default;
    };
}