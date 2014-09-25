public class Parser {
	
	/**
	 * Parses the port number from the given String[].
	 * 
	 * @param args The command line arguments passed to the program.
	 * @return     The port number.
	 */
	public int parseport(String[] args) {

		boolean foundPort = false, badPort = false;
		int port = 0;
		for(int i = 0; i < args.length; i++) {
			if (args[i].equalsIgnoreCase("-p")) {
				try {
					port = new Integer(args[i+1]);
					if(port <=65535)
						foundPort = true;
					if (port <= 0)
						badPort = true;
				} catch (ArrayIndexOutOfBoundsException e) {
					foundPort = false;
				} catch (NumberFormatException e) {
					foundPort = false;
				}
			}
		}
		if(!foundPort) {
			System.out.println("No/Bad port number found. Try again.");
			System.exit(2);
		}
		if(badPort) {
			System.out.println("Bad port number found. Try again.");
			System.exit(2);
		}
	return port;
	}
	
	/**
	 * Parses the hostname from the given String[].
	 * 
	 * @param  args The command line arguments given to the program.
	 * @return The hostname.
	 */
	public String parseip(String[] args) {
		boolean foundIp = false, badIp = false;
		int sum = 0;
		String ip = "";
			for(int i = 0; i < args.length; i++) {
				if (args[i].equalsIgnoreCase("-ip")) {
					try {
						ip = args[i+1];
						foundIp = true;
					} catch (ArrayIndexOutOfBoundsException e) {
						foundIp = false;
					}
				}
			}
			if(!foundIp) {
				System.out.println("No IP found. Try again.");
				System.exit(3);
			}
			
			int[] test = new int[4];
			String[] broken = ip.split("\\.");

			try {
				for (int i = 0; i < 4; i++) {
				    test[i] = Integer.parseInt(broken[i]);
				    sum += test[i];
				    if(test[i] > 255 || test[i] < 0) {
				    	badIp = true;
				    	break;
				    }
				}
				if (sum == 0)
					badIp = true;
			} catch (ArrayIndexOutOfBoundsException e) {
				badIp = true;
			} catch (NumberFormatException e) {
				badIp = true;
			}
			
			if(badIp) {
				System.out.println("Bad IP found. Try again.");
				System.exit(3);
			}
			
		return ip;
	}
	
	/**
	 * Gets the hostname from the commandline
	 * 
	 * @param args The command line arguments
	 * @return The hostname.
	 */
	public String parseHn(String[] args) {
		String hn = "";
		boolean foundHn = false;
		for(int i = 0; i < args.length; i++) {
			if (args[i].equalsIgnoreCase("-hn")) {
				try {
					hn = args[i+1];
					foundHn = true;
				} catch (ArrayIndexOutOfBoundsException e) {
					foundHn = false;
				}
			}
		}
		
		if(!foundHn) {
			System.out.println("Bad/No HostName found. Try again.");
			System.exit(3);
		}
		
		return hn;
	}
}
