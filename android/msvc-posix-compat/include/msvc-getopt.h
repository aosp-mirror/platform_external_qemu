/* __BEGIN_DECLS should be used at the beginning of your declarations,
   so that C++ compilers don't mangle their names.  Use __END_DECLS at
   the end of C declarations. */
#ifndef __BEGIN_DECLS
#ifdef __cplusplus
# define __BEGIN_DECLS extern "C" {
# define __END_DECLS }
#else
# define __BEGIN_DECLS /* empty */
# define __END_DECLS /* empty */
#endif
#endif

__BEGIN_DECLS

// <getopt.h>
extern int optind;   /* index of first non-option in argv      */
extern int optopt;   /* single option character, as parsed     */
extern int opterr;   /* flag to enable built-in diagnostics... */
                     /* (user may set to zero, to suppress)    */
extern char* optarg; /* pointer to argument of current option  */

extern int getopt(int nargc, char* const* nargv, const char* options);

struct option /* specification for a long form option...	*/
{
    const char* name; /* option name, without leading hyphens */
    int has_arg;      /* does it take an argument?		*/
    int* flag;        /* where to save its status, or NULL	*/
    int val;          /* its associated status value		*/
};

enum                 /* permitted values for its `has_arg' field...	*/
{ no_argument = 0,   /* option never takes an argument	*/
  required_argument, /* option always requires an argument	*/
  optional_argument  /* option may take an argument		*/
};

extern int getopt_long(int nargc,
                       char* const* nargv,
                       const char* options,
                       const struct option* long_options,
                       int* idx);
extern int getopt_long_only(int nargc,
                            char* const* nargv,
                            const char* options,
                            const struct option* long_options,
                            int* idx);
__END_DECLS
