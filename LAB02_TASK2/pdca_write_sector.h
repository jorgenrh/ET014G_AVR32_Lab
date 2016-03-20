/*
 * pdca_write_sector.h
 *
 * Created: 28.02.2016 22.18.51
 *  Author: jrh
 */ 


#ifndef PDCA_WRITE_SECTOR_H_
#define PDCA_WRITE_SECTOR_H_

#include "compiler.h"

bool sd_mmc_spi_write_open_PDCA(U32 pos);

bool sd_mmc_spi_write_close_PDCA(void);


#endif /* PDCA_WRITE_SECTOR_H_ */