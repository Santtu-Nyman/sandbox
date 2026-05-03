Foundation Library Project Directory Structure:
 - "FoundationLibrary/include/" (Header files of the Foundation Library)
 - "FoundationLibrary/source/" (Implementation source files of the Foundation Library)
 - "FoundationLibrary/ut/" (Unit tests of the Foundation Library)
 - "FoundationLibrary/ut/build_ut.bat" (Unit test build script)
 - "FoundationLibrary/ut/build" (UT build output binary directory)

Coding Conventions:
 - Write code in C99 (ISO/IEC 9899:1999) and compiler specific intrinsics following professional low-level systems programming practices.
 - Make sure that the code written is robust quility intended for production.
 - For written code the robubustnes is always the first priority and performance should be always the second priority.
 - When possible write functional code to avoid side-effect.
 - When possible avoid modifying any global state in code.
 - Avoid complexity when writing code. Prefer simple solutions.
 - Prefer simple data structures over complex ones.
 - Prefer POD types over non-POD types. When using non-POD type there should be a good reason for it.
 - Try to reuse existing code when possible instead of writing new code.
 - Avoid writing unnecessary code. Only write code when it is needed to implement a feature.
 - Prefer using types from <stddef.h> and <stdint.h>.
 - Prefer using size_t for indexing when the maximum index is unknown at run-time.
 - Never use thread-local storage.
 - Use Allman style braces.
 - Use screaming snake casing ("SCREAMING_SNAKE_CASING") for naming macros, enum types and enum values. 
 - Use Pascal casing with initial uppercase letter ("PascalCasing") when naming functions, structure types. 
 - Use camel casing with initial lowercase letter ("camelCasing") when naming any variables.
 - Use upper casing for hexadecimal digits numbers.
 - Use meaningful and descriptive names for functions, variables and macros.
 - Prefer clarity over brevity for identifier names. Never use abbreviated or shortened versions of words for identifier names.
 - Do not use any acronyms that are not widely accepted, and even if they are, only when necessary.
 - Avoid using single-letter names, except for simple loop counters and spacial coordinates. Letter n maybe used as the number of iterations on a loop like "for(size_t n = GetItemCount(list), i = 0; i < n; i++)".
 - Whenever an object or function is declared or defined, its type shall be explicitly stated.
 - Functions with no parameters shall be declared and defined with the parameter list void.
 - A pointer parameter in a function prototype should be declared as pointer to const if the pointer is not used to modify the addressed object.
 - Do not hide dereference operation inside macros or typedefs.
 - Identifiers in an inner scope shall not use the same name as an identifier in an outer scope, and therefore hide that identifier.
 - The functions strdup, strndup and strerror from <string.h> shall not be used.
 - Assume that the target platform endianness is little-endian.
 - Assume that the target platform is x86 with SSE4.2 (defined by C macro __i386 or C macro _M_IX86), x64 with SSE4.2 (defined by C macro __x86_64__ or C macro _M_X64) or ARM64 with Advanced SIMD extension (Neon) (defined by C macro __aarch64__ or C macro __ARM_ARCH).
 - Assume the compiler is MSVC (defined by C macro _MSC_VER), Clang (defined by C macro __clang__) or GCC (defined by C macro __GNUC__).
 - Assume that dynamic memory allocations are 16 byte aligned.
 - Unreachable code must be marked as unreachable with a macro that expands to __assume(0) or __builtin_unreachable() depending on the compiler.
 - Use the appropriate namespace (Fl prefix) for the current project.
 - Always name constant values.
 - Strings should always UTF-8 unless other character encoding is required for an external API.
 - Strings should always be accompanied by the length of the string in function parameters and in structure members.

Error Handling:
 - Avoid run-time failures by avoiding acquiring unnecessary runtime resource. Avoid allocating dynamic memory when it is not needed. Avoid opening handles they are not needed.
 - Avoid run-time failures by trying to move possible failure cases to compile-time if possible.
 - Avoid run-time failures by trying to move possible failure cases to program initialization if it is not possible to move the failure case to compile-time. Avoid writing possible run-time failure cases after program initialization.
 - Errors should be propagated by function return values. These error codes should be indicated using HRESULT. Include "FlHRESULT.h" instead of <Windows.h> for HRESULT, this includes <Windows.h> for Windows and has replacements for other targets. Note that when needed HRESULT_FROM_WIN32 can be used to conver Win32 error codes to HRESULT.
 - Functions should always cleanup before returning from recoverable errors.
 - Validate all external inputs at the boundary.
 - Avoid use of dynamic memory allocation after process initialization when possible.
 - For functions that can not fail with valid input and do not have side-effects do not return any error codes. They should assert instead.
 - Always check return values of functions returning error codes.

Security:
 - Write fault tolerant code. The code written should be able to recover from runtime errors that happen after the program has finished initialization.
 - Avoid writing code that may have unexpected run-time behavior.
 - Avoid making calls into external code that may return run-time error whenever possible.
 - Make sure that the written code can execute reliabiably over a long period of time.
 - Use static type checking.
 - Avoid calling any code that relies on the user locale. To ensure predictable and secure results always use explicit locale.
 - Use static and dynamic assertions as sanity checks.
 - Never trust user input. Always validate user input before using it.
 - Ensure security by always validating any data received over a network.
 - Check the return values of non-void functions.
 - Use SAL (Microsoft source code annotation language) annotations to reduce C/C++ code defects. Include "FlSAL.h" for the annotation macros, this includes <sal.h> for MSVC and has replacements for other compilers.

Performance:
 - Avoid writing pessimized code, by avoiding commonly known performance issues.
 - Never acquire runtime resource inside a hot loop. Runtime resources needed inside a hot loop must be preallocated before entering the loop.
 - Be aware what code may block and avoid blocking inside loops. Be sceptical of external code. Don't trust it right be non-blocking or to have predictable execution time.
 - Avoid making unnecessary copies of chunks of data or large objects.
 - When passing chunks of data or large objects to functions pass by reference/pointers.
 - Prefer writing code that accesses memory in predictable linear pattern.
 - Avoid prematurely multi-threading code. Code should still be thread-safe for possible future multi-threading.
 - Never create any dedicated IO threads. If non-blocking IO is needed OS level asynchronous IO should be used.
 - Avoid using strings as internal data format unless needed. Prefer using binary data over strings.
 - Prefer using 32 bit floats over 64 bit floats.

Documentation:
 - Always write documentation for function declarations.
 - Always document header file function declarations in detail similar to Linux manual pages or the official Win32 API documentation.
 - Along with the function documentation annotate function parameters and return values with SAL.
 - Always verify that that documenatation you write is consistent with SAL.

Testing:
 - Code should be tested with UTs.
 - Ensure security of production code by always validating user input and by creating UTs which try to break the code being tested.
 - When creating UTs plan how to cover all corner cases.
 - Never modify or remove existing UTs.
 - After writing or modifying code double check that it follows these rules and run unit tests for it.
 - The UTs are not performance critical. UT cases do not need to follow the performance related coding conventions unlike all other code.
 - In UTs avoiding dynamic memory allocation is not necessary, unlike in other code.
 - After finishing any code modifications always build and run all UTs to verify that UTs are all passing. Use "FoundationLibrary/ut/build_ut.bat" to build and run UTs.
 - Before reporting a task complete, verify it actually works by running UTs.
 - If compiling UTs fail or there are aby failing UT cases fix resolve these issues.
 - Report outcomes faithfully. If tests fail, say so with the relevant output; if you did not run a verification step, say that rather than implying it succeeded.

Tools:
 - Use "FlSAL.h" instead of <sal.h> for annotation.
 - Use "FlHRESULT.h" instead of <Windows.h> for HRESULT.
 - Use MSVC compiler from Microsoft Visual Studio 2022 (64-bit)
 - Use .bat scripts for builds. "FoundationLibrary/ut/build_ut.bat" is to build and run UTs.
