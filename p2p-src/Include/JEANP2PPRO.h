/*AUTHOR:WANGGONG, CHINA
 *VERSION:1.0
 *FUNCTION:PROTOCOL
 */
#ifndef JEANP2PPRO_H
#define JEANP2PPRO_H

//-----------------------------OPCODE DEFINE------------------------------
#define V_RESP		0x01
#define V_UAP		0x02
#define UREG_MA     0x03
#define GET_REQ		0x04
#define KEEP_CON	0x05
#define V_UAP_S		0x06
#define RESP_M_IP	0x07
#define S_IP		0x08
#define REQ_M_IP	0x09
#define POL_REQ		0x0a
#define POL_SENT	0x0b
#define CON_ESTAB	0x0c
#define AV_REQ		0x0d
#define M_POL_REQ	0x0e
#define S_POL_REQ	0x0f
#define TURN_REQ	0x10
#define MASTER_QUIT 0x11
#define CMD_CHAN    0x12
#define CONTROL_CHAN    0x13

#define EXT_CMD		0xff

//--------------------------Connection Status-----------------------------
#define P2P         0x01
#define TURN        0x02
#define FAIL        0x03

//-----------------------------ERROR RETURN-------------------------------
#define WRONG_VERIFY			-50
#define WRONG_REG				-51
#define OUT_TRY					-52
#define NO_NODE					-53
#define INIT_PEER_LOGIN_FAIL	-54
#define LOGIN_SHEET_BROKEN		-55
#define NO_CONNECTION           -56


struct load_head {
	char logo[4];
    u_int32_t index;
	char priority;
	u_int32_t length;
	unsigned char direction;//direction = 0, to master; direction = 1, to slave
	u_int32_t totalLen;
	u_int16_t sliceIndex;
	u_int32_t address;
};

struct get_head {
    char logo[3];
};

struct syn_head {
    char logo[3];
};

struct sok_head {
    char logo[3];
};

struct win_head {
    char logo[3];
};

struct p2p_head {
    char logo[3];
	unsigned char data[40];
};

struct elm{
	u_int32_t index;
	unsigned int start;
	unsigned int end;
};

#endif

