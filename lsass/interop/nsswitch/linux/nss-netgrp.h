/* Editor Settings: expandtabs and use 4 spaces for indentation
 * ex: set softtabstop=4 tabstop=8 expandtab shiftwidth=4: *
 * -*- mode: c, c-basic-offset: 4 -*- */

/*
 * Copyright Likewise Software    2004-2008
 * All rights reserved.
 *
 * This library is free software; you can redistribute it and/or modify it
 * under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation; either version 2.1 of the license, or (at
 * your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of 
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU Lesser
 * General Public License for more details.  You should have received a copy
 * of the GNU Lesser General Public License along with this program.  If
 * not, see <http://www.gnu.org/licenses/>.
 *
 */

/*********************************************************************/

/* Parts of this file were taken from glibc-2.7
   and are subject to the following copyright notice
   -------------------------------------------------------------------

   Copyright (C) 1996, 1997, 2004 Free Software Foundation, Inc.
   This file is part of the GNU C Library.

   The GNU C Library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Lesser General Public
   License as published by the Free Software Foundation; either
   version 2.1 of the License, or (at your option) any later version.

   The GNU C Library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Lesser General Public License for more details.

   You should have received a copy of the GNU Lesser General Public
   License along with the GNU C Library; if not, write to the Free
   Software Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA
   02111-1307 USA.  */

/*********************************************************************/

/* A netgroup can consist of names of other netgroups.  We have to
   track which netgroups were read and which still have to be read.  */
struct name_list
{
  struct name_list *next;
  char name[0];
};


/* Dataset for iterating netgroups.  */
struct __netgrent
{
  enum { triple_val, group_val } type;

  union
  {
    struct
    {
      const char *host;
      const char *user;
      const char *domain;
    }
    triple;

    const char *group;
  } val;

  /* Room for the data kept between the calls to the netgroup
     functions.  We must avoid global variables.  */
  char *data;
  size_t data_size;
  union
  {
    char *cursor;
    unsigned long int position;
  };
  int first;

  struct name_list *known_groups;
  struct name_list *needed_groups;
};

NSS_STATUS
_nss_lsass_setnetgrent (
    char *group,
    struct __netgrent *result
    );

NSS_STATUS
_nss_lsass_endnetgrent (
    struct __netgrent * result
    );

NSS_STATUS
_nss_lsass_getnetgrent_r (
    struct __netgrent *result,
    char *buffer,
    size_t buflen,
    int *errnop
    );
