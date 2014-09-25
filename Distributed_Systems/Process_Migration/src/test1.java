
public class test1 implements MigratableProcess {
	
	/**
	 * 
	 */
	private static final long serialVersionUID = 2591221565731299751L;
	private volatile boolean suspending;
	public test1(String[] args) {
	}
	
	public void run() {
		suspending = false;
		while(!suspending) {
			for(int i = 0; i < 100; i++) {
				System.out.println(i);
				try {
					Thread.sleep(1000);
				} catch (InterruptedException e) {
					e.printStackTrace();
				} 
			}
		}
	}
	
	
	public void suspend() {
		suspending = true;
	}

	public static void main(String[] args) {
//		Socket socket = new Socket();
//		 System.out.println(socket.getLocalSocketAddress());
	}
}
