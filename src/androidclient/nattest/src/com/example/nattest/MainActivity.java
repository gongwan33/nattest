package com.example.nattest;

import java.io.ByteArrayOutputStream;
import java.io.DataOutputStream;  
import java.io.IOException;  
import java.io.OutputStream;
import java.net.DatagramPacket;  
import java.net.DatagramSocket;  
import java.net.InetAddress;  
import java.net.NetworkInterface;
import java.net.Socket;  
import java.net.SocketException;  
import java.net.UnknownHostException;  
import java.util.Enumeration;
  
import android.annotation.SuppressLint;
import android.app.Activity;  
import android.os.Bundle;  
import android.os.StrictMode;
import android.util.Log;
import android.view.View;  
import android.widget.Button;  
import android.widget.TextView;  


public class MainActivity extends Activity {
	int count = 20;
    public static TextView show;  
    public static Button press;  
    public static boolean flag;  
    private int port1 = 61000;
    private int port2 = 61001;
    private int port3 = 60002;
    private int i,j;
    
    private static final int MAX_DATA_PACKET_LENGTH = 40;  
    private byte[] buffer = new byte[MAX_DATA_PACKET_LENGTH];  
    private DatagramPacket dataPacket;  
    private DatagramSocket udpSocket;  
      
    /** Called when the activity is first created. */  
    @SuppressLint("NewApi") @Override  
    public void onCreate(Bundle savedInstanceState) {  
        super.onCreate(savedInstanceState);  
        setContentView(R.layout.activity_main);  
          
        //开辟控件空间  
      //  show = (TextView)findViewById(R.id.editText1);  
        press = (Button)findViewById(R.id.button1);  
        flag = false;  
        //soket_send thread = new soket_send();  
        //thread.init();  
        //thread.start();  
        
        // 详见StrictMode文档
        StrictMode.setThreadPolicy(new StrictMode.ThreadPolicy.Builder()
                .detectDiskReads()
                .detectDiskWrites()
                .detectNetwork()   // or .detectAll() for all detectable problems
                .penaltyLog()
                .build());
        StrictMode.setVmPolicy(new StrictMode.VmPolicy.Builder()
                .detectLeakedSqlLiteObjects()
                .detectLeakedClosableObjects()
                .penaltyLog()
                .penaltyDeath()
                .build());
          
        try  
        {  
            udpSocket = new DatagramSocket(1888);  
        }  
        catch (SocketException e)  
        {  
            // TODO Auto-generated catch block  
            e.printStackTrace();  
            Log.e("erro","udpsocket");
        }  
        dataPacket = new DatagramPacket(buffer, MAX_DATA_PACKET_LENGTH);  
       // String str = "hello,jdh";  //这是要传输的数据  
       // byte out [] = str.getBytes();  //把传输内容分解成字节  
       // dataPacket.setData(out);  
       // dataPacket.setLength(out.length);  
         
//        try  
//        {  
//                  
//            InetAddress broadcastAddr = InetAddress.getByName("192.168.0.248");  
//            dataPacket.setAddress(broadcastAddr);  
//            udpSocket.send(dataPacket);  
//        }  
//        catch (IOException e)  
//        {  
//            // TODO Auto-generated catch block  
//            e.printStackTrace();  
//        }  
          
             
        press.setOnClickListener(new Button.OnClickListener()  
        {  
            @Override  
            public void onClick(View v)  
            {  
                flag = true;  
                String ipaddr = "fail to get addr";
                int local_port;
                /* 
                String str = "hello,jdh";  //这是要传输的数据 
                byte out [] = str.getBytes();  //把传输内容分解成字节 
                dataPacket.setData(out); 
                dataPacket.setLength(out.length); 
                */  
                Enumeration<NetworkInterface> en;
                //获得输入框文本  
                try {
					for (en = NetworkInterface.getNetworkInterfaces(); en.hasMoreElements();) {
					    NetworkInterface intf = en.nextElement();
					    for (Enumeration<InetAddress> enumIpAddr = intf.getInetAddresses(); enumIpAddr.hasMoreElements();) {
					        InetAddress inetAddress = enumIpAddr.nextElement();
					        if (!inetAddress.isLoopbackAddress()) {
					             ipaddr = inetAddress.getHostAddress().toString();
					             
					        }
					    }
					}
				} catch (SocketException e) {
					// TODO Auto-generated catch block
					e.printStackTrace();
				}
                //CharSequence str =MainActivity.show.getText();
                String output;
                local_port = udpSocket.getLocalPort();
                output = ipaddr + "[" + local_port + "]";          
                byte out[] = output.getBytes();
                dataPacket.setData(out);  
                dataPacket.setLength(out.length); 
                for(j=0;j<3;j++){
                for(i=0;i<count;i++){
                	try  
                	{  
                		dataPacket.setPort(port1+j);
                		InetAddress broadcastAddr = InetAddress.getByName("58.214.236.114");  
                		dataPacket.setAddress(broadcastAddr);  
                		udpSocket.send(dataPacket);  
                	}  
                	catch (IOException e)  
                	{  
                    // TODO Auto-generated catch block  
                		e.printStackTrace();  
                	}
                }
                try {
					Thread.sleep(800);
				} catch (InterruptedException e) {
					// TODO Auto-generated catch block
					e.printStackTrace();
				}
                }
            }  
                
        });  
    }  
}  