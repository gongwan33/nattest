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

//-----------------------------ERROR RETURN-------------------------------
#define WRONG_VERIFY			-50
#define WRONG_REG				-51
#define OUT_TRY					-52
#define NO_NODE					-53
#define INIT_PEER_LOGIN_FAIL	-54
#define LOGIN_SHEET_BROKEN		-55
