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
#include <libusb.h>
#include <ctype.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>

#define MAX_XFER		0x1000
#define TIMEOUT_MS		200

#define G60_VID			0x04c5
#define G60_PID			0x124a
#define BULK_EP_IN		0x81
#define BULK_EP_OUT		0x02

static libusb_device_handle *devh;
static int trace_enabled;

static const char *usage =
"usage: cusexmp [options]\n"
"\n"
"options:\n"
"    --help|-h             print this help message\n"
"    --trace|-t            print hex trace of USB activity\n"
"    --maj=MAJ|-M MAJ      device major number\n"
"    --min=MIN|-m MIN      device minor number\n"
"    --name=NAME|-n NAME   device name (mandatory)\n"
"\n";

static void cusexmp_open(fuse_req_t req, struct fuse_file_info *fi)
{
	fuse_reply_open(req, fi);
}

#define LINESZ			16

static void trace_hex(const char *pfx, const unsigned char *data, int len)
{
	if (!trace_enabled)
		return;

	for (; len > 0; len -= LINESZ, data += LINESZ) {
		int i;

		printf("%s", pfx);
		for (i = 0; i < LINESZ; i++) {
			if (i >= len)
				printf("   ");
			else
				printf(" %02x", data[i]);
		}

		printf("   ");

		for (i = 0; i < LINESZ; i++) {
			if (i >= len)
				printf(" ");
			else
				printf("%c",
				       (isgraph(data[i]) || data[i] == ' ') ?
				       data[i] : '.');
		}
		printf("\n");
	}
}

static void cusexmp_read(fuse_req_t req, size_t size, off_t off,
			 struct fuse_file_info *fi)
{
	char buf[MAX_XFER];
	int ret, actual;

	if (size > MAX_XFER)
		size = MAX_XFER;

	ret = libusb_bulk_transfer(devh, BULK_EP_IN, buf, size,
				 &actual, TIMEOUT_MS);
	if (ret == LIBUSB_ERROR_TIMEOUT)
		actual = 0;
	else if (ret != LIBUSB_SUCCESS) {
		fuse_reply_err(req, EIO);
		return;
	}

	trace_hex("USB>", buf, actual);
	fuse_reply_buf(req, buf, actual);
}

static void cusexmp_write(fuse_req_t req, const char *buf, size_t size,
			  off_t off, struct fuse_file_info *fi)
{
	int actual;

	if (size > MAX_XFER) {
		printf("write err\n");
		fuse_reply_err(req, ENOSPC);
		return;
	}

	trace_hex("USB<", buf, size);
	if (libusb_bulk_transfer(devh, BULK_EP_OUT, (char *)buf, size,
				 &actual, TIMEOUT_MS) != LIBUSB_SUCCESS) {
		fuse_reply_err(req, EIO);
		return;
	}
	fuse_reply_write(req, actual);
}

struct cusexmp_param {
	unsigned		major;
	unsigned		minor;
	int			is_help;
};

#define CUSEXMP_OPT(t, p) { t, offsetof(struct cusexmp_param, p), 1 }

static const struct fuse_opt cusexmp_opts[] = {
	CUSEXMP_OPT("-M %u",		major),
	CUSEXMP_OPT("--maj=%u",		major),
	CUSEXMP_OPT("-m %u",		minor),
	CUSEXMP_OPT("--min=%u",		minor),
	FUSE_OPT_KEY("-h",		0),
	FUSE_OPT_KEY("--help",		0),
	FUSE_OPT_KEY("-t",		1),
	FUSE_OPT_KEY("--trace",		1),
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
	case 1:
		trace_enabled = 1;
		return 0;
	default:
		return 1;
	}
}

static const struct cuse_lowlevel_ops cusexmp_clop = {
	.open		= cusexmp_open,
	.read		= cusexmp_read,
	.write		= cusexmp_write,
};

static int open_usb(void)
{
	libusb_init(NULL);

	devh = libusb_open_device_with_vid_pid(NULL, G60_VID, G60_PID);
	if (!devh) {
		printf("could not find device\n");
		goto err;
	}
	libusb_detach_kernel_driver(devh, 0);
	libusb_reset_device(devh);

	if (libusb_set_configuration(devh, 1) != LIBUSB_SUCCESS) {
		printf("could not set configuration\n");
		goto err;
	}

	if (libusb_claim_interface(devh, 0) != LIBUSB_SUCCESS) {
		printf("could not claim interface\n");
		goto err;
	}
	return 0;

err:
	if (devh)
		libusb_close(devh);
	libusb_exit(NULL);
	return -ENOENT;
}

int main(int argc, char **argv)
{
	struct fuse_args args = FUSE_ARGS_INIT(argc, argv);
	struct cusexmp_param param = { 0, 0, 0 };
	struct cuse_info ci;
	const char *dev_info_argv = "DEVNAME=g60";

	if (fuse_opt_parse(&args, &param, cusexmp_opts, cusexmp_process_arg)) {
		printf("failed to parse option\n");
		return 1;
	}

	memset(&ci, 0, sizeof(ci));
	ci.dev_major = param.major;
	ci.dev_minor = param.minor;
	ci.dev_info_argc = 1;
	ci.dev_info_argv = &dev_info_argv;

	if (open_usb() < 0)
		return 1;

	return cuse_lowlevel_main(args.argc, args.argv, &ci, &cusexmp_clop,
				  NULL);
}
