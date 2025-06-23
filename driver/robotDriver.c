#include <linux/init.h>
#include <linux/module.h>
#include <linux/fs.h>
#include <linux/uaccess.h>
#include <linux/device.h>

#define DEVICE_NAME "robot"
#define CLASS_NAME "robot_class"

static int major;
static struct class *robot_class = NULL;

// Buffer para guardar el último mensaje recibido
static char last_msg[128] = {0};
static size_t last_len = 0;

static int robot_open(struct inode *inode, struct file *file) {
    printk(KERN_INFO "robot: dispositivo abierto\n");
    return 0;
}

static int robot_release(struct inode *inode, struct file *file) {
    printk(KERN_INFO "robot: dispositivo cerrado\n");
    return 0;
}

static ssize_t robot_write(struct file *file, const char __user *buf, size_t len, loff_t *offset) {
    if (len > 127) len = 127;

    if (copy_from_user(last_msg, buf, len) != 0)
        return -EFAULT;

    last_msg[len] = '\0';
    last_len = len;

    printk(KERN_INFO "robot: recibido -> %s", last_msg);
    return len;
}

static ssize_t robot_read(struct file *file, char __user *buf, size_t len, loff_t *offset) {
    if (*offset > 0 || last_len == 0)
        return 0;

    if (len > last_len)
        len = last_len;

    if (copy_to_user(buf, last_msg, len) != 0)
        return -EFAULT;

    *offset = len;
    return len;
}

static struct file_operations fops = {
    .owner = THIS_MODULE,
    .open = robot_open,
    .release = robot_release,
    .write = robot_write,
    .read = robot_read
};

static int __init robot_init(void) {
    major = register_chrdev(0, DEVICE_NAME, &fops);
    if (major < 0) {
        printk(KERN_ALERT "robot: fallo al registrar major\n");
        return major;
    }

    robot_class = class_create(CLASS_NAME);  // Para kernel 6.4+
    if (IS_ERR(robot_class)) {
        unregister_chrdev(major, DEVICE_NAME);
        printk(KERN_ALERT "robot: fallo al crear clase\n");
        return PTR_ERR(robot_class);
    }

    device_create(robot_class, NULL, MKDEV(major, 0), NULL, DEVICE_NAME);
    printk(KERN_INFO "robot: módulo cargado correctamente\n");
    return 0;
}

static void __exit robot_exit(void) {
    device_destroy(robot_class, MKDEV(major, 0));
    class_unregister(robot_class);
    class_destroy(robot_class);
    unregister_chrdev(major, DEVICE_NAME);
    printk(KERN_INFO "robot: módulo descargado\n");
}

module_init(robot_init);
module_exit(robot_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("TuNombre");
MODULE_DESCRIPTION("Driver char para RoboticTEC");
MODULE_VERSION("0.1");
