/*!
    \file    qspi_flash_api.c
    \brief   QSPI write and read external flash

    \version 2024-12-31
*/

/*
    Copyright (c) 2024, GigaDevice Semiconductor Inc.

    Redistribution and use in source and binary forms, with or without modification,
are permitted provided that the following conditions are met:

    1. Redistributions of source code must retain the above copyright notice, this
       list of conditions and the following disclaimer.
    2. Redistributions in binary form must reproduce the above copyright notice,
       this list of conditions and the following disclaimer in the documentation
       and/or other materials provided with the distribution.
    3. Neither the name of the copyright holder nor the names of its contributors
       may be used to endorse or promote products derived from this software without
       specific prior written permission.

    THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT,
INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY
OF SUCH DAMAGE.
*/

#include <stdio.h>
#include "gd32vw55x.h"
#include "qspi_flash_api.h"
#include "wrapper_os.h"
#include "ll.h"

#define QSPI_FLASH_TEST                     0
#define QSPI_QUAD_EN                        0

#if (QSPI_FLASH_MEM == 16)
#define QSPI_FLASH_TOTAL_SIZE               (0x1000000)
#define EXT_FLASH_SIZE_LOG_INDX             23      // 16M bytes
#else
#define QSPI_FLASH_TOTAL_SIZE               (0x200000)
#define EXT_FLASH_SIZE_LOG_INDX             20      // 2M bytes
#endif

#define QSPI_FLASH_SECTOR_SIZE              0x1000

#define QSPI_MEMORY_MAP_BASE_ADDR           0x90000000


#define QSPI_POLLING_CYCLES                 0x10

#define WRITE_STATUS_REG                    (0x01)
#define WRITE_ENABLE_CMD                    (0x06)
#define PAGE_PROG_CMD                       (0x02)
#define QUAD_PAGE_PROG_CMD                  (0x32)
#define READ_CMD                            (0x03)
#define QUAD_READ_CMD                       (0xEB)

#define SECTOR_ERASE_CMD                    (0x20)
#define READ_STATUS_REG1_CMD                (0x05)
#define READ_STATUS_REG2_CMD                (0x35)
#define CHIP_ERASE_CMD                      (0xC7)
#if (QSPI_FLASH_MEM == 2)
#define HIGH_PFM_EN_CMD                     (0xA3)
#endif

#define STATUS_REG_WIP_VAL                  0x01
#define STATUS_REG_WIP_MSK                  0x01        //s0

#define STATUS_REG_WEL_VAL                  0x02
#define STATUS_REG_WEL_MSK                  0x02        //s1

#define STATUS_REG_QE_VAL                  0x02
#define STATUS_REG_QE_MSK                  0x02        //s9

#if (QSPI_FLASH_MEM == 2)
#define STATUS_REG_HPF_VAL                 0x20
#define STATUS_REG_HPF_MSK                 0x20        //s13
#endif

#define SPI_FLASH_PAGE_SIZE                 256

#if QSPI_FLASH_TEST
uint8_t tx_buffer[] = { 0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0A, 0x0B, 0x0C, 0x0D, 0x0E, 0x0F };
#define countof(a)                          (sizeof(a) / sizeof(*(a)))
#define BUF_SIZE                            (countof(tx_buffer))

uint8_t rx_buffer[BUF_SIZE];
uint8_t rx_buffer_sector[4096];
#endif

static qspi_command_struct qspi_cmd;
static qspi_polling_struct polling_cmd;

/*!
    \brief      QSPI write
    \param[in]  none
    \param[out] none
    \retval     none
*/
static void qspi_write_enable(void)
{
    /* write enable */
    qspi_cmd.instruction_mode = QSPI_INSTRUCTION_1_LINE;
    qspi_cmd.instruction      = WRITE_ENABLE_CMD;
    qspi_cmd.addr_mode        = QSPI_ADDR_NONE;
    qspi_cmd.altebytes_mode   = QSPI_ALTE_BYTES_NONE;
    qspi_cmd.data_mode        = QSPI_DATA_NONE;
    qspi_cmd.dummycycles      = 0;
    qspi_cmd.sioo_mode        = QSPI_SIOO_INST_EVERY_CMD;
    qspi_command_config(&qspi_cmd);
}

/*!
    \brief      QSPI polling status register with write enable latch
    \param[in]  none
    \param[out] none
    \retval     none
*/
static void qspi_polling_match_wel(void)
{
    polling_cmd.match            = STATUS_REG_WEL_VAL;
    polling_cmd.mask             = STATUS_REG_WEL_MSK;
    polling_cmd.match_mode       = QSPI_MATCH_MODE_AND;
    polling_cmd.statusbytes_size = 1;
    polling_cmd.interval         = QSPI_POLLING_CYCLES;
    polling_cmd.polling_stop     = QSPI_POLLING_STOP_ENABLE;
    qspi_cmd.instruction         = READ_STATUS_REG1_CMD;
    qspi_cmd.data_mode           = QSPI_DATA_1_LINE;

    qspi_polling_config(&qspi_cmd, &polling_cmd);
}

/*!
    \brief      QSPI polling match write not in progress
    \param[in]  none
    \param[out] none
    \retval     none
*/
static void qspi_polling_match_not_wip(void)
{
    qspi_cmd.instruction         = READ_STATUS_REG1_CMD;
    qspi_cmd.addr_mode           = QSPI_ADDR_NONE;
    qspi_cmd.data_mode           = QSPI_DATA_1_LINE;
    polling_cmd.match            = 0x00;
    polling_cmd.mask             = STATUS_REG_WIP_MSK;
    polling_cmd.match_mode       = QSPI_MATCH_MODE_AND;
    polling_cmd.statusbytes_size = 1;
    polling_cmd.interval         = QSPI_POLLING_CYCLES;
    polling_cmd.polling_stop     = QSPI_POLLING_STOP_ENABLE;

    qspi_polling_config(&qspi_cmd, &polling_cmd);
}

static void qspi_polling_match_qe(bool enable)
{
    polling_cmd.match            = enable ? STATUS_REG_QE_VAL : 0x00;
    polling_cmd.mask             = STATUS_REG_QE_MSK;
    polling_cmd.match_mode       = QSPI_MATCH_MODE_AND;
    polling_cmd.statusbytes_size = 1;
    polling_cmd.interval         = QSPI_POLLING_CYCLES;
    polling_cmd.polling_stop     = QSPI_POLLING_STOP_ENABLE;
    qspi_cmd.instruction         = READ_STATUS_REG2_CMD;
    qspi_cmd.data_mode           = QSPI_DATA_1_LINE;
    qspi_cmd.addr_mode           = QSPI_ADDR_NONE;

    qspi_polling_config(&qspi_cmd, &polling_cmd);
}

static void qspi_flash_quad_enable(void)
{
#if (QSPI_FLASH_MEM == 2)
    // FIX TODO status register may need to read value first
    uint8_t write_status[2] ={0x00,0x02};

    /* QSPI write enable */
    qspi_write_enable();
    while(RESET != qspi_flag_get(QSPI_FLAG_BUSY)) {
    }
    /* configure read polling mode to wait for write enabling */
    qspi_polling_match_wel();
    /* wait for the BUSY flag to be reset */
    while(RESET != qspi_flag_get(QSPI_FLAG_BUSY)) {
    }

    qspi_cmd.instruction = WRITE_STATUS_REG; //write status register
    qspi_cmd.instruction_mode = QSPI_INSTRUCTION_1_LINE;
    qspi_cmd.addr = 0;
    qspi_cmd.addr_mode = QSPI_ADDR_NONE;
    qspi_cmd.addr_size = QSPI_ADDR_8_BITS;
    qspi_cmd.altebytes = 0;
    qspi_cmd.altebytes_mode = QSPI_ALTE_BYTES_NONE;
    qspi_cmd.altebytes_size = QSPI_ALTE_BYTES_8_BITS;
    qspi_cmd.data_mode = QSPI_DATA_1_LINE;
    qspi_cmd.data_length = 2;
    qspi_cmd.dummycycles = 0;
    qspi_cmd.sioo_mode = QSPI_SIOO_INST_EVERY_CMD;

    sys_enter_critical();
    qspi_command_config(&qspi_cmd);
    // QSPI_DTLEN = 1;
    qspi_data_transmit(write_status);
    sys_exit_critical();

    /* clear the TC flag */
    qspi_flag_clear(QSPI_FLAG_TC);
    qspi_polling_match_not_wip();
    /* wait for the BUSY flag to be reset */
    while(RESET != qspi_flag_get(QSPI_FLAG_BUSY)) {
    }

    qspi_polling_match_qe(true);
    while(RESET != qspi_flag_get(QSPI_FLAG_BUSY)) {
    }
#endif
}

static void qspi_flash_quad_disable(void)
{
#if (QSPI_FLASH_MEM == 2)
    // FIX TODO status register may need to read value first
    uint8_t write_status[2] ={0x00,0x00};

    /* QSPI write enable */
    qspi_write_enable();
    while(RESET != qspi_flag_get(QSPI_FLAG_BUSY)) {
    }
    /* configure read polling mode to wait for write enabling */
    qspi_polling_match_wel();
    /* wait for the BUSY flag to be reset */
    while(RESET != qspi_flag_get(QSPI_FLAG_BUSY)) {
    }

    qspi_cmd.instruction = WRITE_STATUS_REG; //write status register
    qspi_cmd.instruction_mode = QSPI_INSTRUCTION_1_LINE;
    qspi_cmd.addr = 0;
    qspi_cmd.addr_mode = QSPI_ADDR_NONE;
    qspi_cmd.addr_size = QSPI_ADDR_8_BITS;
    qspi_cmd.altebytes = 0;
    qspi_cmd.altebytes_mode = QSPI_ALTE_BYTES_NONE;
    qspi_cmd.altebytes_size = QSPI_ALTE_BYTES_8_BITS;
    qspi_cmd.data_mode = QSPI_DATA_1_LINE;
    qspi_cmd.data_length = 2;
    qspi_cmd.dummycycles = 0;
    qspi_cmd.sioo_mode = QSPI_SIOO_INST_EVERY_CMD;
    qspi_command_config(&qspi_cmd);

    // QSPI_DTLEN = 1;
    qspi_data_transmit(write_status);

    /* clear the TC flag */
    qspi_flag_clear(QSPI_FLAG_TC);
    qspi_polling_match_not_wip();
    /* wait for the BUSY flag to be reset */
    while(RESET != qspi_flag_get(QSPI_FLAG_BUSY)) {
    }

    qspi_polling_match_qe(false);
    while(RESET != qspi_flag_get(QSPI_FLAG_BUSY)) {
    }
#endif
}

#if (QSPI_FLASH_MEM == 2)
static void qspi_polling_match_hpf(void)
{
    polling_cmd.match            = STATUS_REG_HPF_VAL;
    polling_cmd.mask             = STATUS_REG_HPF_MSK;
    polling_cmd.match_mode       = QSPI_MATCH_MODE_AND;
    polling_cmd.statusbytes_size = 1;
    polling_cmd.interval         = QSPI_POLLING_CYCLES;
    polling_cmd.polling_stop     = QSPI_POLLING_STOP_ENABLE;
    qspi_cmd.instruction         = READ_STATUS_REG2_CMD;
    qspi_cmd.data_mode           = QSPI_DATA_1_LINE;
    qspi_cmd.addr_mode           = QSPI_ADDR_NONE;

    qspi_polling_config(&qspi_cmd, &polling_cmd);
}

static void qspi_high_performance_enable(void)
{
    qspi_cmd.instruction = HIGH_PFM_EN_CMD;
    qspi_cmd.instruction_mode = QSPI_INSTRUCTION_1_LINE;
    qspi_cmd.addr = 0;
    qspi_cmd.addr_mode = QSPI_ADDR_NONE;
    qspi_cmd.addr_size = QSPI_ADDR_8_BITS;
    qspi_cmd.altebytes = 0;
    qspi_cmd.altebytes_mode = QSPI_ALTE_BYTES_NONE;
    qspi_cmd.altebytes_size = QSPI_ALTE_BYTES_8_BITS;
    qspi_cmd.data_mode = QSPI_DATA_1_LINE;
    qspi_cmd.data_length = 0;
    qspi_cmd.dummycycles = 26;
    qspi_cmd.sioo_mode = QSPI_SIOO_INST_EVERY_CMD;
    qspi_command_config(&qspi_cmd);

    /* wait for the BUSY flag to be reset */
    while(RESET != qspi_flag_get(QSPI_FLAG_BUSY)) {
    }

    // qspi_polling_match_hpf();
    while(RESET != qspi_flag_get(QSPI_FLAG_BUSY)) {

    }
}
#endif

/*!
    \brief      configure the QSPI peripheral
    \param[in]  none
    \param[out] none
    \retval     none
*/
static void qspi_flash_init(void)
{
    qspi_init_struct qspi_init_para;

    /* initialize the QSPI command parameter structure */
    qspi_cmd_struct_para_init(&qspi_cmd);
    /* initialize the QSPI read polling parameter structure */
    qspi_polling_struct_para_init(&polling_cmd);

#if CONFIG_BOARD == PLATFORM_BOARD_32VW55X_START
    /* QSPI GPIO config:SCK/PA9, NSS/PA10, IO0/PA11, IO1/PA12 */
    gpio_af_set(GPIOA, GPIO_AF_4, GPIO_PIN_9 | GPIO_PIN_10 | GPIO_PIN_11 | GPIO_PIN_12);
    gpio_mode_set(GPIOA, GPIO_MODE_AF, GPIO_PUPD_NONE, GPIO_PIN_9 | GPIO_PIN_10 | GPIO_PIN_11 | GPIO_PIN_12);
    gpio_output_options_set(GPIOA, GPIO_OTYPE_PP, GPIO_OSPEED_10MHZ, GPIO_PIN_9 | GPIO_PIN_10 | GPIO_PIN_11 | GPIO_PIN_12);

    /* QSPI GPIO config: IO2/PB3, IO3/PB4 */
    gpio_af_set(GPIOB, GPIO_AF_3, GPIO_PIN_3 | GPIO_PIN_4);
    gpio_mode_set(GPIOB, GPIO_MODE_AF, GPIO_PUPD_NONE, GPIO_PIN_3 | GPIO_PIN_4);
    gpio_output_options_set(GPIOB, GPIO_OTYPE_PP, GPIO_OSPEED_10MHZ, GPIO_PIN_3 | GPIO_PIN_4);
#elif CONFIG_BOARD == PLATFORM_BOARD_32VW55X_EVAL
    /* QSPI GPIO config:SCK/PA4, NSS/PA5, IO0/PA6, IO1/PA7, IO2/PB3, IO3/PB4 */
    gpio_af_set(GPIOA, GPIO_AF_3, GPIO_PIN_4 | GPIO_PIN_5 | GPIO_PIN_6 | GPIO_PIN_7);
    gpio_mode_set(GPIOA, GPIO_MODE_AF, GPIO_PUPD_NONE, GPIO_PIN_4 | GPIO_PIN_5 | GPIO_PIN_6 | GPIO_PIN_7);
    gpio_output_options_set(GPIOA, GPIO_OTYPE_PP, GPIO_OSPEED_10MHZ, GPIO_PIN_4 | GPIO_PIN_5 | GPIO_PIN_6 | GPIO_PIN_7);

#if QSPI_QUAD_EN
    gpio_af_set(GPIOB, GPIO_AF_3, GPIO_PIN_3 | GPIO_PIN_4);
    gpio_mode_set(GPIOB, GPIO_MODE_AF, GPIO_PUPD_NONE, GPIO_PIN_3 | GPIO_PIN_4);
    gpio_output_options_set(GPIOB, GPIO_OTYPE_PP, GPIO_OSPEED_10MHZ, GPIO_PIN_3 | GPIO_PIN_4);
#endif
#endif
    /* initialize the init parameter structure */
    qspi_struct_para_init(&qspi_init_para);

    qspi_init_para.clock_mode     = QSPI_CLOCK_MODE_3;
    qspi_init_para.fifo_threshold = 8;
    qspi_init_para.sample_shift   = QSPI_SAMPLE_SHIFTING_HALFCYCLE;
    qspi_init_para.cs_high_time   = QSPI_CS_HIGH_TIME_8_CYCLE;
    qspi_init_para.flash_size     = EXT_FLASH_SIZE_LOG_INDX;
    qspi_init_para.prescaler      = 1;
#if 0
    qspi_init_para.clock_mode     = QSPI_CLOCK_MODE_0;
    qspi_init_para.fifo_threshold = 4;
    qspi_init_para.sample_shift   = QSPI_SAMPLE_SHIFTING_HALFCYCLE;
    qspi_init_para.cs_high_time   = QSPI_CS_HIGH_TIME_2_CYCLE;
    qspi_init_para.flash_size     = EXT_FLASH_SIZE_LOG_INDX;
    qspi_init_para.prescaler      = 4;
#endif
    qspi_init(&qspi_init_para);

    qspi_enable();

#if QSPI_QUAD_EN
    qspi_flash_quad_enable();
#if (QSPI_FLASH_MEM == 2)
    qspi_high_performance_enable();
#endif
#endif

}

/*!
    \brief      send QSPI command
    \param[in]  instruction: QSPI instruction, reference flash commands description
    \param[in]  address: access address, 0-flash size
    \param[in]  dummy_cycles: dummy cycles, 0 - 31
    \param[in]  instruction_mode: instruction mode
      \arg        QSPI_INSTRUCTION_NONE
      \arg        QSPI_INSTRUCTION_1_LINE
      \arg        QSPI_INSTRUCTION_2_LINES
      \arg        QSPI_INSTRUCTION_4_LINES
    \param[in]  address_mode: address mode
      \arg        QSPI_ADDR_NONE
      \arg        QSPI_ADDR_1_LINE
      \arg        QSPI_ADDR_2_LINES
      \arg        QSPI_ADDR_4_LINES
    \param[in]  address_size: address size
      \arg        QSPI_ADDR_8_BITS
      \arg        QSPI_ADDR_16_BITS
      \arg        QSPI_ADDR_24_BITS
      \arg        QSPI_ADDR_32_BITS
    \param[in]  data_mode: data mode
      \arg        QSPI_DATA_NONE
      \arg        QSPI_DATA_1_LINE
      \arg        QSPI_DATA_2_LINES
      \arg        QSPI_DATA_4_LINES
    \param[out] none
    \retval     none
*/
static void qspi_send_command(uint32_t instruction, uint32_t address, uint32_t dummy_cycles, uint32_t instruction_mode,
                       uint32_t address_mode, uint32_t address_size, uint32_t data_mode, uint32_t altebytes_mode, uint32_t altebytes_size, uint32_t data_length)
{
    qspi_cmd.instruction      = instruction;
    qspi_cmd.instruction_mode = instruction_mode;
    qspi_cmd.addr             = address;
    qspi_cmd.addr_mode        = address_mode;
    qspi_cmd.addr_size        = address_size;
    qspi_cmd.altebytes        = 0;
    qspi_cmd.altebytes_mode   = altebytes_mode;
    qspi_cmd.altebytes_size   = altebytes_size;
    qspi_cmd.data_mode        = data_mode;
    qspi_cmd.data_length      = data_length;
    qspi_cmd.dummycycles      = dummy_cycles;
    qspi_cmd.sioo_mode        = QSPI_SIOO_INST_EVERY_CMD;
    qspi_command_config(&qspi_cmd);
}


/*!
    \brief      read spi flash by memory map mode
    \param[in]  none
    \param[out] none
    \retval     none
*/
static void qspi_memory_map_read(void)
{
#if QSPI_QUAD_EN
    qspi_cmd.instruction_mode = QSPI_INSTRUCTION_1_LINE;
    qspi_cmd.instruction      = QUAD_READ_CMD;
    qspi_cmd.addr_mode        = QSPI_ADDR_4_LINES;
    qspi_cmd.addr_size        = QSPI_ADDR_24_BITS;
    qspi_cmd.altebytes_size   = QSPI_ALTE_BYTES_8_BITS;
    qspi_cmd.altebytes_mode   = QSPI_ALTE_BYTES_4_LINES;
    qspi_cmd.altebytes        = 0;
    qspi_cmd.data_mode        = QSPI_DATA_4_LINES;
    qspi_cmd.dummycycles      = 4;
    qspi_cmd.sioo_mode        = QSPI_SIOO_INST_EVERY_CMD;
#else
    qspi_cmd.instruction_mode = QSPI_INSTRUCTION_1_LINE;
    qspi_cmd.instruction      = READ_CMD;
    qspi_cmd.addr_mode        = QSPI_ADDR_1_LINE;
    qspi_cmd.addr_size        = QSPI_ADDR_24_BITS;
    qspi_cmd.altebytes_mode   = QSPI_ALTE_BYTES_NONE;
    qspi_cmd.data_mode        = QSPI_DATA_1_LINE;
    qspi_cmd.dummycycles      = 0;
    qspi_cmd.sioo_mode        = QSPI_SIOO_INST_EVERY_CMD;
#endif

    qspi_memorymapped_config(&qspi_cmd, 0, QSPI_TMOUT_ENABLE);
}

/*!
    \brief      read spi flash
    \param[in]  none
    \param[out] none
    \retval     none
*/
static void qspi_flash_memory_read(uint32_t offset, uint8_t *data, uint32_t len)
{
    qspi_send_command(READ_CMD, offset, 0, QSPI_INSTRUCTION_1_LINE, QSPI_ADDR_1_LINE, QSPI_ADDR_24_BITS, QSPI_DATA_1_LINE, QSPI_ALTE_BYTES_NONE, QSPI_ALTE_BYTES_8_BITS, len);
    // QSPI_DTLEN = (uint32_t)(len - 1);
    qspi_data_receive((uint8_t *)data);
    while(RESET != qspi_flag_get(QSPI_FLAG_BUSY)) {
    }
}

/*!
    \brief      qspi flash sector erase
    \param[in]  none
    \param[out] none
    \retval     none
*/
static void qspi_flash_sector_erase(uint32_t offset)
{
    /* QSPI write enable */
    qspi_write_enable();
    while(RESET != qspi_flag_get(QSPI_FLAG_BUSY)) {
    }
    /* configure read polling mode to wait for write enabling */
    qspi_polling_match_wel();
    /* wait for the BUSY flag to be reset */
    while(RESET != qspi_flag_get(QSPI_FLAG_BUSY)) {
    }
    qspi_send_command(SECTOR_ERASE_CMD, offset, 0, QSPI_INSTRUCTION_1_LINE, QSPI_ADDR_1_LINE,
                        QSPI_ADDR_24_BITS, QSPI_DATA_NONE, QSPI_ALTE_BYTES_NONE, QSPI_ALTE_BYTES_8_BITS, 0);
    qspi_polling_match_not_wip();
    /* wait for the BUSY flag to be reset */
    while(RESET != qspi_flag_get(QSPI_FLAG_BUSY)) {
    }
}

/*!
    \brief      qspi flash program
    \param[in]  none
    \param[out] none
    \retval     none
*/
static void qspi_flash_program(uint32_t offset, uint8_t *data, int len)
{
    uint16_t num_of_page = 0, num_of_single = 0, addr = 0, count = 0, temp = 0;

    /* QSPI write enable */
    qspi_write_enable();
    while(RESET != qspi_flag_get(QSPI_FLAG_BUSY)) {
    }
    /* configure read polling mode to wait for write enabling */
    qspi_polling_match_wel();
    /* wait for the BUSY flag to be reset */
    while(RESET != qspi_flag_get(QSPI_FLAG_BUSY)) {
    }
    /* write data */
 #if QSPI_QUAD_EN
    qspi_send_command(QUAD_PAGE_PROG_CMD, offset, 0, QSPI_INSTRUCTION_1_LINE, QSPI_ADDR_1_LINE,
                        QSPI_ADDR_24_BITS, QSPI_DATA_4_LINES, QSPI_ALTE_BYTES_NONE, QSPI_ALTE_BYTES_8_BITS, len);
 #else
    qspi_send_command(PAGE_PROG_CMD, offset, 0, QSPI_INSTRUCTION_1_LINE, QSPI_ADDR_1_LINE,
                        QSPI_ADDR_24_BITS, QSPI_DATA_1_LINE, QSPI_ALTE_BYTES_NONE, QSPI_ALTE_BYTES_8_BITS, len);
 #endif

    // QSPI_DTLEN = (uint32_t)(len - 1);
    qspi_data_transmit((uint8_t *)data);
    /* wait for the data transmit completed */
    while(RESET == qspi_flag_get(QSPI_FLAG_TC)) {
    }
    /* clear the TC flag */
    qspi_flag_clear(QSPI_FLAG_TC);
    qspi_polling_match_not_wip();
    /* wait for the BUSY flag to be reset */
    while(RESET != qspi_flag_get(QSPI_FLAG_BUSY)) {
    }
}

static uint32_t qspi_flash_total_size(void)
{
    return QSPI_FLASH_TOTAL_SIZE;
}


static int qspi_flash_is_valid_offset(uint32_t offset)
{
    if (offset < qspi_flash_total_size()) {
        return 1;
    }
    return 0;
}


int qspi_flash_erase(uint32_t offset, int len)
{
    uint16_t i, num_of_sector;
    if (!qspi_flash_is_valid_offset(offset)
        || len <= 0 || !qspi_flash_is_valid_offset(offset + len - 1)) {
        return -1;
    }

    //printf("qspi_flash_erase offset 0x%x, len %d \r\n", offset, len);
    num_of_sector = (len / QSPI_FLASH_SECTOR_SIZE);
    if ((len % QSPI_FLASH_SECTOR_SIZE) != 0)
        num_of_sector++;

    for (i = 0; i < num_of_sector; i++) {
        qspi_flash_sector_erase(offset + i * QSPI_FLASH_SECTOR_SIZE);
    }
    //printf("qspi_flash_erase offset 0x%x, len %d end\r\n", offset, len);

    return 0;
}

int qspi_flash_write(uint32_t offset, uint8_t *data, int len)
{
    if (!qspi_flash_is_valid_offset(offset) || data == NULL
        || len <= 0 || !qspi_flash_is_valid_offset(offset + len - 1)) {
        return -1;
    }
    // printf("qspi_flash_write offset 0x%x, len %d \r\n", offset, len);

    uint16_t num_of_page = len / SPI_FLASH_PAGE_SIZE;
    uint8_t num_of_single = len % SPI_FLASH_PAGE_SIZE;

    while(num_of_page) {
        qspi_flash_program(offset, data, SPI_FLASH_PAGE_SIZE);
        offset += SPI_FLASH_PAGE_SIZE;
        data += SPI_FLASH_PAGE_SIZE;
        num_of_page--;
    }

    if (num_of_single) {
        qspi_flash_program(offset, data, num_of_single);
    }

    //printf("qspi_flash_write offset 0x%x, len %d end\r\n", offset, len);
    return 0;
}


int qspi_flash_read(uint32_t offset, uint8_t *data, int len)
{
    int i;
    if (!qspi_flash_is_valid_offset(offset) || data == NULL
        || len <= 0 || !qspi_flash_is_valid_offset(offset + len - 1)) {
        return -1;
    }

    //printf("qspi_flash_read offset 0x%x, len %d \r\n", offset, len);
    qspi_memory_map_read();

    for(i = 0; i < len; i++) {
        data[i] = *(uint8_t *)(QSPI_MEMORY_MAP_BASE_ADDR + offset + i);
    }

    /* wait for the BUSY flag to be reset */
    while(RESET != qspi_flag_get(QSPI_FLAG_BUSY)) {
    }

    //printf("qspi_flash_read offset 0x%x, len %d end\r\n", offset, len);
    return 0;
}

void qspi_flash_chip_erase(void)
{
    /* QSPI write enable */
    qspi_write_enable();
    while(RESET != qspi_flag_get(QSPI_FLAG_BUSY)) {
    }

    /* configure read polling mode to wait for write enabling */
    qspi_polling_match_wel();
    /* wait for the BUSY flag to be reset */
    while(RESET != qspi_flag_get(QSPI_FLAG_BUSY)) {
    }

    qspi_send_command(CHIP_ERASE_CMD, 0, 0, QSPI_INSTRUCTION_1_LINE, QSPI_ADDR_NONE,
                        QSPI_ADDR_24_BITS, QSPI_DATA_NONE, QSPI_ALTE_BYTES_NONE, QSPI_ALTE_BYTES_8_BITS, 0);

    while(RESET != qspi_flag_get(QSPI_FLAG_BUSY)) {
    }

    qspi_polling_match_not_wip();
    /* wait for the BUSY flag to be reset */
    while(RESET != qspi_flag_get(QSPI_FLAG_BUSY)) {
    }
}


void qspi_flash_api_init(void)
{
    rcu_periph_clock_enable(RCU_GPIOA);
    rcu_periph_clock_enable(RCU_GPIOB);
    rcu_periph_clock_enable(RCU_QSPI);

    qspi_flash_init();
}

#if QSPI_FLASH_TEST
void flash_test(void)
{
    uint32_t i = 0;

    printf("QSPI flash writing...\r\n");
    qspi_flash_api_init();

    /* qspi flash sector erase */
    /* erase sector 0-0x1000 */
    qspi_flash_erase(0x10000, 0x1000);
    qspi_flash_erase(0x20000, 0x1000);
    //qspi_flash_chip_erase();
    printf("QSPI flash erase all complete...\r\n");


    /* qspi flash program */
    qspi_flash_write(0x20200, tx_buffer, BUF_SIZE);
    printf("QSPI flash reading...\r\n");
    qspi_flash_erase(0x10000, 0x2000);

    //qspi_flash_chip_erase();
    /* read data by memory map mode */
    qspi_flash_read(0x20200, rx_buffer, BUF_SIZE);

    if(!memcmp(rx_buffer, tx_buffer, BUF_SIZE)) {
        printf("SPI FLASH WRITE AND READ TEST SUCCESS!\r\n");
    } else {
        printf("SPI FLASH WRITE AND READ TEST ERROR!\r\n");
    }

    memset(rx_buffer, 0, BUF_SIZE);
    // erase again
    qspi_flash_erase(0x20000, 0x2000);
    /* read data by memory map mode */
    qspi_flash_read(0x20200, rx_buffer, BUF_SIZE);

    if(!memcmp(rx_buffer, tx_buffer, BUF_SIZE)) {
        printf("erase before read ERROR!\r\n");
    } else {
        printf("erase before read SUCCESS!\r\n");
    }

    qspi_flash_write(0x20200, tx_buffer, BUF_SIZE);

    memset(rx_buffer, 0, BUF_SIZE);
    qspi_flash_read(0x20200, rx_buffer, BUF_SIZE);
    if(!memcmp(rx_buffer, tx_buffer, BUF_SIZE)) {
        printf("SPI FLASH WRITE AND READ TEST 2 SUCCESS!\r\n");
    } else {
        printf("SPI FLASH WRITE AND READ TEST 2 ERROR!\r\n");
    }


    /* qspi flash program */
    qspi_flash_write(0x20501, tx_buffer, BUF_SIZE);
    printf("QSPI flash reading...\r\n");
    /* read data by memory map mode */
    qspi_flash_read(0x20501, rx_buffer, BUF_SIZE);
    if(!memcmp(rx_buffer, tx_buffer, BUF_SIZE)) {
        printf("SPI FLASH WRITE AND READ TEST 3 SUCCESS!\r\n");
    } else {
        printf("SPI FLASH WRITE AND READ TEST 3 ERROR!\r\n");
    }

    qspi_flash_read(0x40000, rx_buffer_sector, 4096);
    qspi_flash_read(0x42000, rx_buffer_sector, 4096);
    printf("QSPI flash read complete...\r\n");
    qspi_flash_erase(0x7000, 0x1000);
    printf("QSPI flash erase complete4...\r\n");

#if (QSPI_FLASH_MEM == 16)
    // erase again
    qspi_flash_erase(0x300000, 0x2000);

    memset(rx_buffer, 0, BUF_SIZE);
    qspi_flash_write(0x300000, tx_buffer, BUF_SIZE);

    /* read data by memory map mode */
    qspi_flash_read(0x300000, rx_buffer, BUF_SIZE);

    if(!memcmp(rx_buffer, tx_buffer, BUF_SIZE)) {
        printf("SPI 16M FLASH WRITE AND READ TEST SUCCESS!\r\n");
    } else {
        printf("SPI 16M FLASH WRITE AND READ TEST ERROR!\r\n");
    }

#endif

}
#else
void flash_test(void)
{

}
#endif
