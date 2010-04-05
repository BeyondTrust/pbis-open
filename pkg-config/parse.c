/* 
 * Copyright (C) 2001, 2002, 2005-2006 Red Hat Inc.
 * 
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of the
 * License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA
 * 02111-1307, USA.
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "parse.h"
#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#include "popt.h"
#ifdef HAVE_SYS_WAIT_H
#include <sys/wait.h>
#endif
#include <sys/types.h>

#ifdef G_OS_WIN32
int dont_define_prefix = FALSE;
char *prefix_variable = "prefix";
int msvc_syntax = FALSE;
#endif

#ifdef G_OS_WIN32
#ifndef G_IS_DIR_SEPARATOR
#define G_IS_DIR_SEPARATOR(c) ((c) == G_DIR_SEPARATOR || (c) == '/')
#endif
#endif

/**
 * Read an entire line from a file into a buffer. Lines may
 * be delimited with '\n', '\r', '\n\r', or '\r\n'. The delimiter
 * is not written into the buffer. Text after a '#' character is treated as
 * a comment and skipped. '\' can be used to escape a # character.
 * '\' proceding a line delimiter combines adjacent lines. A '\' proceding
 * any other character is ignored and written into the output buffer
 * unmodified.
 * 
 * Return value: %FALSE if the stream was already at an EOF character.
 **/
static gboolean
read_one_line (FILE *stream, GString *str)
{
  gboolean quoted = FALSE;
  gboolean comment = FALSE;
  int n_read = 0;

  g_string_truncate (str, 0);
  
  while (1)
    {
      int c;
      
      c = getc (stream);

      if (c == EOF)
	{
	  if (quoted)
	    g_string_append_c (str, '\\');
	  
	  goto done;
	}
      else
	n_read++;

      if (quoted)
	{
	  quoted = FALSE;
	  
	  switch (c)
	    {
	    case '#':
	      g_string_append_c (str, '#');
	      break;
	    case '\r':
	    case '\n':
	      {
		int next_c = getc (stream);

		if (!(c == EOF ||
		      (c == '\r' && next_c == '\n') ||
		      (c == '\n' && next_c == '\r')))
		  ungetc (next_c, stream);
		
		break;
	      }
	    default:
	      g_string_append_c (str, '\\');	      
	      g_string_append_c (str, c);
	    }
	}
      else
	{
	  switch (c)
	    {
	    case '#':
	      comment = TRUE;
	      break;
	    case '\\':
	      if (!comment)
		quoted = TRUE;
	      break;
	    case '\n':
	      {
		int next_c = getc (stream);

		if (!(c == EOF ||
		      (c == '\r' && next_c == '\n') ||
		      (c == '\n' && next_c == '\r')))
		  ungetc (next_c, stream);

		goto done;
	      }
	    default:
	      if (!comment)
		g_string_append_c (str, c);
	    }
	}
    }

 done:

  return n_read > 0;
}

static char *
trim_string (const char *str)
{
  int len;

  g_return_val_if_fail (str != NULL, NULL);
  
  while (*str && isspace ((guchar)*str))
    str++;

  len = strlen (str);
  while (len > 0 && isspace ((guchar)str[len-1]))
    len--;

  return g_strndup (str, len);
}

static char *
trim_and_sub (Package *pkg, const char *str, const char *path)
{
  char *trimmed;
  GString *subst;
  char *p;
  
  trimmed = trim_string (str);

  subst = g_string_new ("");

  p = trimmed;
  while (*p)
    {
      if (p[0] == '$' &&
          p[1] == '$')
        {
          /* escaped $ */
          g_string_append_c (subst, '$');
          p += 2;
        }
      else if (p[0] == '$' &&
               p[1] == '{')
        {
          /* variable */
          char *var_start;
          char *varname;
          char *varval;
          
          var_start = &p[2];

          /* Get up to close brace. */
          while (*p && *p != '}')
            ++p;

          varname = g_strndup (var_start, p - var_start);

          ++p; /* past brace */
          
          varval = package_get_var (pkg, varname);
          
          if (varval == NULL)
            {
              verbose_error ("Variable '%s' not defined in '%s'\n",
                             varname, path);
              
              exit (1);
            }

          g_free (varname);

          g_string_append (subst, varval);
          g_free (varval);
        }
      else
        {
          g_string_append_c (subst, *p);

          ++p;          
        }
    }

  g_free (trimmed);
  p = subst->str;
  g_string_free (subst, FALSE);

  return p;
}

static void
parse_name (Package *pkg, const char *str, const char *path)
{
  if (pkg->name)
    {
      verbose_error ("Name field occurs twice in '%s'\n", path);

      exit (1);
    }
  
  pkg->name = trim_and_sub (pkg, str, path);
}

static void
parse_version (Package *pkg, const char *str, const char *path)
{
  if (pkg->version)
    {
      verbose_error ("Version field occurs twice in '%s'\n", path);

      exit (1);
    }
  
  pkg->version = trim_and_sub (pkg, str, path);
}

static void
parse_description (Package *pkg, const char *str, const char *path)
{
  if (pkg->description)
    {
      verbose_error ("Description field occurs twice in '%s'\n", path);

      exit (1);
    }
  
  pkg->description = trim_and_sub (pkg, str, path);
}


#define MODULE_SEPARATOR(c) ((c) == ',' || isspace ((guchar)(c)))
#define OPERATOR_CHAR(c) ((c) == '<' || (c) == '>' || (c) == '!' || (c) == '=')

/* A module list is a list of modules with optional version specification,
 * separated by commas and/or spaces. Commas are treated just like whitespace,
 * in order to allow stuff like: Requires: @FRIBIDI_PC@, glib, gmodule
 * where @FRIBIDI_PC@ gets substituted to nothing or to 'fribidi'
 */

typedef enum
{
  /* put numbers to help interpret lame debug spew ;-) */
  OUTSIDE_MODULE = 0,
  IN_MODULE_NAME = 1,
  BEFORE_OPERATOR = 2,
  IN_OPERATOR = 3,
  AFTER_OPERATOR = 4,
  IN_MODULE_VERSION = 5  
} ModuleSplitState;

#define PARSE_SPEW 0

static GSList*
split_module_list (const char *str, const char *path)
{
  GSList *retval = NULL;
  const char *p;
  const char *start;
  ModuleSplitState state = OUTSIDE_MODULE;
  ModuleSplitState last_state = OUTSIDE_MODULE;

  /*   fprintf (stderr, "Parsing: '%s'\n", str); */
  
  start = str;
  p = str;

  while (*p)
    {
#if PARSE_SPEW
      fprintf (stderr, "p: %c state: %d last_state: %d\n", *p, state, last_state);
#endif
      
      switch (state)
        {
        case OUTSIDE_MODULE:
          if (!MODULE_SEPARATOR (*p))
            state = IN_MODULE_NAME;          
          break;

        case IN_MODULE_NAME:
          if (isspace ((guchar)*p))
            {
              /* Need to look ahead to determine next state */
              const char *s = p;
              while (*s && isspace ((guchar)*s))
                ++s;

              if (*s == '\0')
                state = OUTSIDE_MODULE;
              else if (MODULE_SEPARATOR (*s))
                state = OUTSIDE_MODULE;
              else if (OPERATOR_CHAR (*s))
                state = BEFORE_OPERATOR;
              else
                state = OUTSIDE_MODULE;
            }
          else if (MODULE_SEPARATOR (*p))
            state = OUTSIDE_MODULE; /* comma precludes any operators */
          break;

        case BEFORE_OPERATOR:
          /* We know an operator is coming up here due to lookahead from
           * IN_MODULE_NAME
           */
          if (isspace ((guchar)*p))
            ; /* no change */
          else if (OPERATOR_CHAR (*p))
            state = IN_OPERATOR;
          else
            g_assert_not_reached ();
          break;

        case IN_OPERATOR:
          if (!OPERATOR_CHAR (*p))
            state = AFTER_OPERATOR;
          break;

        case AFTER_OPERATOR:
          if (!isspace ((guchar)*p))
            state = IN_MODULE_VERSION;
          break;

        case IN_MODULE_VERSION:
          if (MODULE_SEPARATOR (*p))
            state = OUTSIDE_MODULE;
          break;
          
        default:
          g_assert_not_reached ();
        }

      if (state == OUTSIDE_MODULE &&
          last_state != OUTSIDE_MODULE)
        {
          /* We left a module */
          char *module = g_strndup (start, p - start);
          retval = g_slist_prepend (retval, module);

#if PARSE_SPEW
          fprintf (stderr, "found module: '%s'\n", module);
#endif
          
          /* reset start */
          start = p;
        }
      
      last_state = state;
      ++p;
    }

  if (p != start)
    {
      /* get the last module */
      char *module = g_strndup (start, p - start);
      retval = g_slist_prepend (retval, module);

#if PARSE_SPEW
      fprintf (stderr, "found module: '%s'\n", module);
#endif
      
    }
  
  retval = g_slist_reverse (retval);

  return retval;
}

GSList*
parse_module_list (Package *pkg, const char *str, const char *path)
{
  GSList *split;
  GSList *iter;
  GSList *retval = NULL;

  split = split_module_list (str, path);
  
  iter = split;
  while (iter != NULL)
    {
      RequiredVersion *ver;
      char *p;
      char *start;
      
      p = iter->data;

      ver = g_new0 (RequiredVersion, 1);
      ver->comparison = ALWAYS_MATCH;
      ver->owner = pkg;
      retval = g_slist_prepend (retval, ver);
      
      while (*p && MODULE_SEPARATOR (*p))
        ++p;
      
      start = p;

      while (*p && !isspace ((guchar)*p))
        ++p;

      while (*p && MODULE_SEPARATOR (*p))
        {
          *p = '\0';
          ++p;
        }

      if (*start == '\0')
        {
          verbose_error ("Empty package name in Requires or Conflicts in file '%s'\n", path);
          
          exit (1);
        }
      
      ver->name = g_strdup (start);

      start = p;

      while (*p && !isspace ((guchar)*p))
        ++p;

      while (*p && isspace ((guchar)*p))
        {
          *p = '\0';
          ++p;
        }
      
      if (*start != '\0')
        {
          if (strcmp (start, "=") == 0)
            ver->comparison = EQUAL;
          else if (strcmp (start, ">=") == 0)
            ver->comparison = GREATER_THAN_EQUAL;
          else if (strcmp (start, "<=") == 0)
            ver->comparison = LESS_THAN_EQUAL;
          else if (strcmp (start, ">") == 0)
            ver->comparison = GREATER_THAN;
          else if (strcmp (start, "<") == 0)
            ver->comparison = LESS_THAN;
          else if (strcmp (start, "!=") == 0)
            ver->comparison = NOT_EQUAL;
          else
            {
              verbose_error ("Unknown version comparison operator '%s' after package name '%s' in file '%s'\n", start, ver->name, path);
              
              exit (1);
            }
        }

      start = p;
      
      while (*p && !MODULE_SEPARATOR (*p))
        ++p;

      while (*p && MODULE_SEPARATOR (*p))
        {
          *p = '\0';
          ++p;
        }
      
      if (ver->comparison != ALWAYS_MATCH && *start == '\0')
        {
          verbose_error ("Comparison operator but no version after package name '%s' in file '%s'\n", ver->name, path);
          
          exit (1);
        }

      if (*start != '\0')
        {
          ver->version = g_strdup (start);
        }

      g_assert (ver->name);
      
      iter = g_slist_next (iter);
    }

  g_slist_foreach (split, (GFunc) g_free, NULL);
  g_slist_free (split);

  retval = g_slist_reverse (retval);

  return retval;
}

static void
parse_requires (Package *pkg, const char *str, const char *path)
{
  GSList *parsed;
  GSList *iter;
  char *trimmed;
  
  if (pkg->requires)
    {
      verbose_error ("Requires field occurs twice in '%s'\n", path);

      exit (1);
    }

  trimmed = trim_and_sub (pkg, str, path);
  parsed = parse_module_list (pkg, trimmed, path);
  g_free (trimmed);
  
  iter = parsed;
  while (iter != NULL)
    {
      Package *req;
      RequiredVersion *ver = iter->data;
      
      req = get_package (ver->name);

      if (req == NULL)
        {
          verbose_error ("Package '%s', required by '%s', not found\n",
                         ver->name, pkg->name ? pkg->name : path);
          
          exit (1);
        }

      if (pkg->required_versions == NULL)
        pkg->required_versions = g_hash_table_new (g_str_hash, g_str_equal);
      
      g_hash_table_insert (pkg->required_versions, ver->name, ver);
      
      pkg->requires = g_slist_prepend (pkg->requires, req);

      iter = g_slist_next (iter);
    }

  g_slist_free (parsed);
}

static void
parse_requires_private (Package *pkg, const char *str, const char *path)
{
  GSList *parsed;
  GSList *iter;
  char *trimmed;
  
  if (pkg->requires_private)
    {
      verbose_error ("Requires.private field occurs twice in '%s'\n", path);

      exit (1);
    }

  trimmed = trim_and_sub (pkg, str, path);
  parsed = parse_module_list (pkg, trimmed, path);
  g_free (trimmed);
  
  iter = parsed;
  while (iter != NULL)
    {
      Package *req;
      RequiredVersion *ver = iter->data;
      
      req = get_package (ver->name);

      if (req == NULL)
        {
          verbose_error ("Package '%s', required by '%s', not found\n",
                         ver->name, pkg->name ? pkg->name : path);
          
          exit (1);
        }

      if (pkg->required_versions == NULL)
        pkg->required_versions = g_hash_table_new (g_str_hash, g_str_equal);
      
      g_hash_table_insert (pkg->required_versions, ver->name, ver);
      
      pkg->requires_private = g_slist_prepend (pkg->requires_private, req);

      iter = g_slist_next (iter);
    }

  g_slist_free (parsed);
}

static void
parse_conflicts (Package *pkg, const char *str, const char *path)
{
  GSList *parsed;
  GSList *iter;
  char *trimmed;
  
  if (pkg->conflicts)
    {
      verbose_error ("Conflicts field occurs twice in '%s'\n", path);

      exit (1);
    }

  trimmed = trim_and_sub (pkg, str, path);
  pkg->conflicts = parse_module_list (pkg, trimmed, path);
  g_free (trimmed);
}

static void _do_parse_libs (Package *pkg, int argc, char **argv)
{
  int i;
#ifdef G_OS_WIN32
  char *L_flag = (msvc_syntax ? "/libpath:" : "-L");
  char *l_flag = (msvc_syntax ? "" : "-l");
  char *lib_suffix = (msvc_syntax ? ".lib" : "");
#else
  char *L_flag = "-L";
  char *l_flag = "-l";
  char *lib_suffix = "";
#endif

  i = 0;
  while (i < argc)
    {
      char *arg = trim_string (argv[i]);
      char *p;
      char *start;

      start = arg;
      p = start;      

      if (p[0] == '-' &&
          p[1] == 'l')
        {
          char *libname;          
              
          p += 2;
          while (*p && isspace ((guchar)*p))
            ++p;
              
          start = p;
          while (*p && !isspace ((guchar)*p))
            ++p;

          libname = g_strndup (start, p - start);
          
          pkg->l_libs = g_slist_prepend (pkg->l_libs,
                                         g_strconcat (l_flag, libname, lib_suffix, NULL));

          g_free (libname);
        }
      else if (p[0] == '-' &&
               p[1] == 'L')
        {
          char *libname;          
          
          p += 2;
          while (*p && isspace ((guchar)*p))
            ++p;
              
          start = p;
          while (*p && !isspace ((guchar)*p))
            ++p;

          libname = g_strndup (start, p - start);
          
          pkg->L_libs = g_slist_prepend (pkg->L_libs,
					 g_strconcat (L_flag, libname, NULL));

          g_free (libname);
        }
      else if (strcmp("-framework",p) == 0 && i+1 < argc)
        {
          /* Mac OS X has a -framework Foo which is really one option,
           * so we join those to avoid having -framework Foo
           * -framework Bar being changed into -framework Foo Bar
           * later
          */
          gchar *framework = trim_string (argv[i+1]);

          pkg->other_libs = g_slist_prepend (pkg->other_libs,
                                             g_strconcat(arg, " ", framework, NULL));
          i++;
          g_free(framework);
        }
      else
        {
          if (*arg != '\0')
            pkg->other_libs = g_slist_prepend (pkg->other_libs,
                                               g_strdup (arg));
        }

      g_free (arg);
      
      ++i;
    }

}


static void
parse_libs (Package *pkg, const char *str, const char *path)
{
  /* Strip out -l and -L flags, put them in a separate list. */
  
  char *trimmed;
  char **argv = NULL;
  int argc;
  int result;
  
  if (pkg->libs_num > 0)
    {
      verbose_error ("Libs field occurs twice in '%s'\n", path);

      exit (1);
    }
  
  trimmed = trim_and_sub (pkg, str, path);

  result = poptParseArgvString (trimmed, &argc, &argv);

  if (result < 0)
    {
      verbose_error ("Couldn't parse Libs field into an argument vector: %s\n",
               poptStrerror (result));

      exit (1);
    }

  _do_parse_libs(pkg, argc, argv);

  g_free (trimmed);
  g_free (argv);
  pkg->libs_num++;
}

static void
parse_libs_private (Package *pkg, const char *str, const char *path)
{
  /*
    List of private libraries.  Private libraries are libraries which
    are needed in the case of static linking or on platforms not
    supporting inter-library dependencies.  They are not supposed to
    be used for libraries which are exposed through the library in
    question.  An example of an exposed library is GTK+ exposing Glib.
    A common example of a private library is libm.
    
    Generally, if include another library's headers in your own, it's
    a public dependency and not a private one.
  */
  
  char *trimmed;
  char **argv = NULL;
  int argc;
  int result;
  
  if (pkg->libs_private_num > 0)
    {
      verbose_error ("Libs.private field occurs twice in '%s'\n", path);

      exit (1);
    }
  
  trimmed = trim_and_sub (pkg, str, path);

  result = poptParseArgvString (trimmed, &argc, &argv);

  if (result < 0)
    {
      verbose_error ("Couldn't parse Libs.private field into an argument vector: %s\n",
               poptStrerror (result));

      exit (1);
    }

  _do_parse_libs(pkg, argc, argv);

  g_free (argv);
  g_free (trimmed);

  pkg->libs_private_num++;
}

static void
parse_cflags (Package *pkg, const char *str, const char *path)
{
  /* Strip out -I flags, put them in a separate list. */
  
  char *trimmed;
  char **argv = NULL;
  int argc;
  int result;
  int i;
  
  if (pkg->I_cflags || pkg->other_cflags)
    {
      verbose_error ("Cflags field occurs twice in '%s'\n", path);

      exit (1);
    }
  
  trimmed = trim_and_sub (pkg, str, path);

  result = poptParseArgvString (trimmed, &argc, &argv);

  if (result < 0)
    {
      verbose_error ("Couldn't parse Cflags field into an argument vector: %s\n",
                     poptStrerror (result));

      exit (1);
    }

  i = 0;
  while (i < argc)
    {
      char *arg = trim_string (argv[i]);
      char *p;
      char *start;

      start = arg;
      p = start;      

      if (p[0] == '-' &&
          p[1] == 'I')
        {
          char *libname;          
              
          p += 2;
          while (*p && isspace ((guchar)*p))
            ++p;
              
          start = p;
          while (*p && !isspace ((guchar)*p))
            ++p;

          libname = g_strndup (start, p - start);
          
          pkg->I_cflags = g_slist_prepend (pkg->I_cflags,
                                           g_strconcat ("-I", libname, NULL));

          g_free (libname);
        }
      else
        {
          if (*arg != '\0')
            pkg->other_cflags = g_slist_prepend (pkg->other_cflags,
                                                 g_strdup (arg));
        }

      g_free (arg);
      
      ++i;
    }

  g_free (argv);
  g_free (trimmed);
}

static void
parse_url (Package *pkg, const char *str, const char *path)
{
  if (pkg->url != NULL)
    {
      verbose_error ("URL field occurs twice in '%s'\n", path);

      exit (1);
    }

  pkg->url = trim_and_sub (pkg, str, path);
}

#ifdef G_OS_WIN32
static int
pathnamecmp (const char *a,
	     const char *b)
{
  while (*a && *b &&
	 ((G_IS_DIR_SEPARATOR (*a) && G_IS_DIR_SEPARATOR (*b)) ||
	  g_ascii_toupper (*a) == g_ascii_toupper (*b)))
    {
      a++;
      b++;
    }
  return g_ascii_toupper (*a) - g_ascii_toupper (*b);
}
#endif

static void
parse_line (Package *pkg, const char *untrimmed, const char *path, gboolean ignore_requires, gboolean ignore_private_libs)
{
  char *str;
  char *p;
  char *tag;

  debug_spew ("  line>%s\n", untrimmed);
  
  str = trim_string (untrimmed);
  
  if (*str == '\0') /* empty line */
    {
      g_free(str);
      return;
    }
  
  p = str;

  /* Get first word */
  while ((*p >= 'A' && *p <= 'Z') ||
	 (*p >= 'a' && *p <= 'z') ||
	 (*p >= '0' && *p <= '9') ||
	 *p == '_' || *p == '.')
    p++;

  tag = g_strndup (str, p - str);
  
  while (*p && isspace ((guchar)*p))
    ++p;

  if (*p == ':')
    {
      /* keyword */
      ++p;
      while (*p && isspace ((guchar)*p))
        ++p;

      if (strcmp (tag, "Name") == 0)
        parse_name (pkg, p, path);
      else if (strcmp (tag, "Description") == 0)
        parse_description (pkg, p, path);
      else if (strcmp (tag, "Version") == 0)
        parse_version (pkg, p, path);
      else if (strcmp (tag, "Requires.private") == 0)
	parse_requires_private (pkg, p, path);
      else if (strcmp (tag, "Requires") == 0)
	{
          if (ignore_requires == FALSE)
	    parse_requires (pkg, p, path);
          else
	    goto cleanup;
        }
      else if ((strcmp (tag, "Libs.private") == 0) && 
               ignore_private_libs == FALSE)
        parse_libs_private (pkg, p, path);
      else if (strcmp (tag, "Libs") == 0)
        parse_libs (pkg, p, path);
      else if (strcmp (tag, "Cflags") == 0 ||
               strcmp (tag, "CFlags") == 0)
        parse_cflags (pkg, p, path);
      else if (strcmp (tag, "Conflicts") == 0)
        parse_conflicts (pkg, p, path);
      else if (strcmp (tag, "URL") == 0)
        parse_url (pkg, p, path);
      else
        {
	  /* we don't error out on unknown keywords because they may
	   * represent additions to the .pc file format from future
	   * versions of pkg-config.  We do make a note of them in the
	   * debug spew though, in order to help catch mistakes in .pc
	   * files. */
          debug_spew ("Unknown keyword '%s' in '%s'\n",
		      tag, path);
        }
    }
  else if (*p == '=')
    {
      /* variable */
      char *varname;
      char *varval;
      
      ++p;
      while (*p && isspace ((guchar)*p))
        ++p;
      
      if (pkg->vars == NULL)
        pkg->vars = g_hash_table_new (g_str_hash, g_str_equal);

#ifdef G_OS_WIN32
      if (!dont_define_prefix && strcmp (tag, prefix_variable) == 0)
	{
	  /* This is the prefix variable. Try to guesstimate a value for it
	   * for this package from the location of the .pc file.
	   */

	  gchar *prefix = pkg->pcfiledir;
	  const int prefix_len = strlen (prefix);
	  const char *const lib_pkgconfig = "\\lib\\pkgconfig";
	  const int lib_pkgconfig_len = strlen (lib_pkgconfig);

	  if (strlen (prefix) > lib_pkgconfig_len &&
	      pathnamecmp (prefix + prefix_len - lib_pkgconfig_len,
			   lib_pkgconfig) == 0)
	    {
	      /* It ends in lib\pkgconfig. Good. */
	      
	      gchar *p;
	      
	      prefix = g_strdup (prefix);
	      prefix[prefix_len - lib_pkgconfig_len] = '\0';
	      
	      /* Turn backslashes into slashes or
	       * poptParseArgvString() will eat them when ${prefix}
	       * has been expanded in parse_libs().
	       */
	      p = prefix;
	      while (*p)
		{
		  if (*p == '\\')
		    *p = '/';
		  p++;
		}
	      varname = g_strdup (tag);
	      debug_spew (" Variable declaration, '%s' overridden with '%s'\n",
			  tag, prefix);
	      g_hash_table_insert (pkg->vars, varname, prefix);
	      goto cleanup;
	    }
	}
#endif

      if (g_hash_table_lookup (pkg->vars, tag))
        {
          verbose_error ("Duplicate definition of variable '%s' in '%s'\n",
                         tag, path);

          exit (1);
        }

      varname = g_strdup (tag);
      varval = trim_and_sub (pkg, p, path);     

      debug_spew (" Variable declaration, '%s' has value '%s'\n",
                  varname, varval);
      g_hash_table_insert (pkg->vars, varname, varval);
  
    }

 cleanup:  
  g_free (str);
  g_free (tag);
}

Package*
parse_package_file (const char *path, gboolean ignore_requires, gboolean ignore_private_libs)
{
  FILE *f;
  Package *pkg;
  GString *str;
  gboolean one_line = FALSE;
  
  f = fopen (path, "r");

  if (f == NULL)
    {
      verbose_error ("Failed to open '%s': %s\n",
                     path, strerror (errno));
      
      return NULL;
    }

  debug_spew ("Parsing package file '%s'\n", path);
  
  pkg = g_new0 (Package, 1);

  if (path)
    {
      pkg->pcfiledir = g_dirname (path);
    }
  else
    {
      debug_spew ("No pcfiledir determined for package\n");
      pkg->pcfiledir = g_strdup ("???????");
    }
  
  str = g_string_new ("");

  while (read_one_line (f, str))
    {
      one_line = TRUE;
      
      parse_line (pkg, str->str, path, ignore_requires, ignore_private_libs);

      g_string_truncate (str, 0);
    }

  if (!one_line)
    verbose_error ("Package file '%s' appears to be empty\n",
                   path);
  g_string_free (str, TRUE);
  fclose(f);

  /* make ->requires_private include a copy of the public requires too */
  pkg->requires_private = g_slist_concat(g_slist_copy (pkg->requires),
					 pkg->requires_private);
  
  pkg->requires = g_slist_reverse (pkg->requires);
  
  pkg->requires_private = g_slist_reverse (pkg->requires_private);

  pkg->I_cflags = g_slist_reverse (pkg->I_cflags);
  pkg->other_cflags = g_slist_reverse (pkg->other_cflags);

  pkg->l_libs = g_slist_reverse (pkg->l_libs);
  pkg->L_libs = g_slist_reverse (pkg->L_libs);
  pkg->other_libs = g_slist_reverse (pkg->other_libs);
  
  return pkg;
}

static char *
backticks (const char *command)
{
  FILE *f;
  char buf[4096];
  size_t len;
  int status;
  
  f = popen (command, "r");

  if (f == NULL)
    return NULL;
  
  len = fread (buf, 1, 4090, f);

  if (ferror (f))
    {
      pclose (f);
      return NULL;
    }
  
  buf[len] = '\0';

  status = pclose (f);

  return g_strdup (buf);
}

static gboolean
try_command (const char *command)
{
  int status;
  char *munged;

#ifdef G_OS_WIN32
  munged = g_strdup_printf ("%s > NUL", command);
#else
  munged = g_strdup_printf ("%s > /dev/null 2>&1", command);
#endif
  
  status = system (munged);

  g_free (munged);
  
#ifdef G_OS_WIN32
  return status == 0;
#else
  return WIFEXITED(status) && (WEXITSTATUS(status) == 0);
#endif
}

Package *
get_compat_package (const char *name)
{
#ifdef G_OS_WIN32
  /* There has never been any of these legacy *-config scripts on
   * Windows as far as I know. No use trying to execute them, will
   * only confuse users to see the "blabla is not recognized as an
   * internal or external command, operable program or batch file"
   * messages.
   */
  return NULL;
#else

  Package *pkg;

  if (name_ends_in_uninstalled (name))
    debug_spew ("Suspiciously looking for compat package for -uninstalled: %s\n", name);
  
  debug_spew ("Looking for '%s' using legacy -config scripts\n", name);
  
  pkg = g_new0 (Package, 1);

  pkg->path_position = G_MAXINT;
  
  if (strcmp (name, "glib") == 0)
    {
      char *output;

      debug_spew ("Calling glib-config\n");
      
      pkg->version = backticks ("glib-config --version");
      if (pkg->version == NULL)
        {
          g_free (pkg);
          return NULL;
        }
      
      pkg->name = g_strdup ("GLib");
      pkg->key = g_strdup ("glib");
      pkg->description = g_strdup ("C Utility Library");

      output = backticks ("glib-config --libs");
      parse_libs (pkg, output, "glib-config");
      g_free (output);

      output = backticks ("glib-config --cflags");
      parse_cflags (pkg, output, "glib-config");
      g_free (output);

      return pkg;
    }
  else if (strcmp (name, "gtk+") == 0)
    {
      char *output;

      debug_spew ("Calling gtk-config\n");
      
      pkg->version = backticks ("gtk-config --version");
      if (pkg->version == NULL)
        {
          g_free (pkg);
          return NULL;
        }
      
      pkg->name = g_strdup ("GTK+");
      pkg->key = g_strdup ("gtk+");
      pkg->description = g_strdup ("GIMP Tool Kit");

      output = backticks ("gtk-config --libs");
      parse_libs (pkg, output, "gtk-config");
      g_free (output);

      output = backticks ("gtk-config --cflags");
      parse_cflags (pkg, output, "gtk-config");
      g_free (output);

      return pkg;
    }
  else if (strcmp (name, "libgnomevfs") == 0)
    {
      char *output;

      debug_spew ("Calling gnome-vfs-config\n");
      
      pkg->version = backticks ("gnome-vfs-config --version");
      if (pkg->version == NULL)
        {
          g_free (pkg);
          return NULL;
        }
      
      pkg->name = g_strdup ("GNOME VFS");
      pkg->key = g_strdup ("libgnomevfs");
      pkg->description = g_strdup ("GNOME Virtual File System");

      output = backticks ("gnome-vfs-config --libs");
      parse_libs (pkg, output, "gnome-vfs-config");
      g_free (output);

      output = backticks ("gnome-vfs-config --cflags");
      parse_cflags (pkg, output, "gnome-vfs-config");
      g_free (output);

      return pkg;
    }
  else if (strcmp (name, "imlib") == 0)
    {
      char *output;

      debug_spew ("Calling imlib-config\n");
      
      pkg->version = backticks ("imlib-config --version");
      if (pkg->version == NULL)
        {
          g_free (pkg);
          return NULL;
        }
      
      pkg->name = g_strdup ("Imlib");
      pkg->key = g_strdup ("imlib");
      pkg->description = g_strdup ("Imlib image loading library");

      output = backticks ("imlib-config --libs-gdk");
      parse_libs (pkg, output, "imlib-config");
      g_free (output);

      output = backticks ("imlib-config --cflags-gdk");
      parse_cflags (pkg, output, "imlib-config");
      g_free (output);

      return pkg;
    }
  else if (strcmp (name, "orbit-client") == 0)
    {
      char *output;
      char *p;

      debug_spew ("Calling orbit-config\n");
      
      output = backticks ("orbit-config --version");
      
      if (output == NULL)
        {
          g_free (pkg);
          return NULL;
        }

      p = output;

      while (*p && isspace ((guchar)*p))
        ++p;

      if (*p == '\0')
        {
          /* empty output */
          g_free (output);
          g_free (pkg);
          return NULL;
        }

      /* only heuristic; find a number or . */
      while (*p && ! (isdigit ((guchar)*p) || *p == '.'))
        ++p;      

      pkg->version = g_strdup (p);

      g_free (output);
      
      pkg->name = g_strdup ("ORBit Client");
      pkg->key = g_strdup ("orbit-client");
      pkg->description = g_strdup ("ORBit Client Libraries");

      output = backticks ("orbit-config --libs client");
      parse_libs (pkg, output, "orbit-config");
      g_free (output);

      output = backticks ("orbit-config --cflags client");
      parse_cflags (pkg, output, "orbit-config");
      g_free (output);

      return pkg;
    }
  else if (strcmp (name, "orbit-server") == 0)
    {
      char *output;
      char *p;

      debug_spew ("Calling orbit-config\n");
      
      output = backticks ("orbit-config --version");
      
      if (output == NULL)
        {
          g_free (pkg);
          return NULL;
        }

      p = output;

      while (*p && isspace ((guchar)*p))
        ++p;

      if (*p == '\0')
        {
          /* empty output */
          g_free (output);
          g_free (pkg);
          return NULL;
        }

      /* only heuristic; find a number or . */
      while (*p && ! (isdigit ((guchar)*p) || *p == '.'))
        ++p;      

      pkg->version = g_strdup (p);

      g_free (output);
      
      pkg->name = g_strdup ("ORBit Server");
      pkg->key = g_strdup ("orbit-server");
      pkg->description = g_strdup ("ORBit Server Libraries");

      output = backticks ("orbit-config --libs server");
      parse_libs (pkg, output, "orbit-config");
      g_free (output);

      output = backticks ("orbit-config --cflags server");
      parse_cflags (pkg, output, "orbit-config");
      g_free (output);

      return pkg;
    }
  else
    {
      /* Check for the module in gnome-config */
      char *output;
      char *p;
      char *command;

      debug_spew ("Calling gnome-config\n");
      
      /* Annoyingly, --modversion doesn't return a failure
       * code if the lib is unknown, so we have to use --libs
       * for that.
       */
      
      command = g_strdup_printf ("gnome-config --libs %s",
                                 name);
      
      if (!try_command (command))
        {
          g_free (command);
          g_free (pkg);
          return NULL;
        }
      else
        g_free (command);
      
      command = g_strdup_printf ("gnome-config --modversion %s",
                                 name);
      
      output = backticks (command);
      g_free (command);
      if (output == NULL)
        {
          g_free (pkg);
          return NULL;
        }
      
      /* Unknown modules give "Unknown library `foo'" from gnome-config
       * (but on stderr so this is useless, nevermind)
       */
      if (strstr (output, "Unknown") || *output == '\0')
        {
          g_free (output);
          g_free (pkg);
          return NULL;
        }

      /* gnome-config --modversion gnomeui outputs e.g. "gnome-libs-1.2.4"
       * or libglade-0.12
       */
      p = output;

      while (*p && isspace ((guchar)*p))
        ++p;

      if (*p == '\0')
        {
          /* empty output */
          g_free (output);
          g_free (pkg);
          return NULL;
        }

      /* only heuristic; find a number or . */
      while (*p && ! (isdigit ((guchar)*p) || *p == '.'))
        ++p;      

      pkg->version = g_strdup (p);

      g_free (output);
      
      /* Strip newline */
      p = pkg->version;
      while (*p)
        {
          if (*p == '\n')
            *p = '\0';

          ++p;
        }
      
      pkg->name = g_strdup (name);
      pkg->key = g_strdup (name);
      pkg->description = g_strdup ("No description");

      command = g_strdup_printf ("gnome-config --libs %s", name);
      output = backticks (command);
      g_free (command);
      parse_libs (pkg, output, "gnome-config");
      g_free (output);

      command = g_strdup_printf ("gnome-config --cflags %s", name);
      output = backticks (command);
      g_free (command);
      parse_cflags (pkg, output, "gnome-config");
      g_free (output);

      return pkg;
    }
#endif
}
