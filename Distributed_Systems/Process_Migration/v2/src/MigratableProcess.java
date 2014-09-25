import java.io.Serializable;
import java.lang.Runnable;

public interface MigratableProcess extends Runnable, Serializable {

	void run();

	/**
	 * Method to suspend the process before migration. 
	 */
	void suspend();

//	String toString();
}
