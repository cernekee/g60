/*
  CUSE example: Character device in Userspace
  Copyright (C) 2008-2009  SUSE Linux Products GmbH
  Copyright (C) 2008-2009  Tejun Heo <tj@kernel.org>

  This program can be distributed under the terms of the GNU GPL.
  See the file COPYING.

  gcc -Wall `pkg-config fuse --cflags --libs` cusexmp.c -o cusexmp
*/

#define FUSE_USE_VERSION 29

#include <cuse_lowlevel.h>
#include <fuse_opt.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>

static void *cusexmp_buf;
static size_t cusexmp_size;

static const char *usage =
"usage: cusexmp [options]\n"
"\n"
"options:\n"
"    --help|-h             print this help message\n"
"    --maj=MAJ|-M MAJ      device major number\n"
"    --min=MIN|-m MIN      device minor number\n"
"    --name=NAME|-n NAME   device name (mandatory)\n"
"\n";

static int cusexmp_resize(size_t new_size)
{
	void *new_buf;

	if (new_size == cusexmp_size)
		return 0;

	new_buf = realloc(cusexmp_buf, new_size);
	if (!new_buf && new_size)
		return -ENOMEM;

	if (new_size > cusexmp_size)
		memset(new_buf + cusexmp_size, 0, new_size - cusexmp_size);

	cusexmp_buf = new_buf;
	cusexmp_size = new_size;

	return 0;
}

static int cusexmp_expand(size_t new_size)
{
	if (new_size > cusexmp_size)
		return cusexmp_resize(new_size);
	return 0;
}

static void cusexmp_open(fuse_req_t req, struct fuse_file_info *fi)
{
	fuse_reply_open(req, fi);
}

static void cusexmp_read(fuse_req_t req, size_t size, off_t off,
			 struct fuse_file_info *fi)
{
	(void)fi;

	if (off >= cusexmp_size)
		off = cusexmp_size;
	if (size > cusexmp_size - off)
		size = cusexmp_size - off;

	fuse_reply_buf(req, cusexmp_buf + off, size);
}

static void cusexmp_write(fuse_req_t req, const char *buf, size_t size,
			  off_t off, struct fuse_file_info *fi)
{
	(void)fi;

	if (cusexmp_expand(off + size)) {
		fuse_reply_err(req, ENOMEM);
		return;
	}

	memcpy(cusexmp_buf + off, buf, size);
	fuse_reply_write(req, size);
}

struct cusexmp_param {
	unsigned		major;
	unsigned		minor;
	char			*dev_name;
	int			is_help;
};

#define CUSEXMP_OPT(t, p) { t, offsetof(struct cusexmp_param, p), 1 }

static const struct fuse_opt cusexmp_opts[] = {
	CUSEXMP_OPT("-M %u",		major),
	CUSEXMP_OPT("--maj=%u",		major),
	CUSEXMP_OPT("-m %u",		minor),
	CUSEXMP_OPT("--min=%u",		minor),
	CUSEXMP_OPT("-n %s",		dev_name),
	CUSEXMP_OPT("--name=%s",	dev_name),
	FUSE_OPT_KEY("-h",		0),
	FUSE_OPT_KEY("--help",		0),
	FUSE_OPT_END
};

static int cusexmp_process_arg(void *data, const char *arg, int key,
			       struct fuse_args *outargs)
{
	struct cusexmp_param *param = data;

	(void)outargs;
	(void)arg;

	switch (key) {
	case 0:
		param->is_help = 1;
		fprintf(stderr, "%s", usage);
		return fuse_opt_add_arg(outargs, "-ho");
	default:
		return 1;
	}
}

static const struct cuse_lowlevel_ops cusexmp_clop = {
	.open		= cusexmp_open,
	.read		= cusexmp_read,
	.write		= cusexmp_write,
};

int main(int argc, char **argv)
{
	struct fuse_args args = FUSE_ARGS_INIT(argc, argv);
	struct cusexmp_param param = { 0, 0, NULL, 0 };
	char dev_name[128] = "DEVNAME=";
	const char *dev_info_argv[] = { dev_name };
	struct cuse_info ci;

	if (fuse_opt_parse(&args, &param, cusexmp_opts, cusexmp_process_arg)) {
		printf("failed to parse option\n");
		return 1;
	}

	if (!param.is_help) {
		if (!param.dev_name) {
			fprintf(stderr, "Error: device name missing\n");
			return 1;
		}
		strncat(dev_name, param.dev_name, sizeof(dev_name) - 9);
	}

	memset(&ci, 0, sizeof(ci));
	ci.dev_major = param.major;
	ci.dev_minor = param.minor;
	ci.dev_info_argc = 1;
	ci.dev_info_argv = dev_info_argv;
	ci.flags = CUSE_UNRESTRICTED_IOCTL;

	return cuse_lowlevel_main(args.argc, args.argv, &ci, &cusexmp_clop,
				  NULL);
}
