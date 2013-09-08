#include <linux/fs.h>
#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/proc_fs.h>
#include <asm/uaccess.h>	/* for copy_from_user */

#define MAGIC_WORD_UNLOCK	"unlock_john"
#define MAGIC_WORD_LOCK		"lock_john"

extern int debug_mode_enable;


static int proc_diag_lock_read(struct file *file, char *buf, size_t size, loff_t *ppos);
static int proc_diag_lock_write(struct file *file, const char *buf, size_t size, loff_t *ppos);

struct file_operations proc_diag_lock_fops = {
	read:proc_diag_lock_read,
	write:proc_diag_lock_write,
};

static int proc_diag_lock_read(struct file *file, char *buf, size_t size, loff_t *ppos)
{
	pr_info("%s debug_mode_enable %d \n",__func__,debug_mode_enable);
	return  0;
}


static int proc_diag_lock_write(struct file *file, const char *buf, size_t size, loff_t *ppos)
{
	char buff[64];
	int ret = -EFAULT;

	memset(buff, 0x00, sizeof(buff));
	/* write data to the buffer */
	if ( copy_from_user(buff, buf, size) ) {
		pr_err("COPY_FROM_USER ERR!\n");
		return ret;
	}

	if(strncmp(buff, MAGIC_WORD_LOCK, sizeof(MAGIC_WORD_LOCK)-1) == 0){
		//Lock
		debug_mode_enable = 0;
		ret = sizeof(MAGIC_WORD_LOCK);
	}else if(strncmp(buff, MAGIC_WORD_UNLOCK, sizeof(MAGIC_WORD_UNLOCK)-1) == 0){
		//Unlock
		debug_mode_enable = 1;
		ret = sizeof(MAGIC_WORD_UNLOCK);
	}

	pr_info("%s debug_mode_enable %d \n",__func__,debug_mode_enable);
	return ret;
}



static int __init init_diag_lock(void)
{
	struct proc_dir_entry   *p;
	p = create_proc_entry ("diag_lock", S_IFREG | S_IRUGO | S_IWUGO, NULL);
	if(p)
		p->proc_fops = &proc_diag_lock_fops;
	return 0;
}

static void __exit cleanup_diag_lock(void)
{
	pr_debug("cleanup testdebug\n");
	remove_proc_entry ("diag_lock", NULL);
}

module_init(init_diag_lock);
module_exit(cleanup_diag_lock);

MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("For diag lock/unlock");
