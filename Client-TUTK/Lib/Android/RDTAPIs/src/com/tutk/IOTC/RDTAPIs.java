/******************************************************************************
 *                                                                            *
 * Copyright (c) 2011 by TUTK Co.LTD. All Rights Reserved.                    *
 *                                                                            *
 *                                                                            *
 * Class: IOTCRDTApis.java                                                    *
 * Author: joshua ju                                                          *
 * Date: 2011-05-14                                                           *
	
	Revisions of IOTCRDTApis
	IOTCApis Version joined		Name		Date
	-------						----		----------
	0.5.0.0						Joshua		2011-06-30
	1.1.0.0						Joshua		2011-08-01
	1.2.0.0						Joshua      2011-08-30
	1.2.1.0						Joshua		2011-09-19

 ******************************************************************************/

package com.tutk.IOTC;


public class RDTAPIs {
	public static long ms_verRDTApis=0;
	
	public static final int  API_ER_ANDROID_NULL				=-10000;
	//RDTApis error code===================================================================================
	/** The function is performed successfully. */
	public static final int	RDT_ER_NoERROR							 =0;
	
	/** RDT module is not initialized yet. Please use RDT_Initialize() for initialization. */
	public static final int	RDT_ER_NOT_INITIALIZED					=-10000;
	
	/** RDT module is already initialized. It is not necessary to re-initialize. */
	public static final int	RDT_ER_ALREADY_INITIALIZED				=-10001;
	
	/** The number of RDT channels has reached maximum.
	* Please use RDT_Set_Max_Channel_Number() to set up the max number of RDT channels.
	* By default, the maximum channel number is #MAX_DEFAULT_RDT_CHANNEL_NUMBER. */
	public static final int	RDT_ER_EXCEED_MAX_CHANNEL				=-10002;
	
	/** Insufficient memory for allocation */
	public static final int	RDT_ER_MEM_INSUFF						=-10003;
	
	/** RDT module fails to create threads. Please check if OS has ability to
	* create threads for RDT module. */
	public static final int	RDT_ER_FAIL_CREATE_THREAD				=-10004;
	
	/** RDT module fails to create Mutexs when doing initialization. Please check
	* if OS has sufficient Mutexs for RDT module. */
	public static final int	RDT_ER_FAIL_CREATE_MUTEX				=-10005;
	
	/** RDT channel has been destroyed. Probably caused by local or remote site
	* calls RDT_Destroy(), or remote site has closed IOTC session. */
	public static final int	RDT_ER_RDT_DESTROYED					=-10006;
	
	/** The specified timeout has expired during the execution of some RDT module service.
	* For most cases, it is caused by slow response of remote site or network connection issues */
	public static final int	RDT_ER_TIMEOUT							=-10007;
	
	/** The specified RDT channel ID is valid */
	public static final int	RDT_ER_INVALID_RDT_ID					=-10008;
	
	/** ??? the meaning and which function uses this */
	public static final int	RDT_ER_RCV_DATA_END						=-10009;

	//RDTApis interface
	public native static int RDT_GetRDTApiVer();	//save as Little endian
	public native static int RDT_Initialize();		//return max RDT_ID
	public native static int RDT_DeInitialize();
	public native static int RDT_Create(int nSessID, int TimeOut_ms, int ChID);
	public native static int RDT_Destroy(int RDT_ID);
	public native static int RDT_Write(int RDT_ID, byte[] data, int dataSize);
	public native static int RDT_Read(int RDT_ID, byte[] buf, int bufMaxSize, int Timeout_ms);
	public native static int RDT_Status_Check(int RDT_ID, St_RDT_Status status);
	
	/* Obsoleted in 2011.10.3  */
	//public native long RDT_BufSizeInQueue(int RDT_ID);


	static { try {System.loadLibrary("RDTAPIs");}
			 catch(UnsatisfiedLinkError ule){
			   System.out.println("loadLibrary(RDTAPIs),"+ule.getMessage());
			 }
	}
}
