import java.io.DataInputStream;
import java.io.DataOutputStream;
import java.io.IOException;
import java.io.InputStream;
import java.io.OutputStream;
import java.net.ConnectException;
import java.net.Socket;
import java.net.UnknownHostException;
import java.util.Enumeration;


public class SlavePoll extends Thread{
	
	/*
	* Continuously polls the slaves to see if they are alive.
	*/
	public void run() {
		while(true) {
			if(!ProcessManager.nodeList.isEmpty()) {
				Enumeration<NodeInfo> items = ProcessManager.nodeList.elements();
				while (items.hasMoreElements()) {
					NodeInfo n = items.nextElement();
					if(n.nid == 1)
						continue;
					try {
						Socket client = new Socket(n.hostname, n.port);
						OutputStream outToServer = client.getOutputStream();
				         DataOutputStream out =
				                       new DataOutputStream(outToServer);
				         out.writeUTF("alive");
				         InputStream inFromServer = client.getInputStream();
				         DataInputStream in =
				                        new DataInputStream(inFromServer);
				         if(!in.readUTF().equals("yes")) {
				        	 ProcessManager.nodeList.remove(n.nid);
				         }
				         
				         out.close();
			        	 in.close();
			        	 client.close();

					} catch (ConnectException e) {
						ProcessManager.nodeList.remove(n.nid);
					} catch (UnknownHostException e) {
						 ProcessManager.nodeList.remove(n.nid);
					}  catch (IOException e) {
						 ProcessManager.nodeList.remove(n.nid);
					} 
				}
			}
			try {
				Thread.sleep(1000);
			} catch (InterruptedException e) {
				e.printStackTrace();
			}
		}
	}
}