#include	"u.h"
#include	"../port/lib.h"
#include	"mem.h"
#include	"dat.h"
#include	"fns.h"
#include	"../port/error.h"

#include	"io.h"

#define	DPRINT if(0)iprint

enum{
	Qdir,
	Qrtc,
};

static Dirtab rtctab[]={
	".",		{Qdir,0,QTDIR},	0,	0555,
	"rtc",		{Qrtc},	NUMSIZE,	0664,
};

static ulong
sysrtc(void)
{
	ulong ret = 0;
	ulong secs, *ps = uncached(&secs);
	ulong osecs, *ops = uncached(&osecs);

	*ops = 0x11223344;
	ret |= nbfifoput(F9TSystem|F9Sysrrtc, (ulong)ops);
	while(*ops == 0x11223344); /* wait for arm7 write */

	*ps = 0x11223344;
	ret |= nbfifoput(F9TSystem|F9Sysrrtc, (ulong)ps);
	while(*ps == 0x11223344); /* wait for arm7 write */

	DPRINT("sysrtc: ret %ud secs %ud < %ud\n", ret, secs, osecs);
	return (secs >= osecs) ? secs : osecs;
}

static void
rtcreset(void)
{
	DPRINT("devrtc: reset\n");
}

static Chan*
rtcattach(char *spec)
{
	return devattach('r', spec);
}

static Walkqid*
rtcwalk(Chan *c, Chan *nc, char **name, int nname)
{
	return devwalk(c, nc, name, nname, rtctab, nelem(rtctab), devgen);
}

static int
rtcstat(Chan *c, uchar *dp, int n)
{
	return devstat(c, dp, n, rtctab, nelem(rtctab), devgen);
}

static Chan*
rtcopen(Chan *c, int omode)
{
	return devopen(c, omode, rtctab, nelem(rtctab), devgen);
}

static void	 
rtcclose(Chan*)
{
}

static long	 
rtcread(Chan *c, void *buf, long n, vlong off)
{
	ulong secs;
	if(c->qid.type & QTDIR)
		return devdirread(c, buf, n, rtctab, nelem(rtctab), devgen);

	switch((ulong)c->qid.path){
	case Qrtc:
		secs = sysrtc();
		return readnum(off, buf, n, secs, NUMSIZE);
	}
	error(Egreg);
	return 0;		/* not reached */
}

static long	 
rtcwrite(Chan *c, void *buf, long n, vlong off)
{
	ulong offset = off;
	ulong secs;
	char *cp, sbuf[32];

	switch((ulong)c->qid.path){
	case Qrtc:
		if(offset != 0 || n >= sizeof(sbuf)-1)
			error(Ebadarg);
		memmove(sbuf, buf, n);
		sbuf[n] = '\0';
		cp = sbuf;
		while(*cp){
			if(*cp>='0' && *cp<='9')
				break;
			cp++;
		}
		secs = strtoul(cp, 0, 0);
		fifoput(F9TSystem|F9Syswrtc, (ulong)(&secs));
		return n;

	}
	error(Egreg);
	return 0;		/* not reached */
}

Dev rtcdevtab = {
	'r',
	"rtc",

	rtcreset,
	devinit,
	devshutdown,
	rtcattach,
	rtcwalk,
	rtcstat,
	rtcopen,
	devcreate,
	rtcclose,
	rtcread,
	devbread,
	rtcwrite,
	devbwrite,
	devremove,
	devwstat,
};
