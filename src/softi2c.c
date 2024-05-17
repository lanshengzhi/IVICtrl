
#include "main.h"
#include "softi2c.h"

#define HIGH    1
#define LOW     0

#define MSB_BIT_MASK	0x80

#define I2C_DATA_PIN    P2_0
#define I2C_CLOCK_PIN   P2_1

static INT8U g_i2cDelay = 3;

void usWait(INT16U delay)
{
    do
    {
        __asm("nop");
    }while (--delay);
}

static void SendDataBit(BOOL bHigh)
{
    if (bHigh)
        I2C_DATA_PIN = 1;
    else
        I2C_DATA_PIN = 0;
}

static BOOL ReceiveDataBit()
{
    I2C_DATA_PIN = 1;  
    return I2C_DATA_PIN;
}

static void GenClock(BOOL bHigh)
{
    if (bHigh)
        I2C_CLOCK_PIN = 1;
    else
        I2C_CLOCK_PIN = 0;
}

/*
******************************************************************** 
启动I2C 
SDA--|_____ 
SCL----|___
******************************************************************** 
*/
static void I2CSendStart()
{
    SendDataBit(HIGH);
    usWait(g_i2cDelay);
    GenClock(HIGH);
    usWait(g_i2cDelay);
    SendDataBit(LOW);
    usWait(g_i2cDelay);
    GenClock(LOW);
    usWait(g_i2cDelay);
}

/*
******************************************************************** 
停止I2C 
SDA____|--
SCL__-----
******************************************************************** 
*/
void I2CSendStop()
{
    SendDataBit(LOW);
    usWait(g_i2cDelay);
    GenClock(HIGH);
    usWait(g_i2cDelay);
    SendDataBit(HIGH);
    usWait(g_i2cDelay);
}

/*
******************************************************************** 
发送应答或非应答 
 bXAck = 0; 应答
 bXAck = 1; 非应答 
******************************************************************** 
*/
void I2CSendXACK(BOOL bXAck)
{
    SendDataBit(bXAck);
    GenClock(HIGH);
    usWait(g_i2cDelay);
    GenClock(LOW);
    usWait(g_i2cDelay);
}

/*
******************************************************************** 
读应答，0-应答; 1-非应答
******************************************************************** 
*/
BOOL I2CWaitForXACK()
{
    BOOL bXAck;

    GenClock(HIGH);
    usWait(g_i2cDelay);
    bXAck = ReceiveDataBit();	// 0 - ACK; not 0 - NACK;
    GenClock(LOW);
    usWait(g_i2cDelay);

    return bXAck;
}

void I2CSendByte(BYTE ndata)
{
    int i;

    for (i = 0; i < 8; i++)
    {
        SendDataBit((ndata & MSB_BIT_MASK) ? HIGH : LOW);
        usWait(g_i2cDelay);
        GenClock(HIGH);
        usWait(g_i2cDelay);
        GenClock(LOW);
        usWait(g_i2cDelay);
        ndata <<= 1;
    }
}

BYTE I2CReceiveByte()
{
    int i;
    BYTE ndata = 0;

    for (i = 0; i < 8; i++)
    {
        GenClock(HIGH);
        usWait(g_i2cDelay);

        if (ReceiveDataBit())
            ndata |= 1;

        if (i < 7)
            ndata <<= 1;

        GenClock(LOW);
        usWait(g_i2cDelay);
    }

    return ndata;
}

void I2cSoftInitializeHW()
{
    //Init the I2C bus state:
    GenClock(HIGH);
    SendDataBit(HIGH);
}

void I2cSoftDeinitializeHW()
{
}

BOOL I2cSoftRead(PI2C_DEVINFO pDevHdl)
{
    UINT  seqNum, rdNum;
    BYTE  segFlag, segMask;
    ULONG wData;
    BOOL  bRet = FALSE;

    if (!pDevHdl || !pDevHdl->pReadSeq || !pRegValBuf || !uiValBufLen)
        return FALSE;

    for (seqNum = 0; seqNum < pDevHdl->uiReadSeqLen; seqNum++)
    {
        segFlag = pDevHdl->pReadSeq[seqNum] & ~SEG_MASK_ALL;
        segMask = pDevHdl->pReadSeq[seqNum] & SEG_MASK_ALL;

        if (segFlag == I2C_SEQ_SEG_DEVADDR_W)
            wData = pDevHdl->ulDevAddrW;
        else if (segFlag == I2C_SEQ_SEG_DEVADDR_R)
            wData = pDevHdl->ulDevAddrR;
        else if (segFlag == I2C_SEQ_SEG_REGADDR)
            wData = ulRegAddr;
        else if (segFlag == I2C_SEQ_SEG_DATA)
        {
            for (rdNum = 0; rdNum < uiValBufLen; rdNum++)
            {
                pRegValBuf[rdNum] = I2CReceiveByte();

                if (rdNum == (uiValBufLen - 1))
                {
                    if (segMask & SEG_MASK_NEED_NACK)
                        I2CSendXACK(NACK);

                    if (segMask & SEG_MASK_SEND_STOP)
                        I2CSendStop();
                }
                else
                {
                    I2CSendXACK(ACK);
                }
            }

            continue;
        }
        else
        {
            DEBUGMSG(ZONE_DEBUG, (TEXT("I2C_Read: Invalid segment!\r\n")));
            goto I2CREAD_EXIT;
        }

        if (segMask & SEG_MASK_SEND_START)
            I2CSendStart();

        I2CSendByte((BYTE)wData);

        if (segMask & SEG_MASK_NEED_ACK)
        {
            if (I2CWaitForXACK())
            {
                /*
                * Should not receive NACK, so stop transfer
                */
                DEBUGMSG(ZONE_DEBUG, (TEXT("I2C_Read: Received NACK after write. Send stop & quit!\r\n")));
                I2CSendStop();
                goto I2CREAD_EXIT;
            }
        }

        if (segMask & SEG_MASK_SEND_STOP)
            I2CSendStop();
    }

    bRet = TRUE;

I2CREAD_EXIT:
    return bRet;
}

BOOL I2cSoftWrite(PI2C_DEVINFO pDevHdl)
{
    UINT  seqNum, wrNum, wrBytes;
    BYTE  segFlag, segMask;
    ULONG wData;
    BOOL bRet = FALSE;

    if (!pDevHdl || !pDevHdl->pWriteSeq || !pRegValBuf || !uiValBufLen)
        return FALSE;

    for (seqNum = 0; seqNum < pDevHdl->uiWriteSeqLen; seqNum++)
    {
        segFlag = pDevHdl->pWriteSeq[seqNum] & ~SEG_MASK_ALL;
        segMask = pDevHdl->pWriteSeq[seqNum] & SEG_MASK_ALL;

        if (segFlag == I2C_SEQ_SEG_DEVADDR_W)
        {
            wData = pDevHdl->ulDevAddrW;
            wrBytes = 1;
        }
        else if (segFlag == I2C_SEQ_SEG_REGADDR)
        {
            wData = ulRegAddr;
            wrBytes = 1;
        }
        else if (segFlag == I2C_SEQ_SEG_DATA)
        {
            wrBytes = uiValBufLen;
        }
        else
        {
            DEBUGMSG(ZONE_DEBUG, (TEXT("I2C_Write: Invalid segment!\r\n")));
            goto I2CWRITE_EXIT;
        }

        for (wrNum = 0; wrNum < wrBytes; wrNum++)
        {
            if (segFlag == I2C_SEQ_SEG_DATA)
                wData = pRegValBuf[wrNum];

            if (wrNum == 0)
            {
                if (segMask & SEG_MASK_SEND_START)
                    I2CSendStart();
            }

            I2CSendByte((BYTE)wData);

            if (segMask & SEG_MASK_NEED_ACK)
            {
                if (I2CWaitForXACK())
                {
                    // Should not receive NACK, so stop transfer
                    DEBUGMSG(ZONE_DEBUG, (TEXT("I2C_Write: Received NACK after write. Send stop & quit!\r\n")));
                    I2CSendStop();
                    goto I2CWRITE_EXIT;
                }
            }

            if (wrNum == (wrBytes - 1))
            {
                if (segMask & SEG_MASK_SEND_STOP)
                    I2CSendStop();
            }
        }
    }

    bRet = TRUE;

I2CWRITE_EXIT:
    return bRet;
}