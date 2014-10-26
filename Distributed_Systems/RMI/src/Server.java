public class Server {
	
	/**
	 * Used to start a new thread to listen to.
	 * 
	 * @param port The PortNumber used to listen to.
	 */
	private static void startListen(int port) {
	    System.out.println("Started the server.");
		Listen l = new Listen(port);
		Thread t = new Thread(l);
		t.start();
	}
	
	/**
	 * The main function.
	 * @param args Contains the PortNumber used to listen to.
	 */
	public static void main(String[] args) {
	    
	    if(args.length != 1) {
	        System.out.println("Run in the following format: ");
	        System.out.println("./run_server PortNumber");
	        System.exit(0);
	    }
	    
	    else {
	        try {
    	        int port;
    	        port = Integer.parseInt(args[0]);
    	        if(port <= 0 || port > 65535) {
                    System.out.println("Port number has to be within proper "
                            + "range");
                    System.exit(1);
                } else {
                    RMIRegistry reg = new RMIRegistry(port);
                    reg.populateRegistry();
                    startListen(port);
                }
	        } catch (NumberFormatException e) {
	            System.out.println("Port should be a whole number");
	        }
	    }
		
	}
}
