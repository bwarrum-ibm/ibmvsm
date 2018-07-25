// SPDX-License-Identifier: GPL-2.0+
/*
 * IBM Power Systems Virtual Management Channel Support.
 *
 * Copyright (c) 2018 IBM Corp.
 *   Bryant G. Ly <bryantly@linux.vnet.ibm.com>
 */

#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/fs.h>
#include <linux/poll.h>
#include <linux/init.h>
#include <linux/io.h>
#include <linux/miscdevice.h>

#include <asm/hvcall.h>
#include <asm/vio.h>

#include "ibmvsm.h"

#define IBMVSM_DRIVER_VERSION "0.1"
#define MSG_HI	0
#define MSG_LOW	1

static const char ibmvsm_driver_name[] = "ibmvsm";

static struct ibmvsm_struct ibmvsm;
static struct crq_server_adapter ibmvsm_adapter;

enum crq_entry_header {
	CRQ_FREE = 0x00,
	CRQ_CMD_RSP = 0x80,
	CRQ_INIT_MSG = 0xC0
};

enum crq_init_formats {
	CRQ_INIT = 0x01,
	CRQ_INIT_COMPLETE = 0x02
};

/**
 * ibmvsm_send_init_message() - send initialization message to the client
 */
static long ibmvsm_send_init_msg(struct crq_server_adapter *adapter, u8 type)
{
	struct ibmvsm_crq_msg *crq;
	struct vio_dev *vdev = to_vio_dev(adapter->dev);
	u64 buffer[2] = { 0 , 0 };
	long rc;

	crq = (struct ibmvsm_crq_msg *)&buffer;
	crq->valid = CRQ_INIT_MSG;
	crq->type = type;
	rc = h_send_crq(vdev->unit_address,
			cpu_to_be64(buffer[MSG_HI]),
			cpu_to_be64(buffer[MSG_LOW]));

	return rc;
}

/**
 * ibmvsm_read - Read
 *
 * @file:	file struct
 * @buf:	character buffer
 * @nbytes:	size in bytes
 * @ppos:	offset
 *
 * Return:
 *	0 - Success
 *	Non-zero - Failure
 */
static ssize_t ibmvsm_read(struct file *file, char *buf, size_t nbytes,
			   loff_t *ppos)
{
	return 0;
}

/**
 * ibmvsm_write - Write
 *
 * @file:	file struct
 * @buf:	character buffer
 * @count:	count field
 * @ppos:	offset
 *
 * Return:
 *	0 - Success
 *	Non-zero - Failure
 */
static ssize_t ibmvsm_write(struct file *file, const char *buffer,
			    size_t count, loff_t *ppos)
{
	return 0;
}

/**
 * ibmvsm_poll - Poll
 *
 * @file:	file struct
 * @wait:	Poll Table
 *
 * Return:
 *	poll.h return values
 */
static unsigned int ibmvsm_poll(struct file *file, poll_table *wait)
{
	return 0;
}

/**
 * ibmvsm_ioctl - IOCTL
 *
 * @session:	ibmvsm_file_session struct
 * @cmd:	cmd field
 * @arg:	Argument field
 *
 * Return:
 *	0 - Success
 *	Non-zero - Failure
 */
static long ibmvsm_ioctl(struct file *file,
			 unsigned int cmd, unsigned long arg)
{
	return 0;
}

/**
 * ibmvsm_open - Open Session
 *
 * @inode:	inode struct
 * @file:	file struct
 *
 * Return:
 *	0 - Success
 */
static int ibmvsm_open(struct inode *inode, struct file *file)
{
	return 0;
}

/**
 * ibmvsm_close - Close Session
 *
 * @inode:	inode struct
 * @file:	file struct
 *
 * Return:
 *	0 - Success
 *	Non-zero - Failure
 */
static int ibmvsm_close(struct inode *inode, struct file *file)
{
	return 0;
}

static const struct file_operations ibmvsm_fops = {
	.owner		= THIS_MODULE,
	.read		= ibmvsm_read,
	.write		= ibmvsm_write,
	.poll		= ibmvsm_poll,
	.unlocked_ioctl	= ibmvsm_ioctl,
	.open           = ibmvsm_open,
	.release        = ibmvsm_close,
};

/* Fill in the liobn and riobn fields on the adapter */
static int read_dma_window(struct vio_dev *vdev,
			   struct crq_server_adapter *adapter)
{
	const __be32 *dma_window;
	const __be32 *prop;

	dma_window =
		(const __be32 *)vio_get_attribute(vdev, "ibm,my-dma-window",
						  NULL);
	if (!dma_window) {
		dev_warn(adapter->dev, "Couldn't find ibm,my-dma-window property\n");
		return -1;
	}

	adapter->liobn = be32_to_cpu(*dma_window);
	dma_window++;

	prop = (const __be32 *)vio_get_attribute(vdev, "ibm,#dma-address-cells",
						 NULL);
	if (!prop) {
		dev_warn(adapter->dev, "Couldn't find ibm,#dma-address-cells property\n");
		dma_window++;
	} else {
		dma_window += be32_to_cpu(*prop);
	}

	prop = (const __be32 *)vio_get_attribute(vdev, "ibm,#dma-size-cells",
						 NULL);
	if (!prop) {
		dev_warn(adapter->dev, "Couldn't find ibm,#dma-size-cells property\n");
		dma_window++;
	} else {
		dma_window += be32_to_cpu(*prop);
	}

	/* dma_window should point to the second window now */
	adapter->riobn = be32_to_cpu(*dma_window);

	return 0;
}

static int ibmvsm_probe(struct vio_dev *vdev, const struct vio_device_id *id)
{
	struct crq_server_adapter *adapter = &ibmvsm_adapter;
	int rc;

	dev_set_drvdata(&vdev->dev, NULL);
	memset(adapter, 0, sizeof(*adapter));
	adapter->dev = &vdev->dev;

	dev_info(adapter->dev, "Probe for UA 0x%x\n", vdev->unit_address);

	/* Read DMA Window */
	rc = read_dma_window(vdev, adapter);
	if (rc != 0) {
		ibmvsm.state = ibmvsm_state_failed;
		return -1;
	}

	dev_dbg(adapter->dev, "Probe: liobn 0x%x, riobn 0x%x\n",
		adapter->liobn, adapter->riobn);

	/* Init CRQ */

	return 0;
}

static int ibmvsm_remove(struct vio_dev *vdev)
{
	struct crq_server_adapter *adapter = dev_get_drvdata(&vdev->dev);

	dev_info(adapter->dev, "Entering remove for UA 0x%x\n",
		 vdev->unit_address);

	/* Need to release the CRQ here */

	return 0;
}

static struct vio_device_id ibmvsm_device_table[] = {
	{ "serial-multiplex", "IBM,serial-multiplex" },
	{ "", "" }
};
MODULE_DEVICE_TABLE(vio, ibmvsm_device_table);

static struct vio_driver ibmvsm_driver = {
	.name        = ibmvsm_driver_name,
	.id_table    = ibmvsm_device_table,
	.probe       = ibmvsm_probe,
	.remove      = ibmvsm_remove,
};

static struct miscdevice ibmvsm_miscdev = {
	.name = ibmvsm_driver_name,
	.minor = MISC_DYNAMIC_MINOR,
	.fops = &ibmvsm_fops,
};

static int __init ibmvsm_module_init(void)
{
	int rc;

	ibmvsm.state = ibmvsm_state_initial;
	pr_info("ibmvsm: version %s\n", IBMVSM_DRIVER_VERSION);

	rc = misc_register(&ibmvsm_miscdev);
	if (rc) {
		pr_err("ibmvsm: misc registration failed\n");
		goto misc_register_fail;
	}
	pr_info("ibmvsm: node %d:%d\n", MISC_MAJOR,
		ibmvsm_miscdev.minor);

	rc = vio_register_driver(&ibmvsm_driver);
	if (rc) {
		pr_err("ibmvsm: rc %d from vio_register_driver\n", rc);
		goto vio_reg_fail;
	}
	/* Init data structures */
	return 0;
vio_reg_fail:
	misc_deregister(&ibmvsm_miscdev);
misc_register_fail:
	return rc;
}

static void __exit ibmvsm_module_exit(void)
{
	pr_info("ibmvsm: module exit\n");
	vio_unregister_driver(&ibmvsm_driver);
	misc_deregister(&ibmvsm_miscdev);
}

MODULE_AUTHOR("Bryant G. Ly <bryantly@linux.vnet.ibm.com>");
MODULE_DESCRIPTION("IBM VSM");
MODULE_VERSION(IBMVSM_DRIVER_VERSION);
MODULE_LICENSE("GPL v2");
