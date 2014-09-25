import java.io.Serializable;

public class NodeInfo implements Serializable{
	/**
	 * Contains the information about all the nodes on the system.
	 */
	private static final long serialVersionUID = 1L;
	String ip;
	int port;
	long nid;
	String hostname;
	
	public NodeInfo(String ip, int port, String hostname, long nid) {
		this.ip = ip;
		this.port = port;
		this.nid = nid;
		this.hostname = hostname;
	}
}
