/*
 * xpa.i --
 *
 * Yorick interface to XPA messaging system.
 *
 *-----------------------------------------------------------------------------
 *
 * This file if part of the YorXPA (https://github.com/emmt/YorXPA) licensed
 * under the MIT license.
 *
 * Copyright (C) 2018, Éric Thiébaut.
 */

if (is_func(plug_in)) plug_in, "yor_xpa";

extern xpa_get;
/* DOCUMENT ans = xpa_get(apt [, cmd]);

     This function performs an XPA get command.  Argument `apt` is a string
     identifying the XPA access point(s) of the destination server(s).
     Argument `cmd` is an optional textual command (a string or nil).

     Keyword `nmax` may be used to specify the maximum number of recipients.
     By default, `nmax=1`.  Specifying `nmax=-1` will use the maximum possible
     number of recipients.

     The returned object collects the answers of the recipients and can be
     indexed as follows to retrieve the contents of the received answers:

       ans()      yields the number of replies;
       ans(i)     yields the message of `i`-th reply;
       ans(i,)    yields the data size of the `i`-th reply;
       ans(i,arr) copies data from `i`-th reply into array `arr` (sizes must
                  match) and yields `arr`;
       ans(i,0)   yields `0` if there is no message for `i`-th reply, `1` if
                  it is a normal message, `2` if it is an error message;
       ans(i,1)   yields the message of the `i`-th reply;
       ans(i,2)   yields the server name of the `i`-th reply;
       ans(i,3)   yields the data of the `i`-th reply as an array of bytes
                  (or nil if there are no data);
       ans(i,4)   yields the data of the `i`-th reply as a string.

     If index `i` is less or equal zero, Yorick indexing rules apply (`i=0`
     refers to the last reply, etc.).

     The returned object also have the following members:

       ans.replies   yields the number of replies;
       ans.buffers   yields the number of data buffers in the replies;
       ans.errors    yields the number of errors in the replies;
       ans.messages  yields the number of messages in the replies.

   SEE ALSO xpa_set.
 */

extern xpa_set;
/* DOCUMENT ans = xpa_set(apt [, cmd [, arr]]);

     This function performs an XPA set command.  Argument `apt` is a string
     identifying the XPA access point(s) of the destination server(s).
     Argument `cmd` is an optional textual command (a string or nil).  Argument
     `arr` is an optional data to send to the recipients (a numerical array or
     nil).

     The returned object collects the answers ot the recipients and has
     similar semantic as the object returned by `xpa_get`.

     Keyword `nmax` may be used to specify the maximum number of recipients.
     By default, `nmax=1`.  Specifying `nmax=-1` will use the maximum possible
     number of recipients.


   SEE ALSO xpa_get, xpa_list.
 */

func xpa_list(nil)
/* DOCUMENT lst = xpa_list();
         or xpa_list;

     This function retrieves a list of the current XPA servers.  If called as a
     function, a list of strings (or nil if there are no servers) is returned;
     otherwise the list of servers is printed.

   SEE ALSO xpa_get.
 */
{
    ans = xpa_get("xpans");
    n = ans.replies;
    if (ans.errors > 0) {
        for (i = 1; i <= n; ++i) {
            if (ans(i,0) == 2) {
                write, format="%s\n", ans(i,);
            }
        }
        error, "there are errors in XPA answers";
    }
    lst = [];
    if (ans.buffers > 0) {
        for (i = 1; i <= n; ++i) {
            buf = ans(i,3);
            if (! is_void(buf)) {
                j = where(buf == '\n');
                if (is_array(j)) {
                    buf(j) = '\0';
                }
                grow, lst, strchar(buf);
            }
        }
    }
    if (am_subroutine()) {
        write, format="%s\n", lst;
    } else {
        return lst;
    }
}
