/*
  The oSIP library implements the Session Initiation Protocol (SIP -rfc2543-)
  Copyright (C) 2001  Aymeric MOIZARD jack@atosc.org
  
  This library is free software; you can redistribute it and/or
  modify it under the terms of the GNU Lesser General Public
  License as published by the Free Software Foundation; either
  version 2.1 of the License, or (at your option) any later version.
  
  This library is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
  Lesser General Public License for more details.
  
  You should have received a copy of the GNU Lesser General Public
  License along with this library; if not, write to the Free Software
  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
*/


#ifdef ENABLE_MPATROL
#include <mpatrol.h>
#endif


#include <stdio.h>
#include <stdlib.h>
#include <osip/port.h>
#include <osip/urls.h>

int url_test_accessor_api (url_t * url);


int
main (int argc, char **argv)
{
  FILE *urls_file;


  url_t *url;
  char *a_url;
  char *dest;
  char *res;

  urls_file = fopen (argv[1], "r");
  if (urls_file == NULL)
    {
      fprintf (stdout, "Failed to open %s file.\nUsage: turls urls.txt\n",
               argv[1]);
      exit (0);
    }

  a_url = (char *) smalloc (200);
  res = fgets (a_url, 200, urls_file);  /* lines are under 200 */
  while (res != NULL)
    {
      int errcode;

      /* remove the last '\n' before parsing */
      sstrncpy (a_url + strlen (a_url) - 1, "\0", 1);

      if (0 != strncmp (a_url, "#", 1))
        {
          /* allocate & init url */
          url_init (&url);
          printf ("=================================================\n");
          printf ("URL TO PARSE: |%s|\n", a_url);
          errcode = url_parse (url, a_url);
          if (errcode != -1)
            {
              if (url_2char (url, &dest) != -1)
                {
                  printf ("result:       |%s|\n", dest);
                  url_test_accessor_api (url);
                  sfree (dest);
                }
          } else
            printf ("Bad url format: %s\n", a_url);
          url_free (url);
          sfree (url);
          printf ("=================================================\n");
        }
      res = fgets (a_url, 200, urls_file);      /* lines are under 200 */
    }
  sfree (a_url);
  return 0;
}

int
url_test_accessor_api (url_t * url)
{
  if (url->scheme != NULL)
    fprintf (stdout, "%s:", url->scheme);
  if (url->string != NULL)
    {
      fprintf (stdout, "|%s", url->string);
      fprintf (stdout, "\n");
      return 0;
    }
  if (url->username != NULL)
    fprintf (stdout, "%s|", url->username);

  if ((url->password != NULL) && (url->username != NULL))
    fprintf (stdout, ":%s|", url->password);
  if (url->username != NULL)
    fprintf (stdout, "@|");
  /*   if (url->host!=NULL)  mandatory */
  if (strchr (url->host, ':') != NULL)
    fprintf (stdout, "[%s]|", url->host);
  else
    fprintf (stdout, "%s|", url->host);
  if (url->port != NULL)
    fprintf (stdout, ":%s|", url->port);

  fprintf (stdout, "\nuri-params\n");

  {
    int pos = 0;
    url_param_t *u_param;

    while (!list_eol (url->url_params, pos))
      {
        u_param = (url_param_t *) list_get (url->url_params, pos);
        if (u_param->gvalue != NULL)
          fprintf (stdout, ";%s|=|%s|", u_param->gname, u_param->gvalue);
        else
          fprintf (stdout, ";%s|", u_param->gname);
        pos++;
      }
  }

  fprintf (stdout, "\nheaders\n");

  {
    int pos = 0;
    url_header_t *u_header;

    while (!list_eol (url->url_headers, pos))
      {
        u_header = (url_header_t *) list_get (url->url_headers, pos);
        if (pos == 0)
          fprintf (stdout, "?%s|=|%s|", u_header->gname, u_header->gvalue);
        else
          fprintf (stdout, "&%s|=|%s|", u_header->gname, u_header->gvalue);
        pos++;
      }
  }
  fprintf (stdout, "\n");
  return 0;
}
