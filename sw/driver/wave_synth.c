/*
 * Device driver for the FPGA wave-table synth.
 *
 * Minimal platform driver: exposes the synth's register region to userspace
 * via /dev/wave_synth. Userspace opens the device and mmaps it to write
 * control registers directly (no /dev/mem, no root).
 *
 * Adapted from the lab 3 vga_ball driver (Stephen A. Edwards).
 */

#include "wave_synth.h"
#include <linux/errno.h>
#include <linux/fs.h>
#include <linux/init.h>
#include <linux/io.h>
#include <linux/kernel.h>
#include <linux/miscdevice.h>
#include <linux/mm.h>
#include <linux/module.h>
#include <linux/of.h>
#include <linux/of_address.h>
#include <linux/platform_device.h>
#include <linux/version.h>

struct wave_synth_dev {
    struct resource res;       /* MMIO region for our registers */
    void __iomem   *virtbase;  /* kernel-side mapping (unused by mmap path) */
} dev;

/*
 * mmap: map our register MMIO range into the calling userspace process.
 * Userspace can then write control registers as if they were memory.
 */
static int wave_synth_mmap(struct file *f, struct vm_area_struct *vma) {
    unsigned long size = vma->vm_end - vma->vm_start;
    unsigned long res_size = resource_size(&dev.res);

    if (vma->vm_pgoff != 0) {
        return -EINVAL;
    }
    if (size > res_size) {
        return -EINVAL;
    }

    vma->vm_page_prot = pgprot_noncached(vma->vm_page_prot);

    if (io_remap_pfn_range(vma, vma->vm_start,
                           dev.res.start >> PAGE_SHIFT,
                           size, vma->vm_page_prot)) {
        return -EAGAIN;
    }
    return 0;
}

static const struct file_operations wave_synth_fops = {
    .owner = THIS_MODULE,
    .mmap  = wave_synth_mmap,
};

static struct miscdevice wave_synth_misc_device = {
    .minor = MISC_DYNAMIC_MINOR,
    .name  = WAVE_SYNTH_NAME,
    .fops  = &wave_synth_fops,
};

static int __init wave_synth_probe(struct platform_device *pdev) {
    int ret;

    ret = misc_register(&wave_synth_misc_device);
    if (ret) {
        return ret;
    }

    ret = of_address_to_resource(pdev->dev.of_node, 0, &dev.res);
    if (ret) {
        ret = -ENOENT;
        goto out_deregister;
    }

    if (request_mem_region(dev.res.start, resource_size(&dev.res),
                           WAVE_SYNTH_NAME) == NULL) {
        ret = -EBUSY;
        goto out_deregister;
    }

    dev.virtbase = of_iomap(pdev->dev.of_node, 0);
    if (dev.virtbase == NULL) {
        ret = -ENOMEM;
        goto out_release_mem_region;
    }

    return 0;

out_release_mem_region:
    release_mem_region(dev.res.start, resource_size(&dev.res));
out_deregister:
    misc_deregister(&wave_synth_misc_device);
    return ret;
}

static int wave_synth_remove(struct platform_device *pdev) {
    iounmap(dev.virtbase);
    release_mem_region(dev.res.start, resource_size(&dev.res));
    misc_deregister(&wave_synth_misc_device);
    return 0;
}

#ifdef CONFIG_OF
static const struct of_device_id wave_synth_of_match[] = {
    { .compatible = "csee4840,wave_table_synth-1.0" },
    { },
};
MODULE_DEVICE_TABLE(of, wave_synth_of_match);
#endif

static struct platform_driver wave_synth_driver = {
    .driver = {
        .name           = WAVE_SYNTH_NAME,
        .owner          = THIS_MODULE,
        .of_match_table = of_match_ptr(wave_synth_of_match),
    },
    .remove = __exit_p(wave_synth_remove),
};

static int __init wave_synth_init(void) {
    pr_info(WAVE_SYNTH_NAME ": init\n");
    return platform_driver_probe(&wave_synth_driver, wave_synth_probe);
}

static void __exit wave_synth_exit(void) {
    platform_driver_unregister(&wave_synth_driver);
    pr_info(WAVE_SYNTH_NAME ": exit\n");
}

module_init(wave_synth_init);
module_exit(wave_synth_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Columbia CSEE 4840 Final Project");
MODULE_DESCRIPTION("FPGA wave-table synth driver");
