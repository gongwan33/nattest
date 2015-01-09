int JEAN_init_master(int serverPort, int localPort, char *setIp);
int JEAN_send_master(char *data, int len, unsigned char priority, unsigned char video_analyse);
int JEAN_recv_master(char *data, int len, unsigned char priority, unsigned char video_analyse);
int JEAN_close_master();
int JEAN_init_slave(int setServerPort, int setLocalPort, char *setIp);
int JEAN_send_slave(char *data, int len, unsigned char priority, unsigned char video_analyse);
int JEAN_recv_slave(char *data, int len, unsigned char priority, unsigned char video_analyse);
int JEAN_close_slave();
