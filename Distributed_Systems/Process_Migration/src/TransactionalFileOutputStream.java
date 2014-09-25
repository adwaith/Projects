import java.io.RandomAccessFile;
import  java.io.Serializable;
import java.io.IOException;

public class TransactionalFileOutputStream implements Serializable{
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
	public TransactionalFileOutputStream(String fileName) {
		this.fileName = fileName;
		this.offset = 0L;
	}
	
	/**
	 * The constructor of the class. Assigns the required fields.
	 * @param fileName The file where IO operations are done.
	 * @param app      Flag to check if the mode is append or not.
	 */
	public TransactionalFileOutputStream(String fileName, boolean app) {
		this.fileName = fileName;
		this.offset = 0L;
	}
	
	/**
	 * Opens the file and seeks to where it was before migration.
	 * @param fileName The file where IO operations are done.
	 * @return         The FileInputStream object after seeking.
	 * @throws IOException
	 */
	public RandomAccessFile open(String fileName) throws IOException{
		RandomAccessFile current = new RandomAccessFile(fileName, "rws");
		current.seek(offset);
		return current;
	}
	
	/**
	 * Adds to the offset whatever is currently being written. 
	 * @param off The amount written.
	 */
	public void addToOffset(int off) {
			offset += off;
	}
		
	public void write(int b) throws IOException {
		RandomAccessFile current = open(fileName);
		current.write(b);
		addToOffset(1);
		current.close();
	}
	
	public void write(byte[] b) throws IOException {
		RandomAccessFile current = open(fileName);
		current.write(b);
		addToOffset(b.length);
		current.close();
	}
	
	public void write(byte[] b, int off, int len) throws IOException {
		RandomAccessFile current = open(fileName);
		current.write(b, off, len);
		addToOffset(len);
		current.close();
	}
}
