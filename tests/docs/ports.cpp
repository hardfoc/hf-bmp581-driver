/**
 * @file ports.cpp
 * @brief Instantiates the documented I2C and SPI ports with the driver, so the whole API compiles over both.
 */
#include "my_i2c.hpp"
#include "my_spi.hpp"

template class bmp581::Driver<MyI2c>;
template class bmp581::Driver<MySpi>;
