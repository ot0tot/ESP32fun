/* Shared I2C status values; existing software/hardware numeric ABI retained. */
#ifndef FUN_I2C_ERRORS_H
#define FUN_I2C_ERRORS_H
enum { FUN_I2C_INVALID=-1, FUN_I2C_TIMEOUT=-2, FUN_I2C_NACK=-3,
       FUN_I2C_BUSY=-4, FUN_I2C_ARBITRATION=-5, FUN_I2C_FIFO_ERROR=-6 };
#endif
