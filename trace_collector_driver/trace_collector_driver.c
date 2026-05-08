#include <linux/init.h>
#include <linux/module.h>
#include <linux/fs.h>
#include <linux/kernel.h>
#include <linux/cdev.h>
#include <linux/device.h>
#include <linux/uaccess.h>
#include <linux/mutex.h>
#include <linux/slab.h>
#include <linux/list.h>

#include "cpu_uapi.h"

#define DEVICE_NAME "trace_collector"
#define CLASS_NAME  "trace_collector_class"

#define CHUNK_SIZE 256

typedef struct trace_chunk
{
    mem_access entries[CHUNK_SIZE];
    size_t count;
    struct list_head list;
} trace_chunk;

typedef struct trace_buffer
{
    struct list_head chunks;
    size_t total_count;

    trace_chunk *write_chunk; // current chunk being written to
    trace_chunk *read_chunk;  // current chunk being read from
    size_t read_offset;       // offset within read_chunk

    struct mutex lock;
} trace_buffer;

static trace_buffer g_buffer;

static void trace_buffer_init(trace_buffer *tb)
{
    INIT_LIST_HEAD(&tb->chunks);
    tb->total_count = 0;
    tb->write_chunk = NULL;
    tb->read_chunk  = NULL;
    tb->read_offset = 0;

    mutex_init(&tb->lock);
}

static void trace_buffer_free(trace_buffer *tb)
{
    trace_chunk *chunk, *tmp;

    list_for_each_entry_safe(chunk, tmp, &tb->chunks, list)
    {
        list_del(&chunk->list);
        kfree(chunk);
    }

    tb->total_count = 0;
    tb->write_chunk = NULL;
    tb->read_chunk  = NULL;
    tb->read_offset = 0;
}

// Must be called with lock held
static int trace_buffer_push(trace_buffer *tb, const mem_access *entry)
{
    // If no chunk exists or current write chunk is full, allocate a new one
    if (!tb->write_chunk || tb->write_chunk->count == CHUNK_SIZE)
    {
        trace_chunk *new_chunk = kmalloc(sizeof(trace_chunk), GFP_KERNEL);
        if (!new_chunk)
            return -ENOMEM;

        new_chunk->count = 0;
        INIT_LIST_HEAD(&new_chunk->list);
        list_add_tail(&new_chunk->list, &tb->chunks);

        tb->write_chunk = new_chunk;

        // First chunk ever — init read pointer here
        if (!tb->read_chunk)
        {
            tb->read_chunk  = new_chunk;
            tb->read_offset = 0;
        }
    }

    tb->write_chunk->entries[tb->write_chunk->count++] = *entry;
    tb->total_count++;

    return 0;
}

static dev_t dev_num;
static struct cdev trace_cdev;
static struct class *trace_class;

static ssize_t trace_write(struct file *file, const char __user *buf, size_t len, loff_t *ppos)
{
    if (len < sizeof(mem_access))
        return -EINVAL;

    mem_access entry;
    if (copy_from_user(&entry, buf, sizeof(mem_access)))
        return -EFAULT;

    mutex_lock(&g_buffer.lock);
    int ret = trace_buffer_push(&g_buffer, &entry);
    mutex_unlock(&g_buffer.lock);

    if (ret)
        return ret;

    return sizeof(mem_access);
}

static ssize_t trace_read(struct file *file, char __user *buf, size_t len, loff_t *ppos)
{
    if (len < sizeof(mem_access))
        return -EINVAL;

    size_t max_entries = len / sizeof(mem_access);
    size_t copied = 0;

    mutex_lock(&g_buffer.lock);

    while (copied < max_entries)
    {
        // No read chunk means buffer is empty
        if (!g_buffer.read_chunk)
            break;

        // Current read chunk exhausted — move to next
        if (g_buffer.read_offset >= g_buffer.read_chunk->count)
        {
            struct list_head *next = g_buffer.read_chunk->list.next;

            if (next == &g_buffer.chunks)
                break; // No more chunks, EOF

            g_buffer.read_chunk  = list_entry(next, trace_chunk, list);
            g_buffer.read_offset = 0;
        }

        mem_access *entry = &g_buffer.read_chunk->entries[g_buffer.read_offset];

        if (copy_to_user(buf + copied * sizeof(mem_access), entry, sizeof(mem_access)))
        {
            mutex_unlock(&g_buffer.lock);
            return copied > 0 ? copied * sizeof(mem_access) : -EFAULT;
        }

        g_buffer.read_offset++;
        copied++;
    }

    mutex_unlock(&g_buffer.lock);

    return copied * sizeof(mem_access);
}

static const struct file_operations fops = {
    .read    = trace_read,
    .write   = trace_write,
};

static char *trace_devnode(const struct device *dev, umode_t *mode)
{
    if (mode)
        *mode = 0666;
    return NULL;
}

static int __init trace_init(void)
{
    int ret;

    trace_buffer_init(&g_buffer);

    ret = alloc_chrdev_region(&dev_num, 0, 1, DEVICE_NAME);
    if (ret < 0)
        return ret;

    cdev_init(&trace_cdev, &fops);
    trace_cdev.owner = THIS_MODULE;

    ret = cdev_add(&trace_cdev, dev_num, 1);
    if (ret < 0)
        goto free_dev_num;

    trace_class = class_create(CLASS_NAME);
    if (IS_ERR(trace_class))
    {
        ret = PTR_ERR(trace_class);
        goto delete_cdev;
    }
    trace_class->devnode = trace_devnode;

    if (!device_create(trace_class, NULL, dev_num, NULL, DEVICE_NAME))
    {
        ret = -ENOMEM;
        goto delete_class;
    }

    pr_info("trace_collector: loaded\n");
    return 0;

delete_class:
    class_unregister(trace_class);
    class_destroy(trace_class);
delete_cdev:
    cdev_del(&trace_cdev);
free_dev_num:
    unregister_chrdev_region(dev_num, 1);
    return ret;
}

static void __exit trace_exit(void)
{
    device_destroy(trace_class, dev_num);
    class_unregister(trace_class);
    class_destroy(trace_class);
    cdev_del(&trace_cdev);
    unregister_chrdev_region(dev_num, 1);

    trace_buffer_free(&g_buffer);

    pr_info("trace_collector: unloaded\n");
}

module_init(trace_init);
module_exit(trace_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Filip");
MODULE_DESCRIPTION("Memory access trace collector driver");