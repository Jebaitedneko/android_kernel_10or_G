#include <linux/init.h>
#include <linux/module.h>
#include <linux/ioctl.h>
#include <linux/fs.h>
#include <linux/device.h>
#include <linux/platform_device.h>
#include <linux/err.h>
#include <linux/list.h>
#include <linux/errno.h>
#include <linux/mutex.h>
#include <linux/slab.h>
#include <linux/ctype.h>
#include <linux/compat.h>
#include <linux/of.h>
#include <linux/of_device.h>
#include <linux/mm.h>
#include <linux/vmalloc.h>
#include <linux/workqueue.h>
#include <linux/delay.h>
#include <linux/gpio.h>
#include <linux/of_gpio.h>
#include <linux/interrupt.h>
#include <linux/seq_file.h>

//add by silead for fp_wake_lock
#include <linux/kobject.h>
#include <linux/debugfs.h>
#include <../kernel/power/power.h>
//add by silead for fp_wake_lock
#include <linux/jiffies.h>
#include <linux/spi/spi.h>
#include <linux/timex.h>
#include <linux/timer.h>
#include <linux/wakelock.h>
#include <asm/uaccess.h>

#include <linux/sched.h>
#include <linux/cdev.h>
//#include "../fp_info.h"
#include "slspi.h"
#include <linux/regulator/consumer.h>

/* added by luochuan for fingerprint hardinfo 20170407 begin */
#ifdef CONFIG_HQ_SYSFS_SUPPORT
#include <linux/hqsysfs.h>
#endif
/* added by luochuan for fingerprint hardinfo 20170407 end */

#define VERSION_LOG	"Silead fingerprint drvier V0.1"

#define N_SPI_MINORS			32	/* ... up to 256 */

/*shutdown active/suspend */
#define PINCTRL_STATE_ACTIVE    "active"
#define PINCTRL_STATE_SUSPEND   "suspend"
/*shutdown active/suspend */

#ifdef GSL6313_INTERRUPT_CTRL
#define PINCTRL_STATE_INTERRUPT "irq_active" 
#endif

/* added by luochuan for fingerprint hardinfo 20170407 begin */
#ifdef CONFIG_HQ_SYSFS_SUPPORT
static char device_name_pirntfinger[30] = {"Silead GSL6153"};
#endif
/* added by luochuan for fingerprint hardinfo 20170407 end */

static DECLARE_BITMAP(minors, N_SPI_MINORS);
static int irq_counter = 0;

static struct spidev_data	*fp_spidev = NULL;
static unsigned int spidev_major = 0;
struct cdev spicdev;

//add by silead for fp_wake_lock
static ssize_t fp_wake_lock_store(struct kobject *kobj, struct kobj_attribute *attr, const char *buf, size_t size);
static ssize_t fp_wake_unlock_store(struct kobject *kobj, struct kobj_attribute *attr,const char *buf, size_t size);
static ssize_t fp_wake_lock_show(struct kobject *kobj,struct kobj_attribute *attr,char *buf);
static ssize_t fp_wake_unlock_show(struct kobject *kobj,struct kobj_attribute *attr,char *buf);

/*static void hw_power_enable(struct platform_device *spi, u8 onoff)
{
	static int enable = 1;
    int ret = -1;
    int gpio_pwr;
	if (onoff && enable) {
        gpio_pwr = of_get_named_gpio_flags(spi->dev.of_node, "silead,gpio_pwr", 0,NULL);
        ret = gpio_direction_output(gpio_pwr,1);
        DBG_MSG(MSG_ERR, "silead_fp_probe gpio_pwr= %d,ret = %d\n", gpio_pwr,ret);
		enable = 0;
	}
	else if (!onoff && !enable) {
        gpio_pwr = of_get_named_gpio_flags(spi->dev.of_node, "silead,gpio_pwr", 0,NULL);
        ret = gpio_direction_output(gpio_pwr,0);
        DBG_MSG(MSG_ERR, "silead_fp_probe gpio_pwr= %d,ret = %d\n", gpio_pwr,ret);
		enable = 1;
	}
}
*/
//add by silead for fp_wake_lock
//add by silead for fp_wake_lock
#define silead_attr(_name) \
static struct kobj_attribute _name##_attr = {    \
    .attr    = {                       \
        .name = __stringify(_name),    \
        .mode = 0666,                  \
    },                                 \
    .show    = _name##_show,           \
    .store    = _name##_store,         \
}

silead_attr(fp_wake_lock);
silead_attr(fp_wake_unlock);

static struct attribute * g[] = {
    &fp_wake_lock_attr.attr,
    &fp_wake_unlock_attr.attr,
    NULL,
};

static struct attribute_group attr_group = {
    .attrs = g,
};

struct wakelock {
    char                    *name;
    struct wakeup_source    ws;
};

static struct wakelock * g_wakelock_list[10] = {
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0
};

static DEFINE_MUTEX(wakelocks_lock);

static ssize_t fp_wake_lock_show(struct kobject *kobj,
                              struct kobj_attribute *attr,
                              char *buf) {
    // return pm_show_wakelocks(buf, true);

    char *str = buf;
    char *end = buf + PAGE_SIZE;
    int i;

    mutex_lock(&wakelocks_lock);

    for (i = 0; i < 10; i++) {
        if (g_wakelock_list[i] != NULL) {
            str += scnprintf(str, end - str, "%s ", g_wakelock_list[i]->name);
        }
    }
    if (str > buf) {
        str--;
    }

    str += scnprintf(str, end - str, "\n");
    mutex_unlock(&wakelocks_lock);
    return (str - buf);
}

static ssize_t fp_wake_lock_store(struct kobject *kobj,
                               struct kobj_attribute *attr,
                               const char *buf, size_t size) {
    int i, j;
    int ret= -1;
    char * wl_name;
    struct wakelock *wl;

    wl_name = kstrndup(buf, size, GFP_KERNEL);
    if (!wl_name) {
        return -ENOMEM;
    }

    mutex_lock(&wakelocks_lock);
    for (j = 0; j < 10; j++) {
        if (g_wakelock_list[j] != NULL) {
            if (strcmp(g_wakelock_list[j]->name, buf) == 0) {
                wl = g_wakelock_list[j];
                ret = size;
                break;
            }
        }
    }

    if (j == 10) {
        wl = kzalloc(sizeof(*wl), GFP_KERNEL);
        if (!wl) {
            return -ENOMEM;
        }
        wl->name = wl_name;
        wl->ws.name = wl_name;
        wakeup_source_add(&wl->ws);

        for (i = 0; i < 10; i++) {
            if (g_wakelock_list[i] == NULL) {
                g_wakelock_list[i] = wl;
                ret = size;
                break;
            }
        }
    }

    __pm_stay_awake(&wl->ws);
    mutex_unlock(&wakelocks_lock);

    return ret;
}

static ssize_t fp_wake_unlock_show(struct kobject *kobj,
                                struct kobj_attribute *attr,
                                char *buf) {
    //return pm_show_wakelocks(buf, fasle);

    char *str = buf;
    char *end = buf + PAGE_SIZE;
    int i ;
    mutex_lock(&wakelocks_lock);

    for (i = 0; i < 10; i++) {
        if(g_wakelock_list[i] != NULL) {
            str += scnprintf(str, end - str, "%s ", g_wakelock_list[i]->name);
        }
    }
    if (str > buf) {
        str--;
    }
    str += scnprintf(str, end - str, "\n");

    mutex_unlock(&wakelocks_lock);
    return (str - buf);
}

static ssize_t fp_wake_unlock_store(struct kobject *kobj,
                                 struct kobj_attribute *attr,
                                 const char *buf, size_t size) {
    struct wakelock *wl;
    int ret = -1;
    int i;
    mutex_lock(&wakelocks_lock);

    for (i = 0; i < 10; i++) {
        if (g_wakelock_list[i] != NULL) {
            if (strcmp(g_wakelock_list[i]->name, buf) == 0) {
                wl = g_wakelock_list[i];
                __pm_relax(&wl->ws);
                wakeup_source_remove(&wl->ws);
                kfree(wl->name);
                kfree(wl);
                g_wakelock_list[i] = NULL;
                ret = size;
                break;
            }
        }
    }

    mutex_unlock(&wakelocks_lock);
    printk("fp_wake_unlock_store ret = %d\n", ret);
    return ret;
}
//add by silead for fp_wake_unlock

static int spidev_reset_hw(struct spidev_data *spidev)

{
	int ret = -1;

	DBG_MSG(MSG_TRK, "S\n");
	
	if(spidev->fp_pinctrl)
	{	
		ret = pinctrl_select_state(spidev->fp_pinctrl, spidev->pinctrl_state_suspend);
		if(ret) {
			DBG_MSG(MSG_ERR, "cannot get suspend pinctrl state\n");
		}
	}

    mdelay(1);

    //pull gpio output high;
	if(spidev->fp_pinctrl)
	{
		ret = pinctrl_select_state(spidev->fp_pinctrl, spidev->pinctrl_state_active);
		if(ret) {
			DBG_MSG(MSG_ERR, "cannot get active pinctrl state\n");
		}
	}
    //mdelay(5);
	DBG_MSG(MSG_INFO, "E\n");
	return ret;
}

static int spidev_shutdown_hw(struct spidev_data *spidev)
{
	int ret = -1,value=-1;

	if(spidev->fp_pinctrl)
	{	
		ret = pinctrl_select_state(spidev->fp_pinctrl, spidev->pinctrl_state_suspend);
		if(ret) {
			DBG_MSG(MSG_ERR, "cannot get suspend pinctrl state\n");
		}
	}
    mdelay(5);
	value = gpio_get_value_cansleep(spidev->shutdown_gpio);
	
	DBG_MSG(MSG_TRK, "GPIO %d state is %d\n", spidev->shutdown_gpio,value);
	return ret;
}

/*shutdown active/suspend */
static int silead_fp_pinctrl_init(struct spidev_data *data)
{
    int ret;
   
    /* Get pinctrl if target uses pinctrl */
    data->fp_pinctrl = devm_pinctrl_get(&data->spi->dev);
    
    if (IS_ERR_OR_NULL(data->fp_pinctrl)) {
        ret = PTR_ERR(data->fp_pinctrl);
		
		DBG_MSG(MSG_ERR, "Target does not use pinctrl %d\n", ret);

        goto err_pinctrl_get;
    }
    printk("Target use pinctrl\n");
    data->pinctrl_state_active
        = pinctrl_lookup_state(data->fp_pinctrl,
                PINCTRL_STATE_ACTIVE);
    if (IS_ERR_OR_NULL(data->pinctrl_state_active)) {
        ret = PTR_ERR(data->pinctrl_state_active);

		DBG_MSG(MSG_ERR, "Can not lookup %s pinstate %d\n", PINCTRL_STATE_ACTIVE, ret);
            
        goto err_pinctrl_lookup;
    }

    data->pinctrl_state_suspend
        = pinctrl_lookup_state(data->fp_pinctrl,
            PINCTRL_STATE_SUSPEND);
    if (IS_ERR_OR_NULL(data->pinctrl_state_suspend)) {
        ret = PTR_ERR(data->pinctrl_state_suspend);

		DBG_MSG(MSG_ERR, "Can not lookup %s pinstate %d\n", PINCTRL_STATE_SUSPEND, ret);
            
        goto err_pinctrl_lookup;
    }

/*IRQ wake-up control */
#ifdef GSL6313_INTERRUPT_CTRL
    data->pinctrl_state_interrupt
        = pinctrl_lookup_state(data->fp_pinctrl,
            PINCTRL_STATE_INTERRUPT);
    if (IS_ERR_OR_NULL(data->pinctrl_state_interrupt)) {
        ret = PTR_ERR(data->pinctrl_state_interrupt);

		DBG_MSG(MSG_ERR, "Can not lookup %s pinstate %d\n", PINCTRL_STATE_INTERRUPT, ret);

        goto err_pinctrl_lookup;
    }
#endif

	DBG_MSG(MSG_INFO, "E\n");
    return 0;

err_pinctrl_lookup:
    devm_pinctrl_put(data->fp_pinctrl);
err_pinctrl_get:
    data->fp_pinctrl = NULL;
    return ret;
}

/*shutdown active/suspend */
/*IRQ wake-up control */
/*--------------------------------------------------------------------------
 * work function
 *--------------------------------------------------------------------------*/
#ifdef GSL6313_INTERRUPT_CTRL
static void finger_interrupt_work(struct work_struct *work)
{
	struct spidev_data *spidev = container_of(work, struct spidev_data, int_work);

	char*   env_ext[2] = {"SILEAD_FP_EVENT=IRQ", NULL};
	
	DBG_MSG(MSG_TRK, "irq bottom half spidev_irq_work enter \n");

	kobject_uevent_env(&spidev->spi->dev.kobj, KOBJ_CHANGE, env_ext ); 
}

static irqreturn_t finger_interrupt_handler(int irq, void *dev)
{
	int value;
    struct timex txc;
	struct spidev_data *spidev = dev;
    do_gettimeofday(&(txc.time));
	
	DBG_MSG(MSG_TRK, "txc.time.tv_sec=%ld,txc.time.tv_usec=%ld \n",txc.time.tv_sec,txc.time.tv_usec);
	DBG_MSG(MSG_TRK, "S interrupt top half has entered!\n");
	
    wake_lock_timeout(&spidev->wake_lock, 10*HZ);
    irq_counter--;
	value = gpio_get_value_cansleep(spidev->int_wakeup_gpio);

	DBG_MSG(MSG_TRK, "S IRQ %d , irq_counter is %d GPIO %d state is %d\n", 
					irq, irq_counter,spidev->int_wakeup_gpio,value);
	DBG_MSG(MSG_TRK, "state is %d\n", value);
	
	queue_work(spidev->int_wq,&spidev->int_work);
	return IRQ_HANDLED;
}
#endif

static struct class *spidev_class;

static int silead_fp_probe(struct platform_device *spi)
{	
	struct spidev_data	*spidev;
	int status;
       int error;
	unsigned long minor;
	//int id_val = -1;
#ifdef GSL6313_INTERRUPT_CTRL
    int 	irq_flags; 	/*IRQ wake-up control */
    unsigned int tmp;
#endif
    	printk("silead_fp_probe str\n");
	/* Allocate driver data */
	spidev = kzalloc(sizeof(*spidev), GFP_KERNEL);
	if (!spidev)
		return -ENOMEM;

    /* Initialize the driver data */
    spidev->spi = spi;
	   
	error = silead_fp_pinctrl_init(spidev);
	
	DBG_MSG(MSG_INFO, "S1\n");
	
    if(!error && spidev->fp_pinctrl){
        /*
         * Pinctrl handle is optional. If pinctrl handle is found
         * let pins to be configured in active state. If not
         * found continue further without error.
         */
		spidev_reset_hw(spidev); 

		spin_lock_init(&spidev->spi_lock);
		mutex_init(&spidev->buf_lock);
		wake_lock_init(&spidev->wake_lock, WAKE_LOCK_SUSPEND, "silead_wake_lock");

		spidev->int_wq= create_singlethread_workqueue("int_silead_wq");
		INIT_WORK(&spidev->int_work, finger_interrupt_work);
		
	/*IRQ wake-up control */
#ifdef GSL6313_INTERRUPT_CTRL
        error = pinctrl_select_state(spidev->fp_pinctrl,
                    spidev->pinctrl_state_interrupt);

        if (error < 0) {
			DBG_MSG(MSG_ERR, "failed to select pin to interrupt state\n");
        }
#endif
    }
    //hw_power_enable(spi,1);
    //mdelay(5);
	/*spidev->gpio_id = of_get_named_gpio(spi->dev.of_node,"silead,gpio_id",0);
	if (gpio_is_valid(spidev->gpio_id)) {
		status = gpio_request(spidev->gpio_id,
				"silead_gpio_id");
		if (status) {
			DBG_MSG(MSG_ERR, "gpio_id request failed\n");
		} else {
			DBG_MSG(MSG_ERR, "gpio_id request success\n");
		}
	}
	id_val = gpio_get_value(spidev->gpio_id);
	printk( "[silead silead_fp_probe] id_val = %d \n",id_val );
	if ( 1 == id_val)
	{
		printk( "is not silead fingerprint IC \n");
		gpio_free(spidev->gpio_id);
		gpio_free(47);
		return status;
	}*/
	spidev->shutdown_gpio = of_get_named_gpio(spi->dev.of_node,"silead,shutdown_gpio",0);

	if (gpio_is_valid(spidev->shutdown_gpio)) {
		status = gpio_request(spidev->shutdown_gpio,
				"silead_shutdown_gpio");
		if (status) {
			DBG_MSG(MSG_ERR, "reset gpio request failed\n");
			gpio_free(spidev->shutdown_gpio);
		} else {
			DBG_MSG(MSG_ERR, "reset gpio request success\n");
		}
	}

	irq_flags = IRQF_TRIGGER_RISING | IRQF_ONESHOT;

	spidev->int_wakeup_gpio = of_get_named_gpio_flags(spi->dev.of_node,"silead,gpio_int",0,&tmp);

	if (gpio_is_valid(spidev->int_wakeup_gpio)) {
		status = gpio_request_one(spidev->int_wakeup_gpio, (GPIOF_DIR_IN | GPIOF_INIT_LOW),
				"silead_int_gpio");
		if (status) {			
			DBG_MSG(MSG_ERR, "int gpio request failed\n");
			gpio_free(spidev->int_wakeup_gpio);
		} else {
			DBG_MSG(MSG_ERR, "int gpio request success\n"); 
		}
	}

	spidev->int_irq= gpio_to_irq(spidev->int_wakeup_gpio);
	status = devm_request_threaded_irq(&spi->dev, spidev->int_irq, NULL,
			finger_interrupt_handler,
			irq_flags, "silead_finger", spidev);
	if (status < 0) {
		DBG_MSG(MSG_ERR, "request irq failed : %d\n", spidev->int_irq);
	}
	
	enable_irq_wake(spidev->int_irq);
    irq_counter++;
	
	DBG_MSG(MSG_INFO, "%s  Interrupt  %d  wake up is %d" 
					"irq flag is 0x%X irq_counter is %d\n", 
					__func__,spidev->int_irq, spidev->wakeup,irq_flags,irq_counter);

	
	DBG_MSG(MSG_INFO, "S2\n");

	minor = find_first_zero_bit(minors, N_SPI_MINORS);
	if (minor < N_SPI_MINORS) {
		struct device *dev;

		spidev->devt = MKDEV(spidev_major, minor);
		dev = device_create(spidev_class, &spi->dev, spidev->devt,
				spidev, "silead_fp_dev");
		status = IS_ERR(dev) ? PTR_ERR(dev) : 0;
	} else {
		DBG_MSG(MSG_ERR, "no minor number available!\n");
		status = -ENODEV;
	}
	if (status == 0) {
		set_bit(minor, minors);
	}

	if (status == 0){
		platform_set_drvdata(spi, spidev);
		fp_spidev = spidev;
		//init_fp_fm_info(1,version_info_str,"silead");
	}
	else
		kfree(spidev);

    /* added by luochuan for fingerprint hardinfo 20170407 begin */
#ifdef CONFIG_HQ_SYSFS_SUPPORT
    hq_regiser_hw_info(HWID_FINGERPRINT, device_name_pirntfinger);
#endif
    /* added by luochuan for fingerprint hardinfo 20170407 end */

	return status;
}

static int silead_fp_remove(struct platform_device *spi)
{
	struct spidev_data *spidev = platform_get_drvdata(spi);

	/* make sure ops on existing fds can abort cleanly */
	spin_lock_irq(&spidev->spi_lock);
	spidev->spi = NULL;
	platform_set_drvdata(spi, NULL);
	spin_unlock_irq(&spidev->spi_lock);

	return 0;
}

#ifdef CONFIG_PM_SLEEP
static int silead_fp_suspend(struct device *dev)
{
	DBG_MSG(MSG_INFO, "silead_fp suspend!\n");
	return 0;
}

static int silead_fp_resume(struct device *dev)
{
	DBG_MSG(MSG_INFO, "silead_fp resume!\n");
	return 0;
}
#endif

static SIMPLE_DEV_PM_OPS(silead_fp_pm_ops, silead_fp_suspend, silead_fp_resume);

#ifdef CONFIG_OF
static struct of_device_id silead_of_match[] = {
	{ .compatible = "silead,silead_fp",},
	{ },
};
#else
#define silead_of_match NULL
#endif

static struct platform_driver silead_fp_driver = {
	.driver = {
		.name 	= "silead_fp",
		.owner = THIS_MODULE,
		.pm 	= &silead_fp_pm_ops,
		.of_match_table = silead_of_match,
	},
	.probe 	= silead_fp_probe,
	.remove = silead_fp_remove,
};

/* Write-only message with current device setup */
static ssize_t spidev_write(struct file *filp, const char __user *buf,
		size_t count, loff_t *f_pos)
{
	struct spidev_data	*spidev;

	spidev = (struct spidev_data *)filp->private_data;

	return 0;
}

	/* Read-only message with current device setup */
static ssize_t spidev_read(struct file *filp, char __user *buf, size_t count, loff_t *f_pos)
{
	struct spidev_data	*spidev;

	spidev = (struct spidev_data *)filp->private_data;

	return 0;
}

static long spidev_ioctl(struct file *filp, unsigned int cmd, unsigned long arg)
{
	struct spidev_data	*spidev;

	/* Check type and command number */
	if (_IOC_TYPE(cmd) != SPI_IOC_MAGIC)
		return -ENOTTY;

	spidev = (struct spidev_data *)filp->private_data;
    //printk(KERN_WARNING "thomas for debug %s : %d ,spidev= %p \n",__func__,__LINE__,spidev);
	mutex_lock(&spidev->buf_lock);

	switch (cmd) {
		case SPI_HW_RESET :
			DBG_MSG(MSG_INFO, "SPI_HW_RESET called\n");
			spidev_reset_hw(spidev);
        	break;
			 
	    case SPI_HW_SHUTDOWN:
			DBG_MSG(MSG_INFO, "SPI_HW_SHUTDOWN called\n");
			spidev_shutdown_hw(spidev);
			break; 
			
		case SPI_OPEN_CLOCK:
			DBG_MSG(MSG_INFO, "SPI_OPEN_CLOCK called\n");
			break;
	    case SPI_HW_POWEROFF:
	        //hw_power_enable(spidev->spi,0);
	        mdelay(5);
	        DBG_MSG(MSG_INFO, "SPI_HW_POWEROFF is OK\n");
			break;
	    case SPI_HW_POWERON:
	        //hw_power_enable(spidev->spi,1);
	        mdelay(5);                               
	        DBG_MSG(MSG_INFO, "SPI_HW_POWERON is OK\n");
			break;
			
		case SPI_HW_IRQ_ENBALE:
			DBG_MSG(MSG_INFO, "SPI_HW_IRQ_ENBALE called\n");
			if (!irq_counter)
			{
				enable_irq(spidev->int_irq);
				irq_counter++;
				DBG_MSG(MSG_INFO, "enable_irq, irq_counter is %d\n",irq_counter);
			}

			break;

		default:
			break;
	}

	mutex_unlock(&spidev->buf_lock);
	return 0;
}

#ifdef CONFIG_COMPAT
static long spidev_compat_ioctl(struct file *filp, unsigned int cmd, unsigned long arg)
{
	return spidev_ioctl(filp, cmd, (unsigned long)compat_ptr(arg));
}
#else
#define spidev_compat_ioctl NULL
#endif /* CONFIG_COMPAT */

static int spidev_open(struct inode *inode, struct file *filp)
{
	filp->private_data = fp_spidev;
	return 0;
}
	
static int spidev_release(struct inode *inode, struct file *filp)
{
	filp->private_data = NULL;
	return 0;
}

static const struct file_operations spidev_fops = {
	.owner =	THIS_MODULE,
	/* REVISIT switch to aio primitives, so that userspace
	 * gets more complete API coverage.  It'll simplify things
	 * too, except for the locking.
	 */
	.write =	spidev_write,
	.read =		spidev_read,
	.unlocked_ioctl = spidev_ioctl,
	.compat_ioctl = spidev_compat_ioctl,
	.open =		spidev_open,
	.release =	spidev_release,
};
static struct of_device_id fp_id_match_table[] = {
	{ .compatible = "fp_id,fp_id",},
	{},
};
MODULE_DEVICE_TABLE(of, fp_id_match_table);

static int __init silead_fp_init(void)
{
	int status = 0;
	int error;
	dev_t devno;
       struct kobject *power_kobj;	
	
	struct device_node *fp_id_np = NULL;
	int fp_id_gpio = 0, fp_id_gpio_value = 0, fp_id_pwr = 0;
	
       DBG_MSG(MSG_INFO, "silead_fp_init S\n");
	fp_id_np = of_find_matching_node(fp_id_np, fp_id_match_table);
	DBG_MSG(MSG_INFO,"%s: start\n",__func__);
	if (fp_id_np) {
		fp_id_gpio = of_get_named_gpio(fp_id_np, "silead,gpio_fp_id", 0);
             fp_id_pwr = of_get_named_gpio(fp_id_np, "silead,gpio_pwr_id", 0);
		DBG_MSG(MSG_INFO,"%s: fp_id_gpio=%d\n",__func__, fp_id_gpio);
	}

	if (fp_id_gpio < 0 || fp_id_pwr < 0) {
		
		return status;
	} 
    	gpio_direction_output(fp_id_pwr,1);
	mdelay(5);
	gpio_direction_input(fp_id_gpio);
	fp_id_gpio_value = gpio_get_value(fp_id_gpio);
	DBG_MSG(MSG_INFO,"%s: fp_id_gpio_value=%d\n",__func__, fp_id_gpio_value);
	if(fp_id_gpio_value == 0){
		DBG_MSG(MSG_INFO,"%s:  need to load silead driver \n",__func__);
	}else{
        return status;
	}

	/* Claim our 256 reserved device numbers.  Then register a class
	 * that will key udev/mdev to add/remove /dev nodes.  Last, register
	 * the driver which manages those device numbers.
	 */
	BUILD_BUG_ON(N_SPI_MINORS > 256);

	status = alloc_chrdev_region(&devno, 0,255, "sileadfp");
	if(status <0 ) {
		goto exit;
	}

	spidev_major = MAJOR(devno);
	cdev_init(&spicdev, &spidev_fops);
	spicdev.owner = THIS_MODULE;
	status = cdev_add(&spicdev,MKDEV(spidev_major, 0),N_SPI_MINORS);
	if(status != 0) {
		goto exit;
	}

	spidev_class = class_create(THIS_MODULE, "spidev");
	if (IS_ERR(spidev_class)) {
		unregister_chrdev(spidev_major, silead_fp_driver.driver.name);
		status =  PTR_ERR(spidev_class);
		goto exit;
	}

    DBG_MSG(MSG_INFO, "platform_driver_register S-----\n");
	
	status = platform_driver_register(&silead_fp_driver);
	
	if (status < 0) {
        	DBG_MSG(MSG_INFO, "platform_driver_register Error\n");
	  class_destroy(spidev_class);
	  unregister_chrdev(spidev_major, silead_fp_driver.driver.name);
	  goto exit;
	}
	
    //add by silead for fp_wake_lock
    power_kobj = kobject_create_and_add("silead", NULL);
    if (!power_kobj) {
        return -ENOMEM;
    }
    error = sysfs_create_group(power_kobj, &attr_group);
    if (error) {
        return error;
    }
    //add by silead for fp_wake_lock
	DBG_MSG(MSG_INFO, "silead_fp_init E\n");

exit:
	return status;
}

static void __exit silead_fp_exist(void)
{	
	cdev_del(&spicdev);
	platform_driver_unregister(&silead_fp_driver);
	class_destroy(spidev_class);
	unregister_chrdev(spidev_major, silead_fp_driver.driver.name);
}

module_init(silead_fp_init);
module_exit(silead_fp_exist);

MODULE_AUTHOR("EricLiu <ericclliu@fih-foxconn.com>");
MODULE_DESCRIPTION("driver to control Silead fingerprint module");
MODULE_VERSION(VERSION_LOG);
MODULE_LICENSE("GPL");
MODULE_ALIAS("spi:spidev");
