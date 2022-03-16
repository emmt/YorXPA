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

func xpa_array(ans, i, type, ..)
/* DOCUMENT arr = xpa_array(ans, i, type, dims...);

     yields an array whose elements have type `type` and whose dimensions are
     `dims` from the `i`-th data buffer stored in `ans`.

   SEE ALSO xpa_get, xpa_list.
 */
{
    /* Build dimension list. */
    dims = [0];
    err = 0n;
    while (more_args()) {
        local arg;
        eq_nocopy, arg, next_arg();
        if (is_integer(arg)) {
            if (is_scalar(arg)) {
                grow, dims, arg;
            } else if (is_vector(arg) && (n = numberof(arg)) == arg(1) + 1) {
                if (n >= 2) {
                    grow, dims, arg(2:0);
                }
            } else {
                err = 1n;
                break;
            }
        } else if (! is_void(arg)) {
            err = 1n;
            break;
        }
    }
    if (! err) {
        n = numberof(dims);
        if (n > 1) {
            dims(1) = n - 1;
            err = (min(dims) < 1);
        }
    }
    if (err) {
        error, "invalid dimension(s)";
    }

    /* Return array. */
    return ans((is_void(i) ? 1 : i), array(type, dims));
}

local xpa_text, xpa_get_text, _xpa_text;
/* DOCUMENT txt = xpa_text(ans);
         or txt = xpa_get_text(apt, cmd);

     The function `xpa_text` converts the data of the XPA answer `ans` into a
     textual form.  If the answer has some error message, an error is thrown.

     The function `xpa_get_text` sends an XPA get request for access point
     `apt` and command `cmd` and yields the data resulting from this request in
     a textual form.  If the answer has some error message, an error is thrown.

     Keyword `chomp` may be set true to discard a single trailing newline if
     any.  If keyword `multi` is set true, multiple lines are returned in the
     form of an array of strings.

   SEE ALSO: xpa_get, xpa_check.
 */
func xpa_text(ans, chomp=, multi=)
{
    return _xpa_text(ans);
}

func xpa_get_text(apt, cmd, chomp=, multi=)
{
    return _xpa_text(xpa_get(apt, cmd));
}

func _xpa_text(ans)
{
    extern chomp, multi;
    xpa_check, ans;
    if (multi) {
        buf = ans(1,3);
        j = where(buf == '\n');
        if (is_array(j)) {
            buf(j) = '\0';
        }
        return strchar(buf);
    } else if (chomp) {
        buf = ans(1,3);
        if (buf(0) == '\n') {
            buf(0) = '\0';
        }
        return strchar(buf);
    } else {
        return ans(1,4);
    }
}

func xpa_check(ans, onlyfirst=)
/* DOCUMENT xpa_check, ans;
         or msg = xpa_check(ans);

     asserts that XPA answer `ans` does not contain any errors.  If there are
     any error messages in `ans` and if called as a subroutine, `xpa_check`
     throws an error.  If called as a function, error message(s) are returned
     as a string to the caller (nothing is returned if there are no errors).

   SEE ALSO: xpa_get, xpa_set, errs2caller.
*/
{
    errors = ans.errors;
    if (errors > 0) {
        local msg;
        if (onlyfirst && errors > 1) {
            /* Will just print the first one. */
            errors = 1;
        }
        replies = ans.replies;
        for (i = 1; i <= replies; ++i) {
            if (ans(i,0) == 2) {
                str = strpart(ans(i,1), 11:);
                if (is_void(msg)) {
                    eq_nocopy, msg, str;
                } else {
                    msg += "; " + str;
                }
                if (--errors <= 0) {
                    break;
                }
            }
        }
        if (am_subroutine()) {
            error, msg;
        } else {
            return msg;
        }
    }
}
errs2caller, xpa_check;
