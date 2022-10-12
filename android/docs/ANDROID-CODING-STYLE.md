Coding Style Guide for the Android emulator:
--------------------------------------------

Introduction:
-------------

**IMPORTANT:**
    This guide only applies to the Android-specific code that is currently
    found under $AOSP/external/qemu/android/ only. Other parts of
    external/qemu/ should adhere to the QEMU coding style, whenever possible.

    See the section name "Source Code Organization" for more details.

This document serves as a *guide* for anyone working on the Android-specific
portions of the Android emulator source code. Don't forget that consistency
for the sake of consistency is a waste of time. Use this information as a way
to speed up your coding and code reviews, first and foremost.

A `.clang_format` file is provided to automatically reformat your modified code
through `git clang-format`. Use it, this will save you a lot of time.

Also, there is no real need to reformat existing code to adhere to the guide.
Would you feel the need to do so, do it in independent patches that batch all
reformatting without any bugfix / changes at all.

Each rule below comes with a rationale. Again, strict adherence is not
necessary.

**IMPORTANT:** You need to use `git-config` to ensure `git clang-format` works
           correctly, see dedicated section below.


I. History:
-----------

This guide is a mix of the Android C++ coding style guide (not publicly
available) and the Chromium one, available at [1].

In case of doubt, fallback to the Chromium style guide.


II. Source Code Organization:
-----------------------------

It's important to understand when and how to apply this style guide.

The 'classic' Android emulator code base lives under $AOSP/external/qemu/,
with most of the Android-specific code under $AOSP/external/qemu/android/.

The 'qemu2' Android emulator code base lives under $AOSP/external/qemu/
and is *NOT* affected by this coding style guide.

  - `external/qemu/aemu/base/`

      Set of low-level C++ interfaces to system facilities.
      These should be completely self-contained (i.e. not depend on other
      parts of the code base), and form the 'base platform' to rely on.

      This is very similar (but simpler) than the Chromium base/ component.

      **IMPORTANT:* Any new addition here should come with unit-tests that
                   will be run each time android/rebuild.sh runs.
                   Also see `aemu/base/testing/` for helper classes to use
                   inside your unit tests.

      Generally speaking, the use of C++ makes it easier to provide abstract
      interfaces that can be replaced at runtime by mock versions. Doing so
      in C is also possible, but requires more typing / is more error-prone.

      This is very useful to be able to unit-test a sub-component without
      having to run a full emulator session. In particular try to use the
      features of base/system/System.h to access low-level system features,
      this interface can be easily mocked during unit-tests by creating
      an instance of base/testing/TestSystem.h, allowing your tests to not
      depend on the state of the development machine whenever possible.

      *NOTE:* Do not put high-level features here. Imagine you would reuse the
            classes in this directory in a completely different program.

  - `external/qemu/android/utils/`:

      Similar low-level C-based interfaces, some of them are actual wrappers
      for aemu/base/ features. Should not depend on anything else besides
      system interfaces.

      *NOTE:* Do not put high-level features here.

  - `external/qemu/android-qemu1-glue/base/`:

      Currently contains QEMU-specific implementations of aemu/base/
      interfaces. E.g. `android-qemu1-glue/base/async/Looper.h` provides
      `android::qemu::createLooper()`, which returns a Looper instance
      based on the QEMU main event loop.

      In this specific example, the idea is to use `ThreadLooper::set()` with
      its result to inject it into the main thread. Any code that uses
      `ThreadLooper::get()` to retrieve the current thread's Looper will use the
      QEMU-based main loop when it runs within the emulator's main thread,
      even if it doesn't know anything about QEMU.

      This kind of decoupling is very useful and should be encouraged in
      general.

  - `external/qemu/android/<feature>/`:

      Should contain code related to a specific feature used during emulation.
      Ideally, the code should only depend on aemu/base/ and android/utils/,
      stuff as much as possible. Anything that is QEMU-specific should be moved
      somewhere else.

      A good example is the GSM/SMS emulation code. Generic code is provided
      under `external/qemu/android/telephony/` (with unit-tests), while
      QEMU-specific bindings are implemented under `external/qemu/telephony/`
      which depend on it, and other QEMU headers / functions.

      *NOTE:* It's essentially impossible to properly unit-test code that depends
            on any QEMU-specific function, so move as much of your code as
            possible out of it.

  - external/qemu/android/

      The rest of the sources under this sub-directory are Android-specific
      and is probably still a little bit messy. We plan to sort this out by
      moving, over time, anything specific to QEMU outside of this directory.

  **IMPORTANT:** The end goal is to ensure that the sources under
               `external/qemu/android/` will constitute a standalone component
               that can be compiled as a standalone library, that doesn't
               depend on the QEMU code, or its dependencies (e.g. GLib).
               Ultimately, we plan to link the QEMU2 code base to the same
               library, in order to ensure the same feature set for both the
               classic and new emulator engines.

  - When adding new code, try hard to:

    - Avoid using QEMU-specific functions directly. It's better to decouple
      a 'clean' feature implementation, which can be unit-tested independently,
      and a QEMU-specific binding / user code for it (which will ultimately
      be re-implemented for QEMU2 as well).

    - Avoid using system interfaces directly. Use the abstractions provided
      by aemu/base/ and android/utils/ instead, since they can more easily
      be mocked during unit-tests, making our regression test suite more
      robust / less dependent on host/development system specifics.

  The style guide applies to any new code you want to add under
  `external/qemu/android/` that should not depend on QEMU-specific headers and
  declarations.


  Finally, when adding new code, favor C++ over C if you can. The reason is
  that it's easier to provide abstract / mockable interfaces in this language
  compared to C. This leads to better de-coupling and unit-tests.


III. Rules:
-----------

As said previously, if something is not specified here, fall back to the
output of clang-format or the Chromium coding style guide at [1], otherwise
ask on the public mailing list for information.

1. Basic Code Formatting:

  *NOTE: Most of these should be handled automatically by 'git clang-format' !!
        (See dedicated section below).*

  - No TABs in source code allowed (except for Makefiles).
    Rationale: Life is too short to play with editor settings all the time.
    See also [tabs vs spaces](https://www.youtube.com/watch?v=SsoOG6ZeyUI).

  - Indent width is 4 characters.
    Rationale: Let's choose one reasonable default and stick with it.

  - Continuation indentation should be 8 characters.
    Rationale: This makes it slightly easier to detect that the second line
               is really the continuation from the first one.

    E.g.:

      **YES**:
      ```
      SomeSuperLongTypeName someSuperLongVariableName =
              CreateAnInstanceOfSomeSuperLongTypeName(withParameter);
      ```

      **NO**:
      ```
      SomeSuperLongTypeName someSuperLongVariableName =
          CreateAnInstanceOfSomeSuperLongTypeName(withParameter);
      ```

  - Static word wrap at 80 characters.
    Rationale: That's good enough, let's not fight about it.

  - One space around each binary operator, e.g.:

       x = y + z - 4;

  - The opening curly brace is at the end of the line, e.g.:
    ```
    void foo(int x) {
      return x + 42;
    }
    ```

    Rationale: Let's just choose one convention, this one saves a little
               vertical space.

  - Single-statements conditionals can go into a single line, and always use
    accolades:

        if (error) {                   [YES]
            return ENOPE;
        }

        if (error) { return ENOPE; }   [YES]

        if (error)
            return ENOPE;              [NO]

        if (error) return ENOPE;       [NO]

        if (error) { return ENOPE; } else { return 0; }  [NO]  // not single-statement

        if (error) {                    [YES]
            return ENOPE;
        } else {
            return 0;
        }

        return error ? ENOPE : 0;       [YES]

    Rationale: Reduces the chance of introducing subtle bug later when
               adding more statements. Sadly this still happens occasionally
               to everyone.

  - The closing curly brace should appear at the start of its own line, or at
    the end of the line containing the opening brace:

    E.g.:
        Foo::Foo() {}         [YES]

        if (cond) {           [YES]
            doStuffA();
        } else {
            doStuffB();
        }

        if (cond) { return error; }  [YES]

  - The * and & go by the type rather than the variable name, e.g.:
     ```
       void foo(const char* str);       [YES]
       void foo(const char * str);      [NO]
       void foo(const char *str);       [NO]
     ```
    Rationale: it's more logical since they're really part of the type.

    Special exception: a space is allowed for casts, as in:

        return static_cast<const char*>(foo);   [YES]
        return static_cast<const char *>(foo);  [YES]

    Because there is no ambiguity that this is part of the type.


  - Don't indent within namespaces, and always add a comment when closing one,
    which should be exactly: '}' + 2 spaces + '// namespace' eventually
    followed by the namespace name if any, e.g.:

    [GOOD]

        namespace android {
        namespace foo {

        namespace {

        ... declarations inside anonymous namespace

        }  // namespace            <--- declare end of anonymous namespace

        // Declares android::foo::Foo
        class Foo {
           ...
        };

        }  // namespace foo
        }  // namespace android


  - C++ member initialization in constructors should happen either on a single
    line, or use one line per member, with member initializations aligned
    after the colon, e.g.:

         class Foo {
         public:
             Foo()
                 : mFirstMember(),
                   mSecondMember(),
                   mThirdMember(),
                   mFourthMember() {}
            ....
        };

2. Naming:

  - C++ class/struct names follow 'CapitalCamelCase'

  - C++ Functions and methods follow 'lowerCamelCase()'

  - C++ Class member names begin with an 'm' prefix, as in mFoo, mBar, ...

  - For C++ structs that are POD types (i.e. no constructor/destructor/virtual
    methods and private members), you can use 'lowerCamelCase' instead for
    (necessarily public) members.

  - C++ variables names use 'lowerCamelCase' or 'lower_underscore_spacing' to
    your convenience.

  - C types names follow 'CapitalCamelCase' as well.

  - C functions should preferably use 'lowerCamelCase'. Sometimes an underscore
    can be used to prefix a family of related functions though, as in:

          sandwich_grabCondiments()
          sandwich_addSauce()
          sandwich_putBread()

  - When implementing 'C with classes', follow 'objectType_methodName',
    and distinguish between functions that allocate objects in the heap,
    and those that only initialize them in place.

         typedef struct FooBar FooBar;

         FooBar* fooBar_new();                     // Allocate and initialize.
         void fooBar_free(FooBar* fooBar);         // Finalize and release.
         void fooBar_resetColor(FooBar* fooBar, int newColor);

         // NOTE: _create() and _destroy() are also valid suffixes for
                  functions that also perform heap allocation/releases.

         typedef struct {
             ... field declarations.
         } Zoo;

         void zoo_init(Zoo* bar, ...);  // Initialize instance in place.
         void zoo_done(Zoo* bar);       // Finalize instance.

    NOTE: The 'this' parameter always comes first.

  - C++ header files should be named according to the main class they declare
    and follow 'CapitalCamelCase.h'.

    C header files should be named similarly, but should use
    'lower_underscore_spacing.h' instead. That's actually a simple way to
    differentiate between both header types.

  - Don't use header inclusion guard macros, use '#pragma once' since it's
    supported by all our compilers now. I.e.

        // Copyright disclaimer blah blah
        // ....

        #pragma once

        ... declarations

    Instead of:

        // Copyright disclaimer blah blah
        // ....

        #ifndef ANDROID_STUFF_FOO_BAR_H
        #define ANDROID_STUFF_FOO_BAR_H

        ... declarations

        #endif  // ANDROID_STUFF_FOO_BAR_H

  - C++ namespace names are 'all_lowercase', and avoid using underscores
    if possible. Nested namespaces are ok, but use 'android' as the top-level
    one to avoid conflicts in the future when we need to include headers from
    other third-party C++ libraries (e.g. 'android::base', 'android::testing',
    etc)

  - C++ constants like unscoped enum values and static character arrays
    should be named using a 'k' prefix followed by CamelCase, as in:
    ```
       enum Mode {
          kRead = 1 << 0,
          kWrite = 1 << 1,
          kReadWrite = kRead | kWrite;
       };

       static char kString[] = "Hello World";
    ```
    Note that scoped enums (a.k.a. 'enum classes') don't need the k Prefix
    but should still use CamelCasing as in:
    ```
       enum class Mode {
           Read = 1 << 0,                    // Mode::Read
           Write = 1 << 1,                   // Mode::Write
           ReadWrite = Read | Write          // Mode::ReadWrite
       };
    ```

3. Comments:

  - Favor C++ style comments even in C source files.

    Rationale: They are easier to edit, and all recent C toolchains support
               then now.

  - When declaring a new C++ class, begin with a comment explaining its
    purpose and a usage example. Don't comment on the implementation though.

  - Document each parameter and the result of each function or method.
    Use |paramName| notation to denote a given parameter, or expression that
    uses the parameter inside the comment to make it clearer.

  - Inside the code, use comments to describe the code's intended behaviour,
    not the exact implementation. Anything that is subtle should have a comment
    that explain why things are the way they are, including temporary band-aids,
    work-arounds and 'clever hacks' to help future maintenance.

  - Try to add comments after #else and #endif directives to indicate what
    they relate to, as in:
    ```
       #ifdef _WIN32
       ...
       #endif  // !_WIN32

       #if defined(__linux__) || defined(_WIN32)
       ...
       #else  // !__linux__ && !_WIN32    <-- NOTE: condition inverted on #else
       ...
       #endif  // !__linux__ && !_WIN32
    ```

4. Header and ordering:

  When including header files, follow this order:

  - Each source file should first include the header file it relates to,
    using "" notation, with path relative to external/qemu/.

    For example, the source file 'android/myfeature/Foo.cpp' might use:

        #include "android/myfeature/Foo.h"    [YES]

    But not:

        #include <android/myfeature/Foo.h>    [NO]
        #include "Foo.h"                      [NO]

    Rationale:  If you end up not including a required header in
    your_fancy_module.h, then you catch that mistake while trying to
    compile your_fancy_module.c, instead of someone else hitting it later
    when including your_fancy_module.h in their client code.

  - Then list all other headers from the android/ sub-directory,
    use case-insensitive  alphabetical ordering to do this, with ""
    notation, e.g.:

    android/myfeature/Api.cpp:

        #include "android/myfeature/Api.h"

        #include "aemu/base/Log.h"
        #include "android/myfeature/Helper.h"
        #include "android/otherFeather.h"

  - Followed by all headers from third-party libraries, using <> notation.
  - Followed by all system headers, using <> notation.

  Separate each header group with an empty line.


5. Unnamed namespaces:

  Contrary to the Chromium style guide, avoid them if you want.

  Rationale: Their main drawback is that it's extremely difficult to set
  breakpoints into functions that are defined into anonymous namespaces with
  GDB during debugging. I.e. you need to manually quote the full type with ("),
  and auto-completion doesn't work. This is really not cool when you're already
  having a hard time trying to undestand why something doesn't work.

  Internal functions declared with 'static' don't have this problem.

  It might be a good idea to put whole internal classes declarations inside
  unnamed namespaces though to avoid exporting all its symbols in the
  corresponding object file, and use better relocations as well, e.g.:

        namespace {

        class MySuperDuperClass : public BaseClass {
            .... // lots of methods here.
        };

        }  // namespace

        BaseClass* createSuperDuperInstance() {
            return new MySuperDuperClass();
        }


6. Platform-specific code:

  We don't provide OS_XXX macros, so currently use the following checks for
  host-platform conditional checks:

        #ifdef _WIN32
        ... Code specific to 32-bit and 64-bit Windows!
        #endif  // _WIN32

        #ifdef __linux__
        ... Code specific to Linux
        #endif  // __linux__

        #ifdef __APPLE__
        ... Code specific to Darwin / OS X
        #endif  // __APPLE__

        #ifndef _WIN32
        ... Code specific to Linux or Darwin.
        #endif  // !_WIN32

  By the way, refer to OS X as 'darwin' instead in your comments and source
  file names if possible.


IV. Automatic code formatting with 'git clang-format':
------------------------------------------------------

A file is provided at external/qemu/android/.clang-format that contains rules
to apply with the 'clang-format' tool. More specifically, one should do the
following to manually reformat entire sources:

    cd external/qemu/android/
    clang-format -style=file -i <list-of-files>

The '-style=file' option is required to tell clang-format to use our
.clang-format file, otherwise, it will default to the Google style which
is quite different!

You can also use the 'git clang-format' command, that will only reformat
the lines that you have currently modified and added to your index, however
you need to configure your git project once to ensure it uses the .clang-format
file properly, with:

    git config clangformat.style file

This ensures that git will pass '-style=file' to clang-format each time you
call 'git clang-format' (which itself calls git-clang-format).


V. C++11 and beyond

  As of now, C++11 is enabled for all platforms, and here's the allowed
  features (list is also based on Chromium style [1], but a little more
  relaxed):

0. First of all, be consistent.

  If you're editing some older file which has a bunch of 'typedef's, don't just
  throw 'using' in the middle of them. Either follow the existing pattern, or
  upgrade around your change. Consistency helps the reader much more than some
  single fancy feature.

1. auto

  Use auto when type is obvious, not important or hard to spell:

  good:
  ```
     auto i = 0;
     auto foo = new Foo();
     auto it = map.begin();
  ```
  bad:
  ```
     auto f = doSomeStuff();
     otherStuff(f->bar());
  ```
  Don't forget about "auto&" and "const auto&" to make sure the value is not
  copied or accidentally modified.

2. foreach loop

  Use the foreach loop for iteration over everything you can

     int values[] = {1,2,3};
     for (int i : values) {
         ...
     }

  Use the correct reference form for the variable (& or const &); don't
  just throw &&

3. Uniform initialization and initializer lists

  Prefer uniform initialization syntax {} to disambugate variables from
  functions and suppress implicit type conversions.

    int i = 1.0; // bad, precision loss
    int i = {1.0}; // good, compiler error

    Foo f(); // bad, function f returning foo instead of a variable
    Foo f{}; // good, default-initialized variable f

4. type aliases

  Prefer "using" over "typedef"

    // easier to understand - "=" splits alias and the actual type
    using f = foo::bar::baz;

    // much simpler for functions than typedefed version -
    // typedef char (*func)(blah)[10];
    using func = char (*)(blah)[10];

    // typedef cannot into templates :)
    template <class T>
    using vec<T> = std::vector<T, our_super_duper_allocator<T>>;

5. override and final

  Use these new keywords as much as possible to make sure your derived class
  actually overrides someting from a base class.

6. Defaulted and deleted members

  Prefer "= default" and "= delete" syntax over the empty braces and private
  declaration. It both improves readability and compiler messages/generated
  code.
  Note: you can use this syntax in the implementation file:

    foo.h:
      struct Foo {
          Foo();
      };

    foo.cpp:
      Foo::Foo() = default;

7. Class member initializers

  Use member initializers instead of a standalone constructor if that's the
  only thing it is needed for, or if there are many constructors all requiring
  the same member values

    class Meow {
    public:
        Meow() = default;

        Meow(int i) : i(i) {
        }

    private:
        int i = 0;
        int j = 2;
        std::string meow = "meow";
    };

8. Strongly-typed enums

  Prefer scoped enums, a.k.a. "enum classes", over the old unscoped enums.
  I.e.:

     [YES]
     enum class Mode {
         Read = 1 << 0,
         Write = 1 << 1,
         ReadWrite = (1 << 0) | (1 << 1),
     };

     [NO]
     enum Mode {
         kRead = 1 << 0,
         kWrite = 1 << 1,
         kReadWrite = kRead | kWrite,
     };

  Note: Scoped enums have one drawback: they do not support bitwise operations
        which are typically handy for dealing with bitflags. There is
        fortunately one solution for this, which is to include
        "aemu/base/EnumFlags.h" in your source file, and add the
        following flag if you're not in the android::base namespace:

           #include "aemu/base/EnumFlags.h"

           ...

           using ::android::base::EnumFlags


9. static_assert

  Use where possible over the runtime assert() calls

10. Raw string literals

  Use raw string literals for the things which otherwise look horrible

    auto s = R"(text with lots of "bad" characters\xml\etc
even
 many
  newlines)";

11.  lambdas, noexcept, rvalue references, move semantics, perfect forwarding,
    constexpr

  Do not use the features listed here.

  ...unless:
    - You need to return a move-only type, e.g. std::unique_ptr<>. Use
      std::move(var) for this case.

MAX_INT. Other features not mentioned here

  If there's some missing feature you need, just add it to the list and send a
  code review to the team.


V. Contributing code
-----------------------------

CL descriptions should follow the form (copied off of [2]):

submodule: Summary of change preferably < 65 chars, surely < 80

Longer description of change addressing as appropriate: why the change is made,
context if it is part of many changes, description of previous behavior and
newly introduced differences, etc.

Long lines should be wrapped to 80 columns for easier log message viewing in
terminals.

    BUG=b.android.com/123456
    TEST=Optional description of test steps to be performed by a human to verify
        this patch. This helps cherry-pick changes later.
        For bug fixes, it's a manual regression test (or preferably, just "Run
        (new) unittests" :) ). For new features, some sanity checks.


M. Appendix
-----------------------------

1. https://www.chromium.org/developers/coding-style
2. https://www.chromium.org/developers/contributing-code
