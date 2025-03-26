/*
 * Copyright (C) 2024 
 *
 * This file is subject to the terms and conditions of the GNU Lesser
 * General Public License v2.1. See the file LICENSE in the top level
 * directory for more details.
 */

/**
 * @defgroup    sys_BoRa_decoder BoRa_decoder
 * @ingroup     sys
 * @brief       The decoder part of the BoRa
 *
 * @{
 *
 * @file
 *
 * @author      
 */

#ifndef BORA_DECODER_H
#define BORA_DECODER_H

#ifdef __cplusplus
extern "C" {
#endif

int bora_decoder_init(void);

int bora_recv_start(void);

int bora_recv_stop(void);

int bora_recv_read(void);

// void * bora_recv_mainwork(void *arg);

#ifdef __cplusplus
}
#endif

#endif /* BORA_DECODER_H */
/** @} */
