/*  Copyright (c) 2002, 2003, 2006 The Regents of the University of Michigan.
 *  All rights reserved.
 *
 *  Redistribution and use in source and binary forms, with or without
 *  modification, are permitted provided that the following conditions
 *  are met:
 *
 *  1. Redistributions of source code must retain the above copyright
 *     notice, this list of conditions and the following disclaimer.
 *  2. Redistributions in binary form must reproduce the above copyright
 *     notice, this list of conditions and the following disclaimer in the
 *     documentation and/or other materials provided with the distribution.
 *  3. Neither the name of the University nor the names of its
 *     contributors may be used to endorse or promote products derived
 *     from this software without specific prior written permission.
 *
 *  THIS SOFTWARE IS PROVIDED ``AS IS'' AND ANY EXPRESS OR IMPLIED
 *  WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 *  MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 *  DISCLAIMED. IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 *  FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 *  CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 *  SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR
 *  BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
 *  LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
 *  NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 *  SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */


#ifndef LIBACL_NFS4_H
#define LIBACL_NFS4_H 1

#include <sys/types.h>
#include <pwd.h>
#include <grp.h>
#include <stdlib.h>
#include <stdio.h>
#include <sys/queue.h>
#include <sys/errno.h>
#include <string.h>
#include "nfs4.h"

/* flags'/perms' corresponding display characters */
#define TYPE_ALLOW			'A'
#define TYPE_DENY 			'D'
#define TYPE_AUDIT			'U'
#define TYPE_ALARM			'L'

#define FLAG_FILE_INHERIT		'f'
#define FLAG_DIR_INHERIT		'd'
#define FLAG_NO_PROPAGATE_INHERIT	'n'
#define FLAG_INHERIT_ONLY		'i'
#define FLAG_SUCCESSFUL_ACCESS		'S'
#define FLAG_FAILED_ACCESS		'F'
#define FLAG_GROUP			'g'
#define FLAG_OWNER_AT			'O'
#define FLAG_GROUP_AT			'G'
#define FLAG_EVERYONE_AT		'E'

#define PERM_READ_DATA			'r'
#define PERM_WRITE_DATA			'w'
#define PERM_APPEND_DATA		'a'

#define PERM_LIST_DIR			PERM_READ_DATA
#define PERM_CREATE_FILE		PERM_WRITE_DATA
#define PERM_CREATE_SUBDIR		PERM_APPEND_DATA
#define PERM_DELETE_CHILD		'D'

#define PERM_DELETE			'd'
#define PERM_EXECUTE			'x'
#define PERM_READ_ATTR			't'
#define PERM_WRITE_ATTR			'T'
#define PERM_READ_NAMED_ATTR		'n'
#define PERM_WRITE_NAMED_ATTR		'N'
#define PERM_READ_ACL			'c'
#define PERM_WRITE_ACL			'C'
#define PERM_WRITE_OWNER		'o'
#define PERM_SYNCHRONIZE		'y'

#define PERM_GENERIC_READ		'R'
#define PERM_GENERIC_WRITE		'W'
#define PERM_GENERIC_EXECUTE		'X'

/* mode bit translations: */
#define NFS4_READ_MODE NFS4_ACE_READ_DATA
#define NFS4_WRITE_MODE (NFS4_ACE_WRITE_DATA \
		| NFS4_ACE_APPEND_DATA | NFS4_ACE_DELETE_CHILD)
#define NFS4_EXECUTE_MODE NFS4_ACE_EXECUTE
#define NFS4_ANYONE_MODE (NFS4_ACE_READ_ATTRIBUTES | NFS4_ACE_READ_ACL | \
		NFS4_ACE_SYNCHRONIZE)
#define NFS4_OWNER_MODE (NFS4_ACE_WRITE_ATTRIBUTES | NFS4_ACE_WRITE_ACL)

/* flags used to simulate posix default ACLs */
#define NFS4_INHERITANCE_FLAGS (NFS4_ACE_FILE_INHERIT_ACE \
		| NFS4_ACE_DIRECTORY_INHERIT_ACE | NFS4_ACE_INHERIT_ONLY_ACE)

#define NFS4_ACE_MASK_IGNORE (NFS4_ACE_DELETE | NFS4_ACE_WRITE_OWNER \
		| NFS4_ACE_READ_NAMED_ATTRS | NFS4_ACE_WRITE_NAMED_ATTRS)
/* XXX not sure about the following.  Note that e.g. DELETE_CHILD is wrong in
 * general (should only be ignored on files). */
#define MASK_EQUAL(mask1, mask2) \
	(((mask1) & NFS4_ACE_MASK_ALL & ~NFS4_ACE_MASK_IGNORE & \
	  					~NFS4_ACE_DELETE_CHILD) \
	 == ((mask2) & NFS4_ACE_MASK_ALL & ~NFS4_ACE_MASK_IGNORE & \
		 				~NFS4_ACE_DELETE_CHILD))

/*
 * NFS4_MAX_ACESIZE -- the number of bytes in the string representation we
 * read in (not the same as on-the-wire, which is also not the same as how
 * NFSD actually stores the ACEs).
 *
 * Note that right now NFSD tolerates at most 170 ACEs, regardless of size,
 * and linux in general tolerates at most 64KB xattrs.
 *
 * :     3 of these
 * type  1
 * flag  7 (total number of flag characters)
 * who   NFS4_MAX_PRINCIPALSIZE  (user:128, domain:256, '@':1, NULL:1)
 * mask  14 (total number of dir + common mask characters)
 *
 * which equals 410.  let's try that for now.
 */
#define NFS4_MAX_ACESIZE	(3 + 1 + 7 + NFS4_MAX_PRINCIPALSIZE + 14)

/*
 * NFS4_MAX_ACLSIZE -- the number of bytes in the string representation
 * of a whole ACL, regardless of the number of ACEs; used to set buffer
 * sizes.  since linux limits xattrs to 64KB anyway, we don't have to
 * worry about/can't really handle huge ACLs.  while the string
 * representation doesn't directly compare to the xattr size, this
 * is probably a reasonable guess.
 */
#define NFS4_MAX_ACLSIZE	(65536)


/* NFS4 acl xattr name */
#define ACL_NFS4_XATTR "system.nfs4_acl"

/* Macro for finding empty tailqs */
#define TAILQ_IS_EMPTY(head) (head.tqh_first == NULL)

/* Flags to pass certain properties around */
#define NFS4_ACL_NOFLAGS			0x00
#define NFS4_ACL_ISFILE				0x00
#define NFS4_ACL_ISDIR				0x01
#define NFS4_ACL_OWNER				0x02
#define NFS4_ACL_REQUEST_DEFAULT	0x04
#define NFS4_ACL_RAW				0x01

#define NFS4_XDR_MOD				4

typedef u_int32_t u32;

enum {	ACL_NFS4_NOT_USED = 0,
		ACL_NFS4_USED
};

struct ace_container {
	struct nfs4_ace *ace;
	TAILQ_ENTRY(ace_container) l_ace;
};

TAILQ_HEAD(ace_container_list_head, ace_container);

/**** Public functions ****/

/** Manipulation functions **/
extern int			acl_nfs4_set_who(struct nfs4_ace*, int, char*);
extern struct nfs4_acl *	acl_nfs4_copy_acl(struct nfs4_acl *);
extern struct nfs4_acl *	acl_nfs4_xattr_load(char *, int, u32);
extern int			acl_nfs4_xattr_pack(struct nfs4_acl *, char**);
extern int			acl_nfs4_xattr_size(struct nfs4_acl *);

extern void			nfs4_free_acl(struct nfs4_acl *);
extern int			nfs4_set_acl(struct nfs4_acl *acl, const char *path);
extern int			nfs4_remove_ace(struct nfs4_acl *acl, struct nfs4_ace *ace);
extern int			nfs4_remove_ace_at(struct nfs4_acl *acl, unsigned int index);
extern int			nfs4_insert_ace_at(struct nfs4_acl *acl, struct nfs4_ace *ace, unsigned int index);
#define nfs4_prepend_ace(acl, ace)  nfs4_insert_ace_at(acl, ace, 0)
#define nfs4_append_ace(acl, ace)   nfs4_insert_ace_at(acl, ace, acl->naces)
extern struct nfs4_ace *	nfs4_new_ace(int is_directory, u32 type, u32 flag, u32 access_mask, int whotype, char* who);
extern struct nfs4_acl *	nfs4_new_acl(u32);

extern int 			nfs4_insert_file_aces(struct nfs4_acl *acl, FILE* fd, unsigned int index);
extern int 			nfs4_insert_string_aces(struct nfs4_acl *acl, const char *acl_spec, unsigned int index);
extern int			nfs4_replace_ace(struct nfs4_acl *acl, struct nfs4_ace *old_ace, struct nfs4_ace *new_ace);
extern int			nfs4_replace_ace_spec(struct nfs4_acl *acl, char *from_ace_spec, char *to_ace_spec);
extern int			nfs4_remove_file_aces(struct nfs4_acl *acl, FILE *fd);
extern int			nfs4_remove_string_aces(struct nfs4_acl *acl, char *string);


/** Conversion functions **/
extern struct nfs4_ace * 	nfs4_ace_from_string(char *ace_spec, int is_dir);
extern struct nfs4_acl * 	nfs4_acl_for_path(const char *path);
extern char *			nfs4_acl_spec_from_file(FILE *f);


/** Access Functions **/
extern int			acl_nfs4_get_who(struct nfs4_ace*, int*, char**);
extern inline int		acl_nfs4_get_whotype(char*);

extern struct nfs4_ace *	nfs4_get_first_ace(struct nfs4_acl *);
extern struct nfs4_ace *	nfs4_get_next_ace(struct nfs4_ace **);
extern struct nfs4_ace *	nfs4_get_ace_at(struct nfs4_acl *, unsigned int index);


/** Display Functions **/
extern void			nfs4_print_acl(FILE *fp, struct nfs4_acl *acl);
extern int			nfs4_print_ace(FILE *fp, struct nfs4_ace *ace, u32 isdir);
extern int			nfs4_print_ace_verbose(struct nfs4_ace * ace, u32 isdir);
extern char*			nfs4_get_ace_type(struct nfs4_ace*, char*, int);
extern char*			nfs4_get_ace_flags(struct nfs4_ace*, char*);
extern char*			nfs4_get_ace_access(struct nfs4_ace*, char*, int);


/** misc **/
extern int 			nfs4_ace_cmp(struct nfs4_ace *lhs, struct nfs4_ace *rhs);
extern unsigned long		strtoul_reals(char *s, int base);
#endif
