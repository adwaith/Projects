import java.net.InetAddress;
import java.net.UnknownHostException;
import java.util.concurrent.ConcurrentHashMap;


public class RMIRegistry extends Thread{
    
    /**
     * The PortNumber which is used to listen.
     */
    int port;
    
	/**
	 * A ConcurrentHashMap to store all the RemoteObjectReferences in the 
	 * registry.
	 */
	static ConcurrentHashMap<String, RemoteObjectReference> registry = 
	        new ConcurrentHashMap<String, RemoteObjectReference>();
	
	/**
	 * Used to populate the registry with the different RemoteObjectReferences.
	 */
	public void populateRegistry() {
	    RemoteObjectReference fixed_value_calc;
	    RemoteObjectReference fixed_value_case;
        try {
            fixed_value_calc = new RemoteObjectReference
                    (InetAddress.getLocalHost().getHostName(), port, 
                            "CalcInterface");
            fixed_value_case = new RemoteObjectReference
                    (InetAddress.getLocalHost().getHostName(), port, 
                            "CaseInterface");
            registry.put("add", fixed_value_calc);
            registry.put("sub", fixed_value_calc);
            registry.put("mul", fixed_value_calc);
            registry.put("div", fixed_value_calc);
            registry.put("upper", fixed_value_case);
            registry.put("lower", fixed_value_case);
        } catch (UnknownHostException e) {
            e.printStackTrace();
        }
	}
	
	/**
	 * The constructor to fill the values.
	 * @param port The PortNumber which is used to listen.
	 */
	public RMIRegistry(int port) {
		this.port = port;
	}
}
