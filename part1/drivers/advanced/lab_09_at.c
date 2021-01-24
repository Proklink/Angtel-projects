// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2019 JSC Angstrem-Telecom. All rights reserved.

#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/platform_device.h>
#include <linux/slab.h>
#include <linux/of.h>
#include <linux/interrupt.h>
#include <linux/kobject.h>
#include <linux/sysfs.h>

#define MEM_REG_ID "a100-wdt-registers"
#define DEV_NAME "a100-wdt"
#define SYSFS_DIR_NAME "a100-timers"
#define TIMER_0_GRP_NAME "timer-0"

#define A100_TIMERS_CONFIGURATION          0x0000 /* RW */
#define A100_TIMERS_INTERRUPT_STATUS_CLEAR 0x0004 /* RW */
#define A100_TIMERS_INTERRUPT_ENABLE       0x000C /* RW */
#define A100_TIMERS_T0_C0                  0x0014 /* RW */
#define A100_TIMERS_T0_COUNTER             0x0018 /* RW */

static ssize_t attr_show(struct device *dev,
			 struct device_attribute *attr,
			 char *buf);

static ssize_t attr_store(struct device *dev,
			  struct device_attribute *attr,
			  const char *buf,
			  size_t count);

struct a100_wdt {
	struct platform_device *pdev;
	struct resource *res;
	void __iomem *base;
	int irq;
	int irq_count;
	u32 maximum_count;
};

static DEVICE_ATTR(maximum_count, 0644, attr_show, attr_store);
static DEVICE_ATTR(current_count, 0444, attr_show, attr_store);
static DEVICE_ATTR(interrupt_count, 0444, attr_show, attr_store);

static struct attribute *timer_0_attrs[] = {
	&dev_attr_maximum_count.attr,
	&dev_attr_current_count.attr,
	&dev_attr_interrupt_count.attr,
	NULL
};

static const struct attribute_group timer_0_group = {
	.name  = TIMER_0_GRP_NAME,
	.attrs  = timer_0_attrs
};

static void set_reg_bit(struct a100_wdt *wdt_data,
			u32 base_offset,
			u32 offset,
			u32 bit_value)
{
	u32 reg_value;

	reg_value = readl(wdt_data->base + base_offset);

	if (bit_value == 1) {
		reg_value |= BIT(offset);
		writel(reg_value, wdt_data->base + base_offset);
	}

	if (bit_value == 0) {
		reg_value &= ~BIT(offset);
		writel(reg_value, wdt_data->base + base_offset);
	}
}

static void set_reg_value(struct a100_wdt *wdt_data,
			  u32 base_offset,
			  u32 reg_value)
{
	writel(reg_value, wdt_data->base + base_offset);
}

static void show_reg(struct a100_wdt *wdt_data, u32 base_offset)
{
	u32 reg_value;

	reg_value = readl(wdt_data->base + base_offset);
	printk(KERN_INFO "Register: 0x%08X. Value: 0x%08X.\n",
	       (u32) (wdt_data->base + base_offset),
	       reg_value);
}

static ssize_t attr_show(struct device *dev,
			 struct device_attribute *attr,
			 char *buf)
{
	struct platform_device *pdev;
	struct a100_wdt *wdt_data;
	const char *attr_name;

	pdev = to_platform_device(dev);
	wdt_data = platform_get_drvdata(pdev);
	attr_name = attr->attr.name;

	if (strcmp(attr->attr.name, "maximum_count") == 0) {
		u32 reg_value;

		reg_value = readl(wdt_data->base + A100_TIMERS_T0_C0);
		return scnprintf(buf, PAGE_SIZE, "0x%08X\n", reg_value);
	}

	if (strcmp(attr->attr.name, "current_count") == 0) {
		u32 reg_value;

		reg_value = readl(wdt_data->base + A100_TIMERS_T0_COUNTER);
		return scnprintf(buf, PAGE_SIZE, "0x%08X\n", reg_value);
	}

	if (strcmp(attr->attr.name, "interrupt_count") == 0) {
		return scnprintf(buf, PAGE_SIZE, "%d\n", wdt_data->irq_count);
	}

	return 0;
}

static ssize_t attr_store(struct device *dev,
			  struct device_attribute *attr,
			  const char *buf,
			  size_t count)
{
	struct platform_device *pdev;
	struct a100_wdt *wdt_data;
	const char *attr_name;

	pdev = to_platform_device(dev);
	wdt_data = platform_get_drvdata(pdev);
	attr_name = attr->attr.name;

	if (strcmp(attr->attr.name, "maximum_count") == 0) {
		unsigned long reg_value;
		int ret;

		ret = kstrtoul(buf, 0, &reg_value);
		if (ret < 0) {
			printk(KERN_ERR "a100-wdt: failed to convert string\n");
			return ret;
		}

		if (reg_value < 0x1)
			printk(KERN_ERR "a100-wdt: invalid value\n");
		else
			writel((u32) reg_value, wdt_data->base + A100_TIMERS_T0_C0);
	}

	return count;
}

static irqreturn_t a100_wdt_isr(int irq, void *data)
{
	struct a100_wdt *wdt_data = data;

	wdt_data->irq_count++;
	set_reg_bit(wdt_data, A100_TIMERS_INTERRUPT_STATUS_CLEAR, 2, 1);

	return IRQ_HANDLED;
}

static int a100_wdt_probe(struct platform_device *pdev)
{
	struct a100_wdt *wdt_data;
	struct resource *res;
	void __iomem *base;
	int irq;
	int ret;

	wdt_data = kzalloc(sizeof(struct a100_wdt), GFP_KERNEL);
	if (wdt_data == NULL) {
		printk(KERN_ERR "a100-wdt: cannot allocate memory\n");
		return -ENOMEM;
	}
	wdt_data->pdev = pdev;

	res = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	if (res == NULL) {
		kfree(wdt_data);
		printk(KERN_ERR "a100-wdt: no memory resource\n");
		return -1;
	}
	res = request_mem_region(res->start, resource_size(res), MEM_REG_ID);
	if (res == NULL) {
		kfree(wdt_data);
		printk(KERN_ERR "a100-wdt: failed to request memory region\n");
		return -1;
	}
	wdt_data->res = res;

	base = ioremap(res->start, resource_size(res));
	if (base == NULL) {
		kfree(wdt_data);
		printk(KERN_ERR "a100-wdt: failed to remap registers\n");
		return -1;
	}
	wdt_data->base = base;

	irq = platform_get_irq(pdev, 1);
	if (irq < 0) {
		kfree(wdt_data);
		printk(KERN_ERR "a100-wdt: no irq resource %d\n", irq);
		return irq;
	}
	wdt_data->irq = irq;

	ret = request_irq(irq, a100_wdt_isr, 0, DEV_NAME, wdt_data);
	if (ret < 0) {
		kfree(wdt_data);
		printk(KERN_ERR "a100-wdt: failed to set irq handler\n");
		return ret;
	}

	wdt_data->irq_count = 0;

	platform_set_drvdata(pdev, wdt_data);

	/* Show registers values. */
	show_reg(wdt_data, A100_TIMERS_CONFIGURATION);
	show_reg(wdt_data, A100_TIMERS_INTERRUPT_ENABLE);
	show_reg(wdt_data, A100_TIMERS_T0_C0);

	/* Set interval of 10 microseconds for timer counter. */
	set_reg_bit(wdt_data, A100_TIMERS_CONFIGURATION, 16, 1);
	/* Set Timer 0 autoreload when it is equal to Compare 0 value. */
	set_reg_bit(wdt_data, A100_TIMERS_CONFIGURATION, 20, 1);

	/* Set Timer 0 Compare 0 value. */
	set_reg_value(wdt_data, A100_TIMERS_T0_C0, 0xFFFF);
	wdt_data->maximum_count = 0xFFFF;

	/* Enable Timer 0 Compare 0 interrupts. */
	set_reg_bit(wdt_data, A100_TIMERS_INTERRUPT_ENABLE, 2, 1);

	ret = sysfs_create_group(&pdev->dev.kobj, &timer_0_group);
	if (ret < 0) {
		kfree(wdt_data);
		printk(KERN_ERR "a100-wdt: failed to create sysfs group\n");
		return ret;
	}

	return 0;
}

static int a100_wdt_remove(struct platform_device *pdev)
{
	struct a100_wdt *wdt_data;

	wdt_data = platform_get_drvdata(pdev);
	sysfs_remove_group(&pdev->dev.kobj, &timer_0_group);
	free_irq(wdt_data->irq, wdt_data);
	iounmap(wdt_data->base);
	release_mem_region(wdt_data->res->start, resource_size(wdt_data->res));
	kfree(wdt_data);

	return 0;
}

static const struct of_device_id a100_wdt_of_match[] = {
	{ .compatible = "ctlm,a100-wdt", },
	{}
};

static struct platform_driver a100_wdt_drv = {
	.driver = {
		.owner = THIS_MODULE,
		.name = "a100_wdt",
		.of_match_table = a100_wdt_of_match,
	},
	.probe = a100_wdt_probe,
	.remove = a100_wdt_remove,
};

static int __init a100_wdt_init(void)
{
	return platform_driver_register(&a100_wdt_drv);
}

static void __exit a100_wdt_exit(void)
{
	platform_driver_unregister(&a100_wdt_drv);
}

module_init(a100_wdt_init);
module_exit(a100_wdt_exit);

MODULE_AUTHOR("JSC Angstrem-Telecom");
MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("Laboratory work #9");
MODULE_SUPPORTED_DEVICE("Centillium Atlanta 100 WDT");
