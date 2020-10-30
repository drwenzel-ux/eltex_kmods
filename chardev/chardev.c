#include <linux/cdev.h>
#include <linux/device.h>
#include <linux/fs.h>
#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/semaphore.h>
#include <linux/slab.h>
#include <linux/string.h>
#include <linux/uaccess.h>

#define DEVNAME "chardev"
#define SIZE 128

MODULE_DESCRIPTION("Char device");
MODULE_AUTHOR("Wenzel");
MODULE_LICENSE("GPL");

static dev_t major;
static struct cdev *cdev;
static struct class *class;
static struct semaphore sem;
static unsigned char *mbuf;

static void free_resources(void);
static int chardev_open(struct inode *inode, struct file *file);
static int chardev_release(struct inode *inode, struct file *file);
static ssize_t chardev_read(struct file *fd, char __user *buf, size_t size,
                            loff_t *off);
static ssize_t chardev_write(struct file *fd, const char __user *buf,
                             size_t size, loff_t *off);

static struct file_operations fops = {.owner = THIS_MODULE,
                                      .read = chardev_read,
                                      .write = chardev_write,
                                      .open = chardev_open,
                                      .release = chardev_release};

static int __init chardev_init(void) {
  int ret;

  pr_info("%s: started loading module!\n", DEVNAME);

  ret = alloc_chrdev_region(&major, 0, 1, DEVNAME);
  if (ret < 0) {
    pr_err("%s: alloc_chrdev_region failed!\n", DEVNAME);
    return -1;
  }

  class = class_create(THIS_MODULE, DEVNAME);
  if (class == NULL) {
    pr_err("%s: class_create failed!\n", DEVNAME);
    free_resources();
    return -1;
  }

  if (device_create(class, NULL, major, NULL, "chardev") == NULL) {
    pr_err("%s: device_create failed!\n", DEVNAME);
    free_resources();
    return -1;
  }

  cdev = cdev_alloc();
  cdev->ops = &fops;
  cdev->owner = THIS_MODULE;

  mbuf = kcalloc(SIZE, 1, GFP_KERNEL);
  if (mbuf == NULL) {
    pr_err("%s: kcalloc failed!\n", DEVNAME);
    free_resources();
    return -1;
  }

  ret = cdev_add(cdev, major, 1);
  if (ret == -1) {
    pr_err("%s: cdev_add failed!\n", DEVNAME);
    free_resources();
    return -1;
  }

  strncpy(mbuf, "start\n", 6);

  sema_init(&sem, 1);

  pr_info("%s: finished loading module!\n", DEVNAME);
  return 0;
}

static void __exit chardev_exit(void) {
  free_resources();

  pr_info("%s: unloading module!\n", DEVNAME);
}

int chardev_open(struct inode *inode, struct file *file) {
  pr_info("%s: opened char device!\n", DEVNAME);
  return 0;
}

int chardev_release(struct inode *inode, struct file *file) {
  pr_info("%s: closed char device!\n", DEVNAME);
  return 0;
}

ssize_t chardev_read(struct file *fd, char __user *buf, size_t size,
                     loff_t *off) {
  pr_info("%s: read from char device!\n", DEVNAME);
  return simple_read_from_buffer(buf, size, off, mbuf, sizeof(mbuf));
}

ssize_t chardev_write(struct file *fd, const char __user *buf, size_t size,
                      loff_t *off) {
  ssize_t rc;

  if (size > SIZE) return -EINVAL;

  if (down_interruptible(&sem)) return -ERESTARTSYS;

  pr_info("%s: write to char device!\n", DEVNAME);
  rc = simple_write_to_buffer(mbuf, SIZE, off, buf, size);

  if (rc < 0) return rc;

  up(&sem);
  return rc;
}

void free_resources() {
  if (mbuf != NULL) kfree(mbuf);

  if (cdev != NULL) cdev_del(cdev);

  if (class != NULL && major != 0) {
    device_destroy(class, major);
    class_destroy(class);
    unregister_chrdev_region(major, 1);
  }
}

module_init(chardev_init);
module_exit(chardev_exit);
