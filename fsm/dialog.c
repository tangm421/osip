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

#include <osip/osip.h>
#include <osip/port.h>
#include "fsm.h"
#include <osip/dialog.h>


void
dialog_set_state(dialog_t *dialog, dlg_type_t state)
{
  dialog->state = state;
}

int
dialog_update_route_set_as_uas(dialog_t *dialog, sip_t *invite)
{
  contact_t *contact;
  int i;
  if (dialog->remote_contact_uri!=NULL)
    {
      contact_free(dialog->remote_contact_uri);
      sfree(dialog->remote_contact_uri);
    }
  dialog->remote_contact_uri = NULL;
  if (!list_eol(invite->contacts, 0))
    {
      contact = list_get(invite->contacts, 0);
      i = contact_clone(contact, &(dialog->remote_contact_uri));
      if (i!=0)
	return -1;
    }
  else
    {
      dialog->remote_contact_uri = NULL;
      TRACE(trace(__FILE__,__LINE__,TRACE_LEVEL1,NULL,"WARNING: Remote UA seems to be compliant with rfc2543 only (No contact in response)!\n"));
    }
  return 0;
}

int
dialog_update_cseq_as_uas(dialog_t *dialog, sip_t *invite)
{
  dialog->remote_cseq = satoi(invite->cseq->number);
  return 0;
}

int
dialog_update_route_set_as_uac(dialog_t *dialog, sip_t *response)
{
  /* only the remote target URI is updated here... */
  /* don't ask me why we call that "update of the route set"!    :-) */
  contact_t *contact;
  int i;

  /* I personnaly think it's a bad idea to keep the old
     value in case the new one is broken... */
  if (dialog->remote_contact_uri!=NULL)
    {
      contact_free(dialog->remote_contact_uri);
      sfree(dialog->remote_contact_uri);
    }
  dialog->remote_contact_uri = NULL;
  if (!list_eol(response->contacts, 0))
    {
      contact = list_get(response->contacts, 0);
      i = contact_clone(contact, &(dialog->remote_contact_uri));
      if (i!=0)
	return -1;
    }
  else
    {
      TRACE(trace(__FILE__,__LINE__,TRACE_LEVEL1,NULL,"WARNING: Remote UA seems to be compliant with rfc2543 only (No contact in response)!\n"));
    }
  return 0;
}

int
dialog_update_tag_as_uac(dialog_t *dialog, sip_t *response)
{
  generic_param_t *tag;
  int i;
  i = to_get_tag(response->to,&tag);
  if (i!=0)
    {
      TRACE(trace(__FILE__,__LINE__,TRACE_LEVEL1,NULL,"WARNING: Remote UA seems to be compliant with rfc2543 only!\n"));
      dialog->remote_tag = NULL;
    }
  else
    dialog->remote_tag = sgetcopy(tag->gvalue);
  return 0;
}

int
dialog_match_as_uac(dialog_t *dlg, sip_t *answer)
{
  generic_param_t *tag_param;
  char *tmp;
  int i;
  call_id_2char(answer->call_id, &tmp); 
  if (0!=strcmp(dlg->call_id, tmp))
    {
      sfree(tmp);
      return -1;
    }
  sfree(tmp);
  /* for UAC, the remote tag is in the from header */
  if (dlg->type==CALLER)
    i = to_get_tag(answer->to, &tag_param);
  else
    i = from_get_tag(answer->from, &tag_param);
  if (0!=i)
    {
      if (dlg->remote_tag!=NULL)  /* no tag in answer but tag in dialog */
	return -1;
      /* no tag in answer AND no tag in dialog! */
      if (dlg->type==CALLER)
	{
	  if (0==from_compare((from_t *)dlg->remote_uri,(from_t *)answer->to)
	      &&0==from_compare(dlg->local_uri,answer->from))
	    return 0;
	}else{
	  if (0==from_compare(dlg->local_uri,(from_t *)answer->to)
	      &&0==from_compare((from_t *)dlg->remote_uri,answer->from))
	    return 0;
	}
      return -1;
    }
  else
    {
      if (dlg->remote_tag==NULL) return -1;
      /* AND FOR RFC2543 User Agent??? If an early dialog was created without
	 a tag, then we can't match it with a 2xx that contains one!! */
      if (0!=from_compare((from_t *)dlg->remote_uri,(from_t *)answer->to)
	  ||0!=from_compare(dlg->local_uri,answer->from))
	{
	  if (0!=from_compare(dlg->local_uri,(from_t *)answer->to)
	      ||0!=from_compare((from_t *)dlg->remote_uri,answer->from))
	    return -1;
	}
      if (0==strcmp(tag_param->gvalue, dlg->remote_tag))
	return 0;
    }
  return -1;
}

int
dialog_match_as_uas(dialog_t *dlg, sip_t *request)
{
  int i;
  generic_param_t *tag_param;
  char *tmp;
  call_id_2char(request->call_id, &tmp); 
  if (0!=strcmp(dlg->call_id, tmp))
    {
      sfree(tmp);
      return -1;
    }
  sfree(tmp);
  /* for UAS, the remote tag is in the from header */
  if (dlg->type==CALLEE)
    i = from_get_tag(request->from, &tag_param);
  else
    i = to_get_tag(request->to, &tag_param);
  if (0!=i)
    {
      if (dlg->remote_tag!=NULL)  /* no tag in request but tag in dialog */
	return -1;
      /* no tag in request AND no tag in dialog! */
      /* this test depends if we initiate the call or not! */
      if (dlg->type==CALLEE) {
	if (0==from_compare((from_t *)dlg->local_uri,(from_t *)request->to)
	    &&0==from_compare(dlg->remote_uri,request->from))
	  return 0;
      } else { /* I was the CALLER */
	if (0==from_compare(dlg->remote_uri,(from_t *)request->to)
	    &&0==from_compare((from_t *)dlg->local_uri,request->from))
	  return 0;
      }
      
      return -1;
    }
  else
    {
      if (dlg->remote_tag==NULL) return -1;
      if (0!=from_compare((from_t *)dlg->local_uri,(from_t *)request->to)
	  ||0!=from_compare(dlg->remote_uri,request->from))
	{
	  if (0!=from_compare(dlg->remote_uri,(from_t *)request->to)
	      ||0!=from_compare((from_t *)dlg->local_uri,request->from))
	    return -1;
	}
      if (0==strcmp(tag_param->gvalue, dlg->remote_tag))
	return 0;
    }
  return -1;
}

int
dialog_init_as_uac(dialog_t **dialog, sip_t *response)
{
  int i;
  int pos;
  generic_param_t *tag;
  (*dialog) = (dialog_t*)smalloc(sizeof(dialog_t));
  if (*dialog==NULL) return -1;

  (*dialog)->type  = CALLER;
  if (MSG_IS_STATUS_2XX(response))
    (*dialog)->state = DIALOG_CONFIRMED;
  else /* 1XX */
    (*dialog)->state = DIALOG_EARLY;

  i = call_id_2char(response->call_id, &((*dialog)->call_id));
  if (i!=0)
    goto diau_error_0;

  i = from_get_tag(response->from,&tag);
  if (i!=0)
    goto diau_error_1;
  (*dialog)->local_tag = sgetcopy(tag->gvalue);

  i = to_get_tag(response->to,&tag);
  if (i!=0)
    {
      TRACE(trace(__FILE__,__LINE__,TRACE_LEVEL1,NULL,"WARNING: Remote UA seems to be compliant with rfc2543 only!\n"));
      (*dialog)->remote_tag = NULL;
    }
  else
    (*dialog)->remote_tag = sgetcopy(tag->gvalue);

  (*dialog)->route_set = (list_t*)smalloc(sizeof(list_t));
  list_init((*dialog)->route_set);

  pos=0;
  while (!list_eol(response->record_routes, pos))
    {
      record_route_t *rr;
      record_route_t *rr2;
      rr = (record_route_t*) list_get(response->record_routes, pos);
      i = record_route_clone(rr, &rr2);
      if (i!=0) goto diau_error_2;
      list_add((*dialog)->route_set, rr2, -1);
      pos++;
    }

  (*dialog)->local_cseq = satoi(response->cseq->number);
  (*dialog)->remote_cseq = -1;

  i = to_clone(response->to, &((*dialog)->remote_uri));
  if (i!=0) goto diau_error_3;

  i = from_clone(response->from, &((*dialog)->local_uri));
  if (i!=0) goto diau_error_4;

  {
    contact_t *contact;
    if (!list_eol(response->contacts, 0))
      {
	contact = list_get(response->contacts, 0);
	i = contact_clone(contact, &((*dialog)->remote_contact_uri));
	if (i!=0)
	  goto  diau_error_5;
      }
    else
      {
	(*dialog)->remote_contact_uri = NULL;
	TRACE(trace(__FILE__,__LINE__,TRACE_LEVEL1,NULL,"WARNING: Remote UA seems to be compliant with rfc2543 only! (no contact in response)\n"));
      }
  }
  (*dialog)->secure = -1; /* non secure */

  return 0;

 diau_error_5:
  from_free((*dialog)->local_uri);
  sfree((*dialog)->local_uri);
 diau_error_4:
  from_free((*dialog)->remote_uri);
  sfree((*dialog)->remote_uri);
 diau_error_3:
 diau_error_2:
  list_special_free((*dialog)->route_set, (void *(*)(void *))&record_route_free);
  sfree((*dialog)->route_set);
  sfree((*dialog)->remote_tag);
  sfree((*dialog)->local_tag);
 diau_error_1:
  sfree((*dialog)->call_id);
 diau_error_0:
  TRACE(trace(__FILE__,__LINE__,TRACE_LEVEL0,NULL,"ERROR: Establishment of dialog failed!\n"));
  sfree(*dialog);
  return -1;
}

int
dialog_init_as_uas(dialog_t **dialog, sip_t *invite, sip_t *response)
{
  int i;
  int pos;
  generic_param_t *tag;
  (*dialog) = (dialog_t*)smalloc(sizeof(dialog_t));
  if (*dialog==NULL) return -1;

  (*dialog)->type = CALLEE;
  if (MSG_IS_STATUS_2XX(response))
    (*dialog)->state = DIALOG_CONFIRMED;
  else /* 1XX */
    (*dialog)->state = DIALOG_EARLY;

  i = call_id_2char(response->call_id, &((*dialog)->call_id));
  if (i!=0)
    goto diau_error_0;

  i = to_get_tag(response->to,&tag);
  if (i!=0)
    goto diau_error_1;
  (*dialog)->local_tag = sgetcopy(tag->gvalue);

  i = from_get_tag(response->from,&tag);
  if (i!=0)
    {
      TRACE(trace(__FILE__,__LINE__,TRACE_LEVEL1,NULL,"WARNING: Remote UA seems to be compliant with rfc2543 only!\n"));
      (*dialog)->remote_tag = NULL;
    }
  else
    (*dialog)->remote_tag = sgetcopy(tag->gvalue);

  (*dialog)->route_set = (list_t*)smalloc(sizeof(list_t));
  list_init((*dialog)->route_set);

  pos=0;
  while (!list_eol(response->record_routes, pos))
    {
      record_route_t *rr;
      record_route_t *rr2;
      rr = (record_route_t*) list_get(response->record_routes, pos);
      i = record_route_clone(rr, &rr2);
      if (i!=0) goto diau_error_2;
      list_add((*dialog)->route_set, rr2, -1);
      pos++;
    }

  (*dialog)->local_cseq = -1;
  (*dialog)->remote_cseq = satoi(response->cseq->number);

  i = to_clone(response->from, &((*dialog)->remote_uri));
  if (i!=0) goto diau_error_3;

  i = from_clone(response->to, &((*dialog)->local_uri));
  if (i!=0) goto diau_error_4;

  {
    contact_t *contact;
    if (!list_eol(invite->contacts, 0))
      {
	contact = list_get(invite->contacts, 0);
	i = contact_clone(contact, &((*dialog)->remote_contact_uri));
	if (i!=0)
	  goto  diau_error_5;
      }
    else
      {
	(*dialog)->remote_contact_uri = NULL;
	TRACE(trace(__FILE__,__LINE__,TRACE_LEVEL1,NULL,"WARNING: Remote UA seems to be compliant with rfc2543 only! (no contact in response)\n"));
      }
  }
  (*dialog)->secure = -1; /* non secure */

  return 0;

 diau_error_5:
  from_free((*dialog)->local_uri);
  sfree((*dialog)->local_uri);
 diau_error_4:
  from_free((*dialog)->remote_uri);
  sfree((*dialog)->remote_uri);
 diau_error_3:
 diau_error_2:
  list_special_free((*dialog)->route_set, (void *(*)(void *))&record_route_free);
  sfree((*dialog)->route_set);
  sfree((*dialog)->remote_tag);
  sfree((*dialog)->local_tag);
 diau_error_1:
  sfree((*dialog)->call_id);
 diau_error_0:
  TRACE(trace(__FILE__,__LINE__,TRACE_LEVEL0,NULL,"ERROR: Establishment of dialog failed!\n"));
  sfree(*dialog);
  return -1;
}

void
dialog_free(dialog_t *dialog)
{
  contact_free(dialog->remote_contact_uri);
  sfree(dialog->remote_contact_uri);
  from_free(dialog->local_uri);
  sfree(dialog->local_uri);
  to_free(dialog->remote_uri);
  sfree(dialog->remote_uri);
  list_special_free(dialog->route_set, (void *(*)(void *))&record_route_free);
  sfree(dialog->route_set);
  sfree(dialog->remote_tag);
  sfree(dialog->local_tag);
  sfree(dialog->call_id);
}