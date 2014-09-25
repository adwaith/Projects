import java.io.IOException;

public class test3 implements MigratableProcess {
	
	/**
	 * 
	 */
	private static final long serialVersionUID = 111L;
	private volatile boolean suspending;
	private TransactionalFileOutputStream outFile;
	private String input_string = "MORONZ For this lab we will focus our "
			+ "attention on processes that are specially built to be "
			+ "migratable, which will refer to as migratable processes. "
			+ "In order to simply the discussion, we are going to assume that "
			+ "the work is represented by object specially designed to be "
			+ "migratable, where the constraints are captured by an interface "
			+ "or abstract base class. We will refer to objects that implement "
			+ "such an interface or abstract base class as "
			+ "MigratableProcesses.";
	private int i=0;
	public test3(String[] args) {
		outFile = new TransactionalFileOutputStream("filez_write.txt", true);
	}
	
	public void run() {
		suspending = false;
		while(!suspending) {
			try {
				while (i<input_string.length()) {
					outFile.write(input_string.charAt(i));	
					Thread.sleep(50);
					i++;
				}
//				outFile.write(byte_array);
			} 
		catch (IOException e) {
				// TODO Auto-generated catch block
				e.printStackTrace();
			}
			catch (InterruptedException e) {
				// TODO Auto-generated catch block
				e.printStackTrace();
			}
			suspending = true;
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
