#ifndef _NMI326_SPI_DRV_H
#define _NMI326_SPI_DRV_H

#define SPI_DEBUG(fmt,...) printk("%s:",fmt,__func__,##__VA_ARGS__)

#define SPI_WARN(fmt,...) printk(KERN_WARNING fmt,__func__,##__VA_ARGS__)

#define SPI_ERROR(fmt,...) printk(KERN_err "%s:" fmt,##__VA_ARGS__)

//#iffdef CONFIG_VIDEO_SPI_DEBUG
#if 0
#define spi_dbg(fmt,...) SPI_DEBUG(fmt,...)
#else
#define spi_dbg(fmt,...)
#endif

#define spi_warn(fmt,...) SPI_WARN(fmt,##__VA_ARGS__)
#define spi_err(fmt,...)  SPI_ERROR(fmt,##__VA_ARGS__)


void nmi326_spi_read_chip_id(void);
int nmi326_spi_init(void);
void nmi326_spi_exit(void);
int nmi326_spi_read(u8 *buf,size_t len);
int nmi326_spi_write(u8 *buf,size_t len);

#endif
