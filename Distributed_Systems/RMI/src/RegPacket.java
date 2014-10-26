
public class RegPacket {
    /**
     * The HostName of the registry.
     */
    String ip;
    
    /**
     * The PortNumber of the registry. 
     */
    int port;
    
    /**
     * The constructor to fill the values.
     * @param ip   The HostName of the registry.
     * @param port The PortNumber of the registry.
     */
    public RegPacket(String ip, int port) {
        this.ip = ip;
        this.port = port;
    }
}
