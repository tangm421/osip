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

#include <osip/sdp.h>
#include <osip/sdp_negoc.h>
#include <osip/port.h>

int test_sdp_message (char *msg, int verbose);
int test_accessor_get_api (sdp_t * sdp);
int test_accessor_set_api (sdp_t * sdp);

typedef struct _ua_context_t
{

  /* only one audio port is allowed at this time.. In the case, we
     receive more than one m audio line, this may fail... */
  char *m_audio_port;           /* audio port to be used for this session */

}
ua_context_t;

ua_context_t *ua_context = NULL;


int
ua_sdp_accept_audio_codec (sdp_context_t * context,
                           char *port, char *number_of_port,
                           int audio_qty, char *payload)
{
  /* this may come from buggy implementation who                 */
  /* propose several sdp lines while they only want 1 connection */
  if (0 != audio_qty)
    return -1;

  if (0 == strncmp (payload, "0", 1) || 0 == strncmp (payload, "3", 1) ||
      0 == strncmp (payload, "7", 1) || 0 == strncmp (payload, "8", 1))
    return 0;
  return -1;
}

int
ua_sdp_accept_video_codec (sdp_context_t * context,
                           char *port, char *number_of_port,
                           int video_qty, char *payload)
{
  /* this may come from buggy implementation who                 */
  /* propose several sdp lines while they only want 1 connection */
  if (0 != video_qty)
    return -1;
  /* ... */
  return -1;
}

int
ua_sdp_accept_other_codec (sdp_context_t * context,
                           char *type, char *port,
                           char *number_of_port, char *payload)
{
  /* ... */
  return -1;
}

char *
ua_sdp_get_audio_port (sdp_context_t * context, int pos_media)
{
  ua_context_t *ua_con;

  ua_con = (ua_context_t *) context->mycontext;
  return sgetcopy (ua_con->m_audio_port);       /* this port should not be static ... */
  /* also, this method should be called more than once... */
  /* If there is more than one audio line, this may fail :( */
}

int
main (int argc, char **argv)
{
  int i;
  int verbose = 0;              /* 0: verbose, 1 (or nothing: not verbose) */
  char *marker;
  FILE *torture_file;
  char *tmp;
  char *msg;
  char *tmpmsg;
  static int num_test = 0;


  if (argc > 3)
    {
      if (0 == strncmp (argv[3], "-v", 2))
        verbose = 1;
    }

  torture_file = fopen (argv[1], "r");
  if (torture_file == NULL)
    {
      fprintf (stderr,
               "Failed to open \"torture_sdps\" file.\nUsage: %s torture_file [-v]\n",
               argv[0]);
      exit (1);
    }

  ua_context = (ua_context_t *) smalloc (sizeof (ua_context_t));

  ua_context->m_audio_port = sgetcopy ("20030");

  i = sdp_config_init ();
  if (i != 0)
    {
      fprintf (stderr, "Failed to initialize the SDP negociator\n");
      exit (1);
    }
  sdp_config_set_o_username (sgetcopy ("userX"));
  sdp_config_set_o_session_id (sgetcopy ("20000001"));
  sdp_config_set_o_session_version (sgetcopy ("20000001"));
  sdp_config_set_o_nettype (sgetcopy ("IN"));
  sdp_config_set_o_addrtype (sgetcopy ("IP4"));
  sdp_config_set_o_addr (sgetcopy ("192.168.1.114"));

  sdp_config_set_c_nettype (sgetcopy ("IN"));
  sdp_config_set_c_addrtype (sgetcopy ("IP4"));
  sdp_config_set_c_addr (sgetcopy ("192.168.1.114"));

  /* ALL CODEC MUST SHARE THE SAME "C=" line and proto as the media 
     will appear on the same "m" line... */
  sdp_config_add_support_for_audio_codec (sgetcopy ("0"),
                                          NULL,
                                          sgetcopy ("RTP/AVP"),
                                          NULL, NULL, NULL,
                                          NULL, NULL, sgetcopy ("0 PCMU/8000"));
  sdp_config_add_support_for_audio_codec (sgetcopy ("3"),
                                          NULL,
                                          sgetcopy ("RTP/AVP"),
                                          NULL, NULL, NULL,
                                          NULL, NULL, sgetcopy ("3 GSM/8000"));
  sdp_config_add_support_for_audio_codec (sgetcopy ("7"),
                                          NULL,
                                          sgetcopy ("RTP/AVP"),
                                          NULL, NULL, NULL,
                                          NULL, NULL, sgetcopy ("7 LPC/8000"));
  sdp_config_add_support_for_audio_codec (sgetcopy ("8"),
                                          NULL,
                                          sgetcopy ("RTP/AVP"),
                                          NULL, NULL, NULL,
                                          NULL, NULL, sgetcopy ("8 PCMA/8000"));

  sdp_config_set_fcn_accept_audio_codec (&ua_sdp_accept_audio_codec);
  sdp_config_set_fcn_accept_video_codec (&ua_sdp_accept_video_codec);
  sdp_config_set_fcn_accept_other_codec (&ua_sdp_accept_other_codec);
  sdp_config_set_fcn_get_audio_port (&ua_sdp_get_audio_port);

  i = 0;
  tmp = (char *) smalloc (500);
  marker = fgets (tmp, 500, torture_file);      /* lines are under 500 */
  while (marker != NULL && i < atoi (argv[2]))
    {
      if (0 == strncmp (tmp, "|", 1))
        i++;
      marker = fgets (tmp, 500, torture_file);
    }

  num_test++;

  msg = (char *) smalloc (10000);       /* msg are under 10000 */
  tmpmsg = msg;

  if (marker == NULL)
    {
      fprintf (stderr,
               "Error! The message's number you specified does not exist\n");
      exit (1);                 /* end of file detected! */
    }
  /* this part reads an entire message, separator is "|" */
  /* (it is unlinkely that it will appear in messages!) */
  while (marker != NULL && strncmp (tmp, "|", 1))
    {
      sstrncpy (tmpmsg, tmp, strlen (tmp));
      tmpmsg = tmpmsg + strlen (tmp);
      marker = fgets (tmp, 500, torture_file);
    }

  if (verbose)
    {
      fprintf (stdout, "test %s : ============================ \n", argv[2]);
      fprintf (stdout, "%s", msg);

      if (0 == test_sdp_message (msg, verbose))
        fprintf (stdout, "test %s : ============================ OK\n", argv[2]);
      else
        fprintf (stdout, "test %s : ============================ FAILED\n",
                 argv[2]);
  } else
    {
      if (0 == test_sdp_message (msg, verbose))
        fprintf (stdout, "test %s : OK\n", argv[2]);
      else
        fprintf (stdout, "test %s : FAILED\n", argv[2]);
    }

  sfree (msg);
  sfree (tmp);
  sdp_config_free ();
  sfree (ua_context->m_audio_port);
  sfree (ua_context);
  return 0;
}

int
test_sdp_message (char *msg, int verbose)
{
  sdp_t *sdp;

  {
    char *result;

    sdp_init (&sdp);
    if (sdp_parse (sdp, msg) != 0)
      {
        fprintf (stdout, "ERROR: failed while parsing!\n");
        sdp_free (sdp);         /* try to free msg, even if it failed! */
        /* this seems dangerous..... */
        return -1;
    } else
      {
        int i;

        i = sdp_2char (sdp, &result);
        test_accessor_get_api (sdp);
        if (i == -1)
          {
            fprintf (stdout, "ERROR: failed while printing message!\n");
            sdp_free (sdp);
            sfree (sdp);
            return -1;
        } else
          {
            if (verbose)
              fprintf (stdout, "%s", result);
            if (strlen (result) != strlen (msg))
              fprintf (stdout, "length differ from original message!\n");
            if (0 == strncmp (result, msg, strlen (result)))
              fprintf (stdout, "result equals msg!!\n");
            sfree (result);
            {
              sdp_context_t *context;

              sdp_t *dest;

              i = sdp_context_init (&context);
              i = sdp_context_set_mycontext (context, (void *) ua_context);

              {
                sdp_t *sdp;

                sdp_build_offer (context, &sdp, "10020", "10020");
                sdp_2char (sdp, &result);
                fprintf (stdout, "Here is the offer:\n%s\n", result);
                sfree (result);
                sdp_put_on_hold (sdp);
                sdp_2char (sdp, &result);
                fprintf (stdout, "Here is the offer on hold:\n%s\n", result);
                sfree (result);
                sdp_free (sdp);
                sfree (sdp);
              }



              i = sdp_context_set_remote_sdp (context, sdp);
              if (i != 0)
                {
                  fprintf (stdout,
                           "Initialisation of context failed. Could not negociate\n");
              } else
                {
                  fprintf (stdout, "Trying to execute a SIP negociation:\n");
                  i = sdp_context_execute_negociation (context);
                  fprintf (stdout, "return code: %i\n", i);
                  if (i == 200)
                    {
                      dest = sdp_context_get_local_sdp (context);
                      fprintf (stdout, "SDP answer:\n");
                      i = sdp_2char (dest, &result);
                      if (i != 0)
                        fprintf (stdout,
                                 "Error found in SDP answer while printing\n");
                      else
                        fprintf (stdout, "%s\n", result);
                      sfree (result);
                    }
                  sdp_context_free (context);
                  sfree (context);
                  return 0;
                }
            }
          }
        sdp_free (sdp);
        sfree (sdp);
      }
  }
  return 0;
}

int
test_accessor_set_api (sdp_t * sdp)
{
  return 0;
}

int
test_accessor_get_api (sdp_t * sdp)
{
  char *tmp;
  char *tmp2;
  char *tmp3;
  char *tmp4;
  char *tmp5;
  int i;
  int k;

  printf ("v_version:      |%s|\n", sdp_v_version_get (sdp));
  printf ("o_originator:   |%s|", sdp_o_username_get (sdp));
  printf (" |%s|", sdp_o_sess_id_get (sdp));
  printf (" |%s|", sdp_o_sess_version_get (sdp));
  printf (" |%s|", sdp_o_nettype_get (sdp));
  printf (" |%s|", sdp_o_addrtype_get (sdp));
  printf (" |%s|\n", sdp_o_addr_get (sdp));
  if (sdp_s_name_get (sdp))
    printf ("s_name:         |%s|\n", sdp_s_name_get (sdp));
  if (sdp_i_info_get (sdp, -1))
    printf ("i_info:         |%s|\n", sdp_i_info_get (sdp, -1));
  if (sdp_u_uri_get (sdp))
    printf ("u_uri:          |%s|\n", sdp_u_uri_get (sdp));

  i = 0;
  do
    {
      tmp = sdp_e_email_get (sdp, i);
      if (tmp != NULL)
        printf ("e_email:        |%s|\n", tmp);
      i++;
    }
  while (tmp != NULL);
  i = 0;
  do
    {
      tmp = sdp_p_phone_get (sdp, i);
      if (tmp != NULL)
        printf ("p_phone:        |%s|\n", tmp);
      i++;
    }
  while (tmp != NULL);

  k = 0;
  tmp = sdp_c_nettype_get (sdp, -1, k);
  tmp2 = sdp_c_addrtype_get (sdp, -1, k);
  tmp3 = sdp_c_addr_get (sdp, -1, k);
  tmp4 = sdp_c_addr_multicast_ttl_get (sdp, -1, k);
  tmp5 = sdp_c_addr_multicast_int_get (sdp, -1, k);
  if (tmp != NULL)
    printf ("c_connection:   |%s| |%s| |%s| |%s| |%s|\n",
            tmp, tmp2, tmp3, tmp4, tmp5);

  k = 0;
  do
    {
      tmp = sdp_b_bwtype_get (sdp, -1, k);
      tmp2 = sdp_b_bandwidth_get (sdp, -1, k);
      if (tmp != NULL)
        printf ("b_bandwidth:    |%s|:|%s|\n", tmp, tmp2);
      k++;
    }
  while (tmp != NULL);

  k = 0;
  do
    {
      tmp = sdp_t_start_time_get (sdp, k);
      tmp2 = sdp_t_stop_time_get (sdp, k);
      if (tmp != NULL)
        printf ("t_descr_time:   |%s| |%s|\n", tmp, tmp2);
      i = 0;
      do
        {
          tmp2 = sdp_r_repeat_get (sdp, k, i);
          i++;
          if (tmp2 != NULL)
            printf ("r_repeat:    |%s|\n", tmp2);
        }
      while (tmp2 != NULL);
      k++;
    }
  while (tmp != NULL);

  /* TODO r */

  if (sdp_z_adjustments_get (sdp) != NULL)
    printf ("z_adjustments:  |%s|\n", sdp_z_adjustments_get (sdp));

  tmp = sdp_k_keytype_get (sdp, -1);
  tmp2 = sdp_k_keydata_get (sdp, -1);
  if (tmp != NULL)
    printf ("k_key:          |%s|:|%s|\n", tmp, tmp2);

  k = 0;
  do
    {
      tmp = sdp_a_att_field_get (sdp, -1, k);
      tmp2 = sdp_a_att_value_get (sdp, -1, k);
      if (tmp != NULL)
        printf ("a_attribute:    |%s|:|%s|\n", tmp, tmp2);
      k++;
    }
  while (tmp != NULL);

  i = 0;
  while (!sdp_endof_media (sdp, i))
    {

      tmp = sdp_m_media_get (sdp, i);
      tmp2 = sdp_m_port_get (sdp, i);
      tmp3 = sdp_m_number_of_port_get (sdp, i);
      tmp4 = sdp_m_proto_get (sdp, i);
      if (tmp != NULL)
        printf ("m_media:        |%s| |%s| |%s| |%s|", tmp, tmp2, tmp3, tmp4);
      k = 0;
      do
        {
          tmp = sdp_m_payload_get (sdp, i, k);
          if (tmp != NULL)
            printf (" |%s|", tmp);
          k++;
        }
      while (tmp != NULL);
      printf ("\n");
      k = 0;
      do
        {
          tmp = sdp_c_nettype_get (sdp, i, k);
          tmp2 = sdp_c_addrtype_get (sdp, i, k);
          tmp3 = sdp_c_addr_get (sdp, i, k);
          tmp4 = sdp_c_addr_multicast_ttl_get (sdp, i, k);
          tmp5 = sdp_c_addr_multicast_int_get (sdp, i, k);
          if (tmp != NULL)
            printf ("c_connection:   |%s| |%s| |%s| |%s| |%s|\n",
                    tmp, tmp2, tmp3, tmp4, tmp5);
          k++;
        }
      while (tmp != NULL);

      k = 0;
      do
        {
          tmp = sdp_b_bwtype_get (sdp, i, k);
          tmp2 = sdp_b_bandwidth_get (sdp, i, k);
          if (tmp != NULL)
            printf ("b_bandwidth:    |%s|:|%s|\n", tmp, tmp2);
          k++;
        }
      while (tmp != NULL);


      tmp = sdp_k_keytype_get (sdp, i);
      tmp2 = sdp_k_keydata_get (sdp, i);
      if (tmp != NULL)
        printf ("k_key:          |%s|:|%s|\n", tmp, tmp2);

      k = 0;
      do
        {
          tmp = sdp_a_att_field_get (sdp, i, k);
          tmp2 = sdp_a_att_value_get (sdp, i, k);
          if (tmp != NULL)
            printf ("a_attribute:    |%s|:|%s|\n", tmp, tmp2);
          k++;
        }
      while (tmp != NULL);

      i++;
    }

  return 0;
}

/*
int
ua_sdp_set_info(sdp_context_t *context, sdp_t *dest) {
  return 0;

}
int
ua_sdp_set_uri(sdp_context_t *context, sdp_t *dest) {
  return 0;
}

int
ua_sdp_add_email(sdp_context_t *context, sdp_t *dest) {
  return 0;
}

int
ua_sdp_add_phone(sdp_context_t *context, sdp_t *dest) {
  return 0;
}

int
ua_sdp_add_attributes(sdp_context_t *context, sdp_t *dest, int pos_media) {
  return 0;
}
*/
