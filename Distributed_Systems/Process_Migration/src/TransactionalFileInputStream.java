import java.io.FileInputStream;
import java.io.InputStream;
import java.io.Serializable;
import java.io.IOException;

public class TransactionalFileInputStream extends InputStream implements Serializable {
	/**
	 * Needed for serialization correctness.
	 */
	private static final long serialVersionUID = 123L;
	
	/**
	 * The file where IO operations are done.
	 */
	private String fileName;
	
	/**
	 * The counter to see how much of the file has been previously processed. 
	 */
	private long offset;
	
	/**
	 * The constructor of the class. Assigns the required fields.
	 * 
	 * @param fileName The file where IO operations are done.
	 */
	public TransactionalFileInputStream(String fileName) {
		this.fileName = fileName;
		this.offset = 0L;
	}	
	
	/**
	 * Opens the file and seeks to where it was before migration.
	 * 
	 * @param fileName The file where IO operations are done.
	 * @return		   The FileInputStream object after seeking.
	 * @throws IOException
	 */
	public FileInputStream open(String fileName) throws IOException{
		FileInputStream current = new FileInputStream(fileName);
		current.skip(offset);
		return current;
	}
	
	/**
	 * Adds to the offset whatever is currently being read. 
	 * 
	 * @param ret The amount read.
	 * @return	  The amount read.
	 */
	public int addToOffset(int ret) {
		if (ret == -1)
			return -1;
		else
			offset += ret;
		return ret;
	}
	
	public int read() throws IOException {
		FileInputStream current = open(fileName);
		int ret = current.read();
		current.close();
		return addToOffset(ret);
	}
	
	public int read(byte[] b) throws IOException {
		FileInputStream current = open(fileName);
		int ret = current.read(b);
		current.close();
		return addToOffset(ret);
	}
	
	public int read(byte[] b, int off, int len) throws IOException {
		FileInputStream current = open(fileName);
		int ret = current.read(b, off, len);
		current.close();
		return addToOffset(ret);
	}
}
