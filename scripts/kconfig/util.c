/*
 * Copyright (C) 2002-2005 Roman Zippel <zippel@linux-m68k.org>
 * Copyright (C) 2002-2005 Sam Ravnborg <sam@ravnborg.org>
 *
 * Released under the terms of the GNU GPL v2.0.
 */

#define _GNU_SOURCE
#include <stdio.h>
#include <string.h>
#include "lkc.h"

ssize_t getline(char **lineptr, size_t *n, FILE *stream);

/* file already present in list? If not add it */
struct file *file_lookup(const char *name)
{
	struct file *file;

	for (file = file_list; file; file = file->next) {
		if (!strcmp(name, file->name))
			return file;
	}

	file = malloc(sizeof(*file));
	memset(file, 0, sizeof(*file));
	file->name = strdup(name);
	file->next = file_list;
	file_list = file;
	return file;
}

/* write a dependency file as used by kbuild to track dependencies */
int file_write_dep(const char *name)
{
	struct file *file;
	FILE *out;

	if (!name)
		name = ".kconfig.d";
	out = fopen("..config.tmp", "w");
	if (!out)
		return 1;
	fprintf(out, "deps_config := \\\n");
	for (file = file_list; file; file = file->next) {
		if (file->next)
			fprintf(out, "\t%s \\\n", file->name);
		else
			fprintf(out, "\t%s\n", file->name);
	}
	fprintf(out, "\ninclude/config/auto.conf: \\\n"
		     "\t$(deps_config)\n\n"
		     "$(deps_config): ;\n");
	fclose(out);
	rename("..config.tmp", name);
	return 0;
}


/* Allocate initial growable sting */
struct gstr str_new(void)
{
	struct gstr gs;
	gs.s = malloc(sizeof(char) * 64);
	gs.len = 16;
	strcpy(gs.s, "\0");
	return gs;
}

/* Allocate and assign growable string */
struct gstr str_assign(const char *s)
{
	struct gstr gs;
	gs.s = strdup(s);
	gs.len = strlen(s) + 1;
	return gs;
}

/* Free storage for growable string */
void str_free(struct gstr *gs)
{
	if (gs->s)
		free(gs->s);
	gs->s = NULL;
	gs->len = 0;
}

/* Append to growable string */
void str_append(struct gstr *gs, const char *s)
{
	size_t l = strlen(gs->s) + strlen(s) + 1;
	if (l > gs->len) {
		gs->s   = realloc(gs->s, l);
		gs->len = l;
	}
	strcat(gs->s, s);
}

/* Append printf formatted string to growable string */
void str_printf(struct gstr *gs, const char *fmt, ...)
{
	va_list ap;
	char s[10000]; /* big enough... */
	va_start(ap, fmt);
	vsnprintf(s, sizeof(s), fmt, ap);
	str_append(gs, s);
	va_end(ap);
}

/* Retrieve value of growable string */
const char *str_get(struct gstr *gs)
{
	return gs->s;
}

/* display help for generic TI_option_parse 
   no return; exits immediately */
void TI_option_help(char *progname)
{
  printf("%s [-v] [-p <path>] [-c <file>] [-u <file>] <config>\n", progname);
  printf("   -p, --path <path>:      search paths for toplevels of other Kconfigs\n"
	 "   -c, --config <file>:    toplevel Kconfig, optional format\n"
	 "   -v:                     verbose\n"
	 "   -u, --unselect <file>:  implicit unselect input symbols\n"
	 "   <config>:               toplevel Kconfig\n");
  exit(0);
} /* TI_option_help */

/* Parse the unselect exception list. This file lists the kconfig symbols that will have the selectee bound to the selector. File is symbol per line with # as the first character on a line to comment. Creates string taking each line of
input and putting = delim around each symbol input of each line:

input: 
A
B
C

becomes: "=A==B==C=" 

input filename string
returns pointer to string. returns NULL on nonfatal error. exits with perror
on fatal.
*/
char * TI_unselect_read(char *filename)
{

  FILE * fp;
  char * line = NULL, * symstr = NULL, * tmpstr, * ptr;
  int symstrsz = 1000, tmpstrsz = 1000;
  size_t len = 0;
  ssize_t read;

  
  if (!filename) {
    fprintf(stderr, "TI_unselect_read: bad filename\n");
    return(NULL);
  }
  if ((fp = fopen(filename, "r")) == NULL) {
    perror("TI_unselect_read: fopen");
    return(NULL);
  }

  tmpstr = (char *)malloc(tmpstrsz * sizeof(char));
  symstr = (char *)calloc(symstrsz, sizeof(char));

  while ((read = getline(&line, &len, fp)) != -1) {
    if ((line[0] == '#') || (line[0] == '\n')) /* skip comment and newlines */
      continue;
    if (line[read-1] == '\n') 
      line[--read] = (char)NULL;
    if (strlen(symstr) + read > symstrsz) {
      symstrsz += 1000;
      if((symstr = realloc(symstr, symstrsz)) == NULL) {
	perror("TI_unselect_read: realloc");
	exit(-1);
      }
    }
    if (read + 4 > tmpstrsz) {	/* make sure we have room to add delim */
      tmpstrsz += 1000;
      if((tmpstr = realloc(tmpstr, tmpstrsz)) == NULL) {
	perror("TI_unselect_read: realloc 2");
	exit(-1);
      }
    }

    if ((ptr = strstr(line, " menuhide"))) {	/* hide menu prompt */
      line[ptr-line] = 0;
      sprintf(tmpstr, TI_MHIDEDEL"%s"TI_MHIDEDEL, line);
    } else {			/* otherwise this is an unselect var */
      sprintf(tmpstr, TI_UNSELDEL"%s"TI_UNSELDEL, line);
    }
    strcat(symstr, tmpstr);
  }
  if (cdebug) printf("Unselect: %s\n", symstr);

  if (line) free(line);
  if (tmpstr) free(tmpstr);
  
  return(symstr);

} /* TI_unselect_read */

/* parse command line args using getopt; common for m/q/conf; 
   returns config file name; 0 for no file name; or calls TI_option_help */
char *TI_option_parse(int ac, char **av) 
{
  char *name = NULL;

  while (1) {
    int option_index = 0, c, prefix_total = 0;
    static struct option long_options[6] = {
      {"path", 1, 0, 'p'},
      {"help", 1, 0, 'h'},
      {"config", 1, 0, 'c'},
      {"verbose", 1, 0, 'v'},
      {"unselect", 1, 0, 'u'},
      {0, 0, 0, 0}
    };

    c = getopt_long (ac, av, "h?p:c:v0u:",
		     long_options, &option_index);
    if (c == -1)
      break;

    switch (c) {
    case '0':	/* fall through for options parsed outside this func */
      break;
    case '?':
    case 'h':
      TI_option_help(av[0]);
      break;
    case 'p':
      if (prefix_total == TI_prefix_cnt) {
	prefix_total += 10;
	TI_path_prefix = (char **)realloc(TI_path_prefix, 
					  prefix_total * sizeof(char *));
      }
      TI_path_prefix[TI_prefix_cnt++] = strdup(optarg);
      break;
    case 'c':
      name = strdup(optarg);
      break;
    case 'v':
      cdebug++;		/* increase parse to DEBUG_PARSE */
      break;
    case 'u':
      if ((TI_unsel_str = TI_unselect_read(optarg)) != NULL)
	TI_options |= TI_UNSELECT;
	break;
    default:
      /* possible in conf and qconf option handling*/
      printf("possibly unhandled option: %c\n", c); 
    }
  }	
  if (optind < ac) /* nonoption parameters; this overrides -c */
    name = strdup(av[optind++]);
  if (!name)
    TI_option_help(av[0]);

  return(name);

} /* end TI_option_parse */
