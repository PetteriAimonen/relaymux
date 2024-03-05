// Low level driver interface used by STM32 USB driver

#include "usbd_core.h"

PCD_HandleTypeDef g_pcd_handle;

void USB_IRQHandler(void)
{
    HAL_PCD_IRQHandler(&g_pcd_handle);
}

void HAL_PCD_MspInit(PCD_HandleTypeDef *hpcd)
{
    HAL_GPIO_Init(GPIOA, &(GPIO_InitTypeDef){
        .Pin = (GPIO_PIN_11 | GPIO_PIN_12),
        .Mode = GPIO_MODE_ANALOG,
        .Pull = GPIO_NOPULL,
        .Speed = GPIO_SPEED_HIGH,
        .Alternate = GPIO_AF2_USB
    });

    SYSCFG->CFGR1 |= SYSCFG_CFGR1_PA11_PA12_RMP;
    
    __HAL_RCC_USB_FORCE_RESET();
    __HAL_RCC_USB_CLK_ENABLE();
    HAL_Delay(1);
    __HAL_RCC_USB_RELEASE_RESET();
    HAL_Delay(1);
    
    HAL_NVIC_SetPriority(USB_IRQn, 3, 0);
    HAL_NVIC_EnableIRQ(USB_IRQn);
}

void HAL_PCD_MspDeInit(PCD_HandleTypeDef *hpcd)
{
    __HAL_RCC_USB_CLK_DISABLE();
}

USBD_StatusTypeDef  USBD_LL_Init(USBD_HandleTypeDef *pdev)
{
    /* Link the driver to the stack. */
    g_pcd_handle.pData = pdev;
    pdev->pData = &g_pcd_handle;

    g_pcd_handle.Instance = USB;
    g_pcd_handle.Init.speed = PCD_SPEED_FULL;
    g_pcd_handle.Init.dev_endpoints = 3;
    g_pcd_handle.Init.ep0_mps = USB_MAX_EP0_SIZE;
    g_pcd_handle.Init.phy_itface = PCD_PHY_EMBEDDED;
    g_pcd_handle.Init.Sof_enable = ENABLE;
    g_pcd_handle.Init.low_power_enable = DISABLE;
    g_pcd_handle.Init.lpm_enable = DISABLE;
    g_pcd_handle.Init.battery_charging_enable = DISABLE;

    if (HAL_PCD_Init(&g_pcd_handle) != HAL_OK)
    {
        return USBD_FAIL;
    }

    // Allocate memory areas to endpoints
    HAL_PCDEx_PMAConfig(&g_pcd_handle, 0x00, PCD_SNG_BUF, 0x100 + 64 * 0);
    HAL_PCDEx_PMAConfig(&g_pcd_handle, 0x01, PCD_SNG_BUF, 0x100 + 64 * 1);
    HAL_PCDEx_PMAConfig(&g_pcd_handle, 0x02, PCD_SNG_BUF, 0x100 + 64 * 2);
    HAL_PCDEx_PMAConfig(&g_pcd_handle, 0x03, PCD_SNG_BUF, 0x100 + 64 * 3);
    HAL_PCDEx_PMAConfig(&g_pcd_handle, 0x80, PCD_SNG_BUF, 0x100 + 64 * 4);
    HAL_PCDEx_PMAConfig(&g_pcd_handle, 0x81, PCD_SNG_BUF, 0x100 + 64 * 5);
    HAL_PCDEx_PMAConfig(&g_pcd_handle, 0x82, PCD_SNG_BUF, 0x100 + 64 * 6);
    HAL_PCDEx_PMAConfig(&g_pcd_handle, 0x83, PCD_SNG_BUF, 0x100 + 64 * 6);

    return USBD_OK;
}

static USBD_StatusTypeDef USBD_Get_USB_Status(HAL_StatusTypeDef hal_status)
{
  switch (hal_status)
  {
    case HAL_OK:        return USBD_OK;
    case HAL_ERROR:     return USBD_FAIL;
    case HAL_BUSY:      return USBD_BUSY;
    case HAL_TIMEOUT:   return USBD_FAIL;
    default:            return USBD_FAIL;
  }
}

USBD_StatusTypeDef  USBD_LL_DeInit(USBD_HandleTypeDef *pdev)
{
    return USBD_Get_USB_Status(HAL_PCD_DeInit(pdev->pData));
}

USBD_StatusTypeDef  USBD_LL_Start(USBD_HandleTypeDef *pdev)
{
    return USBD_Get_USB_Status(HAL_PCD_Start(pdev->pData));
}

USBD_StatusTypeDef  USBD_LL_Stop(USBD_HandleTypeDef *pdev)
{
    return USBD_Get_USB_Status(HAL_PCD_Stop(pdev->pData));
}

USBD_StatusTypeDef  USBD_LL_OpenEP(USBD_HandleTypeDef *pdev, uint8_t  ep_addr, uint8_t  ep_type, uint16_t ep_mps)
{
    return USBD_Get_USB_Status(HAL_PCD_EP_Open(pdev->pData, ep_addr, ep_mps, ep_type));
}

USBD_StatusTypeDef  USBD_LL_CloseEP(USBD_HandleTypeDef *pdev, uint8_t ep_addr)
{
    return USBD_Get_USB_Status(HAL_PCD_EP_Close(pdev->pData, ep_addr));
}

USBD_StatusTypeDef  USBD_LL_FlushEP(USBD_HandleTypeDef *pdev, uint8_t ep_addr)
{
    return USBD_Get_USB_Status(HAL_PCD_EP_Flush(pdev->pData, ep_addr));
}

USBD_StatusTypeDef  USBD_LL_StallEP(USBD_HandleTypeDef *pdev, uint8_t ep_addr)
{
    return USBD_Get_USB_Status(HAL_PCD_EP_SetStall(pdev->pData, ep_addr));
}

USBD_StatusTypeDef  USBD_LL_ClearStallEP(USBD_HandleTypeDef *pdev, uint8_t ep_addr)
{
    return USBD_Get_USB_Status(HAL_PCD_EP_ClrStall(pdev->pData, ep_addr));
}

uint8_t USBD_LL_IsStallEP(USBD_HandleTypeDef *pdev, uint8_t ep_addr)
{
    PCD_HandleTypeDef *hpcd = (PCD_HandleTypeDef*) pdev->pData;

    if((ep_addr & 0x80) == 0x80)
    {
        return hpcd->IN_ep[ep_addr & 0x7F].is_stall;
    }
    else
    {
        return hpcd->OUT_ep[ep_addr & 0x7F].is_stall;
    }
}

USBD_StatusTypeDef  USBD_LL_SetUSBAddress(USBD_HandleTypeDef *pdev, uint8_t dev_addr)
{
    return USBD_Get_USB_Status(HAL_PCD_SetAddress(pdev->pData, dev_addr));
}

USBD_StatusTypeDef  USBD_LL_Transmit(USBD_HandleTypeDef *pdev, uint8_t ep_addr, uint8_t *pbuf, uint16_t size)
{
    return USBD_Get_USB_Status(HAL_PCD_EP_Transmit(pdev->pData, ep_addr, pbuf, size));
}

USBD_StatusTypeDef  USBD_LL_PrepareReceive(USBD_HandleTypeDef *pdev, uint8_t ep_addr, uint8_t *pbuf, uint16_t size)
{
    return USBD_Get_USB_Status(HAL_PCD_EP_Receive(pdev->pData, ep_addr, pbuf, size));
}

uint32_t USBD_LL_GetRxDataSize(USBD_HandleTypeDef *pdev, uint8_t  ep_addr)
{
    return HAL_PCD_EP_GetRxCount((PCD_HandleTypeDef*) pdev->pData, ep_addr);
}

void USBD_LL_Delay(uint32_t Delay)
{
    HAL_Delay(Delay);
}

void HAL_PCD_SetupStageCallback(PCD_HandleTypeDef *hpcd)
{
    USBD_LL_SetupStage(hpcd->pData, (uint8_t *)hpcd->Setup);
}

void HAL_PCD_DataOutStageCallback(PCD_HandleTypeDef *hpcd, uint8_t epnum)
{
    USBD_LL_DataOutStage(hpcd->pData, epnum, hpcd->OUT_ep[epnum].xfer_buff);
}

void HAL_PCD_DataInStageCallback(PCD_HandleTypeDef *hpcd, uint8_t epnum)
{
    USBD_LL_DataInStage(hpcd->pData, epnum, hpcd->IN_ep[epnum].xfer_buff);
}

void HAL_PCD_SOFCallback(PCD_HandleTypeDef *hpcd)
{
    USBD_LL_SOF(hpcd->pData);
}

void HAL_PCD_ResetCallback(PCD_HandleTypeDef *hpcd)
{   
    USBD_LL_SetSpeed(hpcd->pData, USBD_SPEED_FULL);
    USBD_LL_Reset(hpcd->pData);
}

void HAL_PCD_SuspendCallback(PCD_HandleTypeDef *hpcd)
{
}

void HAL_PCD_ResumeCallback(PCD_HandleTypeDef *hpcd)
{
}

void HAL_PCD_ISOOUTIncompleteCallback(PCD_HandleTypeDef *hpcd, uint8_t epnum)
{
    USBD_LL_IsoOUTIncomplete(hpcd->pData, epnum);
}

void HAL_PCD_ISOINIncompleteCallback(PCD_HandleTypeDef *hpcd, uint8_t epnum)
{
    USBD_LL_IsoINIncomplete(hpcd->pData, epnum);
}

void HAL_PCD_ConnectCallback(PCD_HandleTypeDef *hpcd)
{
    USBD_LL_DevConnected(hpcd->pData);
}

void HAL_PCD_DisconnectCallback(PCD_HandleTypeDef *hpcd)
{
    USBD_LL_DevDisconnected(hpcd->pData);
}
