/* $Xorg: auth.c,v 1.5 2001/02/09 02:05:40 xorgcvs Exp $ */
/*

Copyright 1988, 1998  The Open Group

Permission to use, copy, modify, distribute, and sell this software and its
documentation for any purpose is hereby granted without fee, provided that
the above copyright notice appear in all copies and that both that
copyright notice and this permission notice appear in supporting
documentation.

The above copyright notice and this permission notice shall be included
in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
IN NO EVENT SHALL THE OPEN GROUP BE LIABLE FOR ANY CLAIM, DAMAGES OR
OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
OTHER DEALINGS IN THE SOFTWARE.

Except as contained in this notice, the name of The Open Group shall
not be used in advertising or otherwise to promote the sale, use or
other dealings in this Software without prior written authorization
from The Open Group.

*/
/* $XFree86: xc/programs/xdm/auth.c,v 3.24 2001/12/14 20:01:19 dawes Exp $ */

/*
 * xdm - display manager daemon
 * Author:  Keith Packard, MIT X Consortium
 *
 * auth.c
 *
 * maintain the authorization generation daemon
 */

#include <X11/X.h>
#include <sys/types.h>
#include <sys/stat.h>

#include <dm.h>
#include <dm_auth.h>

#include <errno.h>

#include <sys/ioctl.h>

#include <wdmlib.h>

#if defined(TCPCONN)
# include <dm_socket.h>
#endif

#if (defined(_POSIX_SOURCE) && !defined(AIXV3) && !defined(__QNX__)) || defined(hpux) || defined(USG) || defined(SVR4) || (defined(SYSV) && defined(i386))
#define NEED_UTSNAME
#include <sys/utsname.h>
#endif

#if defined(SYSV) && defined(i386)
# include <sys/stream.h>
# ifdef ISC
#  include <stropts.h>
#  include <sys/sioctl.h>
# endif /* ISC */
#endif /* i386 */

#ifdef SVR4
# include <netdb.h>
# ifndef SCO325
# include <sys/sockio.h>
# endif
# include <sys/stropts.h>
#endif
#ifdef __convex__
# include <sync/queue.h>
# include <sync/sema.h>
#endif
#ifdef __GNU__
#include <netdb.h>
#undef SIOCGIFCONF
#else /* __GNU__ */
#include <net/if.h>
#endif /* __GNU__ */

#if ((defined(SVR4) && !defined(sun)) || defined(ISC)) && defined(SIOCGIFCONF)
#define SYSV_SIOCGIFCONF
#endif

#ifdef CSRG_BASED
#include <sys/param.h>
#if (BSD >= 199103)
#define VARIABLE_IFREQ
#endif
#endif

#ifdef __EMX__
#define link rename
int chown(int a,int b,int c) {}
#include <io.h>
#endif

struct AuthProtocol {
    unsigned short  name_length;
    char	    *name;
    void	    (*InitAuth)(unsigned short len, char *name);
    Xauth	    *(*GetAuth)(unsigned short len, char *name);
    void	    (*GetXdmcpAuth)(
    			struct protoDisplay	*pdpy,
    			unsigned short	authorizationNameLen,
    			char		*authorizationName);
    int		    inited;
};

static struct AuthProtocol AuthProtocols[] = {
{ (unsigned short) 18,	"MIT-MAGIC-COOKIE-1",
    MitInitAuth, MitGetAuth, NULL
},
#ifdef HASXDMAUTH
{ (unsigned short) 19,	"XDM-AUTHORIZATION-1",
    XdmInitAuth, XdmGetAuth, XdmGetXdmcpAuth,
},
#endif
#ifdef SECURE_RPC
{ (unsigned short) 9, "SUN-DES-1",
    SecureRPCInitAuth, SecureRPCGetAuth, NULL,
},
#endif
#ifdef K5AUTH
{ (unsigned short) 14, "MIT-KERBEROS-5",
    Krb5InitAuth, Krb5GetAuth, NULL,
},
#endif
};

#define NUM_AUTHORIZATION (sizeof (AuthProtocols) / sizeof (AuthProtocols[0]))

static struct AuthProtocol *
findProtocol (unsigned short name_length, char *name)
{
    int	i;

    for (i = 0; i < NUM_AUTHORIZATION; i++)
	if (AuthProtocols[i].name_length == name_length &&
	    memcmp(AuthProtocols[i].name, name, name_length) == 0)
	{
	    return &AuthProtocols[i];
	}
    return (struct AuthProtocol *) 0;
}

int
ValidAuthorization (unsigned short name_length, char *name)
{
    if (findProtocol (name_length, name))
	return TRUE;
    return FALSE;
}

static Xauth *
GenerateAuthorization (unsigned short name_length, char *name)
{
    struct AuthProtocol	*a;
    Xauth   *auth = 0;
    int	    i;

    WDMDebug("GenerateAuthorization %*.*s\n", name_length, name_length, name);
    a = findProtocol (name_length, name);
    if (a)
    {
	if (!a->inited)
	{
	    (*a->InitAuth) (name_length, name);
	    a->inited = TRUE;
	}
	auth = (*a->GetAuth) (name_length, name);
	if (auth)
	{
	    WDMDebug("Got %p (%d %*.*s) ", (void*)auth, auth->name_length,
		auth->name_length, auth->name_length, auth->name);
	    for (i = 0; i < (int)auth->data_length; i++)
		WDMDebug(" %02x", auth->data[i] & 0xff);
	    WDMDebug("\n");
	}
	else
	    WDMDebug("Got (null)\n");
    }
    else
    {
	WDMDebug("Unknown authorization %*.*s\n", name_length, name_length, name);
    }
    return auth;
}

#ifdef XDMCP

void
SetProtoDisplayAuthorization (
    struct protoDisplay	*pdpy,
    unsigned short	authorizationNameLen,
    char		*authorizationName)
{
    struct AuthProtocol	*a;
    Xauth   *auth;

    a = findProtocol (authorizationNameLen, authorizationName);
    pdpy->xdmcpAuthorization = pdpy->fileAuthorization = 0;
    if (a)
    {
	if (!a->inited)
	{
	    (*a->InitAuth) (authorizationNameLen, authorizationName);
	    a->inited = TRUE;
	}
	if (a->GetXdmcpAuth)
	{
	    (*a->GetXdmcpAuth) (pdpy, authorizationNameLen, authorizationName);
	    auth = pdpy->xdmcpAuthorization;
	}
	else
	{
	    auth = (*a->GetAuth) (authorizationNameLen, authorizationName);
	    pdpy->fileAuthorization = auth;
	    pdpy->xdmcpAuthorization = 0;
	}
	if (auth)
	    WDMDebug("Got %p (%d %*.*s)\n", (void *)auth, auth->name_length,
		auth->name_length, auth->name_length, auth->name);
	else
	    WDMDebug("Got (null)\n");
    }
}

#endif /* XDMCP */

void
CleanUpFileName (char *src, char *dst, int len)
{
    while (*src) {
	if (--len <= 0)
		break;
	switch (*src & 0x7f)
	{
	case '/':
	    *dst++ = '_';
	    break;
	case '-':
	    *dst++ = '.';
	    break;
	default:
	    *dst++ = (*src & 0x7f);
	}
	++src;
    }
    *dst = '\0';
}

static char authdir1[] = "authdir";
static char authdir2[] = "authfiles";

static int
MakeServerAuthFile (struct display *d)
{
    int len;
#ifdef SYSV
#define NAMELEN	14
#else
#define NAMELEN	255
#endif
    char    cleanname[NAMELEN];
    int r;
    struct stat	statb;

    if (d->clientAuthFile && *d->clientAuthFile)
	len = strlen (d->clientAuthFile) + 1;
    else
    {
    	CleanUpFileName (d->name, cleanname, NAMELEN - 8);
    	len = strlen (authDir) + strlen (authdir1) + strlen (authdir2)
	    + strlen (cleanname) + 14;
    }
    if (d->authFile)
	free (d->authFile);
    d->authFile = malloc ((unsigned) len);
    if (!d->authFile)
	return FALSE;
    if (d->clientAuthFile && *d->clientAuthFile)
	strcpy (d->authFile, d->clientAuthFile);
    else
    {
	sprintf (d->authFile, "%s/%s", authDir, authdir1);
	r = stat(d->authFile, &statb);
	if (r == 0) {
	    if (statb.st_uid != 0)
		(void) chown(d->authFile, 0, statb.st_gid);
	    if ((statb.st_mode & 0077) != 0)
		(void) chmod(d->authFile, statb.st_mode & 0700);
	} else {
	    if (errno == ENOENT)
		r = mkdir(d->authFile, 0700);
	    if (r < 0) {
		free (d->authFile);
		d->authFile = NULL;
		return FALSE;
	    }
	}
	sprintf (d->authFile, "%s/%s/%s", authDir, authdir1, authdir2);
	r = mkdir(d->authFile, 0700);
	if (r < 0  &&  errno != EEXIST) {
	    free (d->authFile);
	    d->authFile = NULL;
	    return FALSE;
	}
    	sprintf (d->authFile, "%s/%s/%s/A%s-XXXXXX",
		 authDir, authdir1, authdir2, cleanname);
#ifdef HAVE_MKSTEMP
    	(void) mkstemp (d->authFile);
#else
    	(void) mktemp (d->authFile);
#endif
    }
    return TRUE;
}

int
SaveServerAuthorizations (
    struct display  *d,
    Xauth	    **auths,
    int		    count)
{
    FILE	*auth_file;
    int		mask;
    int		ret;
    int		i;

    mask = umask (0077);
    if (!d->authFile && !MakeServerAuthFile (d))
	return FALSE;
    (void) unlink (d->authFile);
    auth_file = fopen (d->authFile, "w");
    umask (mask);
    if (!auth_file) {
	WDMDebug("Can't creat auth file %s\n", d->authFile);
	WDMError("Cannot open server authorization file %s\n", d->authFile);
	free (d->authFile);
	d->authFile = NULL;
	ret = FALSE;
    }
    else
    {
	WDMDebug("File: %s auth: %p\n", d->authFile, (void *)auths);
	ret = TRUE;
	for (i = 0; i < count; i++)
	{
	    /*
	     * User-based auths may not have data until
	     * a user logs in.  In which case don't write
	     * to the auth file so xrdb and setup programs don't fail.
	     */
	    if (auths[i]->data_length > 0)
		if (!XauWriteAuth (auth_file, auths[i]) ||
		    fflush (auth_file) == EOF)
		{
		    WDMError("Cannot write server authorization file %s\n",
			      d->authFile);
		    ret = FALSE;
		    free (d->authFile);
		    d->authFile = NULL;
		}
    	}
	fclose (auth_file);
    }
    return ret;
}

void
SetLocalAuthorization (struct display *d)
{
    Xauth	*auth, **auths;
    int		i, j;

    if (d->authorizations)
    {
	for (i = 0; i < d->authNum; i++)
	    XauDisposeAuth (d->authorizations[i]);
	free ((char *) d->authorizations);
	d->authorizations = (Xauth **) NULL;
	d->authNum = 0;
    }
    if (!d->authNames)
	return;
    for (i = 0; d->authNames[i]; i++)
	;
    d->authNameNum = i;
    if (d->authNameLens)
	free ((char *) d->authNameLens);
    d->authNameLens = (unsigned short *) malloc
				(d->authNameNum * sizeof (unsigned short));
    if (!d->authNameLens)
	return;
    for (i = 0; i < d->authNameNum; i++)
	d->authNameLens[i] = strlen (d->authNames[i]);
    auths = (Xauth **) malloc (d->authNameNum * sizeof (Xauth *));
    if (!auths)
	return;
    j = 0;
    for (i = 0; i < d->authNameNum; i++)
    {
	auth = GenerateAuthorization (d->authNameLens[i], d->authNames[i]);
	if (auth)
	    auths[j++] = auth;
    }
    if (SaveServerAuthorizations (d, auths, j))
    {
	d->authorizations = auths;
	d->authNum = j;
    }
    else
    {
	for (i = 0; i < j; i++)
	    XauDisposeAuth (auths[i]);
	free ((char *) auths);
    }
}

/*
 * Set the authorization to use for xdm's initial connection
 * to the X server.  Cannot use user-based authorizations
 * because no one has logged in yet, so we don't have any
 * user credentials.
 * Well, actually we could use SUN-DES-1 because we tell the server
 * to allow root in.  This is bogus and should be fixed.
 */
void
SetAuthorization (struct display *d)
{
    register Xauth **auth = d->authorizations;
    int i;

    for (i = 0; i < d->authNum; i++)
    {
	if (auth[i]->name_length == 9 &&
	    memcmp(auth[i]->name, "SUN-DES-1", 9) == 0)
	    continue;
	if (auth[i]->name_length == 14 &&
	    memcmp(auth[i]->name, "MIT-KERBEROS-5", 14) == 0)
	    continue;
	XSetAuthorization (auth[i]->name, (int) auth[i]->name_length,
			   auth[i]->data, (int) auth[i]->data_length);
    }
}

static int
openFiles (char *name, char *new_name, FILE **oldp, FILE **newp)
{
	int	mask;

	strcpy (new_name, name);
	strcat (new_name, "-n");
	mask = umask (0077);
	(void) unlink (new_name);
	*newp = fopen (new_name, "w");
	(void) umask (mask);
	if (!*newp) {
		WDMDebug("can't open new file %s\n", new_name);
		return 0;
	}
	*oldp = fopen (name, "r");
	WDMDebug("opens succeeded %s %s\n", name, new_name);
	return 1;
}

static int
binaryEqual (char *a, char *b, unsigned short len)
{
	while (len-- > 0)
		if (*a++ != *b++)
			return FALSE;
	return TRUE;
}

static void
dumpBytes (unsigned short len, char *data)
{
	unsigned short	i;

	WDMDebug("%d: ", len);
	for (i = 0; i < len; i++)
		WDMDebug("%02x ", data[i] & 0377);
	WDMDebug("\n");
}

static void
dumpAuth (Xauth *auth)
{
	WDMDebug("family: %d\n", auth->family);
	WDMDebug("addr:   ");
	dumpBytes(auth->address_length, auth->address);
	WDMDebug("number: ");
	dumpBytes(auth->number_length, auth->number);
	WDMDebug("name:   ");
	dumpBytes(auth->name_length, auth->name);
	WDMDebug("data:   ");
	dumpBytes(auth->data_length, auth->data);
}

struct addrList {
	unsigned short	family;
	unsigned short	address_length;
	char	*address;
	unsigned short	number_length;
	char	*number;
	unsigned short	name_length;
	char	*name;
	struct addrList	*next;
};

static struct addrList	*addrs;

static void
initAddrs (void)
{
	addrs = 0;
}

static void
doneAddrs (void)
{
	struct addrList	*a, *n;
	for (a = addrs; a; a = n) {
		n = a->next;
		if (a->address)
			free (a->address);
		if (a->number)
			free (a->number);
		free ((char *) a);
	}
}

static int checkEntry (Xauth *auth);

static void
saveEntry (Xauth *auth)
{
	struct addrList	*new;

	new = (struct addrList *) malloc (sizeof (struct addrList));
	if (!new) {
		WDMError("saveEntry: out of memory");
		return;
	}
	if ((new->address_length = auth->address_length) > 0) {
		new->address = malloc (auth->address_length);
		if (!new->address) {
			WDMError("saveEntry: out of memory");
			free ((char *) new);
			return;
		}
		memmove( new->address, auth->address, (int) auth->address_length);
	} else
		new->address = 0;
	if ((new->number_length = auth->number_length) > 0) {
		new->number = malloc (auth->number_length);
		if (!new->number) {
			WDMError("saveEntry: out of memory");
			free (new->address);
			free ((char *) new);
			return;
		}
		memmove( new->number, auth->number, (int) auth->number_length);
	} else
		new->number = 0;
	if ((new->name_length = auth->name_length) > 0) {
		new->name = malloc (auth->name_length);
		if (!new->name) {
			WDMError("saveEntry: out of memory");
			free (new->number);
			free (new->address);
			free ((char *) new);
			return;
		}
		memmove( new->name, auth->name, (int) auth->name_length);
	} else
		new->name = 0;
	new->family = auth->family;
	new->next = addrs;
	addrs = new;
}

static int
checkEntry (Xauth *auth)
{
	struct addrList	*a;

	for (a = addrs; a; a = a->next) {
		if (a->family == auth->family &&
		    a->address_length == auth->address_length &&
 		    binaryEqual (a->address, auth->address, auth->address_length) &&
		    a->number_length == auth->number_length &&
 		    binaryEqual (a->number, auth->number, auth->number_length) &&
		    a->name_length == auth->name_length &&
		    binaryEqual (a->name, auth->name, auth->name_length))
		{
			return 1;
		}
	}
	return 0;
}

static int  doWrite;

static void
writeAuth (FILE *file, Xauth *auth)
{
    if (debugLevel.i >= 15) {	/* normally too verbose */
        WDMDebug("writeAuth: doWrite = %d\n", doWrite);
	dumpAuth (auth);	/* does Debug only */
    }
	if (doWrite)
	    XauWriteAuth (file, auth);
}

static void
writeAddr (
    int		family,
    int		addr_length,
    char	*addr,
    FILE	*file,
    Xauth	*auth)
{
	auth->family = (unsigned short) family;
	auth->address_length = addr_length;
	auth->address = addr;
	WDMDebug("writeAddr: writing and saving an entry\n");
	writeAuth (file, auth);
	saveEntry (auth);
}

static void
DefineLocal (FILE *file, Xauth *auth)
{
	char	displayname[100];
	char	tmp_displayname[100];

	strcpy(tmp_displayname, "");

	/* stolen from xinit.c */

/* Make sure this produces the same string as _XGetHostname in lib/X/XlibInt.c.
 * Otherwise, Xau will not be able to find your cookies in the Xauthority file.
 *
 * Note: POSIX says that the ``nodename'' member of utsname does _not_ have
 *       to have sufficient information for interfacing to the network,
 *       and so, you may be better off using gethostname (if it exists).
 */

#ifdef NEED_UTSNAME

	/* hpux:
	 * Why not use gethostname()?  Well, at least on my system, I've had to
	 * make an ugly kernel patch to get a name longer than 8 characters, and
	 * uname() lets me access to the whole string (it smashes release, you
	 * see), whereas gethostname() kindly truncates it for me.
	 */
	{
	struct utsname name;

	uname(&name);
	strcpy(displayname, name.nodename);
	}
	writeAddr (FamilyLocal, strlen (displayname), displayname, file, auth);

	strcpy(tmp_displayname, displayname);
#endif

#if (!defined(NEED_UTSNAME) || defined (hpux))
        /* AIXV3:
	 * In AIXV3, _POSIX_SOURCE is defined, but uname gives only first
	 * field of hostname. Thus, we use gethostname instead.
	 */

	/*
	 * For HP-UX, HP's Xlib expects a fully-qualified domain name, which
	 * is achieved by using gethostname().  For compatability, we must
	 * also still create the entry using uname() above.
	 */
	gethostname(displayname, sizeof(displayname));

	/*
	 * If gethostname and uname both returned the same name,
	 * do not write a duplicate entry.
	 */
	if (strcmp (displayname, tmp_displayname))
	    writeAddr (FamilyLocal, strlen (displayname), displayname, 
		       file, auth);
#endif
}

#ifdef SYSV_SIOCGIFCONF

/* Deal with different SIOCGIFCONF ioctl semantics on SYSV, SVR4 */

static int
ifioctl (int fd, int cmd, char *arg)
{
    struct strioctl ioc;
    int ret;

    bzero((char *) &ioc, sizeof(ioc));
    ioc.ic_cmd = cmd;
    ioc.ic_timout = 0;
    if (cmd == SIOCGIFCONF)
    {
	ioc.ic_len = ((struct ifconf *) arg)->ifc_len;
	ioc.ic_dp = ((struct ifconf *) arg)->ifc_buf;
#ifdef ISC
	/* SIOCGIFCONF is somewhat brain damaged on ISC. The argument
	 * buffer must contain the ifconf structure as header. Ifc_req
	 * is also not a pointer but a one element array of ifreq
	 * structures. On return this array is extended by enough
	 * ifreq fields to hold all interfaces. The return buffer length
	 * is placed in the buffer header.
	 */
        ((struct ifconf *) ioc.ic_dp)->ifc_len =
                                         ioc.ic_len - sizeof(struct ifconf);
#endif
    }
    else
    {
	ioc.ic_len = sizeof(struct ifreq);
	ioc.ic_dp = arg;
    }
    ret = ioctl(fd, I_STR, (char *) &ioc);
    if (ret >= 0 && cmd == SIOCGIFCONF)
#ifdef SVR4
	((struct ifconf *) arg)->ifc_len = ioc.ic_len;
#endif
#ifdef ISC
    {
	((struct ifconf *) arg)->ifc_len =
				 ((struct ifconf *)ioc.ic_dp)->ifc_len;
	((struct ifconf *) arg)->ifc_buf = 
			(caddr_t)((struct ifconf *)ioc.ic_dp)->ifc_req;
    }
#endif
    return(ret);
}
#else /* SYSV_SIOCGIFCONF */
#define ifioctl ioctl
#endif /* SYSV_SIOCGIFCONF */

#ifdef WINTCP /* NCR with Wollongong TCP */

#include <sys/un.h>
#include <stropts.h>
#include <tiuser.h>

#include <sys/stream.h>
#include <net/if.h>
#include <netinet/ip.h>
#include <netinet/ip_var.h>
#include <netinet/in.h>
#include <netinet/in_var.h>

static void
DefineSelf (int fd, FILE *file, Xauth *auth)
{
    /*
     * The Wollongong drivers used by NCR SVR4/MP-RAS don't understand the
     * socket IO calls that most other drivers seem to like. Because of
     * this, this routine must be special cased for NCR. Eventually,
     * this will be cleared up.
     */

    struct ipb ifnet;
    struct in_ifaddr ifaddr;
    struct strioctl str;
    unsigned char *addr;
    int	len, ipfd;

    if ((ipfd = open ("/dev/ip", O_RDWR, 0 )) < 0)
        LogError ("Getting interface configuration");

    /* Indicate that we want to start at the begining */
    ifnet.ib_next = (struct ipb *) 1;

    while (ifnet.ib_next)
    {
	str.ic_cmd = IPIOC_GETIPB;
	str.ic_timout = 0;
	str.ic_len = sizeof (struct ipb);
	str.ic_dp = (char *) &ifnet;

	if (ioctl (ipfd, (int) I_STR, (char *) &str) < 0)
	{
	    close (ipfd);
	    LogError ("Getting interface configuration");
	}

	ifaddr.ia_next = (struct in_ifaddr *) ifnet.if_addrlist;
	str.ic_cmd = IPIOC_GETINADDR;
	str.ic_timout = 0;
	str.ic_len = sizeof (struct in_ifaddr);
	str.ic_dp = (char *) &ifaddr;

	if (ioctl (ipfd, (int) I_STR, (char *) &str) < 0)
	{
	    close (ipfd);
	    LogError ("Getting interface configuration");
	}

	/*
	 * Ignore the 127.0.0.1 entry.
	 */
	if (IA_SIN(&ifaddr)->sin_addr.s_addr == htonl(0x7f000001) )
		continue;

	writeAddr (FamilyInternet, 4, (char *)&(IA_SIN(&ifaddr)->sin_addr), file, auth);
 
    }
    close(ipfd);
}

#else /* WINTCP */

#ifdef SIOCGIFCONF

/* Handle variable length ifreq in BNR2 and later */
#ifdef VARIABLE_IFREQ
#define ifr_size(p) (sizeof (struct ifreq) + \
		     (p->ifr_addr.sa_len > sizeof (p->ifr_addr) ? \
		      p->ifr_addr.sa_len - sizeof (p->ifr_addr) : 0))
#else
#define ifr_size(p) (sizeof (struct ifreq))
#endif

/* Define this host for access control.  Find all the hosts the OS knows about 
 * for this fd and add them to the selfhosts list.
 */
static void
DefineSelf (int fd, FILE *file, Xauth *auth)
{
    char		buf[2048], *cp, *cplim;
    struct ifconf	ifc;
    int 		len;
    char 		*addr;
    int 		family;
    register struct ifreq *ifr;
    
    ifc.ifc_len = sizeof (buf);
    ifc.ifc_buf = buf;
    if (ifioctl (fd, SIOCGIFCONF, (char *) &ifc) < 0)
        WDMError("Trouble getting network interface configuration");

#ifdef ISC
#define IFC_IFC_REQ (struct ifreq *) ifc.ifc_buf
#else
#define IFC_IFC_REQ ifc.ifc_req
#endif

    cplim = (char *) IFC_IFC_REQ + ifc.ifc_len;

    for (cp = (char *) IFC_IFC_REQ; cp < cplim; cp += ifr_size (ifr))
    {
	ifr = (struct ifreq *) cp;
	if (ConvertAddr ((XdmcpNetaddr) &ifr->ifr_addr, &len, &addr) < 0)
		continue;
	if (len == 0)
	{
		WDMDebug("Skipping zero length address\n");
		continue;
	}
	/*
	* don't write out 'localhost' entries, as
	* they may conflict with other local entries.
	* DefineLocal will always be called to add
	* the local entry anyway, so this one can
	* be tossed.
	*/
	if (len == 4 &&
		addr[0] == 127 && addr[1] == 0 &&
		addr[2] == 0 && addr[3] == 1)
	{
		WDMDebug("Skipping localhost address\n");
		continue;
	}
	family = FamilyInternet;
	WDMDebug("DefineSelf: write network address, length %d\n", len);
	writeAddr (family, len, addr, file, auth);
    }
}

#else /* SIOCGIFCONF */

/* Define this host for access control.  Find all the hosts the OS knows about 
 * for this fd and add them to the selfhosts list.
 */
static void
DefineSelf (int fd, int file, int auth)
{
    register int n;
    int	len;
    caddr_t	addr;
    int		family;

    struct utsname name;
    register struct hostent  *hp;

    union {
	struct  sockaddr   sa;
	struct  sockaddr_in  in;
    } saddr;
	
    struct	sockaddr_in	*inetaddr;

    /* hpux:
     * Why not use gethostname()?  Well, at least on my system, I've had to
     * make an ugly kernel patch to get a name longer than 8 characters, and
     * uname() lets me access to the whole string (it smashes release, you
     * see), whereas gethostname() kindly truncates it for me.
     */
    uname(&name);
    hp = gethostbyname (name.nodename);
    if (hp != NULL) {
	saddr.sa.sa_family = hp->h_addrtype;
	inetaddr = (struct sockaddr_in *) (&(saddr.sa));
	memmove( (char *) &(inetaddr->sin_addr), (char *) hp->h_addr, (int) hp->h_length);
	family = ConvertAddr ( &(saddr.sa), &len, &addr);
	if ( family >= 0) {
	    writeAddr (FamilyInternet, sizeof (inetaddr->sin_addr),
			(char *) (&inetaddr->sin_addr), file, auth);
	}
    }
}

#endif /* SIOCGIFCONF else */
#endif /* WINTCP else */


static void
setAuthNumber (Xauth *auth, char *name)
{
    char	*colon;
    char	*dot, *number;

    WDMDebug("setAuthNumber %s\n", name);
    colon = strrchr(name, ':');
    if (colon) {
	++colon;
	dot = strchr(colon, '.');
	if (dot)
	    auth->number_length = dot - colon;
	else
	    auth->number_length = strlen (colon);
	number = malloc (auth->number_length + 1);
	if (number) {
	    strncpy (number, colon, auth->number_length);
	    number[auth->number_length] = '\0';
	} else {
	    WDMError("setAuthNumber: out of memory");
	    auth->number_length = 0;
	}
	auth->number = number;
	WDMDebug("setAuthNumber: %s\n", number);
    }
}

static void
writeLocalAuth (FILE *file, Xauth *auth, char *name)
{
    int	fd;

    WDMDebug("writeLocalAuth: %s %.*s\n", name, auth->name_length, auth->name);
    setAuthNumber (auth, name);
#ifdef TCPCONN
    fd = socket (AF_INET, SOCK_STREAM, 0);
    DefineSelf (fd, file, auth);
    close (fd);
#endif
    DefineLocal (file, auth);
}

#ifdef XDMCP

static void
writeRemoteAuth (FILE *file, Xauth *auth, XdmcpNetaddr peer, int peerlen, char *name)
{
    int	    family = FamilyLocal;
    char    *addr;

    WDMDebug("writeRemoteAuth: %s %.*s\n", name, auth->name_length, auth->name);
    if (!peer || peerlen < 2)
	return;
    setAuthNumber (auth, name);
    family = ConvertAddr (peer, &peerlen, &addr);
    WDMDebug("writeRemoteAuth: family %d\n", family);
    if (family != FamilyLocal)
    {
/*	WDMDebug("writeRemoteAuth: %d, %d, %x\n",
		family, peerlen, *(int *)addr);*/
	writeAddr (family, peerlen, addr, file, auth);
    }
    else
    {
	writeLocalAuth (file, auth, name);
    }
}

#endif /* XDMCP */

void
SetUserAuthorization (struct display *d, struct verify_info *verify)
{
    FILE	*old, *new;
    char	home_name[1024], backup_name[1024], new_name[1024];
    char	*name = 0;
    const char	*home;
    char	*envname = 0;
    int	lockStatus;
    Xauth	*entry, **auths;
    int		setenv = 0;
    struct stat	statb;
    int		i;
    int		magicCookie;
    int		data_len;

    WDMDebug("SetUserAuthorization\n");
    auths = d->authorizations;
    if (auths) {
	home = WDMGetEnv(verify->userEnviron, "HOME");
	lockStatus = LOCK_ERROR;
	if (home) {
	    strcpy (home_name, home);
	    if (home[strlen(home) - 1] != '/')
		strcat (home_name, "/");
	    strcat (home_name, ".Xauthority");
	    WDMDebug("XauLockAuth %s\n", home_name);
	    lockStatus = XauLockAuth (home_name, 1, 2, 10);
	    WDMDebug("Lock is %d\n", lockStatus);
	    if (lockStatus == LOCK_SUCCESS) {
		if (openFiles (home_name, new_name, &old, &new)) {
		    name = home_name;
		    setenv = 0;
		} else {
		    WDMDebug("openFiles failed\n");
		    XauUnlockAuth (home_name);
		    lockStatus = LOCK_ERROR;
		}	
	    }
	}
	if (lockStatus != LOCK_SUCCESS) {
	    sprintf (backup_name, "%s/.XauthXXXXXX", d->userAuthDir);
#ifdef HAVE_MKSTEMP
    	(void) mkstemp (backup_name);
#else
    	(void) mktemp (backup_name);
#endif
	    lockStatus = XauLockAuth (backup_name, 1, 2, 10);
	    WDMDebug("backup lock is %d\n", lockStatus);
	    if (lockStatus == LOCK_SUCCESS) {
		if (openFiles (backup_name, new_name, &old, &new)) {
		    name = backup_name;
		    setenv = 1;
		} else {
		    XauUnlockAuth (backup_name);
		    lockStatus = LOCK_ERROR;
		}	
	    }
	}
	if (lockStatus != LOCK_SUCCESS) {
	    WDMDebug("can't lock auth file %s or backup %s\n",
			    home_name, backup_name);
	    WDMError("can't lock authorization file %s or backup %s\n",
			    home_name, backup_name);
	    return;
	}
	initAddrs ();
	doWrite = 1;
	WDMDebug("%d authorization protocols for %s\n", d->authNum, d->name);
	/*
	 * Write MIT-MAGIC-COOKIE-1 authorization first, so that
	 * R4 clients which only knew that, and used the first
	 * matching entry will continue to function
	 */
	magicCookie = -1;
	for (i = 0; i < d->authNum; i++)
	{
	    if (auths[i]->name_length == 18 &&
		!strncmp (auths[i]->name, "MIT-MAGIC-COOKIE-1", 18))
	    {
		magicCookie = i;
	    	if (d->displayType.location == Local)
	    	    writeLocalAuth (new, auths[i], d->name);
#ifdef XDMCP
	    	else
	    	    writeRemoteAuth (new, auths[i], d->peer, d->peerlen, d->name);
#endif
		break;
	    }
	}
	/* now write other authorizations */
	for (i = 0; i < d->authNum; i++)
	{
	    if (i != magicCookie)
	    {
		data_len = auths[i]->data_length;
		/* client will just use default Kerberos cache, so don't
		 * even write cache info into the authority file.
		 */
		if (auths[i]->name_length == 14 &&
		    !strncmp (auths[i]->name, "MIT-KERBEROS-5", 14))
		    auths[i]->data_length = 0;
	    	if (d->displayType.location == Local)
	    	    writeLocalAuth (new, auths[i], d->name);
#ifdef XDMCP
	    	else
	    	    writeRemoteAuth (new, auths[i], d->peer, d->peerlen, d->name);
#endif
		auths[i]->data_length = data_len;
	    }
	}
	if (old) {
	    if (fstat (fileno (old), &statb) != -1)
		chmod (new_name, (int) (statb.st_mode & 0777));
	    /*SUPPRESS 560*/
	    while ((entry = XauReadAuth (old))) {
		if (!checkEntry (entry))
		{
		    WDMDebug("Writing an entry\n");
		    writeAuth (new, entry);
		}
		XauDisposeAuth (entry);
	    }
	    fclose (old);
	}
	doneAddrs ();
	fclose (new);
	if (unlink (name) == -1)
	    WDMDebug("unlink %s failed\n", name);
	envname = name;
	if (link (new_name, name) == -1) {
	    WDMDebug("link failed %s %s\n", new_name, name);
	    WDMError("Can't move authorization into place\n");
	    setenv = 1;
	    envname = new_name;
	} else {
	    WDMDebug("new is in place, go for it!\n");
	    unlink (new_name);
	}
	if (setenv) {
	    verify->userEnviron = WDMSetEnv(verify->userEnviron,
				    "XAUTHORITY", envname);
	    verify->systemEnviron = WDMSetEnv(verify->systemEnviron,
				    "XAUTHORITY", envname);
	}
	XauUnlockAuth (name);
	if (envname)
	    chown (envname, verify->uid, verify->gid);
    }
    WDMDebug("done SetUserAuthorization\n");
}

void
RemoveUserAuthorization (struct display *d, struct verify_info *verify)
{
    const char    *home;
    Xauth   **auths, *entry;
    char    name[1024], new_name[1024];
    int	    lockStatus;
    FILE    *old, *new;
    struct stat	statb;
    int	    i;

    if (!(auths = d->authorizations))
	return;
    home = WDMGetEnv(verify->userEnviron, "HOME");
    if (!home)
	return;
    WDMDebug("RemoveUserAuthorization\n");
    strcpy (name, home);
    if (home[strlen(home) - 1] != '/')
	strcat (name, "/");
    strcat (name, ".Xauthority");
    WDMDebug("XauLockAuth %s\n", name);
    lockStatus = XauLockAuth (name, 1, 2, 10);
    WDMDebug("Lock is %d\n", lockStatus);
    if (lockStatus != LOCK_SUCCESS)
	return;
    if (openFiles (name, new_name, &old, &new))
    {
	initAddrs ();
	doWrite = 0;
	for (i = 0; i < d->authNum; i++)
	{
	    if (d->displayType.location == Local)
	    	writeLocalAuth (new, auths[i], d->name);
#ifdef XDMCP
	    else
	    	writeRemoteAuth (new, auths[i], d->peer, d->peerlen, d->name);
#endif
	}
	doWrite = 1;
	if (old) {
	    if (fstat (fileno (old), &statb) != -1)
		chmod (new_name, (int) (statb.st_mode & 0777));
	    /*SUPPRESS 560*/
	    while ((entry = XauReadAuth (old))) {
		if (!checkEntry (entry))
		{
		    WDMDebug("Writing an entry\n");
		    writeAuth (new, entry);
		}
		XauDisposeAuth (entry);
	    }
	    fclose (old);
	}
	doneAddrs ();
	fclose (new);
	if (unlink (name) == -1)
	    WDMDebug("unlink %s failed\n", name);
	if (link (new_name, name) == -1) {
	    WDMDebug("link failed %s %s\n", new_name, name);
	    WDMError("Can't move authorization into place\n");
	} else {
	    WDMDebug("new is in place, go for it!\n");
	    unlink (new_name);
	}
    }
    XauUnlockAuth (name);
}

