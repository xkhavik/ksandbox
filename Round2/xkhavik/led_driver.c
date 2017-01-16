/*
 * led_driver.c
 *
 *  Created on: 15.01.2017
 *      Author: House
 */

#include <linux/module.h>
#include <linux/moduleparam.h>
#include <linux/platform_device.h>
#include <linux/of.h>
#include <linux/kernel.h>
#include <linux/timer.h>
#include <linux/gpio.h>
#include <linux/delay.h>
#include <linux/leds.h>
#include <linux/spinlock.h>

#define DRIVER_AUTHOR  "Viktor Khabalevskyi"
#define DRIVER_DESC "Test driver"
#define DRIVER_NAME "xkhavik_bbb"
#define DRIVER_COMPATIBLE "gl,console-heartbeat"

struct xkh_gpio_led {
	unsigned int on;
	unsigned int off;
	const char *name;
	struct timer_list timer;
	struct class class_led;
	struct gpio_desc *gpiod;
	spinlock_t lock;
};
static struct xkh_gpio_led *leds;
//static int number_leds;
static int index;
struct timer_list timer_leds;

static void timer_leds_handle(unsigned long param)
{
	struct xkh_gpio_led *leds = (struct xkh_gpio_led *)param;
	unsigned long exp;
	printk(KERN_INFO "index = %d", index);
	spin_lock_bh(&leds[index].lock);
	gpiod_set_value(leds[index].gpiod, 0);
	spin_unlock_bh(&leds[index].lock);
	if (index+1 > 3)
	{
	  printk(KERN_INFO " -1 index = %d", index);
	  index = -1;
	}
	printk(KERN_INFO " index = %d, set true", index);
	//dev_info(dev,"index = %d, set true", index);
	spin_lock_bh(&leds[index+1].lock);
	gpiod_set_value(leds[index+1].gpiod, 1);
	spin_unlock_bh(&leds[index+1].lock);
	index = index+1;
	exp = jiffies + msecs_to_jiffies(500);
	printk(KERN_INFO " exp = %d, jiffies = [%u], index = %d", exp , jiffies, index);
	mod_timer(&timer_leds, exp);
}


static const struct of_device_id xkhavik_of_match[] = {
	{.compatible = DRIVER_COMPATIBLE,},
	{ },
};

MODULE_DEVICE_TABLE(of, xkhavik_of_match);

static int xkhavik_probe(struct platform_device *pdev)
{
	const char         *str_property;
	struct device      *dev         = &pdev->dev;
	struct device_node *pnode        = dev->of_node;
	//int i = 0;
	int number_leds;
	struct fwnode_handle *child;
	int res = 0;
	index = 0;

	if (dev == NULL)
		return -ENODEV;

	if (pnode == NULL) {
		dev_err(dev, "There is not such device node");
		return -EINVAL;
	}

	dev_info(dev, "started prob module");

	if (of_property_read_string(pnode, "string-property",
					&str_property) == 0) {
		dev_info(dev, "string-property: %s\n", str_property);
	} else {
		dev_err(dev, "Error of reading string-property");
		return -EINVAL;
	}
	number_leds = device_get_child_node_count(dev);
	if (!number_leds) {
		dev_info(dev, "the leds are absent");
		return -ENODEV;
	}
	else
		dev_info(dev, "Number of leds is %d", number_leds);


	leds = devm_kzalloc(dev, sizeof(struct xkh_gpio_led) * number_leds, GFP_KERNEL);
	if (!leds) {
		dev_err(dev, "Error of memory allocation");
		return -ENOMEM;

	}

	device_for_each_child_node(dev, child) {

		leds[index].gpiod = devm_get_gpiod_from_child(dev, NULL, child);
		if (IS_ERR(leds[index].gpiod)) {
			dev_err(dev, "Gettig error of gpiod \n");
			res = -ENOENT;
			continue;
		}
		res = gpiod_direction_output(leds[index].gpiod, 0);
		if (res < 0) {
			dev_err(dev, "Error gpiod direction output()\n");
			continue;
		}
		dev_info(dev,"gpiod number %d is registred", index);
		gpiod_set_value(leds[index].gpiod, 0);
		index++;
	}

	/* rize timer */
	index = 0;
	init_timer(&timer_leds);
	timer_leds.data = (unsigned long)leds;
	timer_leds.expires = jiffies+msecs_to_jiffies(500);
	timer_leds.function = timer_leds_handle;
	add_timer(&timer_leds);
	return res;
}

static int xkhavik_remove(struct platform_device *pdev)
{
	if (timer_pending(&timer_leds))	{
		del_timer_sync(&timer_leds);
		dev_info(&pdev->dev, "delete timer");
	}
	dev_info(&pdev->dev, "Remove module");
	return 0;
}

static struct platform_driver xkhavik_driver = {
	.probe = xkhavik_probe,
	.remove = xkhavik_remove,
	.driver = {
			.name = DRIVER_NAME,
			.of_match_table = of_match_ptr(xkhavik_of_match),
			.owner = THIS_MODULE,
		  },
};
module_platform_driver(xkhavik_driver);

MODULE_LICENSE("GPL");
MODULE_AUTHOR(DRIVER_AUTHOR);
MODULE_DESCRIPTION(DRIVER_DESC);

