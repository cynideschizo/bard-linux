#ifndef UDLFB_H
#define UDLFB_H

/* as libdlo */
#define BUF_HIGH_WATER_MARK	1024
#define BUF_SIZE		(64*1024)

struct dlfb_data {
	struct usb_device *udev;
	struct usb_interface *interface;
	struct urb *tx_urb, *ctrl_urb;
	struct usb_ctrlrequest dr;
	struct fb_info *info;
	char *buf;
	char *bufend;
	char *backing_buffer;
	struct mutex bulk_mutex;
	atomic_t fb_count;
	char edid[128];
	int sku_pixel_limit;
	int screen_size;
	int line_length;
	struct completion done;
	int base16;
	int base16d;
	int base8;
	int base8d;
	u32 pseudo_palette[256];
	/* blit-only rendering path metrics, exposed through sysfs */
	atomic_t bytes_rendered; /* raw pixel-bytes driver asked to render */
	atomic_t bytes_identical; /* saved effort with backbuffer comparison */
	atomic_t bytes_sent; /* to usb, after compression including overhead */
	atomic_t cpu_kcycles_used; /* transpired during pixel processing */
	/* interface usage metrics. Clients can call driver via several */
	atomic_t blit_count;
	atomic_t copy_count;
	atomic_t fill_count;
	atomic_t damage_count;
	atomic_t defio_fault_count;
};

static void dlfb_bulk_callback(struct urb *urb)
{
	struct dlfb_data *dev_info = urb->context;
	complete(&dev_info->done);
}

static void dlfb_get_edid(struct dlfb_data *dev_info)
{
	int i;
	int ret;
	char rbuf[2];

	for (i = 0; i < 128; i++) {
		ret =
		    usb_control_msg(dev_info->udev,
				    usb_rcvctrlpipe(dev_info->udev, 0), (0x02),
				    (0x80 | (0x02 << 5)), i << 8, 0xA1, rbuf, 2,
				    0);
		/*printk("ret control msg edid %d: %d [%d]\n",i, ret, rbuf[1]); */
		dev_info->edid[i] = rbuf[1];
	}

}

static int dlfb_bulk_msg(struct dlfb_data *dev_info, int len)
{
	int ret;

	init_completion(&dev_info->done);

	dev_info->tx_urb->actual_length = 0;
	dev_info->tx_urb->transfer_buffer_length = len;

	ret = usb_submit_urb(dev_info->tx_urb, GFP_KERNEL);
	if (!wait_for_completion_timeout(&dev_info->done, 1000)) {
		usb_kill_urb(dev_info->tx_urb);
		printk("usb timeout !!!\n");
	}

	return dev_info->tx_urb->actual_length;
}

#define dlfb_set_register insert_command

#endif
