/*****************************************************************************
* Copyright 2013 Broadcom Corporation.  All rights reserved.
 *
* Unless you and Broadcom execute a separate written software license
* agreement governing use of this software, this software is licensed to you
* under the terms of the GNU General Public License version 2, available at
* http://www.broadcom.com/licenses/GPLv2.php (the "GPL").
 *
* Notwithstanding the above, under no circumstances may you combine this
* software in any way with any other Broadcom software provided under a
* license other than the GPL, without Broadcom's express prior written
* consent.
*****************************************************************************/

#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/fs.h>
#include <linux/slab.h>
#include <linux/init.h>
#include <linux/list.h>
#include <linux/i2c.h>
#include <linux/irq.h>
#include <linux/jiffies.h>
#include <linux/uaccess.h>
#include <linux/delay.h>
#include <linux/interrupt.h>
#include <linux/io.h>
#include <linux/platform_device.h>
#include <linux/gpio.h>
#include <linux/miscdevice.h>
#include <linux/spinlock.h>
#include <linux/poll.h>
#include <linux/version.h>
#include <linux/pm_wakeup.h>
#include <linux/timer.h>

#define USE_PLATFORM_DATA

#ifdef USE_PLATFORM_DATA
#include <linux/nfc/bcm2079x.h>
#else
#include <linux/of.h>
#include <linux/of_irq.h>
#include <linux/of_platform.h>
#include <linux/of_gpio.h>
#endif

#ifdef CONFIG_DEBUG_FS
#include <linux/debugfs.h>
#endif
#include <linux/nfc/bcm2079x.h>
#ifdef CONFIG_HAS_WAKELOCK
#include <linux/wakelock.h>
#endif

#define TRUE		1
#define FALSE		0
#define STATE_HIGH	1
#define STATE_LOW	0


#ifdef CONFIG_DEBUG_FS
static int debug_en = 1;
#define DBG(a...) do {	if (debug_en) a; } while (0)
#define DBG2(a...) do {	if (debug_en > 1) a; } while (0)
#else
#define DBG(a...)
#define DBG2(a...)
#endif

#define MAX_BUFFER_SIZE		780

/* Read data */
#define PACKET_HEADER_SIZE_NCI	(4)
#define PACKET_HEADER_SIZE_HCI	(3)
#define PACKET_TYPE_NCI		(16)
#define PACKET_TYPE_HCIEV	(4)
#define MAX_PACKET_SIZE		(PACKET_HEADER_SIZE_NCI + 255)

#define BCMNFC_MAGIC	0xFA
/*
 * BCMNFC power control via ioctl
 * BCMNFC_POWER_CTL(0): power off
 * BCMNFC_POWER_CTL(1): power on
 * BCMNFC_WAKE_CTL(0): wake off
 * BCMNFC_WAKE_CTL(1): wake on
 */
#define BCMNFC_POWER_CTL	_IO(BCMNFC_MAGIC, 0x01)
#define BCMNFC_WAKE_CTL         _IO(BCMNFC_MAGIC, 0x05)
#define BCMNFC_SET_ADDR		_IO(BCMNFC_MAGIC, 0x07)


struct bcm2079x_dev {
	wait_queue_head_t read_wq;
	struct mutex read_mutex;
	struct i2c_client *client;
	struct miscdevice bcm2079x_device;
	unsigned int wake_gpio;
	unsigned int en_gpio;
	unsigned int irq_gpio;
	bool irq_enabled;
	spinlock_t irq_enabled_lock;
	unsigned int error_write;
	unsigned int error_read;
	unsigned int count_read;
	unsigned int count_irq;
	unsigned int original_address;
	struct wakeup_source *host_wake_ws;
	struct timer_list wake_timer;
};

#ifdef CONFIG_HAS_WAKELOCK
struct wake_lock nfc_soft_wake_lock;
struct wake_lock nfc_pin_wake_lock;
#endif

static void bcm2079x_init_stat(struct bcm2079x_dev *bcm2079x_dev)
{
	bcm2079x_dev->error_write = 0;
	bcm2079x_dev->error_read = 0;
	bcm2079x_dev->count_read = 0;
	bcm2079x_dev->count_irq = 0;
}

#define INTERRUPT_TRIGGER_TYPE  (IRQF_TRIGGER_RISING | IRQF_NO_SUSPEND)

/*
 The alias address 0x79, when sent as a 7-bit address from the host processor
 will match the first byte (highest 2 bits) of the default client address
 (0x1FA) that is programmed in bcm20791.
 When used together with the first byte (0xFA) of the byte sequence below,
 it can be used to address the bcm20791 in a system that does not support
 10-bit address and change the default address to 0x38.
 the new address can be changed by changing the CLIENT_ADDRESS below if 0x38
 conflicts with other device on the same i2c bus.
 */
#define ALIAS_ADDRESS	  0x77

static void set_client_addr(struct bcm2079x_dev *bcm2079x_dev, int addr)
{
	struct i2c_client *client = bcm2079x_dev->client;
	client->addr = addr;
	if (addr > 0x7F)
		client->flags |= I2C_CLIENT_TEN;

	DBG2(dev_info(&client->dev,
		"Set client device changed to (0x%04X) flag = %04x\n",
		client->addr, client->flags));
}

static void change_client_addr(struct bcm2079x_dev *bcm2079x_dev, int addr)
{
	struct i2c_client *client;
	int ret;
	int i;
	int offset = 1;
	char addr_data[] = {
		0xFA, 0xF2, 0x00, 0x00, 0x00, 0x38, 0x00, 0x00, 0x00, 0x2A
	};

	client = bcm2079x_dev->client;
	if (client->addr == addr)
		return;

	if ((client->flags & I2C_CLIENT_TEN) == I2C_CLIENT_TEN) {
		client->addr = ALIAS_ADDRESS;
		client->flags &= ~I2C_CLIENT_TEN;
		offset = 0;
	}

	addr_data[5] = addr & 0xFF;
	ret = 0;
	for (i = 1; i < sizeof(addr_data) - 1; ++i)
		ret += addr_data[i];
	addr_data[sizeof(addr_data) - 1] = (ret & 0xFF);
	dev_info(&client->dev,
		 "Change client device from (0x%04X) flag = "\
		 "%04x, addr_data[%d] = %02x\n",
		 client->addr, client->flags, sizeof(addr_data) - 1,
		 addr_data[sizeof(addr_data) - 1]);

	ret = i2c_master_send(client,
			addr_data+offset, sizeof(addr_data)-offset);
	if (ret != sizeof(addr_data)-offset) {
		client->addr = ALIAS_ADDRESS;
		client->flags &= ~I2C_CLIENT_TEN;
		dev_info(&client->dev,
			 "Change client device from (0x%04X) flag = "\
			 "%04x, addr_data[%d] = %02x\n",
			 client->addr, client->flags, sizeof(addr_data) - 1,
			 addr_data[sizeof(addr_data) - 1]);
		ret = i2c_master_send(client, addr_data, sizeof(addr_data));
	}
	client->addr = addr_data[5];

	dev_info(&client->dev,
		 "Change client device changed to (0x%04X) flag = %04x, ret = %d\n",
		 client->addr, client->flags, ret);
}

static irqreturn_t bcm2079x_dev_irq_handler(int irq, void *dev_id)
{
	struct bcm2079x_dev *bcm2079x_dev = dev_id;
	unsigned long flags;
	DBG2(dev_info(&bcm2079x_dev->client->dev,
		      "irq go high\n"));

	spin_lock_irqsave(&bcm2079x_dev->irq_enabled_lock, flags);
	bcm2079x_dev->count_irq++;
	__pm_stay_awake(bcm2079x_dev->host_wake_ws);
	mod_timer(&bcm2079x_dev->wake_timer, jiffies + msecs_to_jiffies(500));
	spin_unlock_irqrestore(&bcm2079x_dev->irq_enabled_lock, flags);
	wake_up(&bcm2079x_dev->read_wq);

#ifdef CONFIG_HAS_WAKELOCK
	wake_lock(&nfc_soft_wake_lock);
#endif

	return IRQ_HANDLED;
}

void wake_timer_callback(unsigned long data)
{
	struct bcm2079x_dev *bcm2079x_dev = (struct bcm2079x_dev *)data;
	__pm_relax(bcm2079x_dev->host_wake_ws);

}

static unsigned int bcm2079x_dev_poll(struct file *filp, poll_table *wait)
{
	struct bcm2079x_dev *bcm2079x_dev = filp->private_data;
	unsigned int mask = 0;
	unsigned long flags;

	poll_wait(filp, &bcm2079x_dev->read_wq, wait);

	spin_lock_irqsave(&bcm2079x_dev->irq_enabled_lock, flags);
	if (bcm2079x_dev->count_irq > 0)
		mask |= POLLIN | POLLRDNORM;
	spin_unlock_irqrestore(&bcm2079x_dev->irq_enabled_lock, flags);
	DBG2(dev_info(&bcm2079x_dev->client->dev,
		      "bcm2079x_dev_poll\n"));
	return mask;
}

static ssize_t bcm2079x_dev_read(struct file *filp, char __user *buf,
				  size_t count, loff_t *offset)
{
	struct bcm2079x_dev *bcm2079x_dev = filp->private_data;
	unsigned char tmp[MAX_BUFFER_SIZE];
	int total, len, ret;

	total = 0;
	len = 0;
	DBG2(dev_info(&bcm2079x_dev->client->dev,
		      "bcm2079x_dev_read\n"));
	if (bcm2079x_dev->count_irq > 0)
		bcm2079x_dev->count_irq--;

	bcm2079x_dev->count_read++;
	if (count > MAX_BUFFER_SIZE)
		count = MAX_BUFFER_SIZE;

	mutex_lock(&bcm2079x_dev->read_mutex);

	/* Read the first 4 bytes to include
	the length of the NCI or HCI packet. */
	ret = i2c_master_recv(bcm2079x_dev->client,
		tmp, PACKET_HEADER_SIZE_NCI);
	if (ret == PACKET_HEADER_SIZE_NCI) {
		total = ret;
		/* First byte is the packet type */
		switch (tmp[0]) {
		case PACKET_TYPE_NCI:
			len = tmp[PACKET_HEADER_SIZE_NCI-1];
			break;

		case PACKET_TYPE_HCIEV:
			len = tmp[PACKET_HEADER_SIZE_HCI-1];
			if (len == 0)
				total--;
		/*Since payload is 0, decrement total size (from 4 to 3) */
			else
				len--;
		/*First byte of payload is in tmp[3] already */
			break;

		default:
			len = 0;/*Unknown packet byte */
			break;
		}

		/* make sure full packet fits in the buffer */
		if (len > 0 && (len + total) <= count) {
			/* read the remainder of the packet. */
			ret = i2c_master_recv(bcm2079x_dev->client,
				tmp + total, len);
			if (ret < 0) {
				mutex_unlock(&bcm2079x_dev->read_mutex);
				return ret;
			}

			if (ret == len)
				total += len;

		}
	} else {
		mutex_unlock(&bcm2079x_dev->read_mutex);
		if (ret < 0)
			return ret;
		else {
			dev_err(&bcm2079x_dev->client->dev,
				"received only %d bytes as header\n",
				ret);
		return -EIO;
		}
	}

	mutex_unlock(&bcm2079x_dev->read_mutex);

	if (total > count || copy_to_user(buf, tmp, total)) {
		dev_err(&bcm2079x_dev->client->dev,
			"failed to copy to user space, total = %d\n", total);
		total = -EFAULT;
		bcm2079x_dev->error_read++;
	}

	if (bcm2079x_dev->count_irq == 0) {
		del_timer(&bcm2079x_dev->wake_timer);
		__pm_relax(bcm2079x_dev->host_wake_ws);
	}

	DBG2(dev_info(&bcm2079x_dev->client->dev,
		      "bcm2079x_dev_read %d\n", total));

#ifdef CONFIG_HAS_WAKELOCK
	if (bcm2079x_dev->count_irq == 0){
		wake_unlock(&nfc_soft_wake_lock);
		DBG2(dev_info(&bcm2079x_dev->client->dev, "release wake lock\n"));
	}
#endif


	return total;
}

static ssize_t bcm2079x_dev_write(struct file *filp, const char __user *buf,
				   size_t count, loff_t *offset)
{
	struct bcm2079x_dev *bcm2079x_dev = filp->private_data;
	char tmp[MAX_BUFFER_SIZE];
	int ret;

	DBG2(dev_info(&bcm2079x_dev->client->dev,
		      "bcm2079x_dev_write enter %llu\n", get_jiffies_64()));

	if (count > MAX_BUFFER_SIZE) {
		dev_err(&bcm2079x_dev->client->dev, "out of memory\n");
		return -ENOMEM;
	}

	if (copy_from_user(tmp, buf, count)) {
		dev_err(&bcm2079x_dev->client->dev,
			"failed to copy from user space\n");
		return -EFAULT;
	}

	mutex_lock(&bcm2079x_dev->read_mutex);
	/* Write data */

	ret = i2c_master_send(bcm2079x_dev->client, tmp, count);
	if (ret != count) {
		if ((bcm2079x_dev->client->flags & I2C_CLIENT_TEN) !=
		    I2C_CLIENT_TEN && bcm2079x_dev->error_write == 0) {
			//set_client_addr(bcm2079x_dev, 0x1FA);
			ret = i2c_master_send(bcm2079x_dev->client, tmp, count);
			if (ret != count)
				bcm2079x_dev->error_write++;
		} else {
			dev_err(&bcm2079x_dev->client->dev,
				"failed to write %d\n", ret);
			ret = -EIO;
			bcm2079x_dev->error_write++;
		}
	}
	mutex_unlock(&bcm2079x_dev->read_mutex);
	DBG2(dev_info(&bcm2079x_dev->client->dev,
		      "bcm2079x_dev_write leave\n"));

	return ret;
}

static int bcm2079x_dev_open(struct inode *inode, struct file *filp)
{
	int ret = 0;

	struct bcm2079x_dev *bcm2079x_dev = container_of(filp->private_data,
							   struct bcm2079x_dev,
							   bcm2079x_device);


	filp->private_data = bcm2079x_dev;
	bcm2079x_init_stat(bcm2079x_dev);

	DBG(dev_info(&bcm2079x_dev->client->dev,
		 "%d,%d\n", imajor(inode), iminor(inode)));

	return ret;
}

static long bcm2079x_dev_unlocked_ioctl(struct file *filp,
					 unsigned int cmd, unsigned long arg)
{
	struct bcm2079x_dev *bcm2079x_dev = filp->private_data;

	switch (cmd) {
	case BCMNFC_READ_FULL_PACKET:
		break;
	case BCMNFC_READ_MULTI_PACKETS:
		break;
	case BCMNFC_CHANGE_ADDR:
		DBG(dev_info(&bcm2079x_dev->client->dev,
			     "%s, BCMNFC_CHANGE_ADDR (%x, %lx):\n", __func__,
			     cmd, arg));
		//change_client_addr(bcm2079x_dev, arg);
		break;
	case BCMNFC_POWER_CTL:
		DBG(dev_info(&bcm2079x_dev->client->dev,
			 "%s, BCMNFC_POWER_CTL (%x, %lx):\n", __func__, cmd,
			     arg));
		if (arg == 1) {	/* Power On */

			gpio_set_value(bcm2079x_dev->en_gpio, 1);

			if (bcm2079x_dev->irq_enabled == FALSE) {
				bcm2079x_dev->count_irq = 0;
				enable_irq(bcm2079x_dev->client->irq);
				bcm2079x_dev->irq_enabled = true;
			}
		} else {
			if (bcm2079x_dev->irq_enabled == true) {
				bcm2079x_dev->irq_enabled = FALSE;
				disable_irq_nosync(bcm2079x_dev->client->irq);
				if (bcm2079x_dev->count_irq > 0)
					wake_unlock(&nfc_soft_wake_lock);
				}
			gpio_set_value(bcm2079x_dev->en_gpio, 0);
			//set_client_addr(bcm2079x_dev,bcm2079x_dev->original_address);

		}
		break;
	case BCMNFC_WAKE_CTL:
		DBG(dev_info(&bcm2079x_dev->client->dev,
			 "%s, BCMNFC_WAKE_CTL (%x, %lx):\n", __func__, cmd,
			     arg));
		gpio_set_value(bcm2079x_dev->wake_gpio, arg);
#ifdef CONFIG_HAS_WAKELOCK
		if(arg == 1){
			if(wake_lock_active(&nfc_pin_wake_lock))
				wake_unlock(&nfc_pin_wake_lock);
		}
		else{
			if(!wake_lock_active(&nfc_pin_wake_lock))
				wake_lock(&nfc_pin_wake_lock);
		}
#endif
		break;

	default:
		dev_err(&bcm2079x_dev->client->dev,
			"%s, unknown cmd (%x, %lx)\n", __func__, cmd, arg);
		return 0;
	}

	return 0;
}

static int bcm2079x_dev_release(struct inode *inode, struct file *filp)
{

	return 0;
}

static const struct file_operations bcm2079x_dev_fops = {
	.owner = THIS_MODULE,
	.llseek = no_llseek,
	.poll = bcm2079x_dev_poll,
	.read = bcm2079x_dev_read,
	.write = bcm2079x_dev_write,
	.open = bcm2079x_dev_open,
	.release = bcm2079x_dev_release,
	.unlocked_ioctl = bcm2079x_dev_unlocked_ioctl
};

static int bcm2079x_probe(struct i2c_client *client,
			   const struct i2c_device_id *id)
{
	int ret = 0;
	struct bcm2079x_dev *bcm2079x_dev;

#ifdef USE_PLATFORM_DATA
	struct bcm2079x_i2c_platform_data *platform_data;
	platform_data = client->dev.platform_data;
#else
	struct device_node *np = client->dev.of_node;
#endif
	dev_info(&client->dev, "%s, probing bcm2079x driver flags = %x\n",
			__func__, client->flags);
	if (platform_data == NULL) {
		dev_err(&client->dev, "nfc probe fail\n");
		return -ENODEV;
	}

	if (!i2c_check_functionality(client->adapter, I2C_FUNC_I2C)) {
		dev_err(&client->dev, "need I2C_FUNC_I2C\n");
		ret = -EIO;
		goto err_exit;
	}

	ret = gpio_request(platform_data->irq_gpio, "nfc_int");
	if (ret)
		return -ENODEV;
	ret = gpio_request(platform_data->en_gpio, "nfc_en");
	if (ret)
		goto err_en;
	ret = gpio_request(platform_data->wake_gpio, "nfc_wake");
	if (ret)
		goto err_firm;

	gpio_set_value(platform_data->en_gpio, 0);
	gpio_set_value(platform_data->wake_gpio, 1);

	bcm2079x_dev = devm_kzalloc(&client->dev,
		sizeof(*bcm2079x_dev), GFP_KERNEL);
	if (bcm2079x_dev == NULL) {
		dev_err(&client->dev,
			"failed to allocate memory for module data\n");
		ret = -ENOMEM;
		goto err_exit;
	}

#ifdef USE_PLATFORM_DATA
	bcm2079x_dev->wake_gpio = platform_data->wake_gpio;
	bcm2079x_dev->en_gpio = platform_data->en_gpio;
	bcm2079x_dev->irq_gpio = gpio_to_irq(platform_data->irq_gpio);
	platform_data->init(platform_data);

#else
	/*
	 * Obtain the GPIO.
	 */
	bcm2079x_dev->wake_gpio =
			of_get_named_gpio(np, "wake-gpios", 0);
	if (gpio_is_valid(bcm2079x_dev->wake_gpio)) {
		ret = devm_gpio_request_one(&client->dev,
			bcm2079x_dev->wake_gpio,
		       GPIOF_OUT_INIT_LOW,
		       "bcm2079x_wake");
		if (ret) {
			dev_err(&client->dev, "Can not request wake-gpio %d\n",
				bcm2079x_dev->wake_gpio);
			goto err_exit;
		}
	} else {
		dev_err(&client->dev, "Can not find wake-gpio %d\n",
			bcm2079x_dev->wake_gpio);
		ret = bcm2079x_dev->wake_gpio;
		goto err_exit;

	}

	bcm2079x_dev->en_gpio =
			of_get_named_gpio(np, "en-gpios", 0);
	if (gpio_is_valid(bcm2079x_dev->en_gpio)) {
		ret = devm_gpio_request_one(&client->dev, bcm2079x_dev->en_gpio,
		       GPIOF_OUT_INIT_LOW,
		       "bcm2079x_en");
		if (ret) {
			dev_err(&client->dev, "Can not request en-gpio %d\n",
				bcm2079x_dev->en_gpio);
			goto err_exit;
		}
	} else {
		dev_err(&client->dev, "Can not find en-gpio %d\n",
			bcm2079x_dev->en_gpio);
		ret = bcm2079x_dev->en_gpio;
		goto err_exit;

	}


	/*
	 * Obtain the interrupt pin.
	 */
	bcm2079x_dev->irq_gpio = irq_of_parse_and_map(np, 0);
	if (!bcm2079x_dev->irq_gpio) {
		dev_err(&client->dev, "can not get irq\n");
		goto err_exit;
	}

	dev_info(&client->dev,
		"bcm2079x_probe gpio en %d, wake %d, irq %d\n",
		bcm2079x_dev->en_gpio,
		bcm2079x_dev->wake_gpio,
		bcm2079x_dev->irq_gpio);
#endif


	bcm2079x_dev->client = client;

	/* init mutex and queues */
	init_waitqueue_head(&bcm2079x_dev->read_wq);
	mutex_init(&bcm2079x_dev->read_mutex);
	spin_lock_init(&bcm2079x_dev->irq_enabled_lock);

	bcm2079x_dev->bcm2079x_device.minor = MISC_DYNAMIC_MINOR;
	bcm2079x_dev->bcm2079x_device.name = "bcm2079x";
	bcm2079x_dev->bcm2079x_device.fops = &bcm2079x_dev_fops;

	ret = misc_register(&bcm2079x_dev->bcm2079x_device);
	if (ret) {
		dev_err(&client->dev, "misc_register failed %d\n", ret);
		goto err_misc_register;
	}

	DBG(dev_info(&client->dev, "requesting GPIO: %d / IRQ: %d\n",
			platform_data->irq_gpio, bcm2079x_dev->irq_gpio));

	bcm2079x_dev->host_wake_ws = wakeup_source_register(
					"bcm2079x-ws");
	if (bcm2079x_dev->host_wake_ws == NULL) {
		dev_err(&client->dev,
			"failed to register host-wake wakeup source\n");
		goto err_misc_register;
	}

	setup_timer(&bcm2079x_dev->wake_timer,
		wake_timer_callback, (int)bcm2079x_dev);

	/* request irq.  the irq is set whenever the chip has data available
	 * for reading.  it is cleared when all data has been read.
	 */
	DBG(dev_info(&client->dev, "requesting IRQ %d\n", client->irq));

	ret = request_irq(client->irq,
			bcm2079x_dev_irq_handler,
			INTERRUPT_TRIGGER_TYPE,
			client->name, bcm2079x_dev);
	if (ret) {
		dev_err(&client->dev, "request_irq %d failed %d\n",
			bcm2079x_dev->irq_gpio, ret);
		goto err_request_irq_failed;
	}

	ret = irq_set_irq_wake(bcm2079x_dev->irq_gpio, 1);
	if (ret) {
		dev_err(&client->dev, "irq_set_irq_wake %d failed %d\n",
			bcm2079x_dev->irq_gpio, ret);
		goto err_request_irq_failed;
	}

	disable_irq_nosync(client->irq);
	bcm2079x_dev->irq_enabled = FALSE;

	bcm2079x_dev->original_address = client->addr;

#ifdef CONFIG_HAS_WAKELOCK
	wake_lock_init(&nfc_soft_wake_lock, WAKE_LOCK_SUSPEND, "NFCSOFTWAKE");
	wake_lock_init(&nfc_pin_wake_lock, WAKE_LOCK_SUSPEND, "NFCPINWAKE");
#endif
	i2c_set_clientdata(client, bcm2079x_dev);

	DBG(dev_info(&client->dev,
		 "%s, probing bcm2079x driver = 0x%x\n",
		     __func__, client->addr));

	DBG(dev_info(&client->dev,
		 "%s, probing bcm2079x driver exited successfully\n",
		     __func__));

	return 0;

err_request_irq_failed:
	misc_deregister(&bcm2079x_dev->bcm2079x_device);
err_misc_register:
	mutex_destroy(&bcm2079x_dev->read_mutex);
	kfree(bcm2079x_dev);
err_exit:
	gpio_free(platform_data->wake_gpio);
err_firm:
	gpio_free(platform_data->en_gpio);
err_en:
	gpio_free(platform_data->irq_gpio);
	return ret;
}

static int bcm2079x_remove(struct i2c_client *client)
{
	struct bcm2079x_dev *bcm2079x_dev;

	bcm2079x_dev = i2c_get_clientdata(client);
	free_irq(client->irq, bcm2079x_dev);
	misc_deregister(&bcm2079x_dev->bcm2079x_device);
	mutex_destroy(&bcm2079x_dev->read_mutex);
	wakeup_source_unregister(bcm2079x_dev->host_wake_ws);
	gpio_free(bcm2079x_dev->irq_gpio);
	gpio_free(bcm2079x_dev->en_gpio);
	gpio_free(bcm2079x_dev->wake_gpio);
	kfree(bcm2079x_dev);
	return 0;
}

static const struct i2c_device_id bcm2079x_id[] = {
	{"bcm2079x", 0},
	{}
};

static const struct of_device_id bcm2079x_match[] = {
	{.compatible = "bcm,nfc-i2c"},
};

static struct i2c_driver bcm2079x_driver = {
	.id_table = bcm2079x_id,
	.probe = bcm2079x_probe,
	.remove = bcm2079x_remove,
	.driver = {
		.owner = THIS_MODULE,
		.name = "bcm2079x",
		   .of_match_table = bcm2079x_match,
	},
};

/*
 * module load/unload record keeping
 */

static int __init bcm2079x_dev_init(void)
{
	return i2c_add_driver(&bcm2079x_driver);
}
module_init(bcm2079x_dev_init);

static void __exit bcm2079x_dev_exit(void)
{
	i2c_del_driver(&bcm2079x_driver);
}
module_exit(bcm2079x_dev_exit);
#ifdef CONFIG_DEBUG_FS

int __init bcm2079x_debug_init(void)
{
	struct dentry *root_dir;
	root_dir = debugfs_create_dir("bcm2079x", 0);
	if (!root_dir)
		return -ENOMEM;
	if (!debugfs_create_u32
	    ("debug", S_IRUSR | S_IWUSR, root_dir, &debug_en))
		return -ENOMEM;

	return 0;

}

late_initcall(bcm2079x_debug_init);

#endif

MODULE_AUTHOR("Broadcom");
MODULE_DESCRIPTION("NFC bcm2079x driver");
MODULE_LICENSE("GPL");
