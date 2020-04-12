/*
 * include/linux/spi/spidev.h
 *
 * Copyright (C) 2006 SWAPP
 *	Andrea Paterniani <a.paterniani@swapp-eng.it>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
  */

#ifndef SPIDEV_H
#define SPIDEV_H

#include <linux/types.h>

#define	MSG_NONE	0
#define	MSG_INFO	1
#define	MSG_DBG		2
#define	MSG_TRK		3
#define	MSG_WARN	4
#define	MSG_ERR		5
#define	MSG_MAX		6

#define debug_level MSG_INFO 
#define DBG_MSG(level, msg, ...)\
do {\
	if ((level < MSG_MAX) && (level >= debug_level)) \
	{\
		char   buf[200];\
		char  *s = buf;\
		\
		s += snprintf(s, sizeof(buf) - (size_t)(s-buf), "[silead %s %d]:",  __func__, __LINE__);\
		\
		snprintf(s, sizeof(buf) - (size_t)(s-buf), msg, ##__VA_ARGS__);\
		printk(KERN_ERR "%s", buf);\
	}\
}while(0)

/* User space versions of kernel symbols for SPI clocking modes,
 * matching <linux/spi/spi.h>
 */

#define SPI_CPHA		0x01
#define SPI_CPOL		0x02

#define SPI_MODE_0		(0|0)
#define SPI_MODE_1		(0|SPI_CPHA)
#define SPI_MODE_2		(SPI_CPOL|0)
#define SPI_MODE_3		(SPI_CPOL|SPI_CPHA)

#define SPI_CS_HIGH		0x04
#define SPI_LSB_FIRST		0x08
#define SPI_3WIRE		0x10
#define SPI_LOOP		0x20
#define SPI_NO_CS		0x40
#define SPI_READY		0x80

/*---------------------------------------------------------------------------*/

/* IOCTL commands */

#define SPI_IOC_MAGIC			'k'

/**
 * struct spi_ioc_transfer - describes a single SPI transfer
 * @tx_buf: Holds pointer to userspace buffer with transmit data, or null.
 *	If no data is provided, zeroes are shifted out.
 * @rx_buf: Holds pointer to userspace buffer for receive data, or null.
 * @len: Length of tx and rx buffers, in bytes.
 * @speed_hz: Temporary override of the device's bitrate.
 * @bits_per_word: Temporary override of the device's wordsize.
 * @delay_usecs: If nonzero, how long to delay after the last bit transfer
 *	before optionally deselecting the device before the next transfer.
 * @cs_change: True to deselect device before starting the next transfer.
 *
 * This structure is mapped directly to the kernel spi_transfer structure;
 * the fields have the same meanings, except of course that the pointers
 * are in a different address space (and may be of different sizes in some
 * cases, such as 32-bit i386 userspace over a 64-bit x86_64 kernel).
 * Zero-initialize the structure, including currently unused fields, to
 * accommodate potential future updates.
 *
 * SPI_IOC_MESSAGE gives userspace the equivalent of kernel spi_sync().
 * Pass it an array of related transfers, they'll execute together.
 * Each transfer may be half duplex (either direction) or full duplex.
 *
 *	struct spi_ioc_transfer mesg[4];
 *	...
 *	status = ioctl(fd, SPI_IOC_MESSAGE(4), mesg);
 *
 * So for example one transfer might send a nine bit command (right aligned
 * in a 16-bit word), the next could read a block of 8-bit data before
 * terminating that command by temporarily deselecting the chip; the next
 * could send a different nine bit command (re-selecting the chip), and the
 * last transfer might write some register values.
 */
struct spi_ioc_transfer {
	__u64		tx_buf;
	__u64		rx_buf;

	__u32		len;
	__u32		speed_hz;

	__u16		delay_usecs;
	__u8		bits_per_word;
	__u8		cs_change;
	__u32		pad;

	/* If the contents of 'struct spi_ioc_transfer' ever change
	 * incompatibly, then the ioctl number (currently 0) must change;
	 * ioctls with constant size fields get a bit more in the way of
	 * error checking than ones (like this) where that field varies.
	 *
	 * NOTE: struct layout is the same in 64bit and 32bit userspace.
	 */
};

/* not all platforms use <asm-generic/ioctl.h> or _IOC_TYPECHECK() ... */
#define SPI_MSGSIZE(N) \
	((((N)*(sizeof (struct spi_ioc_transfer))) < (1 << _IOC_SIZEBITS)) \
		? ((N)*(sizeof (struct spi_ioc_transfer))) : 0)
#define SPI_IOC_MESSAGE(N) _IOW(SPI_IOC_MAGIC, 0, char[SPI_MSGSIZE(N)])

#define SPI_SYNC_READ        _IOR(SPI_IOC_MAGIC, 10, __u32)
#define SPI_SET_WAKE_UP      _IOR(SPI_IOC_MAGIC, 20, __u32)
#define SPI_ASYNC_READ_PRE   _IOR(SPI_IOC_MAGIC, 30, __u32)
#define SPI_ASYNC_READ       _IOR(SPI_IOC_MAGIC, 40, __u32)
#define SPI_GET_BUFFER_SIZE  _IOR(SPI_IOC_MAGIC, 50, __u32)

/* Read / Write of SPI mode (SPI_MODE_0..SPI_MODE_3) */
#define SPI_IOC_RD_MODE			_IOR(SPI_IOC_MAGIC, 1, __u8)
#define SPI_IOC_WR_MODE			_IOW(SPI_IOC_MAGIC, 1, __u8)

/* Read / Write SPI bit justification */
#define SPI_IOC_RD_LSB_FIRST		_IOR(SPI_IOC_MAGIC, 2, __u8)
#define SPI_IOC_WR_LSB_FIRST		_IOW(SPI_IOC_MAGIC, 2, __u8)

/* Read / Write SPI device word length (1..N) */
#define SPI_IOC_RD_BITS_PER_WORD	_IOR(SPI_IOC_MAGIC, 3, __u8)
#define SPI_IOC_WR_BITS_PER_WORD	_IOW(SPI_IOC_MAGIC, 3, __u8)

/* Read / Write SPI device default max speed hz */
#define SPI_IOC_RD_MAX_SPEED_HZ		_IOR(SPI_IOC_MAGIC, 4, __u32)
#define SPI_IOC_WR_MAX_SPEED_HZ		_IOW(SPI_IOC_MAGIC, 4, __u32)

/* ioctl  reset shutdown hight or low */
#define SPI_HW_RESET         _IOW(SPI_IOC_MAGIC, 70, __u32)
#define SPI_HW_SHUTDOWN			_IOW(SPI_IOC_MAGIC, 71, __u32)
#define SPI_OPEN_CLOCK          _IOW(SPI_IOC_MAGIC, 73, __u32)
#define SPI_HW_POWEROFF         _IOW(SPI_IOC_MAGIC, 75, __u32)
#define SPI_HW_POWERON          _IOW(SPI_IOC_MAGIC, 76, __u32)
#define SPI_HW_IRQ_ENBALE		_IOW(SPI_IOC_MAGIC, 80, __u32)
#define SPI_HW_FINGER_STATE_INFO	_IOW(SPI_IOC_MAGIC, 81, __u32)

#define SL_HEAD_SIZE 3
#define SL_PAGE_SIZE 128 
#define SL_ONE_FRAME_PAGES (110*118/SL_PAGE_SIZE)

#define SPI_SPEED			(2 * 1000 * 1000)
#define SPI_BITS				8
#define SPI_DELAY			100
#define SPI_BUF_SIZE	 4096	


#define VM_RESERVED (VM_DONTEXPAND | VM_DONTDUMP)
#define GSL6313_POWER_CTRL 1
#define GSL6313_INTERRUPT_CTRL 1
struct spidev_data {
    dev_t			devt;
    spinlock_t		spi_lock;
    struct platform_device	*spi;
    struct list_head	device_entry;

    /* buffer is NULL unless this device is open (users > 0) */
    struct mutex		buf_lock;
    unsigned		users;
    u8			*buffer;

    struct work_struct work;
    struct workqueue_struct *wqueue;
    u8    *mmap_buf;
    u8    *tx_mmap_buf;
    u8    *u_mmap_buf;
    u8    *k_mmap_buf;

    atomic_t frame_num;
    atomic_t is_opened;
    unsigned int max_buf_size;
    unsigned int max_frame_num;


    struct wake_lock wake_lock;
    unsigned int wake_up_gpio;
    unsigned int irq;

    unsigned int hw_reset_gpio;
    atomic_t is_cal_mode;
    atomic_t is_suspend;
	u32 shutdown_gpio;
	u32 gpio_id;
    /*IRQ wake-up control */
#ifdef GSL6313_INTERRUPT_CTRL
    struct workqueue_struct *int_wq;
    struct work_struct int_work;
    u32 int_wakeup_gpio; 
    int int_irq; 
    bool wakeup;
#endif
    /*IRQ wake-up control */
    /* power control */
#if GSL6313_POWER_CTRL
    struct regulator *vio;
    struct regulator *vdd;
#endif
    /*shutdown active/suspend */
    struct pinctrl *fp_pinctrl;
    struct pinctrl_state *pinctrl_state_active;
    struct pinctrl_state *pinctrl_state_suspend;
    /*shutdown active/suspend */
    /*IRQ wake-up control */
#ifdef GSL6313_INTERRUPT_CTRL
    struct pinctrl_state *pinctrl_state_interrupt;
#endif
    /*IRQ wake-up control */
};

unsigned int spidev_read_reg(struct spidev_data *spidev, unsigned char reg);
void init_frame(struct spidev_data *spidev);
int spidev_write_reg(struct spidev_data *spidev, unsigned int data, unsigned char reg);

ssize_t spidev_sync(struct spidev_data *spidev, struct spi_message *message);

typedef struct sl_page{
    unsigned char head[SL_HEAD_SIZE];
    unsigned char data[SL_PAGE_SIZE];
}sl_page_t;

typedef struct sl_frame{
    sl_page_t pages[SL_ONE_FRAME_PAGES];
}sl_frame_t;
typedef struct sl_frames{
    struct sl_frame *frame;
    int is_alloced;
}sl_frames_t;

#endif /* SPIDEV_H */
