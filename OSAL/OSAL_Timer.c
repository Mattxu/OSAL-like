/******************************************************************************
* File       : OSAL_Timer.c
* Function   : Provide timer services.
* description: To be done.          
* Version    : V1.00
* Author     : Ian
* Date       : 6th May 2016
* History    :  No.  When           Who           What
*               1    06/May/2016    Ian           Create
******************************************************************************/

#include "type_def.h"
#include "common_head.h"
#include "project_config.h"
#include "OSAL.h"
#include "OSAL_Timer.h"
#include "debug.h"

static T_TIMER_NODE* Osal_Timer_Add();
static T_TIMER_NODE *Osal_Timer_Find(T_TIMER_NODE* ptNode);
static T_TIMER_NODE* Osal_Timer_Del(T_TIMER_NODE* ptNode);
static uint16 Osal_Timer_Test_Max_Cnt();
static uint8  Osal_Timer_Test_StartStop();


static T_TIMER_NODE *sg_ptTmHead = NULL;
static T_TIMER_NODE *sg_ptTmTail = NULL;

static PF_TIMER_SRC  sg_pfSysTm  = NULL;

/******************************************************************************
* Name       : uint8 Osal_Timer_Init(PF_TIMER_SRC pfSysTm)
* Function   : Init OSAL timer
* Input      : PF_TIMER_SRC pfSysTm   Funtion which returns system time
* Output:    : None
* Return     : SW_OK   Successful.
*              SW_ERR  Failed.
* description: To be done.
* Version    : V1.00
* Author     : Ian
* Date       : 6th May 2016
******************************************************************************/
uint8 Osal_Timer_Init(PF_TIMER_SRC pfSysTm)
{   /* Check if the input function is NULL or NOT */
    if (NULL == pfSysTm)
    {   
        DBG_PRINT("System time function pointer is invalid!!\n");
        return SW_ERR;    /* If invalid, return error */   
    }
    /* If the input function is OK, then go on */

    sg_pfSysTm = pfSysTm;      /* Save the system time function for further using */
    DBG_PRINT("OSAL timer inited successfully!!\n");   
 
    return SW_OK;
}

/******************************************************************************
* Name       : static T_TIMER_NODE* Osal_Timer_Add()
* Function   : Add a timer node
* Input      : PF_TIMER_SRC pfSysTm   Funtion which returns system time
* Output:    : None
* Return     : SW_OK   Successful.
*              SW_ERR  Failed.
* description: To be done.
* Version    : V1.00
* Author     : Ian
* Date       : 6th May 2016
******************************************************************************/
static T_TIMER_NODE* Osal_Timer_Add()
{
    T_TIMER_NODE* ptNode;

    ptNode = (T_TIMER_NODE*)OSAL_MALLOC(sizeof(T_TIMER_NODE)); /* Allocate a timer node */
    /* If the allocation is FAILED */
    if (NULL == ptNode)                                   
    {
        DBG_PRINT("No more heap space for timer node!!\n");
        return NULL; /* Return NULL to indicate failed operation */
    }
    /* Else, new timer node is allocated */

    ptNode->ptNext = NULL;               /* Set the next node as NULL       */

    if(NULL == sg_ptTmTail)              /* If there is NO nodes            */
    {
        sg_ptTmHead = ptNode;            /* Add new node as the fisrt one   */
    }
    else                                 /* If node exsits                  */
    {
        sg_ptTmTail->ptNext = ptNode;    /* Add new node after the tail one */
    }
    sg_ptTmTail = ptNode;                /* Update the tail node            */

    return ptNode;

}


/******************************************************************************
* Name       : T_TIMER_NODE* Osal_Timer_Start(uint8 u8TaskID, uint16 u16Evt, uint16 u16Cnt, uint32 u32TmOut)
* Function   : Start a timer
* Input      : T_TIMER *ptTm     Pointer of timers set by user.
* Output:    : None
* Return     : NULL           Fail to start a timer.
*              T_TIMER_NODE*  The pointer of the timer which is started.
* description: To be done.
* Version    : V1.00
* Author     : Ian
* Date       : 6th May 2016
******************************************************************************/
T_TIMER_NODE* Osal_Timer_Start(T_TIMER *ptTm)
{
    T_TIMER_NODE* ptNode;
    uint32 u32IntSt;

    if(0 == ptTm->u16Cnt)                                 /* If the start count is 0            */
    {
        return NULL;                                      /* Unnecessary to start the timer     */
    }

    ENTER_CRITICAL_ZONE(u32IntSt);  /* Enter the critical zone to prevent event updating unexpectedly */
    /**************************************************************************************************/
    ptNode = Osal_Timer_Add();                            /* Allocate a timer node              */
    if (NULL == ptNode)                                   /* If new timer is failed to allocate */
    {
        DBG_PRINT("New timer node is failed to start\n");
        return NULL;                                      /* Return NULL, fail to start a timer */
    }
    /* A timer node was added successfully */
        
    ptNode->tTimer.u32TmStart   = sg_pfSysTm();           /* Get the system time                */     
    ptNode->tTimer.u32TmOut     = ptTm->u32TmOut;         /* Set the timeout time               */
    ptNode->tTimer.u16Cnt       = ptTm->u16Cnt;           /* Set the restart count              */
    ptNode->tTimer.u16Evt       = ptTm->u16Evt;           /* Set the event                      */
    ptNode->tTimer.u8TaskID     = ptTm->u8TaskID;         /* Set the task ID                    */
#ifdef __TIMER_CALLBACK_SUPPORTED
    ptNode->tTimer.pfTmCallback = ptTm->pfTmCallback;     /* Set the callback function          */
    /* If the parameter is NULL */
    if(NULL == ptTm->pPara)                               
    {
        ptNode->tTimer.pPara    = (void*)ptNode;          /* If the parameter is NULL           */
    }
    else
    {
        ptNode->tTimer.pPara    = ptTm->pPara;            /* Set the parameter of callback      */
    }
#endif
    /**************************************************************************************************/
    EXIT_CRITICAL_ZONE(u32IntSt);   /* Exit the critical zone                                         */

    DBG_PRINT("New timer is started!!\n");

    return ptNode;

}


/******************************************************************************
* Name       : static T_TIMER_NODE* Osal_Timer_Del(T_TIMER_NODE* ptNode)
* Function   : Delete a timer node
* Input      : T_TIMER_NODE* ptNode  The timer node to be delete.
* Output:    : None
* Return     : NULL           Fail to delete a timer.
*              T_TIMER_NODE*  The pointer of the timer which is deleted.
* description: To be done.
* Version    : V1.00
* Author     : Ian
* Date       : 6th May 2016
******************************************************************************/
static T_TIMER_NODE* Osal_Timer_Del(T_TIMER_NODE* ptNode)
{
    T_TIMER_NODE* ptFind;

    uint32 u32IntSt;

    ENTER_CRITICAL_ZONE(u32IntSt);  /* Enter the critical zone to prevent event updating unexpectedly */
    /**************************************************************************************************/
    if (NULL == ptNode)                         /* If the input node pointer is invalid  */
    {
        return NULL;                            /* Return NULL, no node to be deleted    */
    }

    if(ptNode == sg_ptTmHead)                   /* If the deleting node is the head node */
    {
        if(ptNode == sg_ptTmTail)               /* And if it is the unique node          */
        {
            DBG_PRINT("The last timer node is deleted!!\n");
            sg_ptTmHead = NULL;                 /* Set NULL to head node                 */
            sg_ptTmTail = NULL;                 /* Set NULL to tail node                 */
        }
        else                                    /* Else if it is NOT the unique node     */
        {
            sg_ptTmHead = sg_ptTmHead->ptNext;  /* Make head pointing to the next node   */
        }
        free(ptNode);                           /* Free the deleting node                */
        DBG_PRINT("The deleting node is free!!\n");
        return ptNode;                          
    }    
    /* Else, the deleting node is NOT the head node, then go on */

    ptFind = sg_ptTmHead;                       /* Get the head node                     */
    while (ptFind)                              /* If such node is NOT null              */
    {
        if (ptFind->ptNext == ptNode)           /* Check if the next node is the one     */
        {
            ptFind->ptNext = ptNode->ptNext;    /* Delete the node                       */
            OSAL_FREE(ptNode);                  /* Free the deleting node                */
            DBG_PRINT("The deleting node is free!!\n");
            if(NULL == ptFind->ptNext)          /* If the tail node is deleted           */
            {
                sg_ptTmTail = ptFind;           /* Update the tail node                  */
            }
            return ptFind;                      /* Return the deteled node pointer       */
        }
        ptFind = ptFind->ptNext;                /* Update the searching node             */
    }

    /**************************************************************************************************/
    EXIT_CRITICAL_ZONE(u32IntSt);   /* Exit the critical zone                                         */

    return NULL;                    /* Have NOT found the done */
}


/******************************************************************************
* Name       : T_TIMER_NODE* Osal_Timer_Stop(T_TIMER_NODE* ptNode)
* Function   : Stop a started timer.
* Input      : T_TIMER_NODE* ptNode  The pointer of node to be stopped
* Output:    : None
* Return     : NULL           Fail to stop a timer.
*              T_TIMER_NODE*  The pointer of the timer which is stopped.
* description: To be done.
* Version    : V1.00
* Author     : Ian
* Date       : 6th May 2016
******************************************************************************/
T_TIMER_NODE* Osal_Timer_Stop(T_TIMER_NODE* ptNode)
{
    T_TIMER_NODE* ptFind;

    uint32 u32IntSt;

    ENTER_CRITICAL_ZONE(u32IntSt);  /* Enter the critical zone to prevent event updating unexpectedly */
    /**************************************************************************************************/
    ptFind = Osal_Timer_Find(ptNode);     /* Search the timer node    */
    if(NULL == ptFind)                    /* If the node is NOT found */
    {
        DBG_PRINT("The timer node to be stopped is NOT found!!\n");
        return NULL;                      /* NOT found, return NULL   */
    }
    /* Else, the timer node is found */

    ptNode->tTimer.u16Cnt = 0;            /* Set the count as 0       */

    /**************************************************************************************************/
    EXIT_CRITICAL_ZONE(u32IntSt);   /* Exit the critical zone                                         */
    DBG_PRINT("Timer is stopped!!\n");
    return ptNode;
}


/******************************************************************************
* Name       : static T_TIMER_NODE *Osal_Timer_Find(T_TIMER_NODE* ptNode)
* Function   : Try to find a node.
* Input      : T_TIMER_NODE* ptNode  The pointer of node to be found
* Output:    : None
* Return     : NULL           Fail to find the node.
*              T_TIMER_NODE*  The pointer of the founed node.
* description: To be done.
* Version    : V1.00
* Author     : Ian
* Date       : 6th May 2016
******************************************************************************/
static T_TIMER_NODE *Osal_Timer_Find(T_TIMER_NODE* ptNode)
{
    T_TIMER_NODE * ptFind;
    
    ptFind = sg_ptTmHead;             /* Get the head node of timers          */
    while(ptFind)                     /* If such node is avaliable            */
    {                                
        if(ptFind == ptNode)          /* And if it is the searched node       */
        {
            DBG_PRINT("Find the timer node!!\n");
            break;                    /* Find it and break the loop           */
        }
        ptFind = ptFind->ptNext;      /* Update the node pointer              */
    }
    return ptFind;
}

/******************************************************************************
* Name       : static T_TIMER_NODE *Osal_Timer_Find(T_TIMER_NODE* ptNode)
* Function   : Try to find a node.
* Input      : T_TIMER_NODE* ptNode  The pointer of node to be found
* Output:    : None
* Return     : NULL           Fail to find the node.
*              T_TIMER_NODE*  The pointer of the founed node.
* description: To be done.
* Version    : V1.00
* Author     : Ian
* Date       : 6th May 2016
******************************************************************************/
T_TIMER_NODE* Osal_Timer_Restart(T_TIMER_NODE* ptNode)
{
    T_TIMER_NODE *ptFind;
    uint32 u32IntSt;

    ENTER_CRITICAL_ZONE(u32IntSt);  /* Enter the critical zone to prevent event updating unexpectedly */
    /**************************************************************************************************/
    ptFind = Osal_Timer_Find(ptNode);          /* Search the timer node    */
    if(NULL == ptFind)                         /* If the node is NOT found */
    {
        DBG_PRINT("The timer node to be restarted is NOT found!!\n");
        return NULL;                           /* NOT found, return NULL   */
    }
    /* Else, the timer node is found */

    ptNode->tTimer.u32TmStart = sg_pfSysTm();  /* Update the start point   */
    /**************************************************************************************************/
    EXIT_CRITICAL_ZONE(u32IntSt);   /* Exit the critical zone                                         */
    DBG_PRINT("Timer is restarted!!\n");
    return ptNode;                      
}


/******************************************************************************
* Name       : uint8 Osal_Timer_Process(void)
* Function   : Main process for timer updating.
* Input      : None
* Output:    : None
* Return     : SW_OK   Successful.
*              SW_ERR  Failed.
* description: To be done.
* Version    : V1.00
* Author     : Ian
* Date       : 6th May 2016
******************************************************************************/
uint8 Osal_Timer_Process(void)
{
    T_TIMER_NODE* ptFind;
    T_TIMER_NODE* ptNodeFree;

    uint32 u32IntSt;

    ENTER_CRITICAL_ZONE(u32IntSt);  /* Enter the critical zone to prevent event updating unexpectedly */
    /**************************************************************************************************/
    ptFind = sg_ptTmHead;                       /* Get the head timer         */
    while(ptFind)                               /* If such timer is avaliable */
    {
        if(0 == ptFind->tTimer.u16Cnt)          /* If the timing count is 0   */
        {                                     
            ptNodeFree = ptFind;                /* Get the deleting timer     */
            ptFind = ptFind->ptNext;            /* Point the next timer       */
            Osal_Timer_Del(ptNodeFree);         /* Delete the timer           */
            continue;                           /* Go on check next timer     */
        }          

        /* If the time is up */   
        if((sg_pfSysTm() - ptFind->tTimer.u32TmStart) >= ptFind->tTimer.u32TmOut)         
        {   /* Set the desired evnet */
            Osal_Event_Set(ptFind->tTimer.u8TaskID,ptFind->tTimer.u16Evt);  
            DBG_PRINT("Time is up, Task %d has a 0x%x type event\n",ptFind->tTimer.u8TaskID,ptFind->tTimer.u16Evt);
#ifdef __TIMER_CALLBACK_SUPPORTED
            if (NULL != ptFind->tTimer.pfTmCallback)                /* If we have callback for such timer */
            {
                ptFind->tTimer.pfTmCallback(ptFind->tTimer.pPara);  /* Call the callback function         */
            }
#endif

            /* If the it is NOT infinite count */
            if(ptFind->tTimer.u16Cnt != OSAL_TMR_INFINITE_CNT)              
            {
                ptFind->tTimer.u16Cnt--;    /* Update the count     */
            }
            /* If the timing count is NOT 0 */
            if(ptFind->tTimer.u16Cnt)
            {
                DBG_PRINT("Timer is restarted, %d times left\n",ptFind->tTimer.u16Cnt);
                Osal_Timer_Restart(ptFind); /* Restart the timer    */   
            }
        }
        ptFind = ptFind->ptNext;             /* Check the next timer */
    }
    /**************************************************************************************************/
    EXIT_CRITICAL_ZONE(u32IntSt);   /* Exit the critical zone                                         */

    return SW_OK;                                               
}

/******************************************************************************
* Name       : uint16 Osal_Timer_Cnt()
* Function   : Get the count of timers
* Input      : None
* Output:    : None
* Return     : uint16   The count of timers
* description: To be done.
* Version    : V1.00
* Author     : Ian
* Date       : 6th May 2016
******************************************************************************/
uint16 Osal_Timer_Cnt()
{
    T_TIMER_NODE *ptFind;
    uint16 u16TmrCnt = 0;

    uint32 u32IntSt;

    ENTER_CRITICAL_ZONE(u32IntSt);  /* Enter the critical zone to prevent event updating unexpectedly */
    /**************************************************************************************************/
    ptFind = sg_ptTmHead;           /* Get the head timer         */
    while(ptFind)                   /* If such timer is avaliable */
    {
        u16TmrCnt++;                /* Increase the count         */
        ptFind = ptFind->ptNext;    /* Check the next one         */
    }
    /**************************************************************************************************/
    EXIT_CRITICAL_ZONE(u32IntSt);   /* Exit the critical zone                                         */
    DBG_PRINT("The count of the timer is %d\n",u16TmrCnt);
    return u16TmrCnt;
}

/******************************************************************************
* Name       : static uint16 Osal_Timer_Test_Max_Cnt()
* Function   : Get the Max. count of timers
* Input      : None
* Output:    : None
* Return     : uint16   The Max. count of timers
* description: Warning: This test function is just show how many timers can be 
*              allocated in heap space. It will run out all heap space and free
*              all of them after about 100ms.
*              **IT IS A TEST FUNCTION! DO NOT USE IT IN YOUR APPLICATION!**
* Version    : V1.00
* Author     : Ian
* Date       : 9th May 2016
******************************************************************************/
static uint16 Osal_Timer_Test_Max_Cnt()
{
    uint16 u16Idx;
    uint32 u32IntSt;

    T_TIMER_NODE *ptNode = (T_TIMER_NODE*)(&sg_ptTmHead); /* Make it a non-NULL value       */

    DBG_PRINT("**IT IS A TEST FUNCTION! DO NOT USE IT IN YOUR APPLICATION!**\n");
    DBG_PRINT("Warning: This test function is just show how many timers can be allocated in heap space.\n");
    
    ENTER_CRITICAL_ZONE(u32IntSt);  /* Enter the critical zone to prevent event updating unexpectedly */
    /**************************************************************************************************/    
    /* Check how many timers can be added */
    for (u16Idx = 0; ptNode != NULL ; u16Idx++)
    {
        ptNode = Osal_Timer_Start(0, 0, 1, 10);            /* Set 10ms timeout for each timer */
    }
    /**************************************************************************************************/
    EXIT_CRITICAL_ZONE(u32IntSt);   /* Exit the critical zone                                         */

    DBG_PRINT("Max timer count is %d, Heap is full currently, please wait 100ms for heap free operation!!\n",u16Idx);
    return u16Idx;
}

/******************************************************************************
* Name       : static uint8 Osal_Timer_Test_StartStop()
* Function   : Test start and stop function
* Input      : None
* Output:    : None
* Return     : To be done
* description: To be done.
*              **IT IS A TEST FUNCTION! DO NOT USE IT IN YOUR APPLICATION!**
* Version    : V1.00
* Author     : Ian
* Date       : 9th May 2016
******************************************************************************/
static uint8 Osal_Timer_Test_StartStop()
{
    T_TIMER_NODE *ptNode;
    uint32 u32IntSt;

    ENTER_CRITICAL_ZONE(u32IntSt);  /* Enter the critical zone to prevent event updating unexpectedly */
    /**************************************************************************************************/    
    DBG_PRINT("Try to start timer for 10ms\n"); 
    ptNode = Osal_Timer_Start(0, 0, 1, 10);             /* Start a timer               */
    if(NULL == ptNode)                                  /* Check if successful or NOT  */
    {
        DBG_PRINT("Failed to start a timer!!\n");       
        return SW_ERR;
    }
    DBG_PRINT("The time is started successfully\n");

    /* Find the started timer */
    if(Osal_Timer_Find(ptNode) == ptNode)
    {
        DBG_PRINT("The started timer is found in the timer talbe.\n");
    }
    else
    {
        DBG_PRINT("The started timer is NOT found!!\n");
        return SW_ERR;
    }

    DBG_PRINT("Try to stop the timer\n"); 
    ptNode = Osal_Timer_Stop(ptNode);                   /* Stop the timer              */
    if(NULL == ptNode)                                  /* Check if successful or NOT  */
    {
        DBG_PRINT("Failed to stop a timer!!\n");
        return SW_ERR;
    }
    DBG_PRINT("The time is stopped successfully\n");
    
    Osal_Timer_Process();                               /* To delete the stopped timer */

    DBG_PRINT("Try to start another timer for 5000ms\n"); 
    ptNode = Osal_Timer_Start(0, 0, 1, 5000);           /* Start a timer               */
    if(NULL == ptNode)                                  /* Check if successful or NOT  */
    {
        DBG_PRINT("Failed to start a timer!!\n");       
        return SW_ERR;
    }
    DBG_PRINT("The time is started successfully\n");

    DBG_PRINT("Wait a time up message after 5000ms\n");
    while(NULL != sg_ptTmHead)
    {
        Osal_Timer_Process();                           /* Wait for time up            */
    }
    DBG_PRINT("Time up!! Is it a 5000ms delay?\n");

    DBG_PRINT("Try to stopped a deleted timer\n");
    ptNode = Osal_Timer_Stop(ptNode);                   /* Stop the timer              */
    if(NULL == ptNode)                                  /* Check if successful or NOT  */
    {
        DBG_PRINT("Failed to stop a timer!!\n");
    } 

    DBG_PRINT("Start a 10-times 1 second timer!!\n");
    ptNode = Osal_Timer_Start(0, 0, 10, 1000);           /* Start a timer               */
    if(NULL == ptNode)                                  /* Check if successful or NOT  */
    {
        DBG_PRINT("Failed to start a timer!!\n");       
        return SW_ERR;
    }
    while(NULL != sg_ptTmHead)
    {
        Osal_Timer_Process();                           /* Wait for time up            */
    }
    DBG_PRINT("General test for timer is finished!\n");

    /**************************************************************************************************/
    EXIT_CRITICAL_ZONE(u32IntSt);   /* Exit the critical zone                                         */

    return SW_OK;
}

/******************************************************************************
* Name       : void Osal_Timer_Test_General()
* Function   : General test
* Input      : None
* Output:    : None
* Return     : None
* description: To be done
*              **IT IS A TEST FUNCTION! DO NOT USE IT IN YOUR APPLICATION!**
* Version    : V1.00
* Author     : Ian
* Date       : 9th May 2016
******************************************************************************/
void Osal_Timer_Test_General()
{
    Osal_Timer_Cnt();              /* Get the current timer count          */
    Osal_Timer_Test_Max_Cnt();     /* Check how many timers can be created */
    while(NULL != sg_ptTmHead)     /* Wait all timers timeout              */
    {
        Osal_Timer_Process();      /* Process all timers                   */
    }

    Osal_Timer_Cnt();              /* Get the current timer count          */

    Osal_Timer_Test_StartStop();   /* Test the start & stop function       */
    return;
}
/* end of file */

