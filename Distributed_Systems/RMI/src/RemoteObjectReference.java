import java.io.Serializable;


public class RemoteObjectReference implements Serializable {

    /**
     * Needed for serialization correctness. 
     */
    private static final long serialVersionUID = 1L;
    
    /**
     * The HostName of the registry.
     */
    String ip;
    
    /**
     * The PortNumber of the registry.
     */
    int port;
    
    /**
     * The interface of the class.
     */
    String classInterface;
    
    /**
     * The constructor to fill the values.
     * 
     * @param ip              The HostName of the server.
     * @param port            The PortNumber of the server.
     * @param classInterface  The interface of the class.
     */
    public RemoteObjectReference(String ip, int port, String classInterface) {
        this.ip = ip;
        this.port = port;
        this.classInterface = classInterface;
    }
}
