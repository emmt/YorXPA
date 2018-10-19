# YorXPA

[Yorick](http://yorick.github.com/) plugin to the
[XPA](https://github.com/ericmandel/xpa) messaging system.


## Usage

Once installed (see intructions below), YorXPA provides routines to communicate
with any XPA server.

In a nutshell:

```{.c}
xpa_list
```

prints a list of the available XPA servers.  To retrieve information (and data)
from an XPA server (or several servers), call:

```{.c}
ans = xpa_get(apt [, cmd]);
```

where `apt` is the XPA access point to identify the destination server(s) and
`cmd` is an optional textual command (a string or nil).  The returned object
collects the answers of the recipients and can be indexed as follows to
retrieve the contents of the received answers:

- `ans()` yields the number of replies;

- `ans(i)` yields the message of `i`-th reply;

- `ans(i,)` yields the data size of the `i`-th reply;

- `ans(i,arr)` copies data from `i`-th reply into array `arr` (sizes must
  match) and yields `arr`;

- `ans(i,0)` yields `0` if there is no message for `i`-th reply, `1` if it is a
  normal message, `2` if it is an error message;

- `ans(i,1)` yields the message of the `i`-th reply;

- `ans(i,2)` yields the server name of the `i`-th reply;

- `ans(i,3)` yields the data of the `i`-th reply as an array of bytes (or nil
  if there are no data);

- `ans(i,4)` yields the data of the `i`-th reply as a string.

If index `i` is less or equal zero, Yorick indexing rules apply (`i=0` refers
to the last entry, etc.).  The returned object also have the following members:

- `ans.replies` yields the number of replies;
- `ans.buffers` yields the number of data buffers in the replies;
- `ans.errors` yields the number of errors in the replies;
- `ans.messages` yields the number of messages in the replies.

Keyword `nmax` may be used to specify the maximum number of recipients.
By default, `nmax=1`.  Specifying `nmax=-1` will use the maximum possible
number of recipients.

To send information (and data)
to an XPA server (or several servers), call:

```{.c}
ans = xpa_set(apt [, cmd [, arr]]);
```

where `apt` is the XPA access point to identify the destination server(s),
`cmd` is an optional textual command (a string or nil) and `arr` is an optional
data to send to the recipients (a numerical array or nil).  The returned object
collects the answers ot the recipients and has similar semantic as the object
returned by `xpa_get` and keyword `nmax` can also be used to allow for more
than one reply (the default).


## Installation

You must have installed the [XPA](https://github.com/ericmandel/xpa) library
and its development files.  You may compile it from the sources available
[here](https://github.com/ericmandel/xpa/releases) or, depending on your
operating system, install pre-compiled packages.  For instance, on Ubuntu:

```{.sh}
sudo apt install libxpa-dev
```

In short, building and installing the plug-in can be as quick as:

```{.sh}
cd "$BUILD_DIR"
"$SRC_DIR"/configure
make
make install
```

where `$BUILD_DIR` is the build directory (at your convenience) and `$SRC_DIR`
is the source directory of the plug-in code.  The build and source directories
can be the same in which case, call `./configure` to configure for building.

If the plug-in has been properly installed, it is sufficient to use any of its
functions to automatically load the plug-in.  You may force the loading of the
plug-in by something like:

```{.cpp}
#include "xpa.i"
```

or

```{.cpp}
require, "xpa.i";
```

in your code.

More detailled installation explanations are given below.

1. You must have Yorick installed on your machine.

2. Unpack the plug-in code somewhere.

3. Configure for compilation.  There are two possibilities:

   For an **in-place build**, go to the source directory of the plug-in code
   and run the configuration script:

   ```{.sh}
   cd "$SRC_DIR"
   ./configure
   ```

   To see the configuration options, type:

   ```{.sh}
   ./configure --help
   ```

   To compile in a **different build directory**, say `$BUILD_DIR`, create the
   build directory, go to the build directory, and run the configuration
   script:

   ```{.sh}
   mkdir -p "$BUILD_DIR"
   cd "$BUILD_DIR"
   "$SRC_DIR"/configure
   ```
   where `$SRC_DIR` is the path to the source directory of the plug-in code.
   To see the configuration options, type:

   ```{.sh}
   "$SRC_DIR"/configure --help
   ```

4. Compile the code:

   ```{.sh}
   make clean
   make
   ```

5. Install the plug-in in Yorick directories:

   ```{.sh}
   make install
   ```
