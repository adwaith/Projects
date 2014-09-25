import java.io.File;
import java.io.IOException;

public class test4 implements MigratableProcess {
	
	/**
	 * 
	 */
	private static final long serialVersionUID = 111L;
	private volatile boolean suspending;
	private TransactionalFileInputStream inFile;
	private TransactionalFileOutputStream outFile;
	int j = 0, i = 0;
    char c;
    byte[] bs = new byte[1];
    File file = new File("/afs/andrew.cmu.edu/usr10/adwaithv/public/pm/Process-Migration/filez_read.txt");

	public test4(String[] args) {
		inFile = new TransactionalFileInputStream("/afs/andrew.cmu.edu/usr10/adwaithv/public/pm/Process-Migration/filez_read.txt");
		outFile = new TransactionalFileOutputStream("/afs/andrew.cmu.edu/usr10/adwaithv/public/pm/Process-Migration/filez_write.txt", true);
	}
	
	public void run() {
		suspending = false;
		
		while(!suspending) {
			try {
				while (i< file.length()) {
					inFile.read(bs);
//					System.out.println((char)bs[0]);
					outFile.write((char)bs[0]);
					Thread.sleep(100);
					i++;
				}
			} 
		catch (IOException e) {
				// TODO Auto-generated catch block
				e.printStackTrace();
			} catch (InterruptedException e) {
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
