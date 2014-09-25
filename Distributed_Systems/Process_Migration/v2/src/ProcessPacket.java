import java.io.Serializable;

public class ProcessPacket implements Serializable{

	/**
	 * Needed for serialization correctness.
	 */
	private static final long serialVersionUID = 123L;
	
	/**
	 * The object of a MigratableProcess
	 */
	MigratableProcess mp;
	
	/**
	 * The process' name along with it's arguments.
	 */
	String name;
	
	/**
	 * The PID of the process. Not saved during serialization. 
	 */
	transient long pid;
	
	/**
	 * The thread object of the process. Not saved during serialization.
	 */
	transient Thread t;
	
	/**
	 * The constructor of the class. Assigns the required fields.
	 * 
	 * @param obj  The MigratableProcess object.
	 * @param pid  The PID of the process.
	 * @param name The process' name along with it's arguments.
	 * @param t    The thread object of the process.
	 */
	public ProcessPacket(MigratableProcess obj, long pid, String name, Thread t) {
		this.mp = obj;
		this.pid = pid;
		this.name = name;
		this.t = t;
	}
}
