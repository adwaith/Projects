import java.io.IOException;
import java.net.ServerSocket;
import java.net.Socket;


public class Listen extends Thread {

	/**
	 * The socket with which communication is done.
	 */
	private ServerSocket s;
	
	public void run() {
		while(true) {
			Socket server;
			try {
				server = s.accept();
				ListenAction la = new ListenAction(server);
				Thread t = new Thread(la);
				t.start();
			} catch (IOException e) {
				e.printStackTrace();
			}
		}
		
	}
	
	/**
	 * Starts a new socket with the given port.
	 * 
	 * @param port The port with which the socket is made. 
	 */
	public Listen(int port) {
		try {
			s = new ServerSocket(port);
		} catch (IOException e) {
			e.printStackTrace();
		}
	}
}
