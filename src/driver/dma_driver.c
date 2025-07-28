/**
 ******************************************************************************
 * @file    dma_driver.c
 * @author  IC Simulator Team
 * @brief   DMA HAL Driver Source File (CMSIS Style)
 * @version V1.0.0
 * @date    26-July-2025
 ******************************************************************************
 * @attention
 *
 * Copyright (c) 2025 IC Simulator Project.
 * All rights reserved.
 *
 * This software is licensed under terms that can be found in the LICENSE file
 * in the root directory of this software component.
 * If no LICENSE file comes with this software, it is provided AS-IS.
 *
 ******************************************************************************
 */

/* Includes ------------------------------------------------------------------*/
#define _BSD_SOURCE
#define _DEFAULT_SOURCE

#include "dma_driver.h"
#include "../common/register_map.h"
#include "../sim_interface/interrupt_manager.h"
#include <stdio.h>
#include <string.h>
#include <stdbool.h>

#ifdef _WIN32
    #include <windows.h>
    #define usleep(us) Sleep((us)/1000)
#else
    #include <unistd.h>
#endif

/** @addtogroup IC_Simulator_Driver
  * @{
  */

/** @defgroup DMA DMA
  * @brief DMA HAL module driver
  * @{
  */

/* Private typedef -----------------------------------------------------------*/
/* Private define ------------------------------------------------------------*/

/** @defgroup DMA_Private_Constants DMA Private Constants
  * @{
  */
#define DMA_TIMEOUT_VALUE                5000U          /* 5 seconds timeout */

/* DMA Control Register Bit Definitions */
#define DMA_CTRL_ENABLE         (1 << 0)
#define DMA_CTRL_START          (1 << 1)
#define DMA_CTRL_ABORT          (1 << 2)

/* DMA Status Register Bit Definitions */
#define DMA_STATUS_BUSY         (1 << 0)
#define DMA_STATUS_DONE         (1 << 1)
#define DMA_STATUS_ERROR        (1 << 2)

/* DMA Configuration Register Bit Definitions */
#define DMA_CONFIG_MEM_TO_MEM   (0 << 0)
#define DMA_CONFIG_MEM_TO_PER   (1 << 0)
#define DMA_CONFIG_PER_TO_MEM   (2 << 0)
#define DMA_CONFIG_PER_TO_PER   (3 << 0)
#define DMA_CONFIG_INC_SRC      (1 << 4)
#define DMA_CONFIG_INC_DST      (1 << 5)
#define DMA_CONFIG_INT_ENABLE   (1 << 8)

/* HAL common definitions */
#ifndef RESET
#define RESET                   0U
#endif
#ifndef SET  
#define SET                     1U
#endif

/**
  * @}
  */

/* Private macro -------------------------------------------------------------*/
/* Private variables ---------------------------------------------------------*/

/** @defgroup DMA_Private_Variables DMA Private Variables
  * @{
  */

/* DMA register access macros for legacy support */
#define DMA_GLOBAL_CTRL_PTR     ((volatile uint32_t*)DMA_GLOBAL_CTRL_REG)
#define DMA_GLOBAL_STATUS_PTR   ((volatile uint32_t*)DMA_GLOBAL_STATUS_REG)
#define DMA_INT_STATUS_PTR      ((volatile uint32_t*)DMA_INT_STATUS_REG)
#define DMA_INT_CLEAR_PTR       ((volatile uint32_t*)DMA_INT_CLEAR_REG)

#define DMA_CH_CTRL_PTR(ch)     ((volatile uint32_t*)DMA_CH_CTRL_REG(ch))
#define DMA_CH_STATUS_PTR(ch)   ((volatile uint32_t*)DMA_CH_STATUS_REG(ch))
#define DMA_CH_SRC_PTR(ch)      ((volatile uint32_t*)DMA_CH_SRC_REG(ch))
#define DMA_CH_DST_PTR(ch)      ((volatile uint32_t*)DMA_CH_DST_REG(ch))
#define DMA_CH_SIZE_PTR(ch)     ((volatile uint32_t*)DMA_CH_SIZE_REG(ch))
#define DMA_CH_CONFIG_PTR(ch)   ((volatile uint32_t*)DMA_CH_CONFIG_REG(ch))

/* Legacy DMA channel management structure */
typedef struct {
    bool allocated;             /*!< Whether channel is allocated */
    bool busy;                  /*!< Whether channel is busy */
    dma_callback_t callback;    /*!< Transfer completion callback */
    DMA_HandleTypeDef *hdma;    /*!< Associated HAL handle */
} dma_channel_info_t;

/* DMA controller global state */
static dma_channel_info_t g_dma_channels[DMA_MAX_CHANNELS];
static bool g_dma_initialized = false;

/* HAL DMA handles for all channels */
static DMA_HandleTypeDef g_dma_handles[DMA_MAX_CHANNELS];

/**
  * @}
  */

/* Private function prototypes -----------------------------------------------*/

/** @defgroup DMA_Private_Functions DMA Private Functions
  * @{
  */

static HAL_StatusTypeDef DMA_SetConfig(DMA_HandleTypeDef *hdma, uint32_t SrcAddress, uint32_t DstAddress, uint32_t DataLength);
static void DMA_ConfigureTransfer(DMA_HandleTypeDef *hdma);
static uint32_t HAL_GetTick(void);

/**
  * @}
  */

/* Exported functions --------------------------------------------------------*/

/** @defgroup DMA_Exported_Functions DMA Exported Functions
  * @{
  */

/** @defgroup DMA_Exported_Functions_Group1 Initialization and de-initialization functions
  * @brief    Initialization and Configuration functions
  * @{
  */

/**
  * @brief  Initialize the DMA according to the specified parameters
  *         in the DMA_InitTypeDef and initialize the associated handle.
  * @param  hdma Pointer to a DMA_HandleTypeDef structure that contains
  *              the configuration information for the specified DMA Channel.
  * @retval HAL status
  */
HAL_StatusTypeDef HAL_DMA_Init(DMA_HandleTypeDef *hdma)
{
    uint32_t tmp;
    uint32_t tickstart;

    /* Check the DMA handle allocation */
    if (hdma == NULL) {
        return HAL_ERROR;
    }

    /* Check the parameters */
    if (hdma->Instance == NULL) {
        return HAL_ERROR;
    }

    /* Allocate lock resource */
    hdma->Lock = HAL_UNLOCKED;

    /* Change DMA peripheral state */
    hdma->State = HAL_DMA_STATE_BUSY;

    /* Initialize the error code */
    hdma->ErrorCode = HAL_DMA_ERROR_NONE;

    /* Configure the channel */
    DMA_ConfigureTransfer(hdma);

    /* Initialize the DMA state */
    hdma->State = HAL_DMA_STATE_READY;

    printf("[%s:%s] DMA HAL channel %u initialization completed\n", 
           __FILE__, __func__, hdma->ChannelIndex);

    return HAL_OK;
}

/**
  * @brief  DeInitialize the DMA peripheral.
  * @param  hdma Pointer to a DMA_HandleTypeDef structure that contains
  *              the configuration information for the specified DMA Channel.
  * @retval HAL status
  */
HAL_StatusTypeDef HAL_DMA_DeInit(DMA_HandleTypeDef *hdma)
{
    /* Check the DMA handle allocation */
    if (hdma == NULL) {
        return HAL_ERROR;
    }

    /* Check the parameters */
    if (hdma->Instance == NULL) {
        return HAL_ERROR;
    }

    /* Disable the selected DMA Channel */
    __HAL_DMA_DISABLE(hdma);

    /* Reset DMA Channel control register */
    hdma->Instance->Configuration = 0U;
    hdma->Instance->SrcAddr = 0U;
    hdma->Instance->DestAddr = 0U;
    hdma->Instance->Control = 0U;

    /* Clean callbacks */
    hdma->XferCpltCallback = NULL;
    hdma->XferHalfCpltCallback = NULL;
    hdma->XferErrorCallback = NULL;
    hdma->XferAbortCallback = NULL;

    /* Initialize the error code */
    hdma->ErrorCode = HAL_DMA_ERROR_NONE;

    /* Initialize the DMA state */
    hdma->State = HAL_DMA_STATE_RESET;

    /* Release Lock */
    hdma->Lock = HAL_UNLOCKED;

    printf("[%s:%s] DMA HAL channel %u deinitialization completed\n", 
           __FILE__, __func__, hdma->ChannelIndex);

    return HAL_OK;
}

/**
  * @}
  */

/** @defgroup DMA_Exported_Functions_Group2 Input and Output operation functions
  * @brief   Input and Output operation functions
  * @{
  */

/**
  * @brief  Start the DMA Transfer.
  * @param  hdma       Pointer to a DMA_HandleTypeDef structure that contains
  *                    the configuration information for the specified DMA Channel.
  * @param  SrcAddress The source memory Buffer address
  * @param  DstAddress The destination memory Buffer address
  * @param  DataLength The length of data to be transferred from source to destination
  * @retval HAL status
  */
HAL_StatusTypeDef HAL_DMA_Start(DMA_HandleTypeDef *hdma, uint32_t SrcAddress, uint32_t DstAddress, uint32_t DataLength)
{
    HAL_StatusTypeDef status = HAL_OK;

    /* Check the parameters */
    if (hdma == NULL) {
        return HAL_ERROR;
    }

    /* Process locked */
    __HAL_LOCK(hdma);

    if (HAL_DMA_STATE_READY == hdma->State) {
        /* Change DMA peripheral state */
        hdma->State = HAL_DMA_STATE_BUSY;
        hdma->ErrorCode = HAL_DMA_ERROR_NONE;

        /* Configure the source, destination address and the data length */
        DMA_SetConfig(hdma, SrcAddress, DstAddress, DataLength);

        /* Enable the DMA Channel */
        __HAL_DMA_ENABLE(hdma);
    }
    else {
        /* Process unlocked */
        __HAL_UNLOCK(hdma);

        /* Return error status */
        status = HAL_BUSY;
    }

    return status;
}

/**
  * @brief  Start the DMA Transfer with interrupt enabled.
  * @param  hdma       Pointer to a DMA_HandleTypeDef structure that contains
  *                    the configuration information for the specified DMA Channel.
  * @param  SrcAddress The source memory Buffer address
  * @param  DstAddress The destination memory Buffer address
  * @param  DataLength The length of data to be transferred from source to destination
  * @retval HAL status
  */
HAL_StatusTypeDef HAL_DMA_Start_IT(DMA_HandleTypeDef *hdma, uint32_t SrcAddress, uint32_t DstAddress, uint32_t DataLength)
{
    HAL_StatusTypeDef status = HAL_OK;

    /* Check the parameters */
    if (hdma == NULL) {
        return HAL_ERROR;
    }

    /* Process locked */
    __HAL_LOCK(hdma);

    if (HAL_DMA_STATE_READY == hdma->State) {
        /* Change DMA peripheral state */
        hdma->State = HAL_DMA_STATE_BUSY;
        hdma->ErrorCode = HAL_DMA_ERROR_NONE;

        /* Configure the source, destination address and the data length */
        DMA_SetConfig(hdma, SrcAddress, DstAddress, DataLength);

        /* Enable the transfer complete interrupt */
        __HAL_DMA_ENABLE_IT(hdma, DMA_TRANSFER_COMPLETE_INT);

        /* Enable the transfer error interrupt */
        __HAL_DMA_ENABLE_IT(hdma, DMA_TRANSFER_ERROR_INT);

        /* Enable the DMA Channel */
        __HAL_DMA_ENABLE(hdma);
    }
    else {
        /* Process unlocked */
        __HAL_UNLOCK(hdma);

        /* Return error status */
        status = HAL_BUSY;
    }

    return status;
}

/**
  * @brief  Abort the DMA Transfer.
  * @param  hdma  Pointer to a DMA_HandleTypeDef structure that contains
  *               the configuration information for the specified DMA Channel.
  * @retval HAL status
  */
HAL_StatusTypeDef HAL_DMA_Abort(DMA_HandleTypeDef *hdma)
{
    uint32_t tickstart;

    if (hdma->State != HAL_DMA_STATE_BUSY) {
        hdma->ErrorCode = HAL_DMA_ERROR_NO_XFER;
        return HAL_ERROR;
    }
    else {
        /* Disable the channel */
        __HAL_DMA_DISABLE(hdma);

        /* Get tick */
        tickstart = HAL_GetTick();

        /* Check if the DMA Channel effectively disabled */
        while ((hdma->Instance->Configuration & DMA_CHANNEL_ENABLE) != 0U) {
            /* Check for the Timeout */
            if ((HAL_GetTick() - tickstart) > DMA_TIMEOUT_VALUE) {
                /* Update error code */
                hdma->ErrorCode = HAL_DMA_ERROR_TIMEOUT;

                /* Change the DMA state */
                hdma->State = HAL_DMA_STATE_TIMEOUT;

                return HAL_TIMEOUT;
            }
        }

        /* Change the DMA state */
        hdma->State = HAL_DMA_STATE_READY;

        /* Process Unlocked */
        __HAL_UNLOCK(hdma);
    }

    return HAL_OK;
}

/**
  * @brief  Abort the DMA Transfer in Interrupt mode.
  * @param  hdma  Pointer to a DMA_HandleTypeDef structure that contains
  *               the configuration information for the specified DMA Channel.
  * @retval HAL status
  */
HAL_StatusTypeDef HAL_DMA_Abort_IT(DMA_HandleTypeDef *hdma)
{
    HAL_StatusTypeDef status = HAL_OK;

    if (HAL_DMA_STATE_BUSY != hdma->State) {
        /* No transfer ongoing */
        hdma->ErrorCode = HAL_DMA_ERROR_NO_XFER;

        status = HAL_ERROR;
    }
    else {
        /* Disable the channel */
        __HAL_DMA_DISABLE(hdma);

        /* Disable all the transfer interrupts */
        __HAL_DMA_DISABLE_IT(hdma, (DMA_TRANSFER_COMPLETE_INT | DMA_TRANSFER_ERROR_INT));

        /* Change the DMA state */
        hdma->State = HAL_DMA_STATE_READY;

        /* Process Unlocked */
        __HAL_UNLOCK(hdma);

        /* Call User Abort callback */
        if (hdma->XferAbortCallback != NULL) {
            hdma->XferAbortCallback(hdma);
        }
    }

    return status;
}

/**
  * @brief  Polling for transfer complete.
  * @param  hdma          Pointer to a DMA_HandleTypeDef structure that contains
  *                       the configuration information for the specified DMA Channel.
  * @param  CompleteLevel Specifies the DMA level complete.
  * @param  Timeout       Timeout duration.
  * @retval HAL status
  */
HAL_StatusTypeDef HAL_DMA_PollForTransfer(DMA_HandleTypeDef *hdma, HAL_DMA_LevelCompleteTypeDef CompleteLevel, uint32_t Timeout)
{
    uint32_t temp;
    uint32_t tickstart;

    if (HAL_DMA_STATE_BUSY != hdma->State) {
        /* No transfer ongoing */
        hdma->ErrorCode = HAL_DMA_ERROR_NO_XFER;
        return HAL_ERROR;
    }

    /* Polling mode not supported in circular mode */
    if ((hdma->Instance->Configuration & DMA_CHANNEL_ENABLE) != 0U) {
        hdma->ErrorCode = HAL_DMA_ERROR_NOT_SUPPORTED;
        return HAL_ERROR;
    }

    /* Get the level transfer complete flag */
    if (HAL_DMA_FULL_TRANSFER == CompleteLevel) {
        /* Transfer Complete flag */
        temp = DMA_TRANSFER_COMPLETE_INT;
    }
    else {
        /* Half Transfer Complete flag */
        temp = DMA_TRANSFER_COMPLETE_INT;  /* Simplified - no half complete in this implementation */
    }

    /* Get tick */
    tickstart = HAL_GetTick();

    while (__HAL_DMA_GET_FLAG(hdma, temp) == 0U) {
        if ((__HAL_DMA_GET_FLAG(hdma, DMA_TRANSFER_ERROR_INT) != 0U)) {
            /* When a DMA transfer error occurs */
            /* A hardware clear of its EN bits is performed */
            /* Clear all flags */
            __HAL_DMA_CLEAR_FLAG(hdma, DMA_TRANSFER_ERROR_INT);

            /* Update error code */
            hdma->ErrorCode = HAL_DMA_ERROR_TE;

            /* Change the DMA state */
            hdma->State = HAL_DMA_STATE_READY;

            /* Process Unlocked */
            __HAL_UNLOCK(hdma);

            return HAL_ERROR;
        }
        /* Check for the Timeout */
        if (Timeout != HAL_MAX_DELAY) {
            if (((HAL_GetTick() - tickstart) > Timeout) || (Timeout == 0U)) {
                /* Update error code */
                hdma->ErrorCode = HAL_DMA_ERROR_TIMEOUT;

                /* Change the DMA state */
                hdma->State = HAL_DMA_STATE_TIMEOUT;

                /* Process Unlocked */
                __HAL_UNLOCK(hdma);

                return HAL_TIMEOUT;
            }
        }
    }

    /* Clear the transfer complete flag */
    __HAL_DMA_CLEAR_FLAG(hdma, temp);

    /* The selected channel is disabled */
    hdma->State = HAL_DMA_STATE_READY;

    /* Process unlocked */
    __HAL_UNLOCK(hdma);

    return HAL_OK;
}

/**
  * @brief  Handle DMA interrupt request.
  * @param  hdma Pointer to a DMA_HandleTypeDef structure that contains
  *              the configuration information for the specified DMA Channel.
  * @retval None
  */
void HAL_DMA_IRQHandler(DMA_HandleTypeDef *hdma)
{
    uint32_t flag_it = hdma->Instance->Configuration;
    uint32_t source_it = flag_it;

    /* Transfer Error Interrupt management ***************************************/
    if ((RESET != (flag_it & DMA_TRANSFER_ERROR_INT)) && (RESET != (source_it & DMA_TRANSFER_ERROR_INT))) {
        /* Disable the transfer error interrupt */
        __HAL_DMA_DISABLE_IT(hdma, DMA_TRANSFER_ERROR_INT);

        /* Clear the transfer error flag */
        __HAL_DMA_CLEAR_FLAG(hdma, DMA_TRANSFER_ERROR_INT);

        /* Update error code */
        hdma->ErrorCode = HAL_DMA_ERROR_TE;

        /* Change the DMA state */
        hdma->State = HAL_DMA_STATE_READY;

        /* Process Unlocked */
        __HAL_UNLOCK(hdma);

        if (hdma->XferErrorCallback != NULL) {
            /* Transfer error callback */
            hdma->XferErrorCallback(hdma);
        }
    }

    /* Transfer Complete Interrupt management ***********************************/
    if ((RESET != (flag_it & DMA_TRANSFER_COMPLETE_INT)) && (RESET != (source_it & DMA_TRANSFER_COMPLETE_INT))) {
        /* Disable the transfer complete interrupt */
        __HAL_DMA_DISABLE_IT(hdma, DMA_TRANSFER_COMPLETE_INT);

        /* Clear the transfer complete flag */
        __HAL_DMA_CLEAR_FLAG(hdma, DMA_TRANSFER_COMPLETE_INT);

        /* Update error code */
        hdma->ErrorCode = HAL_DMA_ERROR_NONE;

        /* Change the DMA state */
        hdma->State = HAL_DMA_STATE_READY;

        /* Process Unlocked */
        __HAL_UNLOCK(hdma);

        if (hdma->XferCpltCallback != NULL) {
            /* Transfer complete callback */
            hdma->XferCpltCallback(hdma);
        }
    }
}

/**
  * @}
  */

/** @defgroup DMA_Exported_Functions_Group3 Peripheral State and Error functions
  * @brief    Peripheral State and Error functions
  * @{
  */

/**
  * @brief  Return the DMA handle state.
  * @param  hdma Pointer to a DMA_HandleTypeDef structure that contains
  *              the configuration information for the specified DMA Channel.
  * @retval HAL state
  */
HAL_DMA_StateTypeDef HAL_DMA_GetState(DMA_HandleTypeDef *hdma)
{
    /* Return DMA handle state */
    return hdma->State;
}

/**
  * @brief  Return the DMA error code.
  * @param  hdma Pointer to a DMA_HandleTypeDef structure that contains
  *              the configuration information for the specified DMA Channel.
  * @retval DMA Error Code
  */
uint32_t HAL_DMA_GetError(DMA_HandleTypeDef *hdma)
{
    return hdma->ErrorCode;
}

/**
  * @}
  */

/* Private functions ---------------------------------------------------------*/

/** @defgroup DMA_Private_Functions DMA Private Functions
  * @{
  */

/**
  * @brief  Set the DMA Transfer parameter.
  * @param  hdma       Pointer to a DMA_HandleTypeDef structure that contains
  *                    the configuration information for the specified DMA Channel.
  * @param  SrcAddress The source memory Buffer address
  * @param  DstAddress The destination memory Buffer address
  * @param  DataLength The length of data to be transferred from source to destination
  * @retval HAL status
  */
static HAL_StatusTypeDef DMA_SetConfig(DMA_HandleTypeDef *hdma, uint32_t SrcAddress, uint32_t DstAddress, uint32_t DataLength)
{
    /* Configure DMA Channel source address */
    hdma->Instance->SrcAddr = SrcAddress;

    /* Configure DMA Channel destination address */
    hdma->Instance->DestAddr = DstAddress;

    /* Configure DMA Channel data length */
    hdma->Instance->Control = DataLength;

    return HAL_OK;
}

/**
  * @brief  Configure the DMA transfer parameters.
  * @param  hdma Pointer to a DMA_HandleTypeDef structure that contains
  *              the configuration information for the specified DMA Channel.
  * @retval None
  */
static void DMA_ConfigureTransfer(DMA_HandleTypeDef *hdma)
{
    uint32_t tmp;

    /* Prepare the DMA Channel configuration */
    tmp = hdma->Init.Direction | hdma->Init.PeriphInc | hdma->Init.MemInc |
          hdma->Init.PeriphDataAlignment | hdma->Init.MemDataAlignment |
          hdma->Init.Mode | hdma->Init.Priority;

    /* Write to DMA Channel control register */
    hdma->Instance->Configuration = tmp;
}

/* HAL_GetTick simulation for timeout handling */
static uint32_t HAL_GetTick(void)
{
    static uint32_t tick_counter = 0;
    tick_counter += 1;  /* Simplified tick simulation */
    return tick_counter;
}

/**
  * @}
  */

/* ============================================================================ */
/* ===================== LEGACY COMPATIBILITY FUNCTIONS ===================== */
/* ============================================================================ */

/** @defgroup DMA_Legacy_Functions DMA Legacy Functions
  * @brief Legacy functions for backward compatibility
  * @{
  */

/**
  * @brief  Legacy DMA interrupt handler
  * @retval None
  */
void dma_interrupt_handler(void) {
    printf("[%s:%s] DMA interrupt received\n", __FILE__, __func__);
    
    uint32_t int_status = *DMA_INT_STATUS_PTR;
    
    for (uint8_t ch = 0; ch < DMA_MAX_CHANNELS; ch++) {
        if (int_status & (1 << ch)) {
            printf("[%s:%s] DMA channel %d interrupt\n", __FILE__, __func__, ch);
            
            /* Call HAL IRQ handler if handle is available */
            if (g_dma_channels[ch].hdma != NULL) {
                HAL_DMA_IRQHandler(g_dma_channels[ch].hdma);
            }
            
            /* Legacy callback handling */
            uint32_t ch_status = *DMA_CH_STATUS_PTR(ch);
            dma_channel_status_t status;
            
            if (ch_status & DMA_STATUS_ERROR) {
                status = DMA_CH_ERROR;
                printf("[%s:%s] DMA channel %d error\n", __FILE__, __func__, ch);
            } else if (ch_status & DMA_STATUS_DONE) {
                status = DMA_CH_DONE;
                printf("[%s:%s] DMA channel %d transfer complete\n", __FILE__, __func__, ch);
            } else {
                status = DMA_CH_BUSY;
            }
            
            /* Update channel state */
            g_dma_channels[ch].busy = (status == DMA_CH_BUSY);
            
            /* Call legacy callback function */
            if (g_dma_channels[ch].callback) {
                g_dma_channels[ch].callback(ch, status);
            }
            
            /* Clear interrupt flag */
            *DMA_INT_CLEAR_PTR |= (1 << ch);
        }
    }
}

/**
  * @brief  Legacy DMA initialization
  * @retval 0 on success, -1 on error
  */
int dma_init(void) {
    printf("[%s:%s] DMA driver initializing...\n", __FILE__, __func__);
    
    if (g_dma_initialized) {
        printf("[%s:%s] DMA already initialized\n", __FILE__, __func__);
        return 0;
    }
    
    /* Initialize channel management structure */
    memset(g_dma_channels, 0, sizeof(g_dma_channels));
    memset(g_dma_handles, 0, sizeof(g_dma_handles));
    
    /* Initialize HAL handles */
    for (uint8_t ch = 0; ch < DMA_MAX_CHANNELS; ch++) {
        g_dma_handles[ch].Instance = (DMA_Channel_TypeDef *)((uint32_t)DMA0_Channel0_BASE + (ch * 0x20));
        g_dma_handles[ch].ChannelIndex = ch;
        g_dma_handles[ch].State = HAL_DMA_STATE_RESET;
        
        /* Set default configuration */
        g_dma_handles[ch].Init.Direction = DMA_MEMORY_TO_MEMORY;
        g_dma_handles[ch].Init.PeriphInc = DMA_PINC_ENABLE;
        g_dma_handles[ch].Init.MemInc = DMA_MINC_ENABLE;
        g_dma_handles[ch].Init.PeriphDataAlignment = DMA_PDATAALIGN_BYTE;
        g_dma_handles[ch].Init.MemDataAlignment = DMA_MDATAALIGN_BYTE;
        g_dma_handles[ch].Init.Mode = DMA_NORMAL;
        g_dma_handles[ch].Init.Priority = DMA_PRIORITY_LOW;
        
        /* Initialize HAL DMA */
        HAL_DMA_Init(&g_dma_handles[ch]);
    }
    
    /* Register DMA interrupt handler (IRQ 8) */
    if (register_interrupt_handler(8, dma_interrupt_handler) != 0) {
        printf("[%s:%s] Failed to register DMA interrupt handler\n", __FILE__, __func__);
        return -1;
    }
    
    /* Enable DMA controller */
    *DMA_GLOBAL_CTRL_PTR = DMA_CTRL_ENABLE;
    
    /* Clear all interrupt flags */
    *DMA_INT_CLEAR_PTR = 0xFFFF;
    
    g_dma_initialized = true;
    printf("[%s:%s] DMA driver initialized, %d channels available\n", 
           __FILE__, __func__, DMA_MAX_CHANNELS);
    
    return 0;
}

/**
  * @brief  Legacy DMA cleanup
  * @retval None
  */
void dma_cleanup(void) {
    printf("[%s:%s] DMA driver cleanup...\n", __FILE__, __func__);
    
    if (!g_dma_initialized) {
        return;
    }
    
    /* Stop all channels */
    for (uint8_t ch = 0; ch < DMA_MAX_CHANNELS; ch++) {
        if (g_dma_channels[ch].allocated) {
            dma_stop_transfer(ch);
            dma_free_channel(ch);
        }
        
        /* DeInitialize HAL DMA */
        HAL_DMA_DeInit(&g_dma_handles[ch]);
    }
    
    /* Disable DMA controller */
    *DMA_GLOBAL_CTRL_PTR = 0;
    
    g_dma_initialized = false;
    printf("[%s:%s] DMA driver cleanup completed\n", __FILE__, __func__);
}

/**
  * @brief  Allocate DMA channel
  * @retval Channel index on success, -1 on error
  */
int dma_allocate_channel(void) {
    if (!g_dma_initialized) {
        printf("[%s:%s] DMA not initialized\n", __FILE__, __func__);
        return -1;
    }
    
    for (uint8_t ch = 0; ch < DMA_MAX_CHANNELS; ch++) {
        if (!g_dma_channels[ch].allocated) {
            g_dma_channels[ch].allocated = true;
            g_dma_channels[ch].busy = false;
            g_dma_channels[ch].callback = NULL;
            g_dma_channels[ch].hdma = &g_dma_handles[ch];
            
            printf("[%s:%s] Allocated DMA channel %d\n", __FILE__, __func__, ch);
            return ch;
        }
    }
    
    printf("[%s:%s] No available DMA channels\n", __FILE__, __func__);
    return -1;
}

/**
  * @brief  Free DMA channel
  * @param  channel Channel to free
  * @retval 0 on success, -1 on error
  */
int dma_free_channel(uint8_t channel) {
    if (channel >= DMA_MAX_CHANNELS) {
        printf("[%s:%s] Invalid channel %d\n", __FILE__, __func__, channel);
        return -1;
    }
    
    if (!g_dma_channels[channel].allocated) {
        printf("[%s:%s] Channel %d not allocated\n", __FILE__, __func__, channel);
        return -1;
    }
    
    /* Stop transfer */
    dma_stop_transfer(channel);
    
    /* Reset channel state */
    g_dma_channels[channel].allocated = false;
    g_dma_channels[channel].busy = false;
    g_dma_channels[channel].callback = NULL;
    g_dma_channels[channel].hdma = NULL;
    
    printf("[%s:%s] Freed DMA channel %d\n", __FILE__, __func__, channel);
    return 0;
}

/**
  * @brief  Check if channel is available
  * @param  channel Channel to check
  * @retval true if available, false otherwise
  */
bool dma_is_channel_available(uint8_t channel) {
    if (channel >= DMA_MAX_CHANNELS) {
        return false;
    }
    return !g_dma_channels[channel].allocated;
}

/**
  * @brief  Configure DMA channel
  * @param  channel Channel to configure
  * @param  config Configuration parameters
  * @retval 0 on success, -1 on error
  */
int dma_configure_channel(uint8_t channel, const dma_config_t *config) {
    if (channel >= DMA_MAX_CHANNELS || !config) {
        printf("[%s:%s] Invalid parameters\n", __FILE__, __func__);
        return -1;
    }
    
    if (!g_dma_channels[channel].allocated) {
        printf("[%s:%s] Channel %d not allocated\n", __FILE__, __func__, channel);
        return -1;
    }
    
    if (g_dma_channels[channel].busy) {
        printf("[%s:%s] Channel %d is busy\n", __FILE__, __func__, channel);
        return -1;
    }
    
    /* Configure HAL handle */
    DMA_HandleTypeDef *hdma = g_dma_channels[channel].hdma;
    
    /* Set direction based on type */
    switch (config->type) {
        case DMA_TRANSFER_MEM_TO_MEM:
            hdma->Init.Direction = DMA_MEMORY_TO_MEMORY;
            break;
        case DMA_TRANSFER_MEM_TO_PER:
            hdma->Init.Direction = DMA_MEMORY_TO_PERIPH;
            break;
        case DMA_TRANSFER_PER_TO_MEM:
            hdma->Init.Direction = DMA_PERIPH_TO_MEMORY;
            break;
        case DMA_TRANSFER_PER_TO_PER:
            hdma->Init.Direction = DMA_PERIPH_TO_PERIPH;
            break;
        default:
            printf("[%s:%s] Invalid transfer type\n", __FILE__, __func__);
            return -1;
    }
    
    /* Set increment modes */
    hdma->Init.MemInc = config->inc_src ? DMA_MINC_ENABLE : DMA_MINC_DISABLE;
    hdma->Init.PeriphInc = config->inc_dst ? DMA_PINC_ENABLE : DMA_PINC_DISABLE;
    
    /* Legacy register configuration for compatibility */
    *DMA_CH_SRC_PTR(channel) = config->src_addr;
    *DMA_CH_DST_PTR(channel) = config->dst_addr;
    *DMA_CH_SIZE_PTR(channel) = config->size;
    
    /* Configure transfer parameters */
    uint32_t config_reg = 0;
    config_reg |= (config->type & 0x3);
    if (config->inc_src) config_reg |= DMA_CONFIG_INC_SRC;
    if (config->inc_dst) config_reg |= DMA_CONFIG_INC_DST;
    if (config->interrupt_enable) config_reg |= DMA_CONFIG_INT_ENABLE;
    
    *DMA_CH_CONFIG_PTR(channel) = config_reg;
    
    printf("[%s:%s] Configured DMA channel %d: src=0x%08X, dst=0x%08X, size=%d\n",
           __FILE__, __func__, channel, config->src_addr, config->dst_addr, config->size);
    
    return 0;
}

/**
  * @brief  Start DMA transfer
  * @param  channel Channel to start
  * @retval 0 on success, -1 on error
  */
int dma_start_transfer(uint8_t channel) {
    if (channel >= DMA_MAX_CHANNELS) {
        printf("[%s:%s] Invalid channel %d\n", __FILE__, __func__, channel);
        return -1;
    }
    
    if (!g_dma_channels[channel].allocated) {
        printf("[%s:%s] Channel %d not allocated\n", __FILE__, __func__, channel);
        return -1;
    }
    
    /* Enable channel and start transfer */
    *DMA_CH_CTRL_PTR(channel) = DMA_CTRL_ENABLE | DMA_CTRL_START;
    g_dma_channels[channel].busy = true;
    
    printf("[%s:%s] Started DMA transfer on channel %d\n", __FILE__, __func__, channel);
    return 0;
}

/**
  * @brief  Stop DMA transfer
  * @param  channel Channel to stop
  * @retval 0 on success, -1 on error
  */
int dma_stop_transfer(uint8_t channel) {
    if (channel >= DMA_MAX_CHANNELS) {
        printf("[%s:%s] Invalid channel %d\n", __FILE__, __func__, channel);
        return -1;
    }
    
    /* Abort transfer */
    *DMA_CH_CTRL_PTR(channel) = DMA_CTRL_ABORT;
    g_dma_channels[channel].busy = false;
    
    /* Also abort via HAL */
    if (g_dma_channels[channel].hdma != NULL) {
        HAL_DMA_Abort(g_dma_channels[channel].hdma);
    }
    
    printf("[%s:%s] Stopped DMA transfer on channel %d\n", __FILE__, __func__, channel);
    return 0;
}

/**
  * @brief  Get channel status
  * @param  channel Channel to check
  * @retval Channel status
  */
dma_channel_status_t dma_get_channel_status(uint8_t channel) {
    if (channel >= DMA_MAX_CHANNELS) {
        return DMA_CH_ERROR;
    }
    
    if (!g_dma_channels[channel].allocated) {
        return DMA_CH_IDLE;
    }
    
    /* Check HAL state first */
    if (g_dma_channels[channel].hdma != NULL) {
        HAL_DMA_StateTypeDef hal_state = HAL_DMA_GetState(g_dma_channels[channel].hdma);
        switch (hal_state) {
            case HAL_DMA_STATE_READY:
                return DMA_CH_DONE;
            case HAL_DMA_STATE_BUSY:
                return DMA_CH_BUSY;
            case HAL_DMA_STATE_ERROR:
                return DMA_CH_ERROR;
            default:
                break;
        }
    }
    
    /* Fallback to legacy register check */
    uint32_t status = *DMA_CH_STATUS_PTR(channel);
    
    if (status & DMA_STATUS_ERROR) {
        return DMA_CH_ERROR;
    } else if (status & DMA_STATUS_DONE) {
        return DMA_CH_DONE;
    } else if (status & DMA_STATUS_BUSY) {
        return DMA_CH_BUSY;
    } else {
        return DMA_CH_IDLE;
    }
}

/**
  * @brief  Asynchronous DMA transfer
  * @param  channel Channel index
  * @param  src Source address
  * @param  dst Destination address
  * @param  size Transfer size
  * @param  type Transfer type
  * @param  callback Completion callback
  * @retval 0 on success, -1 on error
  */
int dma_transfer_async(uint8_t channel, uint32_t src, uint32_t dst, uint32_t size,
                      dma_transfer_type_t type, dma_callback_t callback) {
    dma_config_t config = {
        .src_addr = src,
        .dst_addr = dst,
        .size = size,
        .type = type,
        .inc_src = true,
        .inc_dst = true,
        .interrupt_enable = true
    };
    
    if (dma_configure_channel(channel, &config) != 0) {
        return -1;
    }
    
    g_dma_channels[channel].callback = callback;
    
    /* Use HAL for interrupt-driven transfer */
    if (g_dma_channels[channel].hdma != NULL) {
        HAL_StatusTypeDef status = HAL_DMA_Start_IT(g_dma_channels[channel].hdma, src, dst, size);
        if (status != HAL_OK) {
            printf("[%s:%s] HAL_DMA_Start_IT failed: %d\n", __FILE__, __func__, status);
            return -1;
        }
    }
    
    return dma_start_transfer(channel);
}

/**
  * @brief  Synchronous DMA transfer
  * @param  channel Channel index
  * @param  src Source address
  * @param  dst Destination address
  * @param  size Transfer size
  * @param  type Transfer type
  * @retval 0 on success, -1 on error
  */
int dma_transfer_sync(uint8_t channel, uint32_t src, uint32_t dst, uint32_t size,
                     dma_transfer_type_t type) {
    dma_config_t config = {
        .src_addr = src,
        .dst_addr = dst,
        .size = size,
        .type = type,
        .inc_src = true,
        .inc_dst = true,
        .interrupt_enable = false
    };
    
    if (dma_configure_channel(channel, &config) != 0) {
        return -1;
    }
    
    /* Use HAL for polling-based transfer */
    if (g_dma_channels[channel].hdma != NULL) {
        HAL_StatusTypeDef status = HAL_DMA_Start(g_dma_channels[channel].hdma, src, dst, size);
        if (status != HAL_OK) {
            printf("[%s:%s] HAL_DMA_Start failed: %d\n", __FILE__, __func__, status);
            return -1;
        }
        
        /* Poll for completion */
        status = HAL_DMA_PollForTransfer(g_dma_channels[channel].hdma, HAL_DMA_FULL_TRANSFER, 5000);
        if (status != HAL_OK) {
            printf("[%s:%s] HAL_DMA_PollForTransfer failed: %d\n", __FILE__, __func__, status);
            return -1;
        }
    } else {
        /* Legacy implementation */
        if (dma_start_transfer(channel) != 0) {
            return -1;
        }
        
        /* Wait for transfer complete */
        while (g_dma_channels[channel].busy) {
            dma_channel_status_t status = dma_get_channel_status(channel);
            if (status == DMA_CH_DONE) {
                g_dma_channels[channel].busy = false;
                break;
            } else if (status == DMA_CH_ERROR) {
                printf("[%s:%s] DMA transfer error on channel %d\n", __FILE__, __func__, channel);
                return -1;
            }
            usleep(1000);  /* Wait 1ms */
        }
    }
    
    printf("[%s:%s] DMA sync transfer completed on channel %d\n", __FILE__, __func__, channel);
    return 0;
}

/**
  * @brief  Register callback function
  * @param  channel Channel index
  * @param  callback Callback function
  * @retval 0 on success, -1 on error
  */
int dma_register_callback(uint8_t channel, dma_callback_t callback) {
    if (channel >= DMA_MAX_CHANNELS) {
        printf("[%s:%s] Invalid channel %d\n", __FILE__, __func__, channel);
        return -1;
    }
    
    if (!g_dma_channels[channel].allocated) {
        printf("[%s:%s] Channel %d not allocated\n", __FILE__, __func__, channel);
        return -1;
    }
    
    g_dma_channels[channel].callback = callback;
    return 0;
}

/**
  * @}
  */

/* ============================================================================ */
/* ======================= HAL CALLBACK FUNCTIONS =========================== */
/* ============================================================================ */

/** @defgroup DMA_HAL_Callbacks DMA HAL Callback Functions
  * @brief HAL callback functions that can be overridden by user
  * @{
  */

/**
  * @brief  DMA transfer complete callback.
  * @param  hdma pointer to a DMA_HandleTypeDef structure that contains
  *              the configuration information for the specified DMA Channel.
  * @retval None
  */
__weak void HAL_DMA_XferCpltCallback(DMA_HandleTypeDef *hdma)
{
    /* Prevent unused argument(s) compilation warning */
    UNUSED(hdma);

    /* NOTE: This function should not be modified, when the callback is needed,
             the HAL_DMA_XferCpltCallback could be implemented in the user file
     */
    printf("[%s:%s] DMA transfer complete callback for channel %u\n", 
           __FILE__, __func__, hdma->ChannelIndex);
}

/**
  * @brief  DMA transfer half complete callback.
  * @param  hdma pointer to a DMA_HandleTypeDef structure that contains
  *              the configuration information for the specified DMA Channel.
  * @retval None
  */
__weak void HAL_DMA_XferHalfCpltCallback(DMA_HandleTypeDef *hdma)
{
    /* Prevent unused argument(s) compilation warning */
    UNUSED(hdma);

    /* NOTE: This function should not be modified, when the callback is needed,
             the HAL_DMA_XferHalfCpltCallback could be implemented in the user file
     */
    printf("[%s:%s] DMA transfer half complete callback for channel %u\n", 
           __FILE__, __func__, hdma->ChannelIndex);
}

/**
  * @brief  DMA transfer error callback.
  * @param  hdma pointer to a DMA_HandleTypeDef structure that contains
  *              the configuration information for the specified DMA Channel.
  * @retval None
  */
__weak void HAL_DMA_XferErrorCallback(DMA_HandleTypeDef *hdma)
{
    /* Prevent unused argument(s) compilation warning */
    UNUSED(hdma);

    /* NOTE: This function should not be modified, when the callback is needed,
             the HAL_DMA_XferErrorCallback could be implemented in the user file
     */
    printf("[%s:%s] DMA transfer error callback for channel %u, ErrorCode=0x%08X\n", 
           __FILE__, __func__, hdma->ChannelIndex, hdma->ErrorCode);
}

/**
  * @brief  DMA transfer abort callback.
  * @param  hdma pointer to a DMA_HandleTypeDef structure that contains
  *              the configuration information for the specified DMA Channel.
  * @retval None
  */
__weak void HAL_DMA_XferAbortCallback(DMA_HandleTypeDef *hdma)
{
    /* Prevent unused argument(s) compilation warning */
    UNUSED(hdma);

    /* NOTE: This function should not be modified, when the callback is needed,
             the HAL_DMA_XferAbortCallback could be implemented in the user file
     */
    printf("[%s:%s] DMA transfer abort callback for channel %u\n", 
           __FILE__, __func__, hdma->ChannelIndex);
}

/**
  * @}
  */

/**
  * @}
  */

/**
  * @}
  */

/************************ (C) COPYRIGHT IC Simulator Project *****END OF FILE****/
