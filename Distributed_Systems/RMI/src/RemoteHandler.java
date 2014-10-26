import java.io.DataOutputStream;
import java.io.IOException;
import java.io.InputStream;
import java.io.ObjectInputStream;
import java.io.OutputStream;
import java.lang.reflect.InvocationHandler;
import java.lang.reflect.Method;
import java.net.ConnectException;
import java.net.Socket;
import java.net.UnknownHostException;


public class RemoteHandler implements InvocationHandler {
	
	/**
	 * The HostName of the registry.
	 */
	String ip;
	
	/**
	 * The PortNumber of the registry.
	 */
	int port;
	
	@Override
	public Object invoke(Object proxyObj, Method method, Object[] args) 
													throws Throwable {
	    
	    StringBuffer str = new StringBuffer();
        
	    for(Object o:args) {
	        str.append(o.toString() + ":");
	    }
	    
	    str.setLength(str.length() - 1);
        String send = str.toString();
        
        Object ret = null;
		
	    try {
            Socket client = new Socket(ip, port);
            OutputStream outToServer = client.getOutputStream();
            DataOutputStream out =
                           new DataOutputStream(outToServer);
            String s = method.getName() + ":" + send;
            out.writeUTF(s);
            
            InputStream inFromServer = client.getInputStream();
            ObjectInputStream inServer =
                    new ObjectInputStream(inFromServer);
            ret = inServer.readObject();
            inServer.close();
            client.close();
        } catch (ConnectException e) {
            System.out.println("Destination node not configured properly");
        } catch (UnknownHostException e) {
            System.out.println("Destination node not configured properly");
        } catch (IOException e) {
            e.printStackTrace();
        }   
		
		
		return ret;
	}
	
	/**
     * The constructor to fill the values.
     * @param ip   The HostName of the registry.
     * @param port The PortNumber of the registry.
     */
	public RemoteHandler(String ip, int port) {
		this.ip = ip;
		this.port = port;
	}
}
