/*
 * Copyright (C) 2002-2012 Free Software Foundation, Inc.
 *
 * This file is part of LIBTASN1.
 *
 * The LIBTASN1 library is free software; you can redistribute it
 * and/or modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA
 * 02110-1301, USA
 */


/*****************************************************/
/* File: coding.c                                    */
/* Description: Functions to create a DER coding of  */
/*   an ASN1 type.                                   */
/*****************************************************/

#include <int.h>
#include "parser_aux.h"
#include <gstr.h>
#include "element.h"
#include <structure.h>

#define MAX_TAG_LEN 16

/******************************************************/
/* Function : _asn1_error_description_value_not_found */
/* Description: creates the ErrorDescription string   */
/* for the ASN1_VALUE_NOT_FOUND error.                */
/* Parameters:                                        */
/*   node: node of the tree where the value is NULL.  */
/*   ErrorDescription: string returned.               */
/* Return:                                            */
/******************************************************/
static void
_asn1_error_description_value_not_found (asn1_node node,
					 char *ErrorDescription)
{

  if (ErrorDescription == NULL)
    return;

  Estrcpy (ErrorDescription, ":: value of element '");
  _asn1_hierarchical_name (node, ErrorDescription + strlen (ErrorDescription),
			   ASN1_MAX_ERROR_DESCRIPTION_SIZE - 40);
  Estrcat (ErrorDescription, "' not found");

}

/**
 * asn1_length_der:
 * @len: value to convert.
 * @ans: string returned.
 * @ans_len: number of meaningful bytes of ANS (ans[0]..ans[ans_len-1]).
 *
 * Creates the DER coding for the LEN parameter (only the length).
 * The @ans buffer is pre-allocated and must have room for the output.
 **/
void
asn1_length_der (unsigned long int len, unsigned char *ans, int *ans_len)
{
  int k;
  unsigned char temp[SIZEOF_UNSIGNED_LONG_INT];

  if (len < 128)
    {
      /* short form */
      if (ans != NULL)
	ans[0] = (unsigned char) len;
      *ans_len = 1;
    }
  else
    {
      /* Long form */
      k = 0;
      while (len)
	{
	  temp[k++] = len & 0xFF;
	  len = len >> 8;
	}
      *ans_len = k + 1;
      if (ans != NULL)
	{
	  ans[0] = ((unsigned char) k & 0x7F) + 128;
	  while (k--)
	    ans[*ans_len - 1 - k] = temp[k];
	}
    }
}

/******************************************************/
/* Function : _asn1_tag_der                           */
/* Description: creates the DER coding for the CLASS  */
/* and TAG parameters.                                */
/* Parameters:                                        */
/*   class: value to convert.                         */
/*   tag_value: value to convert.                     */
/*   ans: string returned.                            */
/*   ans_len: number of meaningful bytes of ANS       */
/*            (ans[0]..ans[ans_len-1]).               */
/* Return:                                            */
/******************************************************/
static void
_asn1_tag_der (unsigned char class, unsigned int tag_value,
	       unsigned char *ans, int *ans_len)
{
  int k;
  unsigned char temp[SIZEOF_UNSIGNED_INT];

  if (tag_value < 31)
    {
      /* short form */
      ans[0] = (class & 0xE0) + ((unsigned char) (tag_value & 0x1F));
      *ans_len = 1;
    }
  else
    {
      /* Long form */
      ans[0] = (class & 0xE0) + 31;
      k = 0;
      while (tag_value)
	{
	  temp[k++] = tag_value & 0x7F;
	  tag_value = tag_value >> 7;
	}
      *ans_len = k + 1;
      while (k--)
	ans[*ans_len - 1 - k] = temp[k] + 128;
      ans[*ans_len - 1] -= 128;
    }
}

/**
 * asn1_octet_der:
 * @str: OCTET string.
 * @str_len: STR length (str[0]..str[str_len-1]).
 * @der: string returned.
 * @der_len: number of meaningful bytes of DER (der[0]..der[ans_len-1]).
 *
 * Creates the DER coding for an OCTET type (length included).
 **/
void
asn1_octet_der (const unsigned char *str, int str_len,
		unsigned char *der, int *der_len)
{
  int len_len;

  if (der == NULL || str_len < 0)
    return;
  asn1_length_der (str_len, der, &len_len);
  memcpy (der + len_len, str, str_len);
  *der_len = str_len + len_len;
}

/******************************************************/
/* Function : _asn1_time_der                          */
/* Description: creates the DER coding for a TIME     */
/* type (length included).                            */
/* Parameters:                                        */
/*   str: TIME null-terminated string.                */
/*   der: string returned.                            */
/*   der_len: number of meaningful bytes of DER       */
/*            (der[0]..der[ans_len-1]). Initially it  */
/*            if must store the lenght of DER.        */
/* Return:                                            */
/*   ASN1_MEM_ERROR when DER isn't big enough         */
/*   ASN1_SUCCESS otherwise                           */
/******************************************************/
static int
_asn1_time_der (unsigned char *str, unsigned char *der, int *der_len)
{
  int len_len;
  int max_len;
  int str_len = _asn1_strlen (str);

  max_len = *der_len;

  asn1_length_der (str_len, (max_len > 0) ? der : NULL, &len_len);

  if ((len_len + str_len) <= max_len)
    memcpy (der + len_len, str, str_len);
  *der_len = len_len + str_len;

  if ((*der_len) > max_len)
    return ASN1_MEM_ERROR;

  return ASN1_SUCCESS;
}


/*
void
_asn1_get_utctime_der(unsigned char *der,int *der_len,unsigned char *str)
{
  int len_len,str_len;
  char temp[20];

  if(str==NULL) return;
  str_len=asn1_get_length_der(der,*der_len,&len_len);
  if (str_len<0) return;
  memcpy(temp,der+len_len,str_len);
  *der_len=str_len+len_len;
  switch(str_len){
  case 11:
    temp[10]=0;
    strcat(temp,"00+0000");
    break;
  case 13:
    temp[12]=0;
    strcat(temp,"+0000");
    break;
  case 15:
    temp[15]=0;
    memmove(temp+12,temp+10,6);
    temp[10]=temp[11]='0';
    break;
  case 17:
    temp[17]=0;
    break;
  default:
    return;
  }
  strcpy(str,temp);
}
*/

/******************************************************/
/* Function : _asn1_objectid_der                      */
/* Description: creates the DER coding for an         */
/* OBJECT IDENTIFIER  type (length included).         */
/* Parameters:                                        */
/*   str: OBJECT IDENTIFIER null-terminated string.   */
/*   der: string returned.                            */
/*   der_len: number of meaningful bytes of DER       */
/*            (der[0]..der[ans_len-1]). Initially it  */
/*            must store the length of DER.           */
/* Return:                                            */
/*   ASN1_MEM_ERROR when DER isn't big enough         */
/*   ASN1_SUCCESS otherwise                           */
/******************************************************/
static int
_asn1_objectid_der (unsigned char *str, unsigned char *der, int *der_len)
{
  int len_len, counter, k, first, max_len;
  char *temp, *n_end, *n_start;
  unsigned char bit7;
  unsigned long val, val1 = 0;
  int str_len = _asn1_strlen (str);

  max_len = *der_len;

  temp = malloc (str_len + 2);
  if (temp == NULL)
    return ASN1_MEM_ALLOC_ERROR;

  memcpy (temp, str, str_len);
  temp[str_len] = '.';
  temp[str_len + 1] = 0;

  counter = 0;
  n_start = temp;
  while ((n_end = strchr (n_start, '.')))
    {
      *n_end = 0;
      val = strtoul (n_start, NULL, 10);
      counter++;

      if (counter == 1)
	val1 = val;
      else if (counter == 2)
	{
	  if (max_len > 0)
	    der[0] = 40 * val1 + val;
	  *der_len = 1;
	}
      else
	{
	  first = 0;
	  for (k = 4; k >= 0; k--)
	    {
	      bit7 = (val >> (k * 7)) & 0x7F;
	      if (bit7 || first || !k)
		{
		  if (k)
		    bit7 |= 0x80;
		  if (max_len > (*der_len))
		    der[*der_len] = bit7;
		  (*der_len)++;
		  first = 1;
		}
	    }

	}
      n_start = n_end + 1;
    }

  asn1_length_der (*der_len, NULL, &len_len);
  if (max_len >= (*der_len + len_len))
    {
      memmove (der + len_len, der, *der_len);
      asn1_length_der (*der_len, der, &len_len);
    }
  *der_len += len_len;

  free (temp);

  if (max_len < (*der_len))
    return ASN1_MEM_ERROR;

  return ASN1_SUCCESS;
}


static const unsigned char bit_mask[] =
  { 0xFF, 0xFE, 0xFC, 0xF8, 0xF0, 0xE0, 0xC0, 0x80 };

/**
 * asn1_bit_der:
 * @str: BIT string.
 * @bit_len: number of meaningful bits in STR.
 * @der: string returned.
 * @der_len: number of meaningful bytes of DER
 *   (der[0]..der[ans_len-1]).
 *
 * Creates the DER coding for a BIT STRING type (length and pad
 * included).
 **/
void
asn1_bit_der (const unsigned char *str, int bit_len,
	      unsigned char *der, int *der_len)
{
  int len_len, len_byte, len_pad;

  if (der == NULL)
    return;
  len_byte = bit_len >> 3;
  len_pad = 8 - (bit_len & 7);
  if (len_pad == 8)
    len_pad = 0;
  else
    len_byte++;
  asn1_length_der (len_byte + 1, der, &len_len);
  der[len_len] = len_pad;
  memcpy (der + len_len + 1, str, len_byte);
  der[len_len + len_byte] &= bit_mask[len_pad];
  *der_len = len_byte + len_len + 1;
}


/******************************************************/
/* Function : _asn1_complete_explicit_tag             */
/* Description: add the length coding to the EXPLICIT */
/* tags.                                              */
/* Parameters:                                        */
/*   node: pointer to the tree element.               */
/*   der: string with the DER coding of the whole tree*/
/*   counter: number of meaningful bytes of DER       */
/*            (der[0]..der[*counter-1]).              */
/*   max_len: size of der vector                      */
/* Return:                                            */
/*   ASN1_MEM_ERROR if der vector isn't big enough,   */
/*   otherwise ASN1_SUCCESS.                          */
/******************************************************/
static int
_asn1_complete_explicit_tag (asn1_node node, unsigned char *der,
			     int *counter, int *max_len)
{
  asn1_node p;
  int is_tag_implicit, len2, len3;
  unsigned char temp[SIZEOF_UNSIGNED_INT];

  is_tag_implicit = 0;

  if (node->type & CONST_TAG)
    {
      p = node->down;
      /* When there are nested tags we must complete them reverse to
         the order they were created. This is because completing a tag
         modifies all data within it, including the incomplete tags
         which store buffer positions -- simon@josefsson.org 2002-09-06
       */
      while (p->right)
	p = p->right;
      while (p && p != node->down->left)
	{
	  if (type_field (p->type) == ASN1_ETYPE_TAG)
	    {
	      if (p->type & CONST_EXPLICIT)
		{
		  len2 = strtol (p->name, NULL, 10);
		  _asn1_set_name (p, NULL);
		  asn1_length_der (*counter - len2, temp, &len3);
		  if (len3 <= (*max_len))
		    {
		      memmove (der + len2 + len3, der + len2,
			       *counter - len2);
		      memcpy (der + len2, temp, len3);
		    }
		  *max_len -= len3;
		  *counter += len3;
		  is_tag_implicit = 0;
		}
	      else
		{		/* CONST_IMPLICIT */
		  if (!is_tag_implicit)
		    {
		      is_tag_implicit = 1;
		    }
		}
	    }
	  p = p->left;
	}
    }

  if (*max_len < 0)
    return ASN1_MEM_ERROR;

  return ASN1_SUCCESS;
}

tag_and_class_st _asn1_tags[] =
{
  [ASN1_ETYPE_GENERALSTRING] = {ASN1_TAG_GENERALSTRING, ASN1_CLASS_UNIVERSAL},
  [ASN1_ETYPE_NUMERICSTRING] = {ASN1_TAG_NUMERICSTRING, ASN1_CLASS_UNIVERSAL},
  [ASN1_ETYPE_IA5STRING] =     {ASN1_TAG_IA5STRING, ASN1_CLASS_UNIVERSAL},
  [ASN1_ETYPE_TELETEXSTRING] = {ASN1_TAG_TELETEXSTRING, ASN1_CLASS_UNIVERSAL},
  [ASN1_ETYPE_PRINTABLESTRING] = {ASN1_TAG_PRINTABLESTRING, ASN1_CLASS_UNIVERSAL},
  [ASN1_ETYPE_UNIVERSALSTRING] = {ASN1_TAG_UNIVERSALSTRING, ASN1_CLASS_UNIVERSAL},
  [ASN1_ETYPE_BMPSTRING] =     {ASN1_TAG_BMPSTRING, ASN1_CLASS_UNIVERSAL},
  [ASN1_ETYPE_UTF8STRING] =    {ASN1_TAG_UTF8STRING, ASN1_CLASS_UNIVERSAL},
  [ASN1_ETYPE_VISIBLESTRING] = {ASN1_TAG_VISIBLESTRING, ASN1_CLASS_UNIVERSAL},
  [ASN1_ETYPE_OCTET_STRING] = {ASN1_TAG_OCTET_STRING, ASN1_CLASS_UNIVERSAL},
  [ASN1_ETYPE_BIT_STRING] = {ASN1_TAG_BIT_STRING, ASN1_CLASS_UNIVERSAL},
  [ASN1_ETYPE_OBJECT_ID] =  {ASN1_TAG_OBJECT_ID, ASN1_CLASS_UNIVERSAL},
  [ASN1_ETYPE_NULL] =       {ASN1_TAG_NULL, ASN1_CLASS_UNIVERSAL},
  [ASN1_ETYPE_BOOLEAN] =    {ASN1_TAG_BOOLEAN, ASN1_CLASS_UNIVERSAL},
  [ASN1_ETYPE_INTEGER] =    {ASN1_TAG_INTEGER, ASN1_CLASS_UNIVERSAL},
  [ASN1_ETYPE_ENUMERATED] = {ASN1_TAG_ENUMERATED, ASN1_CLASS_UNIVERSAL},
  [ASN1_ETYPE_SEQUENCE] =   {ASN1_TAG_SEQUENCE, ASN1_CLASS_UNIVERSAL | ASN1_CLASS_STRUCTURED},
  [ASN1_ETYPE_SEQUENCE_OF] ={ASN1_TAG_SEQUENCE, ASN1_CLASS_UNIVERSAL | ASN1_CLASS_STRUCTURED},
  [ASN1_ETYPE_SET] =        {ASN1_TAG_SET, ASN1_CLASS_UNIVERSAL | ASN1_CLASS_STRUCTURED},
  [ASN1_ETYPE_SET_OF] =     {ASN1_TAG_SET, ASN1_CLASS_UNIVERSAL | ASN1_CLASS_STRUCTURED},
};

/******************************************************/
/* Function : _asn1_insert_tag_der                    */
/* Description: creates the DER coding of tags of one */
/* NODE.                                              */
/* Parameters:                                        */
/*   node: pointer to the tree element.               */
/*   der: string returned                             */
/*   counter: number of meaningful bytes of DER       */
/*            (counter[0]..der[*counter-1]).          */
/*   max_len: size of der vector                      */
/* Return:                                            */
/*   ASN1_GENERIC_ERROR if the type is unknown,       */
/*   ASN1_MEM_ERROR if der vector isn't big enough,   */
/*   otherwise ASN1_SUCCESS.                          */
/******************************************************/
static int
_asn1_insert_tag_der (asn1_node node, unsigned char *der, int *counter,
		      int *max_len)
{
  asn1_node p;
  int tag_len, is_tag_implicit;
  unsigned char class, class_implicit = 0, temp[SIZEOF_UNSIGNED_INT * 3 + 1];
  unsigned long tag_implicit = 0;
  unsigned char tag_der[MAX_TAG_LEN];

  is_tag_implicit = 0;

  if (node->type & CONST_TAG)
    {
      p = node->down;
      while (p)
	{
	  if (type_field (p->type) == ASN1_ETYPE_TAG)
	    {
	      if (p->type & CONST_APPLICATION)
		class = ASN1_CLASS_APPLICATION;
	      else if (p->type & CONST_UNIVERSAL)
		class = ASN1_CLASS_UNIVERSAL;
	      else if (p->type & CONST_PRIVATE)
		class = ASN1_CLASS_PRIVATE;
	      else
		class = ASN1_CLASS_CONTEXT_SPECIFIC;

	      if (p->type & CONST_EXPLICIT)
		{
		  if (is_tag_implicit)
		    _asn1_tag_der (class_implicit, tag_implicit, tag_der,
				   &tag_len);
		  else
		    _asn1_tag_der (class | ASN1_CLASS_STRUCTURED,
				   _asn1_strtoul (p->value, NULL, 10),
				   tag_der, &tag_len);

		  *max_len -= tag_len;
		  if (*max_len >= 0)
		    memcpy (der + *counter, tag_der, tag_len);
		  *counter += tag_len;

		  _asn1_ltostr (*counter, (char *) temp);
		  _asn1_set_name (p, (const char *) temp);

		  is_tag_implicit = 0;
		}
	      else
		{		/* CONST_IMPLICIT */
		  if (!is_tag_implicit)
		    {
		      if ((type_field (node->type) == ASN1_ETYPE_SEQUENCE) ||
			  (type_field (node->type) == ASN1_ETYPE_SEQUENCE_OF) ||
			  (type_field (node->type) == ASN1_ETYPE_SET) ||
			  (type_field (node->type) == ASN1_ETYPE_SET_OF))
			class |= ASN1_CLASS_STRUCTURED;
		      class_implicit = class;
		      tag_implicit = _asn1_strtoul (p->value, NULL, 10);
		      is_tag_implicit = 1;
		    }
		}
	    }
	  p = p->right;
	}
    }

  if (is_tag_implicit)
    {
      _asn1_tag_der (class_implicit, tag_implicit, tag_der, &tag_len);
    }
  else
    {
      unsigned type = type_field (node->type);
      switch (type)
	{
	case ASN1_ETYPE_TIME:
	  if (node->type & CONST_UTC)
	    {
	      _asn1_tag_der (ASN1_CLASS_UNIVERSAL, ASN1_TAG_UTCTime, tag_der,
			     &tag_len);
	    }
	  else
	    _asn1_tag_der (ASN1_CLASS_UNIVERSAL, ASN1_TAG_GENERALIZEDTime,
			   tag_der, &tag_len);
	  break;
	case ASN1_ETYPE_OBJECT_ID:
	case ASN1_ETYPE_OCTET_STRING:
	case ASN1_ETYPE_GENERALSTRING:
	case ASN1_ETYPE_NUMERICSTRING:
	case ASN1_ETYPE_IA5STRING:
	case ASN1_ETYPE_TELETEXSTRING:
	case ASN1_ETYPE_PRINTABLESTRING:
	case ASN1_ETYPE_UNIVERSALSTRING:
	case ASN1_ETYPE_BMPSTRING:
	case ASN1_ETYPE_UTF8STRING:
	case ASN1_ETYPE_VISIBLESTRING:
	case ASN1_ETYPE_BIT_STRING:
	case ASN1_ETYPE_NULL:
	case ASN1_ETYPE_BOOLEAN:
	case ASN1_ETYPE_INTEGER:
	case ASN1_ETYPE_ENUMERATED:
	case ASN1_ETYPE_SEQUENCE:
	case ASN1_ETYPE_SEQUENCE_OF:
	case ASN1_ETYPE_SET:
	case ASN1_ETYPE_SET_OF:
	  _asn1_tag_der (_asn1_tags[type].class, _asn1_tags[type].tag,
			 tag_der, &tag_len);
	  break;
	case ASN1_ETYPE_TAG:
	case ASN1_ETYPE_CHOICE:
	case ASN1_ETYPE_ANY:
	  tag_len = 0;
	  break;
	default:
	  return ASN1_GENERIC_ERROR;
	}
    }

  *max_len -= tag_len;
  if (*max_len >= 0)
    memcpy (der + *counter, tag_der, tag_len);
  *counter += tag_len;

  if (*max_len < 0)
    return ASN1_MEM_ERROR;

  return ASN1_SUCCESS;
}

/******************************************************/
/* Function : _asn1_ordering_set                      */
/* Description: puts the elements of a SET type in    */
/* the correct order according to DER rules.          */
/* Parameters:                                        */
/*   der: string with the DER coding.                 */
/*   node: pointer to the SET element.                */
/* Return:                                            */
/******************************************************/
static void
_asn1_ordering_set (unsigned char *der, int der_len, asn1_node node)
{
  struct vet
  {
    int end;
    unsigned long value;
    struct vet *next, *prev;
  };

  int counter, len, len2;
  struct vet *first, *last, *p_vet, *p2_vet;
  asn1_node p;
  unsigned char class, *temp;
  unsigned long tag;

  counter = 0;

  if (type_field (node->type) != ASN1_ETYPE_SET)
    return;

  p = node->down;
  while ((type_field (p->type) == ASN1_ETYPE_TAG)
	 || (type_field (p->type) == ASN1_ETYPE_SIZE))
    p = p->right;

  if ((p == NULL) || (p->right == NULL))
    return;

  first = last = NULL;
  while (p)
    {
      p_vet = malloc (sizeof (struct vet));
      if (p_vet == NULL)
	return;

      p_vet->next = NULL;
      p_vet->prev = last;
      if (first == NULL)
	first = p_vet;
      else
	last->next = p_vet;
      last = p_vet;

      /* tag value calculation */
      if (asn1_get_tag_der
	  (der + counter, der_len - counter, &class, &len2,
	   &tag) != ASN1_SUCCESS)
	return;
      p_vet->value = (class << 24) | tag;
      counter += len2;

      /* extraction and length */
      len2 = asn1_get_length_der (der + counter, der_len - counter, &len);
      if (len2 < 0)
	return;
      counter += len + len2;

      p_vet->end = counter;
      p = p->right;
    }

  p_vet = first;

  while (p_vet)
    {
      p2_vet = p_vet->next;
      counter = 0;
      while (p2_vet)
	{
	  if (p_vet->value > p2_vet->value)
	    {
	      /* change position */
	      temp = malloc (p_vet->end - counter);
	      if (temp == NULL)
		return;

	      memcpy (temp, der + counter, p_vet->end - counter);
	      memcpy (der + counter, der + p_vet->end,
		      p2_vet->end - p_vet->end);
	      memcpy (der + counter + p2_vet->end - p_vet->end, temp,
		      p_vet->end - counter);
	      free (temp);

	      tag = p_vet->value;
	      p_vet->value = p2_vet->value;
	      p2_vet->value = tag;

	      p_vet->end = counter + (p2_vet->end - p_vet->end);
	    }
	  counter = p_vet->end;

	  p2_vet = p2_vet->next;
	  p_vet = p_vet->next;
	}

      if (p_vet != first)
	p_vet->prev->next = NULL;
      else
	first = NULL;
      free (p_vet);
      p_vet = first;
    }
}

/******************************************************/
/* Function : _asn1_ordering_set_of                   */
/* Description: puts the elements of a SET OF type in */
/* the correct order according to DER rules.          */
/* Parameters:                                        */
/*   der: string with the DER coding.                 */
/*   node: pointer to the SET OF element.             */
/* Return:                                            */
/******************************************************/
static void
_asn1_ordering_set_of (unsigned char *der, int der_len, asn1_node node)
{
  struct vet
  {
    int end;
    struct vet *next, *prev;
  };

  int counter, len, len2, change;
  struct vet *first, *last, *p_vet, *p2_vet;
  asn1_node p;
  unsigned char *temp, class;
  unsigned long k, max;

  counter = 0;

  if (type_field (node->type) != ASN1_ETYPE_SET_OF)
    return;

  p = node->down;
  while ((type_field (p->type) == ASN1_ETYPE_TAG)
	 || (type_field (p->type) == ASN1_ETYPE_SIZE))
    p = p->right;
  p = p->right;

  if ((p == NULL) || (p->right == NULL))
    return;

  first = last = NULL;
  while (p)
    {
      p_vet = malloc (sizeof (struct vet));
      if (p_vet == NULL)
	return;

      p_vet->next = NULL;
      p_vet->prev = last;
      if (first == NULL)
	first = p_vet;
      else
	last->next = p_vet;
      last = p_vet;

      /* extraction of tag and length */
      if (der_len - counter > 0)
	{

	  if (asn1_get_tag_der
	      (der + counter, der_len - counter, &class, &len,
	       NULL) != ASN1_SUCCESS)
	    return;
	  counter += len;

	  len2 = asn1_get_length_der (der + counter, der_len - counter, &len);
	  if (len2 < 0)
	    return;
	  counter += len + len2;
	}

      p_vet->end = counter;
      p = p->right;
    }

  p_vet = first;

  while (p_vet)
    {
      p2_vet = p_vet->next;
      counter = 0;
      while (p2_vet)
	{
	  if ((p_vet->end - counter) > (p2_vet->end - p_vet->end))
	    max = p_vet->end - counter;
	  else
	    max = p2_vet->end - p_vet->end;

	  change = -1;
	  for (k = 0; k < max; k++)
	    if (der[counter + k] > der[p_vet->end + k])
	      {
		change = 1;
		break;
	      }
	    else if (der[counter + k] < der[p_vet->end + k])
	      {
		change = 0;
		break;
	      }

	  if ((change == -1)
	      && ((p_vet->end - counter) > (p2_vet->end - p_vet->end)))
	    change = 1;

	  if (change == 1)
	    {
	      /* change position */
	      temp = malloc (p_vet->end - counter);
	      if (temp == NULL)
		return;

	      memcpy (temp, der + counter, (p_vet->end) - counter);
	      memcpy (der + counter, der + (p_vet->end),
		      (p2_vet->end) - (p_vet->end));
	      memcpy (der + counter + (p2_vet->end) - (p_vet->end), temp,
		      (p_vet->end) - counter);
	      free (temp);

	      p_vet->end = counter + (p2_vet->end - p_vet->end);
	    }
	  counter = p_vet->end;

	  p2_vet = p2_vet->next;
	  p_vet = p_vet->next;
	}

      if (p_vet != first)
	p_vet->prev->next = NULL;
      else
	first = NULL;
      free (p_vet);
      p_vet = first;
    }
}

/**
 * asn1_der_coding:
 * @element: pointer to an ASN1 element
 * @name: the name of the structure you want to encode (it must be
 *   inside *POINTER).
 * @ider: vector that will contain the DER encoding. DER must be a
 *   pointer to memory cells already allocated.
 * @len: number of bytes of *@ider: @ider[0]..@ider[len-1], Initialy
 *   holds the sizeof of der vector.
 * @errorDescription : return the error description or an empty
 *   string if success.
 *
 * Creates the DER encoding for the NAME structure (inside *POINTER
 * structure).
 *
 * Returns: %ASN1_SUCCESS if DER encoding OK, %ASN1_ELEMENT_NOT_FOUND
 *   if @name is not a valid element, %ASN1_VALUE_NOT_FOUND if there
 *   is an element without a value, %ASN1_MEM_ERROR if the @ider
 *   vector isn't big enough and in this case @len will contain the
 *   length needed.
 **/
int
asn1_der_coding (asn1_node element, const char *name, void *ider, int *len,
		 char *ErrorDescription)
{
  asn1_node node, p, p2;
  unsigned char temp[SIZEOF_UNSIGNED_LONG_INT * 3 + 1];
  int counter, counter_old, len2, len3, tlen, move, max_len, max_len_old;
  int err;
  unsigned char *der = ider;

  node = asn1_find_node (element, name);
  if (node == NULL)
    return ASN1_ELEMENT_NOT_FOUND;

  /* Node is now a locally allocated variable.
   * That is because in some point we modify the
   * structure, and I don't know why! --nmav
   */
  node = _asn1_copy_structure3 (node);
  if (node == NULL)
    return ASN1_ELEMENT_NOT_FOUND;

  max_len = *len;

  counter = 0;
  move = DOWN;
  p = node;
  while (1)
    {

      counter_old = counter;
      max_len_old = max_len;
      if (move != UP)
	{
	  err = _asn1_insert_tag_der (p, der, &counter, &max_len);
	  if (err != ASN1_SUCCESS && err != ASN1_MEM_ERROR)
	    goto error;
	}
      switch (type_field (p->type))
	{
	case ASN1_ETYPE_NULL:
	  max_len--;
	  if (max_len >= 0)
	    der[counter] = 0;
	  counter++;
	  move = RIGHT;
	  break;
	case ASN1_ETYPE_BOOLEAN:
	  if ((p->type & CONST_DEFAULT) && (p->value == NULL))
	    {
	      counter = counter_old;
	      max_len = max_len_old;
	    }
	  else
	    {
	      if (p->value == NULL)
		{
		  _asn1_error_description_value_not_found (p,
							   ErrorDescription);
		  err = ASN1_VALUE_NOT_FOUND;
		  goto error;
		}
	      max_len -= 2;
	      if (max_len >= 0)
		{
		  der[counter++] = 1;
		  if (p->value[0] == 'F')
		    der[counter++] = 0;
		  else
		    der[counter++] = 0xFF;
		}
	      else
		counter += 2;
	    }
	  move = RIGHT;
	  break;
	case ASN1_ETYPE_INTEGER:
	case ASN1_ETYPE_ENUMERATED:
	  if ((p->type & CONST_DEFAULT) && (p->value == NULL))
	    {
	      counter = counter_old;
	      max_len = max_len_old;
	    }
	  else
	    {
	      if (p->value == NULL)
		{
		  _asn1_error_description_value_not_found (p,
							   ErrorDescription);
		  err = ASN1_VALUE_NOT_FOUND;
		  goto error;
		}
	      len2 = asn1_get_length_der (p->value, p->value_len, &len3);
	      if (len2 < 0)
		{
		  err = ASN1_DER_ERROR;
		  goto error;
		}
	      max_len -= len2 + len3;
	      if (max_len >= 0)
		memcpy (der + counter, p->value, len3 + len2);
	      counter += len3 + len2;
	    }
	  move = RIGHT;
	  break;
	case ASN1_ETYPE_OBJECT_ID:
	  if ((p->type & CONST_DEFAULT) && (p->value == NULL))
	    {
	      counter = counter_old;
	      max_len = max_len_old;
	    }
	  else
	    {
	      if (p->value == NULL)
		{
		  _asn1_error_description_value_not_found (p,
							   ErrorDescription);
		  err = ASN1_VALUE_NOT_FOUND;
		  goto error;
		}
	      len2 = max_len;
	      err = _asn1_objectid_der (p->value, der + counter, &len2);
	      if (err != ASN1_SUCCESS && err != ASN1_MEM_ERROR)
		goto error;

	      max_len -= len2;
	      counter += len2;
	    }
	  move = RIGHT;
	  break;
	case ASN1_ETYPE_TIME:
	  if (p->value == NULL)
	    {
	      _asn1_error_description_value_not_found (p, ErrorDescription);
	      err = ASN1_VALUE_NOT_FOUND;
	      goto error;
	    }
	  len2 = max_len;
	  err = _asn1_time_der (p->value, der + counter, &len2);
	  if (err != ASN1_SUCCESS && err != ASN1_MEM_ERROR)
	    goto error;

	  max_len -= len2;
	  counter += len2;
	  move = RIGHT;
	  break;
	case ASN1_ETYPE_OCTET_STRING:
	case ASN1_ETYPE_GENERALSTRING:
	case ASN1_ETYPE_NUMERICSTRING:
	case ASN1_ETYPE_IA5STRING:
	case ASN1_ETYPE_TELETEXSTRING:
	case ASN1_ETYPE_PRINTABLESTRING:
	case ASN1_ETYPE_UNIVERSALSTRING:
	case ASN1_ETYPE_BMPSTRING:
	case ASN1_ETYPE_UTF8STRING:
	case ASN1_ETYPE_VISIBLESTRING:
	case ASN1_ETYPE_BIT_STRING:
	  if (p->value == NULL)
	    {
	      _asn1_error_description_value_not_found (p, ErrorDescription);
	      err = ASN1_VALUE_NOT_FOUND;
	      goto error;
	    }
	  len2 = asn1_get_length_der (p->value, p->value_len, &len3);
	  if (len2 < 0)
	    {
	      err = ASN1_DER_ERROR;
	      goto error;
	    }
	  max_len -= len2 + len3;
	  if (max_len >= 0)
	    memcpy (der + counter, p->value, len3 + len2);
	  counter += len3 + len2;
	  move = RIGHT;
	  break;
	case ASN1_ETYPE_SEQUENCE:
	case ASN1_ETYPE_SET:
	  if (move != UP)
	    {
	      _asn1_ltostr (counter, (char *) temp);
	      tlen = _asn1_strlen (temp);
	      if (tlen > 0)
		_asn1_set_value (p, temp, tlen + 1);
	      if (p->down == NULL)
		{
		  move = UP;
		  continue;
		}
	      else
		{
		  p2 = p->down;
		  while (p2 && (type_field (p2->type) == ASN1_ETYPE_TAG))
		    p2 = p2->right;
		  if (p2)
		    {
		      p = p2;
		      move = RIGHT;
		      continue;
		    }
		  move = UP;
		  continue;
		}
	    }
	  else
	    {			/* move==UP */
	      len2 = _asn1_strtol (p->value, NULL, 10);
	      _asn1_set_value (p, NULL, 0);
	      if ((type_field (p->type) == ASN1_ETYPE_SET) && (max_len >= 0))
		_asn1_ordering_set (der + len2, max_len - len2, p);
	      asn1_length_der (counter - len2, temp, &len3);
	      max_len -= len3;
	      if (max_len >= 0)
		{
		  memmove (der + len2 + len3, der + len2, counter - len2);
		  memcpy (der + len2, temp, len3);
		}
	      counter += len3;
	      move = RIGHT;
	    }
	  break;
	case ASN1_ETYPE_SEQUENCE_OF:
	case ASN1_ETYPE_SET_OF:
	  if (move != UP)
	    {
	      _asn1_ltostr (counter, (char *) temp);
	      tlen = _asn1_strlen (temp);

	      if (tlen > 0)
		_asn1_set_value (p, temp, tlen + 1);
	      p = p->down;
	      while ((type_field (p->type) == ASN1_ETYPE_TAG)
		     || (type_field (p->type) == ASN1_ETYPE_SIZE))
		p = p->right;
	      if (p->right)
		{
		  p = p->right;
		  move = RIGHT;
		  continue;
		}
	      else
		p = _asn1_find_up (p);
	      move = UP;
	    }
	  if (move == UP)
	    {
	      len2 = _asn1_strtol (p->value, NULL, 10);
	      _asn1_set_value (p, NULL, 0);
	      if ((type_field (p->type) == ASN1_ETYPE_SET_OF)
		  && (max_len - len2 > 0))
		{
		  _asn1_ordering_set_of (der + len2, max_len - len2, p);
		}
	      asn1_length_der (counter - len2, temp, &len3);
	      max_len -= len3;
	      if (max_len >= 0)
		{
		  memmove (der + len2 + len3, der + len2, counter - len2);
		  memcpy (der + len2, temp, len3);
		}
	      counter += len3;
	      move = RIGHT;
	    }
	  break;
	case ASN1_ETYPE_ANY:
	  if (p->value == NULL)
	    {
	      _asn1_error_description_value_not_found (p, ErrorDescription);
	      err = ASN1_VALUE_NOT_FOUND;
	      goto error;
	    }
	  len2 = asn1_get_length_der (p->value, p->value_len, &len3);
	  if (len2 < 0)
	    {
	      err = ASN1_DER_ERROR;
	      goto error;
	    }
	  max_len -= len2;
	  if (max_len >= 0)
	    memcpy (der + counter, p->value + len3, len2);
	  counter += len2;
	  move = RIGHT;
	  break;
	default:
	  move = (move == UP) ? RIGHT : DOWN;
	  break;
	}

      if ((move != DOWN) && (counter != counter_old))
	{
	  err = _asn1_complete_explicit_tag (p, der, &counter, &max_len);
	  if (err != ASN1_SUCCESS && err != ASN1_MEM_ERROR)
	    goto error;
	}

      if (p == node && move != DOWN)
	break;

      if (move == DOWN)
	{
	  if (p->down)
	    p = p->down;
	  else
	    move = RIGHT;
	}
      if (move == RIGHT)
	{
	  if (p->right)
	    p = p->right;
	  else
	    move = UP;
	}
      if (move == UP)
	p = _asn1_find_up (p);
    }

  *len = counter;

  if (max_len < 0)
    {
      err = ASN1_MEM_ERROR;
      goto error;
    }

  err = ASN1_SUCCESS;

error:
  asn1_delete_structure (&node);
  return err;
}
