#include <linux/module.h>
#include <linux/moduleparam.h>
#include <linux/kernel.h>
#include <linux/fs.h>
#include <linux/types.h>
#include <linux/kdev_t.h>
#include <linux/errno.h>
#include <linux/cdev.h>
#include <linux/sched.h>
#include <linux/stat.h>
#include <linux/io.h>
#include <linux/ioport.h>
#include <linux/init.h>
#include <linux/version.h>
#include <linux/device.h>
#include <linux/slab.h>
#include <linux/delay.h>
#include <asm/uaccess.h>
#include <linux/uaccess.h>
#include <linux/gpio.h>


#include "rpi_gpio.h"

MODULE_AUTHOR("Satoshi Yoneda");
MODULE_LICENSE("Dual BSD/GPL");

#define SSEGLED_NUM_DEVS	1			/* このドライバが制御するデバイスの数 */
#define SSEGLED_DEVNAME		"ssegled"	/* このデバイスドライバの名称 */
#define SSEGLED_MAJOR		0			/* メジャー番号だが自動設定なので0 */
#define SSEGLED_MINOR		0			/* マイナー番号のベース番号 */

#define SSEG_GPIO_MAPNAME	"sseg_gpio_map"

#define GPIO_SSEG_A 7
#define GPIO_SSEG_B 8
#define GPIO_SSEG_C 9
#define GPIO_SSEG_D 10

static int _ssegled_major = SSEGLED_MAJOR;
static int _ssegled_minor = SSEGLED_MINOR;

static struct cdev *ssegled_cdev_array = NULL;
static struct class *ssegled_class = NULL;

static void __iomem *gpio_map;
static volatile uint32_t *gpio_base;


#define LED_BASE	7

static int ssegled_display_value = 0;

static int ssegled_gpio_map(void)
{
	if( !request_mem_region(RPI_GPIO_BASE,
							RPI_BLOCK_SIZE,
							SSEG_GPIO_MAPNAME) ) {
		printk( KERN_ALERT "request_mem_region failed.\n");
		return -EBUSY;
	}
	gpio_map = ioremap_nocache(RPI_GPIO_BASE, BLOCK_SIZE);
	gpio_base = (volatile uint32_t *)gpio_map;
	
	return 0;
}

static int ssegled_gpio_unmap(void)
{
	iounmap(gpio_map);
	release_mem_region(RPI_GPIO_BASE, RPI_BLOCK_SIZE);

	gpio_map = NULL;
	gpio_base = NULL;
	return 0;
}

static int rpi_gpio_function_set(int pin, uint32_t func)
{
	int index = RPI_GPFSEL0_INDEX + pin / 10;
	uint32_t mask = ~(0x7 << ((pin % 10) * 3));
	gpio_base[index] = (gpio_base[index] & mask) | ((func & 0x7) << ((pin % 10) * 3));
	
	return 1;
}

static void rpi_gpio_set32( uint32_t mask, uint32_t val )
{
	gpio_base[RPI_GPSET0_INDEX] = val & mask;
}

static void rpi_gpio_clear32( uint32_t mask, uint32_t val )
{
	gpio_base[RPI_GPCLR0_INDEX] = val & mask;
}

static void ssegled_gpio_setup(void)
{
        gpio_request(GPIO_SSEG_A, "sseg-a");
        gpio_request(GPIO_SSEG_B, "sseg-b");
        gpio_request(GPIO_SSEG_C, "sseg-c");
        gpio_request(GPIO_SSEG_D, "sseg-d");

        gpio_direction_output(GPIO_SSEG_A, 0);
        gpio_direction_output(GPIO_SSEG_B, 0);
        gpio_direction_output(GPIO_SSEG_C, 0);
        gpio_direction_output(GPIO_SSEG_D, 0);
}

static int ssegled_put( int arg )
{
	int v = 0;
	
	if( arg < 0 ) return -1;
	if( arg > 9 ) {
		if( ('0' <= arg) && (arg <= '9') ) {
			// ASCIIコード
			v = arg - 0x30;
		}
		else return -1;
	}
	else
		v = arg;
	
	ssegled_display_value = arg;
	//rpi_gpio_clear32( RPI_GPIO_P1MASK, 0x0F << LED_BASE);
	//rpi_gpio_set32( RPI_GPIO_P1MASK, (v & 0x0F) << LED_BASE);
        gpio_set_value(GPIO_SSEG_A, (v >> 0) & 0x1);
        gpio_set_value(GPIO_SSEG_B, (v >> 1) & 0x1);
        gpio_set_value(GPIO_SSEG_C, (v >> 2) & 0x1);
        gpio_set_value(GPIO_SSEG_D, (v >> 3) & 0x1);

	return 0;
}


static int ssegled_open(struct inode *inode, struct file *filep)
{
	int retval;
        /*	
	if( gpio_base != NULL ) {
		printk(KERN_ERR "ssegled is already open.\n" );
		return -EIO;
	}
	retval = ssegled_gpio_map();
	if( retval != 0 ) {
		printk(KERN_ERR "Can not open ssegled.\n" );
		return retval;
	}
	*/
	return 0;
}


static int ssegled_release(struct inode *inode, struct file *filep)
{
	//ssegled_gpio_unmap();
	return 0;
}

static ssize_t ssegled_write(
	struct file *filep,
	const char __user *buf,
    size_t count,
    loff_t *f_pos)
{
	char cvalue;
	int retval;
	
	if(count > 0) {
		if(copy_from_user( &cvalue, buf, sizeof(char) )) {
			return -EFAULT;
		}
		retval = ssegled_put(cvalue);
		if( retval != 0 ) {
			printk(KERN_ALERT "Can not display %d\n", cvalue );
		}
		else {
			ssegled_display_value = cvalue;
		}
		return sizeof(char);
	}
	return 0;
}

struct file_operations ssegled_fops = {
	.open      = ssegled_open,
	.release   = ssegled_release,
	.write     = ssegled_write,
};

static int ssegled_register_dev(void)
{
	int retval;
	dev_t dev;
	size_t size;
	int i;
	
	/* 空いているメジャー番号を使ってメジャー&
	   マイナー番号をカーネルに登録する */
	retval =  alloc_chrdev_region(
		&dev,				/* 結果を格納するdev_t構造体 */
		SSEGLED_MINOR,		/* ベースマイナー番号 */
		SSEGLED_NUM_DEVS,	/* デバイスの数 */
		SSEGLED_DEVNAME		/* デバイスドライバの名前 */
	);
	
	if( retval < 0 ) {
		printk(KERN_ERR "alloc_chrdev_region failed.\n" );
		return retval;
	}
	_ssegled_major = MAJOR(dev);
    _ssegled_minor = MINOR(dev);
	
	/* デバイスクラスを作成する */
	ssegled_class = class_create(THIS_MODULE,SSEGLED_DEVNAME);
	if(IS_ERR(ssegled_class))
		return PTR_ERR(ssegled_class);
	
	/* cdev構造体の用意 */
	size = sizeof(struct cdev) * SSEGLED_NUM_DEVS;
	ssegled_cdev_array =  (struct cdev*)kmalloc(size, GFP_KERNEL);
	
	/* デバイスの数だけキャラクタデバイスを登録する */
	/* ただし7セグLEDは1個しかない */
	for( i = 0; i < SSEGLED_NUM_DEVS; i++ ) {
		dev_t devno = MKDEV(_ssegled_major, _ssegled_minor+i);
		/* キャラクタデバイスとしてこのモジュールをカーネルに登録する */
		cdev_init(&(ssegled_cdev_array[i]), &ssegled_fops);
		ssegled_cdev_array[i].owner = THIS_MODULE;
		if( cdev_add( &(ssegled_cdev_array[i]), devno, 1) < 0 ) {
			/* 登録に失敗した */
			printk(KERN_ERR "cdev_add failed minor = %d\n", _ssegled_minor+i );
		}
		else {
			/* デバイスノードの作成 */
			device_create(
					ssegled_class,
					NULL,
					devno,
					NULL,
					SSEGLED_DEVNAME"%u",_ssegled_minor+i
			);
		}
	}
	return 0;
}


static int ssegled_init(void)
{
	int retval;
	int i;
	
	/* 開始のメッセージ */
	printk(KERN_INFO "%s loading...\n", SSEGLED_DEVNAME );
	
	/* GPIO初期化 */
	ssegled_gpio_setup();
	/* ちょっとしたデモを見せる */
	for( i=9; i>=0; i-- ) {
		ssegled_put( i );
		msleep(100);
	}
	/* デバイスドライバをカーネルに登録 */
	retval = ssegled_register_dev();
	if( retval != 0 ) {
		printk( KERN_ALERT "ssegled driver register failed.\n");
		return retval;
	}
	printk( KERN_INFO "ssegled driver register sccessed.\n");
	
	/* GPIOレジスタのアンマップ */
	//ssegled_gpio_unmap();

	return 0;
}

static void ssegled_exit(void)
{
	int i;
	dev_t devno;

	ssegled_put( 0 );
	/* キャラクタデバイスの登録解除 */
	for( i = 0; i < SSEGLED_NUM_DEVS; i++ ) {
		cdev_del(&(ssegled_cdev_array[i]));
		devno = MKDEV(_ssegled_major, _ssegled_minor+i);
		device_destroy(ssegled_class, devno);
	}
	/* メジャー番号/マイナー番号を取り除く */
	devno = MKDEV(_ssegled_major,_ssegled_minor);
	unregister_chrdev_region(devno, SSEGLED_NUM_DEVS);
	/* デバイスノードを取り除く */
	class_destroy( ssegled_class );
	kfree(ssegled_cdev_array);
}

module_init(ssegled_init);
module_exit(ssegled_exit);

module_param( ssegled_display_value, int, S_IRUSR | S_IRGRP | S_IROTH );
